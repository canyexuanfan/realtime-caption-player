// src/player/MpvRenderWidget.cpp
#include "MpvRenderWidget.h"
#include "MpvPlayer.h"
#include "MpvTrace.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QMetaObject>

MpvRenderWidget::MpvRenderWidget(QWidget* parent) : QOpenGLWidget(parent) {
    // 渲染交换后通知 mpv 完成一帧，使其可继续送帧。
    connect(this, &QOpenGLWidget::frameSwapped, this, [this] {
        if (m_mpvGL) mpv_render_context_report_swap(m_mpvGL);
    });
}

MpvRenderWidget::~MpvRenderWidget() {
    if (m_mpvGL) {
        mpv_render_context_set_update_callback(m_mpvGL, nullptr, nullptr);
        makeCurrent();
        mpv_render_context_free(m_mpvGL);
        doneCurrent();
        m_mpvGL = nullptr;
    }
}

bool MpvRenderWidget::attachPlayer(MpvPlayer* player) {
    m_player = player;
    m_attached = (player != nullptr);
    rcpTrace(QStringLiteral("attachPlayer attached=%1 hasHandle=%2 hasCtx=%3")
                 .arg(m_attached)
                 .arg(m_player && m_player->handle() ? 1 : 0)
                 .arg(context() && context()->isValid() ? 1 : 0));
    // 若 GL 上下文已存在（如重新绑定），立即建立渲染上下文。
    if (m_attached && m_player->handle() && context() && context()->isValid() && !m_mpvGL) {
        createRenderContextNow();
    }
    return m_attached;
}

void MpvRenderWidget::createRenderContextNow() {
    if (!m_player || !m_player->handle() || m_mpvGL) return;

    // 诊断：记录 Qt 给到的 GL 上下文类型/版本（区分 desktop GL / GLES / 软件）。
    if (auto* c = context()) {
        const QSurfaceFormat f = c->format();
        const QByteArray renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const QByteArray glver = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        rcpTrace(QStringLiteral("GL context type=%1 ver=%2.%3 profile=%4 renderer=[%5] glver=[%6]")
                     .arg(int(c->openGLModuleType()))
                     .arg(f.majorVersion()).arg(f.minorVersion())
                     .arg(int(f.profile()))
                     .arg(QString::fromLatin1(renderer), QString::fromLatin1(glver)));
    }

    mpv_opengl_init_params glInit{};
    glInit.get_proc_address = [](void* /*ctx*/, const char* name) -> void* {
        QOpenGLContext* glctx = QOpenGLContext::currentContext();
        if (!glctx) return nullptr;
        return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
    };
    glInit.get_proc_address_ctx = nullptr;

    const char* apiType = MPV_RENDER_API_TYPE_OPENGL;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE,
         const_cast<void*>(static_cast<const void*>(apiType))},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    if (mpv_render_context_create(&m_mpvGL, m_player->handle(), params) < 0) {
        rcpTrace(QStringLiteral("mpv_render_context_create FAILED"));
        m_mpvGL = nullptr;
        return;
    }
    rcpTrace(QStringLiteral("mpv_render_context_create OK"));
    emit renderContextReady();
    mpv_render_context_set_update_callback(m_mpvGL, &MpvRenderWidget::onUpdate, this);
}

void MpvRenderWidget::onUpdate(void* ctx) {
    auto* w = static_cast<MpvRenderWidget*>(ctx);
    // mpv 线程请求重绘：转投 GUI 线程执行 maybeUpdate。
    QMetaObject::invokeMethod(w, "maybeUpdate", Qt::QueuedConnection);
}

void MpvRenderWidget::maybeUpdate() {
    // 窗口最小化时 QWidget::update() 会被 Qt 跳过，mpv render API 会因
    // 渲染超时产生间歇卡顿——此时手动驱动一次完整绘制（官方示例同款兜底）。
    if (window() && window()->isMinimized()) {
        makeCurrent();
        paintGL();
        context()->swapBuffers(context()->surface());
        doneCurrent();
    } else {
        update();
    }
}

void MpvRenderWidget::initializeGL() {
    rcpTrace(QStringLiteral("initializeGL attached=%1").arg(m_attached));
    if (m_attached && m_player && m_player->handle()) {
        createRenderContextNow();
    }
}

void MpvRenderWidget::paintGL() {
    static bool tracedFirstFrame = false;
    if (!tracedFirstFrame) {
        tracedFirstFrame = true;
        rcpTrace(QStringLiteral("paintGL: first frame (mpvGL=%1)").arg(m_mpvGL ? 1 : 0));
    }
    if (!m_mpvGL) {
        // 尚无渲染上下文：清为参考 .video-surface 底色（非纯黑）。
        glClearColor(0x10 / 255.0f, 0x15 / 255.0f, 0x1d / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    // QOpenGLWidget 的绘制目标是 Qt 内部 FBO，不是 0 号默认帧缓冲：
    // 传 0 会画进被 Qt 合成流程丢弃的表面 → 永久黑屏（官方 qt_opengl 示例
    // 用 defaultFramebufferObject()）。尺寸需用物理像素（FBO 含 DPR 缩放）。
    const qreal dpr = devicePixelRatioF();
    mpv_opengl_fbo fbo{};
    fbo.fbo = static_cast<int>(defaultFramebufferObject());
    fbo.w = static_cast<int>(width() * dpr);
    fbo.h = static_cast<int>(height() * dpr);
    fbo.internal_format = 0;

    int flipY = 1; // QOpenGLWidget 默认帧缓冲需垂直翻转
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    mpv_render_context_render(m_mpvGL, params);

    // 诊断（一次性）：FBO 绑定 + 中心像素回读——区分"mpv 画了黑"vs"画错目标"。
    static bool diagDone = false;
    if (!diagDone && qEnvironmentVariableIsSet("RCP_DIAG_PIXEL")) {
        diagDone = true;
        QOpenGLFunctions* gl = context()->functions();
        GLint bound = -1;
        gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
        unsigned char px[4] = {0, 0, 0, 0};
        gl->glReadPixels(width() / 2, height() / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        rcpTrace(QStringLiteral("diag fbo-bound=%1 center-px=%2,%3,%4,%5")
                     .arg(bound).arg(px[0]).arg(px[1]).arg(px[2]).arg(px[3]));
    }
}

void MpvRenderWidget::resizeGL(int /*w*/, int /*h*/) {
    // mpv 每帧在 paintGL 读取当前尺寸，无需手动处理。
}
