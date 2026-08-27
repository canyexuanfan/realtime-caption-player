// src/player/MpvRenderWidget.cpp
#include "MpvRenderWidget.h"
#include "MpvPlayer.h"

#include <QOpenGLContext>
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
        mpv_render_context_free(m_mpvGL);
        m_mpvGL = nullptr;
    }
}

bool MpvRenderWidget::attachPlayer(MpvPlayer* player) {
    m_player = player;
    m_attached = (player != nullptr);
    // 若 GL 上下文已存在（如重新绑定），立即建立渲染上下文。
    if (m_attached && m_player->handle() && context() && context()->isValid() && !m_mpvGL) {
        createRenderContextNow();
    }
    return m_attached;
}

void MpvRenderWidget::createRenderContextNow() {
    if (!m_player || !m_player->handle() || m_mpvGL) return;

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
        m_mpvGL = nullptr;
        return;
    }
    mpv_render_context_set_update_callback(m_mpvGL, &MpvRenderWidget::onUpdate, this);
}

void MpvRenderWidget::onUpdate(void* ctx) {
    auto* w = static_cast<MpvRenderWidget*>(ctx);
    // mpv 请求重绘：线程安全地触发 Qt 重绘（update 为 QWidget 槽，跨线程安全）。
    QMetaObject::invokeMethod(w, "update", Qt::QueuedConnection);
}

void MpvRenderWidget::initializeGL() {
    if (m_attached && m_player && m_player->handle()) {
        createRenderContextNow();
    }
}

void MpvRenderWidget::paintGL() {
    if (!m_mpvGL) {
        // 尚无渲染上下文：清为黑屏。
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    mpv_opengl_fbo fbo{};
    fbo.fbo = 0; // 默认帧缓冲
    fbo.w = width();
    fbo.h = height();
    fbo.internal_format = 0;

    int flipY = 1; // QOpenGLWidget 默认帧缓冲需垂直翻转
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    mpv_render_context_render(m_mpvGL, params);
}

void MpvRenderWidget::resizeGL(int /*w*/, int /*h*/) {
    // mpv 每帧在 paintGL 读取当前尺寸，无需手动处理。
}
