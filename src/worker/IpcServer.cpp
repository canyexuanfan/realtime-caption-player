// src/worker/IpcServer.cpp
#include "IpcServer.h"

#include "ipc/JsonMessageCodec.h"
#include "ipc/Protocol.h"

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QUuid>

namespace rcp::worker {

IpcServer::IpcServer(QObject* parent) : QObject(parent) {
    m_engine.setCaptionCallback([this](const rcp::asr::CaptionUtterance& u) {
        emit captionReady(u);
    });
    connect(this, &IpcServer::captionReady, this, &IpcServer::onCaptionReady);

    // 心跳 2s：主进程按"3 次未到即失联"判定（B4 生命周期）。
    m_heartbeat = new QTimer(this);
    m_heartbeat->setInterval(2000);
    connect(m_heartbeat, &QTimer::timeout, this, [this]() {
        if (m_sock) sendEvent(rcp::ipc::EventType::Heartbeat, QJsonObject{}, m_generation);
    });
}

bool IpcServer::listen(const QString& name) {
    m_server = new QLocalServer(this);
    if (!m_server->listen(name)) return false;
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
    return true;
}

void IpcServer::setModelsRoot(const QString& r) {
    m_modelsRoot = r;
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
        const int track = p.value(QStringLiteral("audio_track")).toInt(-1);
        if (path.isEmpty()) {
            sendAck(env, false, QStringLiteral("open_media: path is required"));
            break;
        }
        // 会话级命令：采用主进程下发的 generation（B2 起驱动管线重建）。
        if (env.generation != 0) m_generation = env.generation;
        sendAck(env, true, {});
        startCaptioning(path, track, m_generation);
        break;
    }
    case rcp::ipc::CommandType::Seek:
        if (env.generation != 0) m_generation = env.generation; // 旧代事件将被两端丢弃
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::SetAudioTrack:
    case rcp::ipc::CommandType::SetLanguage:
    case rcp::ipc::CommandType::SetProfile:
        if (env.generation != 0) m_generation = env.generation;
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::CloseMedia:
    case rcp::ipc::CommandType::StopCaptioning:
        m_running = false;
        sendAck(env, true, {});
        break;
    case rcp::ipc::CommandType::Shutdown:
        sendAck(env, true, {});
        QCoreApplication::quit();
        break;
    case rcp::ipc::CommandType::Unknown:
        // 未识别命令：显式报错回执（B1 前会被误当 Hello 回 Ready）。
        sendAck(env, false, QStringLiteral("unknown command: %1").arg(env.type));
        break;
    default:
        // SetPlayhead/SetPaused/SetSpeed/LoadModelBundle/StartCaptioning：
        // B1 仍为全速预识别模型下的空操作；B2 管线接管后转为实时控制。
        sendAck(env, true, {});
        break;
    }
}

void IpcServer::startCaptioning(const QString& path, int audioTrack, quint64 generation) {
    if (m_running) return;
    if (!m_ex.open(path, audioTrack)) {
        sendEvent(rcp::ipc::EventType::Error,
                  QJsonObject{{"message", "open failed: " + m_ex.lastError()}}, generation);
        return;
    }
    if (!m_engine.load(m_modelsRoot + "/zipformer-ctc", m_modelsRoot + "/silero", m_modelsRoot + "/sensevoice")) {
        sendEvent(rcp::ipc::EventType::Error,
                  QJsonObject{{"message", "asr load failed: " + m_engine.lastError()}}, generation);
        return;
    }
    sendEvent(rcp::ipc::EventType::MediaOpened, QJsonObject{{"path", path}}, generation);

    m_running = true;
    // B1 仍为同步全速模型：后台线程抽取+识别，Stop 仅置位（B2 管线接管后可中断）。
    QtConcurrent::run([this]() {
        m_ex.extract(0.1, [this](const float* s, int n, double /*t*/) {
            m_engine.feed(s, n);
        });
        m_engine.flush();
        m_running = false;
    });
}

void IpcServer::onCaptionReady(const rcp::asr::CaptionUtterance& u) {
    QJsonObject payload;
    payload["text"] = u.text;
    payload["start_ms"] = u.startMs;
    payload["end_ms"] = u.endMs;
    sendEvent(u.isPartial ? rcp::ipc::EventType::CaptionPartial
                          : rcp::ipc::EventType::CaptionFinal,
              payload, m_generation);
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
