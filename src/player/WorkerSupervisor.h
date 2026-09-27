// src/player/WorkerSupervisor.h
// 主进程侧 IPC 客户端（P4/IPC 之主进程侧）：启动后台 caption_worker.exe，
// 经 QLocalSocket 发送命令（Hello/OpenMedia/...），接收字幕事件
// （Ready/MediaOpened/CaptionPartial/CaptionFinal/Error/Heartbeat），
// 转成 rcp::CaptionSegment 经信号回报给 MainWindow / CaptionController。
#pragma once

#include <QObject>
#include <QProcess>
#include <QLocalSocket>
#include <QElapsedTimer>
#include "ipc/FrameCodec.h"
#include "ipc/Protocol.h"
#include "captions/CaptionTypes.h"

namespace rcp::player {

class WorkerSupervisor : public QObject {
    Q_OBJECT
public:
    explicit WorkerSupervisor(QObject* parent = nullptr);
    ~WorkerSupervisor() override;

    // 启动后台 caption-worker 并对 mediaPath 识别（generation 由协调器下发）。
    // workerExe : caption_worker.exe 绝对路径
    // modelsRoot: 模型根目录（其下应有 zipformer-ctc / silero / sensevoice）
    // audioTrack: ffmpeg 音频流索引（-1 = 默认首条）
    // generation: 会话代（协调器为权威；事件与命令均携带）
    bool start(const QString& workerExe, const QString& mediaPath,
               const QString& modelsRoot, int audioTrack, quint64 generation);

    // B3 实时控制（worker 端 CaptionPipeline 消费）。
    void setPlayheadMs(qint64 ms);
    void setPaused(bool paused);
    void setSpeed(double speed);
    void seek(qint64 targetMs, quint64 generation);
    void setAudioTrack(int ffIndex, quint64 generation);
    void setLanguage(const QString& lang, quint64 generation);
    void setProfile(const QString& profile, quint64 generation);

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
    void overloadDetected(double rtf);                             // worker 过载（B5 自动降级）

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
    bool isStaleEvent(const rcp::ipc::Envelope& env) const;
    rcp::CaptionSegment buildSegment(const rcp::ipc::Envelope& env, bool isPartial);
    void armWatchdog();
    void scheduleRestart();
    void killProcess();
    void assignJobObject();

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
    QString m_lastSocketError;

    // ---- B4 生命周期 ----
    QTimer* m_watchdog = nullptr;      // 1s 心跳失联检查
    QElapsedTimer m_alive;             // 启动时刻基准
    qint64 m_lastHeartbeatMs = 0;      // 最近一次 Ready/Heartbeat 时刻
    int m_restartAttempts = 0;         // 已自动重启次数（上限 3，指数退避）
    bool m_intentionalStop = false;    // 主动 shutdown 不触发自动重启
    void* m_jobHandle = nullptr;       // Windows Job Object（父进程崩溃连带杀 worker）
    QString m_workerExe;
};

} // namespace rcp::player
