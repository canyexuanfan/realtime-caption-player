// src/player/MpvRenderWidget.h
// 用 QOpenGLWidget 承载 libmpv 的 OpenGL 渲染上下文，把视频帧绘到 Qt 窗口。
// 沙箱无 GL 设备，仅保证编译/链接；真实渲染在目标 Windows 会话验证。
#pragma once

#include <QOpenGLWidget>
#include <mpv/render.h>

class MpvPlayer;

class MpvRenderWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit MpvRenderWidget(QWidget* parent = nullptr);
    ~MpvRenderWidget() override;

    // 绑定 player 的 mpv 句柄。player 须先 create()+initialize()。
    // 若 GL 上下文已就绪则立即建立渲染上下文，否则在 initializeGL() 中建立。
    bool attachPlayer(MpvPlayer* player);

    bool isRenderContextReady() const { return m_mpvGL != nullptr; }

signals:
    // 渲染上下文创建成功——此后 loadfile 的 VO 初始化才不会失败（黑屏竞态根因）。
    void renderContextReady();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    static void getProcAddrThunk(void* ctx, const char* name);
    static void onUpdate(void* ctx);
    void createRenderContextNow();

    MpvPlayer* m_player = nullptr;
    mpv_render_context* m_mpvGL = nullptr;
    bool m_attached = false;
};
