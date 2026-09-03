// src/player/WorkerSupervisor.cpp
#include "WorkerSupervisor.h"

#include "ipc/JsonMessageCodec.h"
#include "ipc/Protocol.h"

#include <QCoreApplication>
#include <QLocalSocket>
#include <QUuid>
#include <QTimer>

namespace rcp::player {

WorkerSupervisor::WorkerSupervisor(QObject* parent) : QObject(parent) {}

bool WorkerSupervisor::start(const QString& workerExe, const QString& mediaPath,
                             const QString& modelsRoot, int audioTrack) {
    if (isRunning()) shutdown();

    m_mediaPath = mediaPath;
    m_modelsRoot = modelsRoot;
    m_audioTrack = audioTrack;
    m_generation += 1;
    m_openSent = false;
    m_connected = false;
    m_connectTries = 0;
    m_lastSocketError.clear();
    m_serverName = QStringLiteral("rcp-caption-") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(QCoreApplication::applicationDirPath());
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
    return true;
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
    case rcp::ipc::EventType::CaptionPartial:
        emit captionSegment(buildSegment(p, true), true);
        break;
    case rcp::ipc::EventType::CaptionFinal:
        emit captionSegment(buildSegment(p, false), false);
        break;
    case rcp::ipc::EventType::Error:
        emit workerError(p.value(QStringLiteral("message")).toString());
        break;
    case rcp::ipc::EventType::Heartbeat:
    default:
        break;
    }
}

rcp::CaptionSegment WorkerSupervisor::buildSegment(const QJsonObject& p, bool isPartial) {
    rcp::CaptionSegment seg;
    seg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    seg.text = p.value(QStringLiteral("text")).toString();
    seg.startMs = p.value(QStringLiteral("start_ms")).toInteger();
    seg.endMs = p.value(QStringLiteral("end_ms")).toInteger();
    seg.kind = isPartial ? rcp::CaptionKind::Partial : rcp::CaptionKind::Final;
    seg.generation = m_generation;
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

void WorkerSupervisor::stop() {
    sendCommand(rcp::ipc::CommandType::StopCaptioning, QJsonObject{});
    sendCommand(rcp::ipc::CommandType::CloseMedia, QJsonObject{});
    emit captioningStopped();
}

void WorkerSupervisor::shutdown() {
    sendCommand(rcp::ipc::CommandType::Shutdown, QJsonObject{});
    if (m_proc) {
        m_proc->terminate();
        if (!m_proc->waitForFinished(2000)) m_proc->kill();
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    if (m_sock) { m_sock->deleteLater(); m_sock = nullptr; }
    m_connected = false;
}

void WorkerSupervisor::onProcessError(QProcess::ProcessError err) {
    Q_UNUSED(err);
    emit workerError(QStringLiteral("worker 进程错误：%1")
                       .arg(m_proc ? m_proc->errorString() : QString()));
}

void WorkerSupervisor::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_connected = false;
    emit workerFinished(exitCode, status);
}

bool WorkerSupervisor::isRunning() const {
    return m_proc && m_proc->state() == QProcess::Running;
}

} // namespace rcp::player
