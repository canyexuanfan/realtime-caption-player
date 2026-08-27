// src/player/WorkerSupervisor.h
// 主进程侧 IPC 客户端（P4/IPC 之主进程侧）：启动后台 caption_worker.exe，
// 经 QLocalSocket 发送命令（Hello/OpenMedia/...），接收字幕事件
// （Ready/MediaOpened/CaptionPartial/CaptionFinal/Error/Heartbeat），
// 转成 rcp::CaptionSegment 经信号回报给 MainWindow / CaptionController。
#pragma once

#include <QObject>
#include <QProcess>
#include <QLocalSocket>
#include "ipc/FrameCodec.h"
#include "ipc/Protocol.h"
#include "captions/CaptionTypes.h"

namespace rcp::player {

class WorkerSupervisor : public QObject {
    Q_OBJECT
public:
    explicit WorkerSupervisor(QObject* parent = nullptr);

    // 启动后台 caption-worker 并对 mediaPath 全速识别。
    // workerExe : caption_worker.exe 绝对路径
    // modelsRoot: 模型根目录（其下应有 zipformer-ctc / silero / sensevoice）
    // audioTrack: mpv 音轨索引（-1 = 默认首条）
    bool start(const QString& workerExe, const QString& mediaPath,
               const QString& modelsRoot, int audioTrack = -1);

    void stop();       // 停止当前识别（保留进程）
    void shutdown();   // 终止 worker 进程
    bool isRunning() const;

signals:
    void sessionStarted(quint64 generation);                       // 新会话开始（控制器应 reset）
    void captionSegment(const rcp::CaptionSegment& seg, bool isPartial);
    void ready();
    void captioningStopped();
    void workerError(const QString& message);
    void workerFinished(int exitCode, QProcess::ExitStatus status);

private slots:
    void onProcessStarted();
    void onProcessError(QProcess::ProcessError err);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onConnected();
    void onSocketError(QLocalSocket::LocalSocketError err);
    void onReadyRead();

private:
    void tryConnect();
    void sendCommand(rcp::ipc::CommandType t, const QJsonObject& payload);
    void readFrames();
    void handleEvent(const rcp::ipc::Envelope& env);
    rcp::CaptionSegment buildSegment(const QJsonObject& p, bool isPartial);

    QProcess* m_proc = nullptr;
    QLocalSocket* m_sock = nullptr;
    rcp::ipc::FrameDecoder m_decoder;
    QString m_serverName;
    QString m_mediaPath;
    QString m_modelsRoot;
    int m_audioTrack = -1;
    quint64 m_generation = 0;
    bool m_openSent = false;
    bool m_connected = false;
    int m_connectTries = 0;
};

} // namespace rcp::player
