// src/worker/IpcServer.h
// caption-worker 的 IPC 服务端：通过 QLocalSocket 接收主进程命令，
// 回报事件（Ready/MediaOpened/CaptionPartial/CaptionFinal/Ack/Overload/Error/Heartbeat）。
//
// B1 协议加固：入站走 FrameDecoder 安全分帧（超限/非法长度即断开）、封包必填校验、
// 未知命令回 Error、状态类命令回 Ack。
// B2 实时管线：字幕产出交给 CaptionPipeline（解码调度 + ASR 线程 + 有界队列），
// SetPlayhead/SetPaused/SetSpeed/Seek 实时生效；出站事件经 QTimer 在主线程排空。
// generation 以主进程为权威：会话级命令（OpenMedia/Seek/SetAudioTrack/
// SetLanguage/SetProfile）携带的新 generation 被 worker 采用，事件原样带回。
#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QJsonObject>

#include "ipc/FrameCodec.h"
#include "ipc/Protocol.h"
#include "worker/CaptionPipeline.h"

namespace rcp::worker {

class IpcServer : public QObject {
    Q_OBJECT
public:
    explicit IpcServer(QObject* parent = nullptr);
    bool listen(const QString& name);
    void setModelsRoot(const QString& r);   // 模型根目录（含 zipformer-ctc/silero/sensevoice）

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketError();
    void drainOutbound();

private:
    void handleCommand(const rcp::ipc::Envelope& env);
    void sendEvent(rcp::ipc::EventType t, const QJsonObject& payload, quint64 generation);
    void sendAck(const rcp::ipc::Envelope& cmd, bool ok, const QString& message);

    QLocalServer*   m_server = nullptr;
    QLocalSocket*   m_sock = nullptr;
    QTimer*         m_heartbeat = nullptr;
    QTimer*         m_drainTimer = nullptr;
    rcp::ipc::FrameDecoder m_decoder;   // 入站安全分帧（1 MiB 上限/非法长度断开）

    CaptionPipeline* m_pipeline = nullptr;
    QString m_modelsRoot = QStringLiteral(".tools/models");

    // 最近播放状态（连续控制命令合成 pipeline.updatePlayback 输入）。
    QString m_mediaPath;
    int     m_audioTrack = -1;
    qint64  m_playheadMs = 0;
    double  m_speed = 1.0;
    bool    m_paused = false;
    quint64 m_generation = 0;           // 当前会话代（主进程为权威，命令带回）
};

} // namespace rcp::worker
