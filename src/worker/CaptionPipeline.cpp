// src/worker/CaptionPipeline.cpp
#include "CaptionPipeline.h"

#include <chrono>
#include <cmath>

namespace rcp::worker {

using clock_t = std::chrono::steady_clock;

namespace {
inline constexpr double kMinLookAheadSec = 30.0;   // tech plan 7.7：30–120s 提前量
inline constexpr double kMaxLookAheadSec = 120.0;
inline constexpr std::chrono::milliseconds kIdleSleep{50};
inline constexpr std::chrono::milliseconds kPausedSleep{120};
inline constexpr qint64 kFinalTimeoutMs = 2500;    // 终稿超时（SegmentAssembler）
inline constexpr int kOverloadStreakLimit = 50;    // 连续超阈值块数 → 过载事件
inline constexpr double kOverloadRtfThreshold = 1.5;
} // namespace

CaptionPipeline::CaptionPipeline(QObject* parent) : QObject(parent) {
    engine_.setCaptionCallback([this](const rcp::asr::CaptionUtterance& u) {
        handleUtterance(u);
    });
}

CaptionPipeline::~CaptionPipeline() { stopThreads(); }

bool CaptionPipeline::start(const QString& mediaPath, int audioTrack, quint64 generation) {
    stopThreads();

    mediaPath_ = mediaPath;
    audioTrack_ = audioTrack;
    state_.setGeneration(generation);
    pcm_.reset();
    outQueue_.reset();
    cancel_ = rcp::CancellationSource();
    segSeq_ = 0;

    engine_.setProfile(profile_);
    if (!ex_.open(mediaPath, audioTrack)) {
        emit pipelineError(QStringLiteral("open failed: %1").arg(ex_.lastError()));
        return false;
    }
    if (!engine_.load(modelsRoot_ + "/zipformer-ctc", modelsRoot_ + "/silero",
                      modelsRoot_ + "/sensevoice")) {
        emit pipelineError(QStringLiteral("asr load failed: %1").arg(engine_.lastError()));
        return false;
    }
    assembler_.reset(generation);
    overloadLatched_ = false;
    hasMedia_ = true;
    running_ = true;
    threadsActive_ = true;
    decodeThread_ = std::thread([this] { decodeLoop(); });
    asrThread_ = std::thread([this] { asrLoop(); });
    emit mediaOpened(mediaPath);
    return true;
}

void CaptionPipeline::stop() {
    stopThreads();
}

void CaptionPipeline::updatePlayback(qint64 playheadMs, double speed, bool paused) {
    state_.update(playheadMs, speed, paused);
}

void CaptionPipeline::seek(qint64 playheadMs, quint64 generation) {
    const auto st = state_.snapshot();
    state_.update(playheadMs, st.speed, st.paused);
    state_.setGeneration(generation);
}

void CaptionPipeline::setLanguage(const QString& lang, quint64 generation) {
    if (lang == language_) return;
    language_ = lang;
    if (hasMedia_) start(mediaPath_, audioTrack_, generation);  // 引擎重载需重建会话
}

void CaptionPipeline::setProfile(const QString& profile, quint64 generation) {
    const auto p = (profile.compare(QStringLiteral("lite"), Qt::CaseInsensitive) == 0)
                       ? asr::AsrEngine::Profile::Lite
                       : asr::AsrEngine::Profile::Balanced;
    if (p == profile_) return;
    profile_ = p;
    if (hasMedia_) start(mediaPath_, audioTrack_, generation);
}

void CaptionPipeline::decodeLoop() {
    const rcp::CancellationToken tok = cancel_.token();
    double decodedPosSec = -1.0;   // 已解码到的媒体位置（下一个块将从此产出）
    quint64 localGen = state_.snapshot().generation;

    while (!tok.isCanceled()) {
        const auto st = state_.snapshot();
        if (st.generation != localGen) {
            // 换代（seek/换轨/重载）：当前窗口作废，从新播放头重锚。
            localGen = st.generation;
            decodedPosSec = -1.0;
        }
        if (!running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(kIdleSleep);
            continue;
        }
        if (st.paused) {
            std::this_thread::sleep_for(kPausedSleep);
            continue;
        }

        const double playheadSec = static_cast<double>(st.playheadMs) / 1000.0;
        // 倍速越快提前量越大：30s * speed，夹在 30–120s。
        double lookAhead = kMinLookAheadSec * st.speed;
        lookAhead = std::min(kMaxLookAheadSec, std::max(kMinLookAheadSec, lookAhead));

        // 窗口外（播放头跳过缓冲起点或越过缓冲终点）→ seek 重锚。
        if (decodedPosSec < 0.0 ||
            decodedPosSec < playheadSec - 0.25 ||
            decodedPosSec > playheadSec + lookAhead + 5.0) {
            if (!ex_.seekToSec(std::max(0.0, playheadSec))) {
                emit pipelineError(QStringLiteral("seek failed: %1").arg(ex_.lastError()));
                std::this_thread::sleep_for(std::chrono::milliseconds{500});
                continue;
            }
            decodedPosSec = std::max(0.0, playheadSec);
        }
        if (decodedPosSec >= playheadSec + lookAhead) {
            // 窗口已满：等待播放头推进（真机下播放头由 SetPlayhead 推进）。
            std::this_thread::sleep_for(kIdleSleep);
            continue;
        }

        // 解码单个 chunk：拿到一块即停（下一次循环继续），保证循环内响应
        // 暂停/seek/取消。extract 的块回调以 PTS 锚定绝对时间。
        rcp::CancellationSource one;
        PcmChunk ch;
        ch.generation = localGen;
        bool got = false;
        ex_.extract(0.1, [&](const float* s, int n, double tSec) {
            if (got) return;
            ch.data.assign(s, s + static_cast<size_t>(n));
            ch.startMs = static_cast<long long>(std::llround(tSec * 1000.0));
            got = true;
            one.cancel();
        }, one.token());
        if (tok.isCanceled()) break;
        if (!got || ch.data.empty()) {
            // EOF 或无可解码数据：等待播放头/暂停态变化，避免空转打满 CPU。
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            continue;
        }
        decodedPosSec = static_cast<double>(ch.startMs + ch.data.size() * 1000LL / 16000) / 1000.0;
        if (!pcm_.push(std::move(ch), tok)) break;  // 关闭/取消
    }
}

void CaptionPipeline::asrLoop() {
    const rcp::CancellationToken tok = cancel_.token();
    quint64 asrGen = state_.snapshot().generation;
    int overloadStreak = 0;

    while (!tok.isCanceled()) {
        auto chunk = pcm_.pop(tok);
        if (!chunk) break;                       // 关闭/取消
        const auto st = state_.snapshot();
        if (chunk->generation != st.generation) continue;  // 旧代块直接丢弃

        if (asrGen != st.generation) {
            // 换代：重置流式引擎与组装器，旧会话的部分结果作废。
            engine_.resetForSeek();
            assembler_.reset(st.generation);
            asrGen = st.generation;
            overloadStreak = 0;
        }

        const auto t0 = clock_t::now();
        engine_.feed(chunk->data.data(), static_cast<int>(chunk->data.size()), chunk->startMs);
        // 终稿超时 tick：以媒体时间为准（2.5s 未落定则强制提交最优 partial）。
        const auto tick = assembler_.tick(chunk->startMs, kFinalTimeoutMs);
        if (tick.action == AssembleResult::Action::Committed) {
            pushOutbound(tick.segment);
            pendingId_.clear();
        }
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - t0).count();

        // 过载检测（tech plan 8.6）：块时长 100ms，ASR 消耗超其 1.5 倍持续出现即过载。
        const double chunkMs = static_cast<double>(chunk->data.size()) * 1000.0 / 16000.0;
        const double r = chunkMs > 0 ? static_cast<double>(elapsedMs) / chunkMs : 0.0;
        rtf_.store(rtf_.load(std::memory_order_relaxed) * 0.9 + r * 0.1, std::memory_order_relaxed);
        if (r > kOverloadRtfThreshold) {
            if (++overloadStreak == kOverloadStreakLimit) {
                overloadLatched_ = true;
                emit overloadDetected(rtf_.load(std::memory_order_relaxed));
            }
        } else if (overloadStreak > 0) {
            --overloadStreak;
        }
    }
}

