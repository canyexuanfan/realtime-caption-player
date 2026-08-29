// src/player/MpvPlayer.cpp
#include "MpvPlayer.h"

#include <QMetaObject>
#include <QCoreApplication>
#include "MpvTrace.h"

namespace {
constexpr int kPropTimePos = 1;
constexpr int kPropDuration = 2;
constexpr int kPropPause    = 3;
}

MpvPlayer::MpvPlayer(QObject* parent) : QObject(parent) {}

MpvPlayer::~MpvPlayer() {
    if (m_handle) {
        // 停止渲染回调并销毁句柄。
        mpv_set_wakeup_callback(m_handle, nullptr, nullptr);
        mpv_destroy(m_handle);
        m_handle = nullptr;
    }
}

unsigned long MpvPlayer::clientApiVersion() {
    return mpv_client_api_version();
}

bool MpvPlayer::create() {
    if (m_handle) return true;
    m_handle = mpv_create();
    return m_handle != nullptr;
}

bool MpvPlayer::setOption(const QString& name, const QString& value) {
    if (!m_handle) return false;
    const QByteArray n = name.toUtf8();
    const QByteArray v = value.toUtf8();
    return mpv_set_option_string(m_handle, n.constData(), v.constData()) == 0;
}

bool MpvPlayer::initialize() {
    if (!m_handle) return false;
    if (mpv_initialize(m_handle) < 0) return false;

    // 注册属性观察（播放进度 / 时长 / 暂停态）。
    mpv_observe_property(m_handle, kPropTimePos, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, kPropDuration, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, kPropPause, "pause", MPV_FORMAT_FLAG);

    // 唤醒回调：mpv 有事件时通知 Qt 主线程排泄队列。
    mpv_set_wakeup_callback(m_handle, &MpvPlayer::wakeupCallback, this);

    // 诊断：把 mpv 内部日志引入事件流（v 级别可定位 GL 报错组件）。
    // 诊断开关：verbose 日志会在播放时产生海量事件（二分挂死用）。
    if (qEnvironmentVariableIsEmpty("RCP_NO_LOG"))
        mpv_request_log_messages(m_handle, "v");
    rcpTrace(QStringLiteral("mpv initialized"));

    applyInitialProperties();
    return true;
}

void MpvPlayer::applyInitialProperties() {
    // 初始属性（音量/静音/倍速）。用 set_property 而非命令行，便于运行期调整。
    setVolume(m_volume);
    setMuted(m_muted);
    setSpeed(m_speed);
}

bool MpvPlayer::loadFile(const QString& path, bool replace) {
    if (!m_handle) return false;
    rcpTrace(QStringLiteral("loadFile replace=%1 -> %2").arg(replace ? 1 : 0).arg(path));
    const QByteArray p = path.toUtf8();
    const char* args[] = {
        "loadfile",
        p.constData(),
        replace ? "replace" : "append",
        nullptr
    };
    // 异步命令：网盘挂载路径（如 X:/）的源打开可能耗时数十秒，
    // 同步 mpv_command 会阻塞 GUI 线程（实测 19.5s 卡死）。异步把耗时
    // 移到 mpv 核心线程，命令回复（MPV_EVENT_COMMAND_REPLY）可忽略。
    m_loaded = (mpv_command_async(m_handle, 0, args) >= 0);
    return m_loaded;
}

int MpvPlayer::createRenderContext(mpv_render_context** outCtx,
                                    void* (*getProcAddr)(void* ctx, const char* name),
                                    void* glCtx) {
    if (!m_handle) return -1;

    mpv_opengl_init_params glInit{};
    glInit.get_proc_address = getProcAddr;
    glInit.get_proc_address_ctx = glCtx;

    const char* apiType = MPV_RENDER_API_TYPE_OPENGL;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE,
         const_cast<void*>(static_cast<const void*>(apiType))},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    return mpv_render_context_create(outCtx, m_handle, params);
}

void MpvPlayer::play() {
    if (!m_handle) return;
    // MPV_FORMAT_FLAG 要求 int*（0/1）；传字符串指针会解引用出非零值=暂停。
    int flag = 0;
    mpv_set_property(m_handle, "pause", MPV_FORMAT_FLAG, &flag);
    m_paused = false;
    emit pauseStateChanged(false);
}

void MpvPlayer::pause() {
    if (!m_handle) return;
    int flag = 1;
    mpv_set_property(m_handle, "pause", MPV_FORMAT_FLAG, &flag);
    m_paused = true;
    emit pauseStateChanged(true);
}

void MpvPlayer::togglePause() {
    if (m_paused) play(); else pause();
}

void MpvPlayer::stop() {
    if (!m_handle) return;
    const char* args[] = {"stop", nullptr};
    mpv_command(m_handle, args);
    m_loaded = false;
    m_timePos = 0.0;
    m_duration = 0.0;
    emit mediaEnded();
}

void MpvPlayer::seek(double seconds, bool relative) {
    if (!m_handle) return;
    const QByteArray fmt = relative ? QByteArray("relative") : QByteArray("absolute");
    const QString secs = QString::number(seconds, 'f', 3);
    const QByteArray s = secs.toUtf8();
    const char* args[] = {
        "seek",
        s.constData(),
        fmt.constData(),
        nullptr
    };
    mpv_command(m_handle, args);
}

