// src/worker/IpcServer.cpp
#include "IpcServer.h"

#include "ipc/JsonMessageCodec.h"
#include "ipc/Protocol.h"

#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <QUuid>

namespace rcp::worker {

IpcServer::IpcServer(QObject* parent) : QObject(parent) {
    m_engine.setCaptionCallback([this](const rcp::asr::CaptionUtterance& u) {
        emit captionReady(u);
    });
    connect(this, &IpcServer::captionReady, this, &IpcServer::onCaptionReady);

    m_heartbeat = new QTimer(this);
    m_heartbeat->setInterval(5000);
    connect(m_heartbeat, &QTimer::timeout, this, [this]() {
        if (m_sock) sendEvent(rcp::ipc::EventType::Heartbeat, QJsonObject{});
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
    if (m_sock) { m_sock->disconnectFromServer(); delete m_sock; }
    m_sock = sock;
    connect(m_sock, &QLocalSocket::readyRead, this, &IpcServer::onReadyRead);
    connect(m_sock, &QLocalSocket::errorOccurred, this, &IpcServer::onSocketError);
}

void IpcServer::onSocketError() {
    if (m_sock) { m_sock->deleteLater(); m_sock = nullptr; }
}

void IpcServer::onReadyRead() {
    if (!m_sock) return;
    m_inbuf.append(m_sock->readAll());
    readFrames();
}

// 帧格式：[uint32 LE 长度][UTF-8 JSON]，长度前缀已在 serialize 中写入。
void IpcServer::readFrames() {
    while (true) {
        if (m_inbuf.size() < 4) return;
        quint32 len = 0;
        memcpy(&len, m_inbuf.constData(), 4);
        if (static_cast<quint32>(m_inbuf.size()) < len + 4) return;
        QByteArray body = m_inbuf.mid(4, static_cast<int>(len));
        m_inbuf.remove(0, static_cast<int>(len) + 4);

        auto parsed = rcp::ipc::JsonMessageCodec::parse(body);
        if (parsed.isError()) {
            sendEvent(rcp::ipc::EventType::Error, QJsonObject{{"message", "malformed envelope"}});
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
        sendEvent(rcp::ipc::EventType::Ready, QJsonObject{});
        m_heartbeat->start();
        break;
    case rcp::ipc::CommandType::OpenMedia: {
        const QString path = p.value("path").toString();
        const int track = p.value("audio_track").toInt(-1);
        startCaptioning(path, track);
        break;
    }
    case rcp::ipc::CommandType::CloseMedia:
    case rcp::ipc::CommandType::StopCaptioning:
        m_running = false;
        break;
    case rcp::ipc::CommandType::Shutdown:
        QCoreApplication::quit();
        break;
    default:
        // SetPlayhead/SetPaused/SetSpeed/SetAudioTrack/SetLanguage/SetProfile/LoadModelBundle
        // 在 MVP 同步模型下暂以全速预识别处理，忽略实时控制（真机调优）。
        break;
    }
}

void IpcServer::startCaptioning(const QString& path, int audioTrack) {
    if (m_running) return;
    if (!m_ex.open(path, audioTrack)) {
        sendEvent(rcp::ipc::EventType::Error,
                  QJsonObject{{"message", "open failed: " + m_ex.lastError()}});
        return;
    }
    if (!m_engine.load(m_modelsRoot + "/zipformer-ctc", m_modelsRoot + "/silero", m_modelsRoot + "/sensevoice")) {
        sendEvent(rcp::ipc::EventType::Error,
                  QJsonObject{{"message", "asr load failed: " + m_engine.lastError()}});
        return;
    }
    sendEvent(rcp::ipc::EventType::MediaOpened, QJsonObject{{"path", path}});

    m_running = true;
    // 在后台线程全速抽取+识别，事件经 captionReady 信号回到主线程汇报。
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
              payload);
}

void IpcServer::sendEvent(rcp::ipc::EventType t, const QJsonObject& payload) {
    if (!m_sock) return;
    rcp::ipc::Envelope env;
    env.version = rcp::ipc::kProtocolVersion;
    env.type = rcp::ipc::wireTypeForEvent(t);
    env.id = QUuid::createUuid().toString();
    env.generation = 1;
    env.payload = payload;
    const QByteArray frame = rcp::ipc::JsonMessageCodec::serialize(env);
    m_sock->write(frame);
    m_sock->flush();
}

} // namespace rcp::worker
