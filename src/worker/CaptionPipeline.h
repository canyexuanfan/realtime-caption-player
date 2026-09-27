// src/worker/CaptionPipeline.h
// 字幕管线（B2，tech plan 7.6-7.8 / 审查阶段 B2）：
//   解码调度线程：按播放头 30–120s 提前量窗口抽音（倍速放大提前量，暂停休眠，
//                seek/换代重锚 + 重置引擎），产出带 PTS 时间戳的 PcmChunk；
//   有界队列：   PcmChunk（背压防内存膨胀）与出站字幕事件；
//   ASR 线程：   Zipformer 流式 partial（200ms 限频去重在引擎内）+ VAD 分句终稿，
//                经 SegmentAssembler（稳定 segment id + 2.5s 超时修订）出站。
// generation 以 PlaybackState 为准：旧代块直接丢弃，换代时重置引擎与组装器。
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <atomic>
#include <thread>
#include <vector>

#include "audio/AudioExtractor.h"
#include "asr/AsrEngine.h"
#include "core/Cancellation.h"
#include "captions/CaptionTypes.h"
#include "worker/BoundedQueue.h"
#include "worker/PlaybackState.h"
#include "worker/SegmentAssembler.h"

namespace rcp::worker {

struct PcmChunk {
    std::vector<float> data;      // 16k mono float
    long long startMs = 0;        // 该块起始的媒体绝对毫秒（PTS 锚定）
    quint64 generation = 0;       // 产出时的会话代
};

class CaptionPipeline : public QObject {
    Q_OBJECT
public:
    explicit CaptionPipeline(QObject* parent = nullptr);
    ~CaptionPipeline() override;

    void setModelsRoot(const QString& r) { modelsRoot_ = r; }

    // OpenMedia / 换轨 / 语言档位重载：重建管线会话（旧线程先停止）。
    bool start(const QString& mediaPath, int audioTrack, quint64 generation);
    // CloseMedia / StopCaptioning：停止解码与识别，清空队列。
    void stop();
    // 播放头/倍速/暂停（连续控制，不换代）。
    void updatePlayback(qint64 playheadMs, double speed, bool paused);
    // seek：换代 + 解码重锚 + 引擎/组装器重置。
    void seek(qint64 playheadMs, quint64 generation);
    // 语言（"auto"/"zh"/"en"）与档位（"lite"/"balanced"）：需重载引擎，走 restart。
    void setLanguage(const QString& lang, quint64 generation);
    void setProfile(const QString& profile, quint64 generation);

    // 出站字幕事件由 IPC 主线程定时排空（非阻塞）。
    bool popOutbound(CaptionSegment& out) {
        auto item = outQueue_.tryPop();
        if (!item) return false;
        out = std::move(*item);
        return true;
    }

    // 过载指标：ASR 每块耗时/块时长的平滑估计（>1 即跟不上实时）。
    double overloadRtf() const { return rtf_.load(std::memory_order_relaxed); }

    bool hasMedia() const { return hasMedia_; }

signals:
    void mediaOpened(const QString& path);
    void pipelineError(const QString& message);
    void overloadDetected(double rtf);

private:
    void decodeLoop();
    void asrLoop();
    void stopThreads();
    void handleUtterance(const rcp::asr::CaptionUtterance& u);
    void pushOutbound(const CaptionSegment& seg);

    audio::AudioExtractor ex_;
    asr::AsrEngine engine_;
    SegmentAssembler assembler_;
    PlaybackState state_;
    rcp::CancellationSource cancel_;
    BoundedQueue<PcmChunk> pcm_{600};            // ~60s @ 0.1s 块的背压上限
    BoundedQueue<CaptionSegment> outQueue_{256}; // 出站字幕事件上限

    std::thread decodeThread_;
    std::thread asrThread_;
    QString modelsRoot_ = QStringLiteral(".tools/models");
    QString mediaPath_;
    int audioTrack_ = -1;
    QString language_ = QStringLiteral("auto");
    asr::AsrEngine::Profile profile_ = asr::AsrEngine::Profile::Balanced;
    std::atomic<bool> running_{false};       // 有媒体且管线在跑
    std::atomic<bool> threadsActive_{false}; // 线程存活（join 需要等待）
    std::atomic<double> rtf_{0.0};
    std::atomic<bool> overloadLatched_{false};

    // ASR 线程私有语义（仅 asrLoop/handleUtterance 访问）：当前未决语句的稳定 id。
    quint64 segSeq_ = 0;
    QString pendingId_;
    bool hasMedia_ = false;
};

} // namespace rcp::worker