bool MpvPlayer::loadSubtitle(const QString& path) {
    if (!m_handle) return false;
    const QByteArray p = path.toUtf8();
    const char* args[] = {
        "sub-add",
        p.constData(),
        "select",   // 手动加载立即显示（init 已 sid=no；auto 不会选中导致"加载没反应"）
        nullptr
    };
    return mpv_command(m_handle, args) >= 0;
}

void MpvPlayer::setVolume(int vol) {
    m_volume = qBound(0, vol, 100);
    if (!m_handle) return;
    const QString v = QString::number(m_volume);
    mpv_set_property_string(m_handle, "volume", v.toUtf8().constData());
}

void MpvPlayer::setMuted(bool muted) {
    m_muted = muted;
    if (!m_handle) return;
    int flag = muted ? 1 : 0;
    mpv_set_property(m_handle, "mute", MPV_FORMAT_FLAG, &flag);
}

void MpvPlayer::setSpeed(double speed) {
    m_speed = speed > 0.0 ? speed : 1.0;
    if (!m_handle) return;
    const QString v = QString::number(m_speed, 'f', 3);
    mpv_set_property_string(m_handle, "speed", v.toUtf8().constData());
}

void MpvPlayer::setAudioTrack(int aid) {
    if (!m_handle) return;
    int64_t id = aid;
    mpv_set_property(m_handle, "aid", MPV_FORMAT_INT64, &id);
}

void MpvPlayer::showSubtitleOverlay(const QString& assEvents) {
    if (!m_handle) return;
    // mpv osd-overlay 语法：osd-overlay <id:int> <format> <data>。
    // 同一 id 重复下发即原位更新；没有 add/remove 子命令。
    const QByteArray data = assEvents.toUtf8();
    const char* args[] = {
        "osd-overlay", "1", "ass-events", data.constData(), nullptr
    };
    mpv_command(m_handle, args);
}

void MpvPlayer::clearSubtitleOverlay() {
    if (!m_handle) return;
    // format "none" 表示移除该 id 的叠加层。
    const char* args[] = {
        "osd-overlay", "1", "none", "", nullptr
    };
    mpv_command(m_handle, args);
}

void MpvPlayer::wakeupCallback(void* ctx) {
    auto* self = static_cast<MpvPlayer*>(ctx);
    // mpv 唤醒可能在非 Qt 线程；用 QueuedConnection 把事件排泄抛回主线程。
    QMetaObject::invokeMethod(self, "processEvents", Qt::QueuedConnection);
}

void MpvPlayer::processEvents() {
    if (!m_handle) return;
    while (true) {
        mpv_event* event = mpv_wait_event(m_handle, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        handleEvent(event);
    }
}

void MpvPlayer::handleEvent(mpv_event* event) {
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        auto* pc = static_cast<mpv_event_property*>(event->data);
        if (!pc || !pc->data) break;
        if (pc->name == QStringLiteral("time-pos") && pc->format == MPV_FORMAT_DOUBLE) {
            m_timePos = *static_cast<double*>(pc->data);
            rcpTrace(QStringLiteral("time-pos %1").arg(m_timePos, 0, 'f', 2));
            emit positionChanged(m_timePos);
        } else if (pc->name == QStringLiteral("duration") && pc->format == MPV_FORMAT_DOUBLE) {
            m_duration = *static_cast<double*>(pc->data);
            emit durationChanged(m_duration);
        } else if (pc->name == QStringLiteral("pause") && pc->format == MPV_FORMAT_FLAG) {
            m_paused = *static_cast<int*>(pc->data) != 0;   // FLAG 数据是 int
            rcpTrace(QStringLiteral("pause -> %1").arg(m_paused ? 1 : 0));
            emit pauseStateChanged(m_paused);
        }
        break;
    }
    case MPV_EVENT_FILE_LOADED: {
        m_loaded = true;
        emit mediaLoaded();
        break;
    }
    case MPV_EVENT_END_FILE: {
        auto* ef = static_cast<mpv_event_end_file*>(event->data);
        rcpTrace(QStringLiteral("end-file reason=%1").arg(ef ? static_cast<int>(ef->reason) : -1));
        if (ef && ef->reason == MPV_END_FILE_REASON_EOF) {
            emit mediaEnded();
        } else if (ef && ef->reason == MPV_END_FILE_REASON_ERROR) {
            // 打开/读取失败（文件不存在、挂载盘离线等）：显式上报，不再无声无息。
            emit mediaError(tr("无法打开媒体文件（文件不存在、无法读取或挂载盘离线）"));
        }
        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        // 句柄将销毁；停止再访问。
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        auto* lm = static_cast<mpv_event_log_message*>(event->data);
        if (lm && lm->text)
            rcpTrace(QStringLiteral("mpv[%1] %2")
                            .arg(QString::fromUtf8(lm->level),
                                 QString::fromUtf8(lm->text).trimmed()));
        break;
    }
    default:
        break;
    }
}
