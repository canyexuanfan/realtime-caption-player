// src/worker/IpcServer.h
// caption-worker 的 IPC 服务端（P4/IPC）：通过 QLocalSocket 接收主进程命令
// （Hello/OpenMedia/SetPlayhead/SetPaused/SetSpeed/StartCaptioning/...），
// 回报事件（Ready/MediaOpened/CaptionPartial/CaptionFinal/Heartbeat/Error）。
//
// 同步模型（MVP 简化版）：收到 OpenMedia 后全速抽取并识别整段媒体，
// 字幕事件携带绝对时间戳，由主进程按播放头显示。精确"跟随播放头抽音"的
// 实时同步在真机调优（沙箱无法验证播放，标注 PARTIAL）。
#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QJsonObject>

#include "audio/AudioExtractor.h"
#include "asr/AsrEngine.h"
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
    void sendEvent(rcp::ipc::EventType t, const QJsonObject& payload);
    void startCaptioning(const QString& path, int audioTrack);
    void readFrames();

    QLocalServer*   m_server = nullptr;
    QLocalSocket*   m_sock = nullptr;
    QTimer*         m_heartbeat = nullptr;
    QByteArray      m_inbuf;            // 入站字节缓冲（含长度前缀帧）

    rcp::audio::AudioExtractor m_ex;
    rcp::asr::AsrEngine        m_engine;
    QString m_modelsRoot = QStringLiteral(".tools/models");
    bool m_running = false;
};

} // namespace rcp::worker
