// src/player/WorkerSupervisor.cpp
#include "WorkerSupervisor.h"

#include "ipc/JsonMessageCodec.h"
#include "ipc/Protocol.h"
#include "core/MpvTrace.h"

#include <QCoreApplication>
#include <QLocalSocket>
#include <QUuid>
#include <QTimer>
#include <qt_windows.h>

namespace rcp::player {

WorkerSupervisor::WorkerSupervisor(QObject* parent) : QObject(parent) {
    // B4 心跳看门狗：Ready/Heartbeat 每次刷新 m_lastHeartbeatMs；
    // 心跳 2s × 3 次未到（+0.5s 余量）判定失联 → 杀进程自动重启（≤3 次退避）。
    m_watchdog = new QTimer(this);
    m_watchdog->setInterval(1000);
    connect(m_watchdog, &QTimer::timeout, this, [this] {
        if (!m_connected || !m_openSent || m_intentionalStop) return;
        if (m_lastHeartbeatMs == 0) return;
        if (m_alive.elapsed() - m_lastHeartbeatMs > 6500) {
            rcpTrace(QStringLiteral("worker heartbeat lost (>6.5s), restarting"));
            killProcess();   // 触发 onProcessFinished → scheduleRestart
        }
    });
    m_watchdog->start();
}

WorkerSupervisor::~WorkerSupervisor() {
    if (m_jobHandle) CloseHandle(static_cast<HANDLE>(m_jobHandle));
}

bool WorkerSupervisor::start(const QString& workerExe, const QString& mediaPath,
                             const QString& modelsRoot, int audioTrack, quint64 generation) {
    if (isRunning()) shutdown();

    m_mediaPath = mediaPath;
    m_modelsRoot = modelsRoot;
    m_audioTrack = audioTrack;
    // generation 以协调器为权威（B3）：不再自增，命令/事件原样携带。
    m_generation = generation;
    m_openSent = false;
    m_connected = false;
    m_connectTries = 0;
    m_lastSocketError.clear();
    m_serverName = QStringLiteral("rcp-caption-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_workerExe = workerExe;
    m_alive.restart();
    m_lastHeartbeatMs = 0;

    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(QCoreApplication::applicationDirPath());
    // 崩溃定位：worker stderr 直通父进程 stderr（诊断期，随 RCP_TRACE 场景取证）。
    m_proc->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(m_proc, &QProcess::started, this, &WorkerSupervisor::onProcessStarted);
    connect(m_proc, &QProcess::errorOccurred, this, &WorkerSupervisor::onProcessError);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &WorkerSupervisor::onProcessFinished);

    const QStringList args = {
        QStringLiteral("--servername"), m_serverName,
        QStringLiteral("--models"), m_modelsRoot
    };
    m_proc->start(workerExe, args);
    if (!m_proc->waitForStarted(5000)) {
        emit workerError(QStringLiteral("worker 进程启动失败"));
        return false;
    }
    assignJobObject();
    return true;
}

// Job Object（B4）：把 worker 绑进 kill-on-close 的作业对象——
// 主进程崩溃/被任务管理器强杀时 worker 不再残留占着模型文件。
void WorkerSupervisor::assignJobObject() {
    if (!m_proc) return;
    if (!m_jobHandle) {
        HANDLE job = CreateJobObjectW(nullptr, nullptr);
        if (!job) return;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info))) {
            CloseHandle(job);
            return;
        }
        m_jobHandle = job;
    }
    AssignProcessToJobObject(static_cast<HANDLE>(m_jobHandle),
                             OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, m_proc->processId()));
}

void WorkerSupervisor::onProcessStarted() {
    tryConnect();
}

// IPC 连接重试：复用同一 socket（反复销毁重建会在连接中打断 pipe），
// 最多重试 60s（600 × 100ms）。worker 冷启动（杀软扫描/磁盘慢/主线程忙）时
// 监听耗时可能远超 5s，窗口太短会被误判为永久失败。
void WorkerSupervisor::tryConnect() {
    if (m_connected) return;
    if (m_proc && m_proc->state() != QProcess::Running) {
        emit workerError(QStringLiteral("worker 进程在 IPC 连接前退出"));
        return;
    }
    if (!m_sock) {
        m_sock = new QLocalSocket(this);
        connect(m_sock, &QLocalSocket::connected, this, &WorkerSupervisor::onConnected);
        connect(m_sock, &QLocalSocket::readyRead, this, &WorkerSupervisor::onReadyRead);
        connect(m_sock, &QLocalSocket::errorOccurred, this, &WorkerSupervisor::onSocketError);
    }
    m_sock->connectToServer(m_serverName);
    if (m_sock->state() == QLocalSocket::ConnectedState) {
        onConnected();
    } else if (m_connectTries++ < 600) {
        // worker 尚未 listen（或连接仍在建立中）；每 100ms 重试，最多 ~60s。
        QTimer::singleShot(100, this, &WorkerSupervisor::tryConnect);
    } else {
        emit workerError(QStringLiteral("无法连接到 worker IPC（超时）：%1")
                         .arg(m_lastSocketError));
    }
}

