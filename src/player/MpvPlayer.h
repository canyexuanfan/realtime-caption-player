// src/player/MpvPlayer.h
// libmpv 客户端封装（播放内核）。提供加载/播放/暂停/停止/seek/音量/倍速/
// 音轨选择，并通过 mpv 属性观察桥接播放进度、时长、暂停态、播放结束等事件。
// 事件通过 Qt 信号暴露，主线程用 processEvents() 排泄 mpv 事件队列。
#pragma once

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
// render_gl.h 会把 mpv_render_context_create 等宏重定向为运行期函数指针
// （需先经 mpv_render_context_create_fn 取址）。本工程以 mpv.lib 静态链接，
// 希望保留 render.h 中真正导出的符号，故撤销该宏重定向，仅借用其中
// mpv_opengl_init_params 结构体定义。
#undef mpv_render_context_create

#include <QString>
#include <QObject>
#include <functional>

class MpvPlayer : public QObject {
    Q_OBJECT
public:
    explicit MpvPlayer(QObject* parent = nullptr);
    ~MpvPlayer();

    MpvPlayer(const MpvPlayer&) = delete;
    MpvPlayer& operator=(const MpvPlayer&) = delete;

    // 客户端 API 版本（高 16 位主版本，低 16 位次版本）。验证 libmpv 可链接可调用。
    static unsigned long clientApiVersion();

    // 创建底层 mpv_handle；失败返回 false。
    bool create();

    // 设置字符串选项（如 "vo"="libmpv"）。需在 initialize() 之前调用。
    bool setOption(const QString& name, const QString& value);

    // 初始化播放核心（创建句柄后、加载媒体前调用）。会注册属性观察与唤醒回调。
    bool initialize();

    // 异步加载媒体文件（loadfile 命令）。
    bool loadFile(const QString& path, bool replace = true);

    // 创建 OpenGL 渲染上下文。需要上层提供真实 GL 上下文（get_proc_address + glCtx）。
    // 沙箱无显示设备故不调用，真实渲染在目标 Windows 会话验证。
    int createRenderContext(mpv_render_context** outCtx,
                             void* (*getProcAddr)(void* ctx, const char* name),
                             void* glCtx);

    mpv_handle* handle() const { return m_handle; }
    bool isValid() const { return m_handle != nullptr; }

    // ---- 播放控制 ----
    void play();
    void pause();
    void togglePause();
    bool isPaused() const { return m_paused; }
    void stop();                       // 停止并卸载当前媒体
    void seek(double seconds, bool relative = false);
    bool loadSubtitle(const QString& path);   // 外挂字幕（mpv sub-add）

    // ---- 音频/速度 ----
    void setVolume(int vol);           // 0..100
    int volume() const { return m_volume; }
    void setMuted(bool muted);
    bool muted() const { return m_muted; }
    void setSpeed(double speed);       // e.g. 1.0, 1.25, 0.5
    double speed() const { return m_speed; }

    // ---- 音轨 ----
    void setAudioTrack(int aid);       // mpv 音频轨 id（aid 属性）

    // ---- 字幕叠加（mpv osd-overlay, ASS 事件）----
    void showSubtitleOverlay(const QString& assEvents);  // assEvents: mpv ass-events 格式文本
    void clearSubtitleOverlay();

    // ---- 查询 ----
    double duration() const { return m_duration; }
    double timePosition() const { return m_timePos; }

signals:
    void durationChanged(double seconds);
    void positionChanged(double seconds);
    void pauseStateChanged(bool paused);
    void mediaLoaded();
    void mediaEnded();
    void mediaError(const QString& message);   // 打开/读取失败（含挂载盘离线）
    void playbackError(const QString& message);
    void eventAvailable();            // mpv 唤醒：主线程应调用 processEvents()

public slots:
    // 排泄 mpv 事件队列（由 eventAvailable() 触发，必须在主线程调用）。
    void processEvents();

private:
    static void wakeupCallback(void* ctx);
    void handleEvent(mpv_event* event);
    void applyInitialProperties();

    mpv_handle* m_handle = nullptr;

    bool    m_paused = false;
    int     m_volume = 100;
    bool    m_muted  = false;
    double  m_speed  = 1.0;
    double  m_duration = 0.0;
    double  m_timePos  = 0.0;
    bool    m_loaded   = false;
};