void CaptionPipeline::stopThreads() {
    if (threadsActive_.exchange(false)) {
        cancel_.cancel();
        pcm_.shutdown();
        if (decodeThread_.joinable()) decodeThread_.join();
        if (asrThread_.joinable()) asrThread_.join();
    }
    cancel_ = rcp::CancellationSource();
    running_ = false;
    hasMedia_ = false;
    ex_.close();
}

void CaptionPipeline::handleUtterance(const rcp::asr::CaptionUtterance& u) {
    // 仅 ASR 线程调用。稳定 segment id：同一未决语句的 partial 与其 final 共用 id。
    CaptionSegment seg;
    seg.generation = state_.snapshot().generation;
    seg.startMs = u.startMs;
    seg.endMs = u.endMs;
    seg.text = u.text;
    seg.confidence = -1.0;
    seg.language = u.language;

    if (u.isPartial) {
        seg.kind = rcp::CaptionKind::Partial;
        if (!assembler_.hasPending() || pendingId_.isEmpty()) {
            pendingId_ = QStringLiteral("g%1-u%2").arg(seg.generation).arg(++segSeq_);
        }
        seg.id = pendingId_;
        assembler_.addPartial(seg);
        pushOutbound(seg);   // 限频/去重已在引擎内完成
        return;
    }

    // 终稿：沿用 pending 的 id（若有），否则分配新 id。
    seg.kind = rcp::CaptionKind::Final;
    seg.id = (assembler_.hasPending() && !pendingId_.isEmpty())
                 ? pendingId_
                 : QStringLiteral("g%1-u%2").arg(seg.generation).arg(++segSeq_);
    const auto res = assembler_.addFinal(seg);
    if (res.action == AssembleResult::Action::Committed) {
        pushOutbound(res.segment);
        pendingId_.clear();
    }
}

void CaptionPipeline::pushOutbound(const CaptionSegment& seg) {
    if (!outQueue_.push(seg)) {
        // 队列关闭（停止）或取消：丢弃（停止场景正常）。
    }
}

} // namespace rcp::worker