void WorkerSupervisor::onConnected() {
    m_connected = true;
    emit sessionStarted(m_generation);
    sendCommand(rcp::ipc::CommandType::Hello, QJsonObject{});
}

void WorkerSupervisor::onSocketError(QLocalSocket::LocalSocketError err) {
    m_lastSocketError = m_sock ? m_sock->errorString() : QString();
    Q_UNUSED(err);
    // 连接阶段的错误由 tryConnect 的定时器重试覆盖；运行期断开则报错。
    if (m_connected && m_proc && m_proc->state() != QProcess::Running) {
        emit workerError(QStringLiteral("worker IPC 连接断开"));
    }
}

void WorkerSupervisor::onReadyRead() {
    if (!m_sock) return;
    QList<QByteArray> frames;
    QString err;
    if (!m_decoder.feed(m_sock->readAll(), frames, &err)) {
        emit workerError(err);
        return;
    }
    for (const auto& f : frames) {
        auto parsed = rcp::ipc::JsonMessageCodec::parse(f);
        if (parsed.isError()) {
            emit workerError(QStringLiteral("IPC 封包解析失败"));
            continue;
        }
        handleEvent(parsed.value());
    }
}

void WorkerSupervisor::handleEvent(const rcp::ipc::Envelope& env) {
    // 仅处理以 "event." 开头的有效事件，避免未知类型被错误归类。
    if (!env.type.startsWith(QStringLiteral("event."))) {
        return;
    }
    const QJsonObject& p = env.payload;
    const rcp::ipc::EventType et = rcp::ipc::eventTypeFromName(env.type);
    switch (et) {
    case rcp::ipc::EventType::Ready:
        m_lastHeartbeatMs = m_alive.elapsed();
        emit ready();
        if (!m_openSent) {
            m_openSent = true;
            QJsonObject open;
            open[QStringLiteral("path")] = m_mediaPath;
            open[QStringLiteral("audio_track")] = m_audioTrack;
            sendCommand(rcp::ipc::CommandType::OpenMedia, open);
        }
        break;
    case rcp::ipc::EventType::MediaOpened:
        break;
    case rcp::ipc::EventType::Overload:
        emit overloadDetected(p.value(QStringLiteral("rtf")).toDouble(0.0));
        break;
    case rcp::ipc::EventType::CaptionPartial:
        if (isStaleEvent(env)) break;
        emit captionSegment(buildSegment(env, true), true);
        break;
    case rcp::ipc::EventType::CaptionFinal:
        if (isStaleEvent(env)) break;
        emit captionSegment(buildSegment(env, false), false);
        break;
    case rcp::ipc::EventType::Error:
        emit workerError(p.value(QStringLiteral("message")).toString());
        break;
    case rcp::ipc::EventType::Ack:
        // 命令回执：失败回执升级为 workerError，成功回执仅忽略。
        if (!p.value(QStringLiteral("ok")).toBool(true)) {
            emit workerError(QStringLiteral("worker 命令回执失败：%1：%2")
                                 .arg(p.value(QStringLiteral("command")).toString(),
                                      p.value(QStringLiteral("message")).toString()));
        }
        break;
    case rcp::ipc::EventType::Heartbeat:
        m_lastHeartbeatMs = m_alive.elapsed();   // B4：失联判定基准
        break;
    default:
        break;
    }
}

// B4：worker 意外退出/心跳失联后自动重启（≤3 次，1s/2s/4s 指数退避）。
// 主动 shutdown（m_intentionalStop）不触发。
void WorkerSupervisor::scheduleRestart() {
    if (m_intentionalStop) return;
    if (!m_openSent || m_mediaPath.isEmpty()) return;   // 无活动会话
    if (++m_restartAttempts > 3) {
        emit workerError(QStringLiteral("worker 自动重启失败（已达 3 次上限）"));
        return;
    }
    const int backoffMs = 1000 << (m_restartAttempts - 1);
    rcpTrace(QStringLiteral("worker restart #%1 in %2ms").arg(m_restartAttempts).arg(backoffMs));
    const QString exe = m_workerExe, media = m_mediaPath, models = m_modelsRoot;
    const int track = m_audioTrack;
    const quint64 gen = m_generation;
    QTimer::singleShot(backoffMs, this, [this, exe, media, models, track, gen] {
        if (m_intentionalStop) return;
        start(exe, media, models, track, gen);
    });
}

void WorkerSupervisor::killProcess() {
    if (m_proc && m_proc->state() == QProcess::Running) {
        m_proc->kill();
        m_proc->waitForFinished(3000);
    }
}

