// src/worker/IpcServer.cpp
#include "IpcServer.h"

#include "ipc/JsonMessageCodec.h"
#include "ipc/Protocol.h"

#include <QCoreApplication>
#include <QUuid>

namespace rcp::worker {

IpcServer::IpcServer(QObject* parent) : QObject(parent) {
    // 心跳 2s：主进程按"3 次未到即失联"判定（B4 生命周期）。
    m_heartbeat = new QTimer(this);
    m_heartbeat->setInterval(2000);
    connect(m_heartbeat, &QTimer::timeout, this, [this]() {
        if (m_sock) sendEvent(rcp::ipc::EventType::Heartbeat, QJsonObject{}, m_generation);
    });

    // B2 管线：出站字幕事件 30ms 排空（partial 限频在引擎内，无高频风暴）。
    m_pipeline = new CaptionPipeline(this);
    connect(m_pipeline, &CaptionPipeline::mediaOpened, this, [this](const QString& path) {
        sendEvent(rcp::ipc::EventType::MediaOpened, QJsonObject{{"path", path}}, m_generation);
    });
    connect(m_pipeline, &CaptionPipeline::pipelineError, this, [this](const QString& msg) {
        sendEvent(rcp::ipc::EventType::Error, QJsonObject{{"message", msg}}, m_generation);
    });
    connect(m_pipeline, &CaptionPipeline::overloadDetected, this, [this](double rtf) {
        sendEvent(rcp::ipc::EventType::Overload,
                  QJsonObject{{"rtf", rtf}}, m_generation);
    });
    m_drainTimer = new QTimer(this);
    m_drainTimer->setInterval(30);
    connect(m_drainTimer, &QTimer::timeout, this, &IpcServer::drainOutbound);
    m_drainTimer->start();
}

bool IpcServer::listen(const QString& name) {
    m_server = new QLocalServer(this);
    if (!m_server->listen(name)) return false;
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
    return true;
}

void IpcServer::setModelsRoot(const QString& r) {
    m_modelsRoot = r;
    m_pipeline->setModelsRoot(r);
}

void IpcServer::onNewConnection() {
    QLocalSocket* sock = m_server->nextPendingConnection();
    if (!sock) return;
    if (m_sock) { m_sock->disconnectFromServer(); m_sock->deleteLater(); }
    m_sock = sock;
    m_decoder.reset();
    connect(m_sock, &QLocalSocket::readyRead, this, &IpcServer::onReadyRead);
    connect(m_sock, &QLocalSocket::errorOccurred, this, &IpcServer::onSocketError);
}

void IpcServer::onSocketError() {
    if (m_sock) { m_sock->deleteLater(); m_sock = nullptr; }
}

void IpcServer::onReadyRead() {
    if (!m_sock) return;
    QList<QByteArray> frames;
    QString err;
    // 安全分帧：致命分帧错误（超 1 MiB / 非法长度）直接断开，不再解析该连接。
    if (!m_decoder.feed(m_sock->readAll(), frames, &err)) {
        sendEvent(rcp::ipc::EventType::Error, QJsonObject{{"message", err}}, m_generation);
        m_sock->disconnectFromServer();
        return;
    }
    for (const auto& f : frames) {
        auto parsed = rcp::ipc::JsonMessageCodec::parse(f);
        if (parsed.isError()) {
            sendEvent(rcp::ipc::EventType::Error,
                      QJsonObject{{"message", QStringLiteral("malformed envelope: %1").arg(parsed.error().technicalMessage)}},
                      m_generation);
            continue;
        }
        handleCommand(parsed.value());
    }
}

void IpcServer::drainOutbound() {
    if (!m_sock) return;
    CaptionSegment seg;
    while (m_pipeline->popOutbound(seg)) {
        QJsonObject payload;
        payload["id"] = seg.id;
        payload["text"] = seg.text;
        payload["start_ms"] = seg.startMs;
        payload["end_ms"] = seg.endMs;
        payload["confidence"] = seg.confidence;
        payload["language"] = seg.language;
        sendEvent(seg.kind == rcp::CaptionKind::Partial
                      ? rcp::ipc::EventType::CaptionPartial
                      : (seg.kind == rcp::CaptionKind::Revision
                             ? rcp::ipc::EventType::CaptionRevision
                             : rcp::ipc::EventType::CaptionFinal),
                  payload, seg.generation);
    }
}

void IpcServer::handleCommand(const rcp::ipc::Envelope& env) {
    if (!m_sock) return;
    const QJsonObject& p = env.payload;

    switch (rcp::ipc::commandTypeFromName(env.type)) {
    case rcp::ipc::CommandType::Hello:
        sendAck(env, true, {});
        sendEvent(rcp::ipc::EventType::Ready, QJsonObject{}, m_generation);
        m_heartbeat->start();
        break;
    case rcp::ipc::CommandType::OpenMedia: {
        const QString path = p.value(QStringLiteral("path")).toString();
        if (path.isEmpty()) {
            sendAck(env, false, QStringLiteral("open_media: path is required"));
            break;
        }
        if (env.generation != 0) m_generation = env.generation;
        m_mediaPath = path;
        m_audioTrack = p.value(QStringLiteral("audio_track")).toInt(-1);
        sendAck(env, true, {});
        m_pipeline->start(m_mediaPath, m_audioTrack, m_generation);
        break;
    }
    case rcp::ipc::CommandType::SetPlayhead:
        m_playheadMs = static_cast<qint64>(p.value(QStringLiteral("playhead_ms")).toInteger());
        m_pipeline->updatePlayback(m_playheadMs, m_speed, m_paused);
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::SetPaused:
        m_paused = p.value(QStringLiteral("paused")).toBool(false);
        m_pipeline->updatePlayback(m_playheadMs, m_speed, m_paused);
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::SetSpeed:
        m_speed = p.value(QStringLiteral("speed")).toDouble(1.0);
        if (m_speed <= 0) m_speed = 1.0;
        m_pipeline->updatePlayback(m_playheadMs, m_speed, m_paused);
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::Seek: {
        if (env.generation != 0) m_generation = env.generation;  // 旧代事件两端丢弃
        m_playheadMs = static_cast<qint64>(p.value(QStringLiteral("target_ms")).toInteger());
        m_pipeline->seek(m_playheadMs, m_generation);
        sendAck(env, true, {});
        break;
    }
    case rcp::ipc::CommandType::SetAudioTrack:
        if (env.generation != 0) m_generation = env.generation;
        m_audioTrack = p.value(QStringLiteral("audio_track")).toInt(-1);
        m_pipeline->start(m_mediaPath, m_audioTrack, m_generation);  // 换轨 = 重建会话
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::SetLanguage:
        if (env.generation != 0) m_generation = env.generation;
        m_pipeline->setLanguage(p.value(QStringLiteral("language")).toString(QStringLiteral("auto")), m_generation);
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::SetProfile:
        if (env.generation != 0) m_generation = env.generation;
        m_pipeline->setProfile(p.value(QStringLiteral("profile")).toString(QStringLiteral("balanced")), m_generation);
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::CloseMedia:
    case rcp::ipc::CommandType::StopCaptioning:
        m_pipeline->stop();
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::Shutdown:
        sendAck(env, true, {});
        m_pipeline->stop();
        QCoreApplication::quit();
        break;
    case rcp::ipc::CommandType::Unknown:
        // 未识别命令：显式报错回执（B1 前会被误当 Hello 回 Ready）。
        sendAck(env, false, QStringLiteral("unknown command: %1").arg(env.type));
        break;
    default:
        sendAck(env, true, {});
        break;
    }
}

void IpcServer::sendEvent(rcp::ipc::EventType t, const QJsonObject& payload, quint64 generation) {
    if (!m_sock) return;
    rcp::ipc::Envelope env;
    env.version = rcp::ipc::kProtocolVersion;
    env.type = rcp::ipc::wireTypeForEvent(t);
    env.id = QUuid::createUuid().toString();
    env.generation = generation;
    env.payload = payload;
    const QByteArray frame = rcp::ipc::JsonMessageCodec::serialize(env);
    m_sock->write(frame);
    m_sock->flush();
}

void IpcServer::sendAck(const rcp::ipc::Envelope& cmd, bool ok, const QString& message) {
    QJsonObject payload;
    payload["for"] = cmd.id;
    payload["command"] = cmd.type;
    payload["ok"] = ok;
    if (!message.isEmpty()) payload["message"] = message;
    sendEvent(rcp::ipc::EventType::Ack, payload, cmd.generation);
}

} // namespace rcp::worker
