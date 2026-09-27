// src/asr/AsrEngine.cpp
#include "AsrEngine.h"

extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

#include <cstring>
#include <cstdio>
#include <vector>

namespace rcp::asr {

namespace {
inline constexpr int kSampleRate = 16000;
inline constexpr long long kPartialEmitIntervalMs = 200;  // partial 限频（tech plan 8.6）
}

struct AsrEngine::Impl {
    const SherpaOnnxVoiceActivityDetector* vad = nullptr;
    const SherpaOnnxOnlineRecognizer*      zf  = nullptr;
    const SherpaOnnxOnlineStream*          zs  = nullptr;
    const SherpaOnnxOfflineRecognizer*     sv  = nullptr;
    // 重置在线流需要重建，保留创建参数副本。
    SherpaOnnxVadModelConfig vadCfg{};
    SherpaOnnxOnlineRecognizerConfig zfCfg{};
    float vadBufferDurationSec = 30.0f;
};

AsrEngine::AsrEngine() : d_(new Impl) {}

AsrEngine::~AsrEngine() {
    if (d_) {
        if (d_->zs) SherpaOnnxDestroyOnlineStream(d_->zs);
        if (d_->zf) SherpaOnnxDestroyOnlineRecognizer(d_->zf);
        if (d_->sv) SherpaOnnxDestroyOfflineRecognizer(d_->sv);
        if (d_->vad) SherpaOnnxDestroyVoiceActivityDetector(d_->vad);
        delete d_;
        d_ = nullptr;
    }
}

bool AsrEngine::load(const QString& zfDir, const QString& vadDir,
                     const QString& svDir, const QString& modelBase) {
    const int32_t sr = kSampleRate;

    // ---------- 1) Silero VAD ----------
    SherpaOnnxVadModelConfig vad_cfg;
    memset(&vad_cfg, 0, sizeof(vad_cfg));
    QByteArray vadModel = (vadDir + "/silero_vad.onnx").toUtf8();
    vad_cfg.silero_vad.model = vadModel.constData();
    vad_cfg.silero_vad.threshold = 0.5f;
    vad_cfg.silero_vad.min_silence_duration = 0.5f;
    vad_cfg.silero_vad.min_speech_duration = 0.25f;
    vad_cfg.silero_vad.max_speech_duration = 20.0f;
    vad_cfg.silero_vad.window_size = 512;  // 必须 512/1024/1536
    vad_cfg.sample_rate = sr;
    vad_cfg.num_threads = 1;
    vad_cfg.provider = "cpu";
    vad_cfg.ten_vad.model = "";            // 必须空串而非 NULL
    vad_cfg.ten_vad.threshold = 0.5f;
    vad_cfg.ten_vad.min_silence_duration = 0.5f;
    vad_cfg.ten_vad.min_speech_duration = 0.25f;
    vad_cfg.ten_vad.max_speech_duration = 20.0f;
    vad_cfg.ten_vad.window_size = 256;
    d_->vad = SherpaOnnxCreateVoiceActivityDetector(&vad_cfg, 30.0f);
    if (!d_->vad) { error_ = QStringLiteral("VAD create failed"); return false; }
    d_->vadCfg = vad_cfg;
    d_->vadBufferDurationSec = 30.0f;

    // ---------- 2) Zipformer2-CTC 在线识别器（实时 partial）----------
    SherpaOnnxOnlineRecognizerConfig zf_cfg;
    memset(&zf_cfg, 0, sizeof(zf_cfg));
    zf_cfg.feat_config.sample_rate = sr;
    zf_cfg.feat_config.feature_dim = 80;
    QByteArray zfModel = (zfDir + "/" + modelBase).toUtf8();
    QByteArray zfTokens = (zfDir + "/tokens.txt").toUtf8();
    zf_cfg.model_config.zipformer2_ctc.model = zfModel.constData();
    zf_cfg.model_config.tokens = zfTokens.constData();
    zf_cfg.model_config.provider = "cpu";
    zf_cfg.model_config.num_threads = 1;
    zf_cfg.decoding_method = "greedy_search";
    d_->zf = SherpaOnnxCreateOnlineRecognizer(&zf_cfg);
    if (!d_->zf) { error_ = QStringLiteral("Zipformer2-CTC create failed"); return false; }
    d_->zfCfg = zf_cfg;
    d_->zs = SherpaOnnxCreateOnlineStream(d_->zf);

    // ---------- 3) SenseVoice 离线识别器（精准终稿 + ITN）----------
    SherpaOnnxOfflineRecognizerConfig sv_cfg;
    memset(&sv_cfg, 0, sizeof(sv_cfg));
    sv_cfg.feat_config.sample_rate = sr;
    sv_cfg.feat_config.feature_dim = 80;
    QByteArray svModel = (svDir + "/model.int8.onnx").toUtf8();
    QByteArray svTokens = (svDir + "/tokens.txt").toUtf8();
    sv_cfg.model_config.tokens = svTokens.constData();
    sv_cfg.model_config.sense_voice.model = svModel.constData();
    sv_cfg.model_config.sense_voice.language = m_language.toUtf8().constData();
    sv_cfg.model_config.sense_voice.use_itn = 1;
    d_->sv = SherpaOnnxCreateOfflineRecognizer(&sv_cfg);
    if (!d_->sv) { error_ = QStringLiteral("SenseVoice create failed"); return false; }

    return true;
}

void AsrEngine::feed(const float* samples, int count, long long startMs) {
    if (!d_ || !d_->zf || !d_->vad || !d_->sv) return;
    const int32_t sr = kSampleRate;

    // 媒体绝对位置推进：优先以喂入块的 PTS 起始为锚；与预期偏差 >20ms 视为
    // 上游重同步（seek），直接重新锚定。startMs<0 = 文件模式累计时钟。
    long long chunkStartMs;
    if (m_streamPosMs < 0) {
        m_streamPosMs = startMs >= 0 ? startMs : 0;
    } else if (startMs >= 0 && std::llabs(startMs - m_streamPosMs) > 20) {
        m_streamPosMs = startMs;
    }
    chunkStartMs = m_streamPosMs;
    m_streamPosMs += static_cast<long long>(count) * 1000LL / sr;

    // 实时 partial（Zipformer2-CTC 逐块解码）
    const bool wasEmpty = m_currentPartialText.isEmpty();
    SherpaOnnxOnlineStreamAcceptWaveform(d_->zs, sr, samples, count);
    while (SherpaOnnxIsOnlineStreamReady(d_->zf, d_->zs)) {
        SherpaOnnxDecodeOnlineStream(d_->zf, d_->zs);
        const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(d_->zf, d_->zs);
        if (r && r->text && r->text[0]) {
            m_currentPartialText = QString::fromUtf8(r->text);
        }
        SherpaOnnxDestroyOnlineRecognizerResult(r);
        emitPartialIfDue();
    }
    // 当前语句开始点：partial 从空到非空的第一个 chunk 起点（供 partial 区间）。
    if (wasEmpty && !m_currentPartialText.isEmpty() && m_partialStartMs < 0) {
        m_partialStartMs = chunkStartMs;
    }

    // VAD 分句 -> 终稿
    SherpaOnnxVoiceActivityDetectorAcceptWaveform(d_->vad, samples, count);
    allSamples_.insert(allSamples_.end(), samples, samples + count);
    drainVad();
}

void AsrEngine::flush() {
    if (!d_ || !d_->vad) return;
    SherpaOnnxVoiceActivityDetectorFlush(d_->vad);
    drainVad();
}

void AsrEngine::resetForSeek() {
    if (!d_) return;
    // 流式识别器：销毁重建在线流，丢弃旧会话的解码上下文。
    if (d_->zs) { SherpaOnnxDestroyOnlineStream(d_->zs); d_->zs = nullptr; }
    if (d_->zf) d_->zs = SherpaOnnxCreateOnlineStream(d_->zf);
    // VAD：销毁重建（内部缓冲的未决语音段属于旧会话，无法安全续用）。
    if (d_->vad) SherpaOnnxDestroyVoiceActivityDetector(d_->vad);
    d_->vad = SherpaOnnxCreateVoiceActivityDetector(&d_->vadCfg, d_->vadBufferDurationSec);
    allSamples_.clear();
    consumed_ = 0;
    m_streamPosMs = -1;
    m_lastFinalEndMs = 0;
    m_lastPartialEmitPosMs = 0;
    m_lastPartialText.clear();
    m_currentPartialText.clear();
    m_partialStartMs = -1;
}

void AsrEngine::emitPartialIfDue() {
    if (!cb_) return;
    if (m_currentPartialText.isEmpty()) return;
    // 限频 + 去重：位置推进不足 200ms 且文本未变化时不发（审查 §1.3 限频/去重）。
    const bool textChanged = m_currentPartialText != m_lastPartialText;
    const bool dueMs = m_streamPosMs - m_lastPartialEmitPosMs >= kPartialEmitIntervalMs;
    if (!dueMs && !textChanged) return;
    CaptionUtterance u;
    u.text = m_currentPartialText;
    u.isPartial = true;
    // partial 区间 = 当前语句开始点 → 已喂入位置（B2：partial 必须带时间戳，
    // 由唯一时间线按播放头选择；不得再用"上一句终稿结束点"）。
    u.startMs = m_partialStartMs >= 0 ? m_partialStartMs : m_streamPosMs;
    u.endMs = m_streamPosMs;
    u.language = QStringLiteral("zh");  // 流式引擎为中文模型
    m_lastPartialText = m_currentPartialText;
    m_lastPartialEmitPosMs = m_streamPosMs;
    cb_(u);
}

void AsrEngine::drainVad() {
    const int32_t sr = kSampleRate;
    // 注意：必须用 Empty() 判断"已完成分句队列"。
    // Detected() 仅表示"当前处于语音内"，此时 Front() 会返回 NULL
    // （真实语音首句进行中即触发空指针解引用，见 questions/ 指南）。
    while (!SherpaOnnxVoiceActivityDetectorEmpty(d_->vad)) {
        const SherpaOnnxSpeechSegment* seg = SherpaOnnxVoiceActivityDetectorFront(d_->vad);
        if (!seg) break;
        const long long localStart = static_cast<long long>(seg->start) - consumed_;
        if (localStart < 0 || localStart + seg->n > static_cast<long long>(allSamples_.size())) {
            // 分句超出当前缓冲区（理论上不应发生），防御性跳过，避免野指针。
            SherpaOnnxVoiceActivityDetectorPop(d_->vad);
            SherpaOnnxDestroySpeechSegment(seg);
            continue;
        }
        CaptionUtterance u;
        u.isPartial = false;

        if (m_profile == Profile::Lite) {
            // Lite 档：跳过 SenseVoice，取分句结束时的 partial 快照作终稿。
            u.text = m_currentPartialText;
        } else {            const float* segSamples = allSamples_.data() + localStart;
            const SherpaOnnxOfflineStream* os = SherpaOnnxCreateOfflineStream(d_->sv);
            SherpaOnnxAcceptWaveformOffline(os, sr, segSamples, seg->n);
            SherpaOnnxDecodeOfflineStream(d_->sv, os);
            const SherpaOnnxOfflineRecognizerResult* r = SherpaOnnxGetOfflineStreamResult(os);
            u.text = (r && r->text) ? QString::fromUtf8(r->text) : QString();
            SherpaOnnxDestroyOfflineStream(os);
            if (r) SherpaOnnxDestroyOfflineRecognizerResult(r);
        }
        // seg->start 是自流开始的绝对采样索引；时间戳必须用绝对值，
        // 减 consumed_ 只用于取缓冲区内的样本指针。
        u.startMs = static_cast<long long>(seg->start) * 1000LL / sr;
        u.endMs   = (static_cast<long long>(seg->start) + seg->n) * 1000LL / sr;
        const long long localEnd = localStart + seg->n;
        // 空文本终稿（seek 边界噪声/静音误分段）不入回调，但状态照常推进。
        if (cb_ && !u.text.isEmpty()) cb_(u);
        m_lastFinalEndMs = u.endMs;
        m_partialStartMs = -1;
        m_currentPartialText.clear();   // 语句已闭合，partial 快照失效
        m_lastPartialText.clear();
        SherpaOnnxVoiceActivityDetectorPop(d_->vad);
        SherpaOnnxDestroySpeechSegment(seg);

        if (localEnd > 0 && localEnd <= static_cast<long long>(allSamples_.size())) {
            allSamples_.erase(allSamples_.begin(), allSamples_.begin() + localEnd);
            consumed_ += localEnd;
        }
    }
}

} // namespace rcp::asr