bool WorkerSupervisor::isStaleEvent(const rcp::ipc::Envelope& env) const {
    // generation 以主进程为权威：事件代早于当前代 = 旧会话残留，直接丢弃，
    // 防止 seek/换轨后旧媒体字幕混入新媒体（tech plan 7.4）。
    return env.generation != 0 && env.generation < m_generation;
}

rcp::CaptionSegment WorkerSupervisor::buildSegment(const rcp::ipc::Envelope& env, bool isPartial) {
    const QJsonObject& p = env.payload;
    rcp::CaptionSegment seg;
    seg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    seg.text = p.value(QStringLiteral("text")).toString();
    seg.startMs = p.value(QStringLiteral("start_ms")).toInteger();
    seg.endMs = p.value(QStringLiteral("end_ms")).toInteger();
    seg.kind = isPartial ? rcp::CaptionKind::Partial : rcp::CaptionKind::Final;
    // 事件自带 generation（worker 采用主进程下发的代），不再改写。
    seg.generation = env.generation != 0 ? env.generation : m_generation;
    return seg;
}

void WorkerSupervisor::sendCommand(rcp::ipc::CommandType t, const QJsonObject& payload) {
    if (!m_sock || m_sock->state() != QLocalSocket::ConnectedState) return;
    rcp::ipc::Envelope env;
    env.version = rcp::ipc::kProtocolVersion;
    env.type = rcp::ipc::wireTypeForCommand(t);
    env.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    env.generation = m_generation;
    env.payload = payload;
    const QByteArray frame = rcp::ipc::JsonMessageCodec::serialize(env);
    m_sock->write(frame);
}

void WorkerSupervisor::setPlayheadMs(qint64 ms) {
    QJsonObject p; p[QStringLiteral("playhead_ms")] = static_cast<qint64>(ms);
    sendCommand(rcp::ipc::CommandType::SetPlayhead, p);
}

void WorkerSupervisor::setPaused(bool paused) {
    QJsonObject p; p[QStringLiteral("paused")] = paused;
    sendCommand(rcp::ipc::CommandType::SetPaused, p);
}

void WorkerSupervisor::setSpeed(double speed) {
    QJsonObject p; p[QStringLiteral("speed")] = speed;
    sendCommand(rcp::ipc::CommandType::SetSpeed, p);
}

void WorkerSupervisor::seek(qint64 targetMs, quint64 generation) {
    m_generation = generation;
    QJsonObject p; p[QStringLiteral("target_ms")] = static_cast<qint64>(targetMs);
    sendCommand(rcp::ipc::CommandType::Seek, p);
}

void WorkerSupervisor::setAudioTrack(int ffIndex, quint64 generation) {
    m_generation = generation;
    m_audioTrack = ffIndex;
    QJsonObject p; p[QStringLiteral("audio_track")] = ffIndex;
    sendCommand(rcp::ipc::CommandType::SetAudioTrack, p);
}

void WorkerSupervisor::setLanguage(const QString& lang, quint64 generation) {
    m_generation = generation;
    QJsonObject p; p[QStringLiteral("language")] = lang;
    sendCommand(rcp::ipc::CommandType::SetLanguage, p);
}

void WorkerSupervisor::setProfile(const QString& profile, quint64 generation) {
    m_generation = generation;
    QJsonObject p; p[QStringLiteral("profile")] = profile;
    sendCommand(rcp::ipc::CommandType::SetProfile, p);
}

void WorkerSupervisor::stop() {
    sendCommand(rcp::ipc::CommandType::StopCaptioning, QJsonObject{});
    sendCommand(rcp::ipc::CommandType::CloseMedia, QJsonObject{});
    emit captioningStopped();
}

void WorkerSupervisor::shutdown() {
    m_intentionalStop = true;   // 主动关闭：不触发自动重启
    sendCommand(rcp::ipc::CommandType::Shutdown, QJsonObject{});
    killProcess();
    if (m_proc) {
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    if (m_sock) { m_sock->deleteLater(); m_sock = nullptr; }
    m_connected = false;
    m_openSent = false;
}

void WorkerSupervisor::onProcessError(QProcess::ProcessError err) {
    Q_UNUSED(err);
    emit workerError(QStringLiteral("worker 进程错误：%1")
                       .arg(m_proc ? m_proc->errorString() : QString()));
}

void WorkerSupervisor::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_connected = false;
    // B4 修复：openSent 不得在 scheduleRestart 之前清除——否则意外退出后
    // 自动重启的"有活动会话"检查永远失败（自动重启形同虚设）。
    emit workerFinished(exitCode, status);
    scheduleRestart();
    m_openSent = false;
}

bool WorkerSupervisor::isRunning() const {
    return m_proc && m_proc->state() == QProcess::Running;
}

} // namespace rcp::player
