// src/worker/IpcServer.h
// caption-worker 的 IPC 服务端：通过 QLocalSocket 接收主进程命令
// （Hello/OpenMedia/SetPlayhead/Seek/SetPaused/SetSpeed/SetAudioTrack/...），
// 回报事件（Ready/MediaOpened/CaptionPartial/CaptionFinal/Ack/Error/Heartbeat）。
//
// B1 协议加固：入站走 FrameDecoder 安全分帧（超限/非法长度即断开）、封包
// version/type/id 必填校验、未知命令回 Error（禁止当 Hello）、状态类命令回 Ack。
// generation 以主进程为权威：会话级命令（OpenMedia/Seek/SetAudioTrack/
// SetLanguage/SetProfile）携带的新 generation 被 worker 采用，后续事件原样
// 带回，两端据此丢弃旧代会话的事件。
#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QJsonObject>

#include "audio/AudioExtractor.h"
#include "asr/AsrEngine.h"
#include "ipc/FrameCodec.h"
#include "ipc/Protocol.h"

namespace rcp::worker {

class IpcServer : public QObject {
    Q_OBJECT
public:
    explicit IpcServer(QObject* parent = nullptr);
    bool listen(const QString& name);
    void setModelsRoot(const QString& r);   // 模型根目录（含 zipformer-ctc/silero/sensevoice）

signals:
    void captionReady(const rcp::asr::CaptionUtterance& u);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketError();
    void onCaptionReady(const rcp::asr::CaptionUtterance& u);

private:
    void handleCommand(const rcp::ipc::Envelope& env);
    void sendEvent(rcp::ipc::EventType t, const QJsonObject& payload, quint64 generation);
    void sendAck(const rcp::ipc::Envelope& cmd, bool ok, const QString& message);
    void startCaptioning(const QString& path, int audioTrack, quint64 generation);

    QLocalServer*   m_server = nullptr;
    QLocalSocket*   m_sock = nullptr;
    QTimer*         m_heartbeat = nullptr;
    rcp::ipc::FrameDecoder m_decoder;   // 入站安全分帧（1 MiB 上限/非法长度断开）

    rcp::audio::AudioExtractor m_ex;
    rcp::asr::AsrEngine        m_engine;
    QString m_modelsRoot = QStringLiteral(".tools/models");
    bool m_running = false;
    quint64 m_generation = 0;           // 当前会话代（主进程为权威，命令带回）
};

} // namespace rcp::worker
