// src/asr/AsrEngine.cpp
#include "AsrEngine.h"

extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

#include <cstring>
#include <cstdio>
#include <vector>

namespace rcp::asr {

struct AsrEngine::Impl {
    const SherpaOnnxVoiceActivityDetector* vad = nullptr;
    const SherpaOnnxOnlineRecognizer*      zf  = nullptr;
    const SherpaOnnxOnlineStream*          zs  = nullptr;
    const SherpaOnnxOfflineRecognizer*     sv  = nullptr;
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
    const int32_t sr = 16000;

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
    sv_cfg.model_config.sense_voice.language = "auto";
    sv_cfg.model_config.sense_voice.use_itn = 1;
    d_->sv = SherpaOnnxCreateOfflineRecognizer(&sv_cfg);
    if (!d_->sv) { error_ = QStringLiteral("SenseVoice create failed"); return false; }

    return true;
}

void AsrEngine::feed(const float* samples, int count) {
    if (!d_ || !d_->zf || !d_->vad || !d_->sv) return;
    const int32_t sr = 16000;

    // 实时 partial（Zipformer2-CTC 逐块解码）
    SherpaOnnxOnlineStreamAcceptWaveform(d_->zs, sr, samples, count);
    while (SherpaOnnxIsOnlineStreamReady(d_->zf, d_->zs)) {
        SherpaOnnxDecodeOnlineStream(d_->zf, d_->zs);
        const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(d_->zf, d_->zs);
        if (r && r->text && r->text[0]) {
            CaptionUtterance u;
            u.text = QString::fromUtf8(r->text);
            u.isPartial = true;
            if (cb_) cb_(u);
        }
        SherpaOnnxDestroyOnlineRecognizerResult(r);
    }

    // VAD 分句 -> SenseVoice 终稿
    SherpaOnnxVoiceActivityDetectorAcceptWaveform(d_->vad, samples, count);
    allSamples_.insert(allSamples_.end(), samples, samples + count);
    drainVad();
}

void AsrEngine::flush() {
    if (!d_ || !d_->vad) return;
    SherpaOnnxVoiceActivityDetectorFlush(d_->vad);
    drainVad();
}

void AsrEngine::drainVad() {
    const int32_t sr = 16000;
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
        const float* segSamples = allSamples_.data() + localStart;
        const SherpaOnnxOfflineStream* os = SherpaOnnxCreateOfflineStream(d_->sv);
        SherpaOnnxAcceptWaveformOffline(os, sr, segSamples, seg->n);
        SherpaOnnxDecodeOfflineStream(d_->sv, os);
        const SherpaOnnxOfflineRecognizerResult* r = SherpaOnnxGetOfflineStreamResult(os);
        CaptionUtterance u;
        u.text = (r && r->text) ? QString::fromUtf8(r->text) : QString();
        u.isPartial = false;
        // seg->start 是自流开始的绝对采样索引；时间戳必须用绝对值，
        // 减 consumed_ 只用于取缓冲区内的样本指针。
        u.startMs = static_cast<long long>(seg->start) * 1000LL / sr;
        u.endMs   = (static_cast<long long>(seg->start) + seg->n) * 1000LL / sr;
        const long long localEnd = localStart + seg->n;
        if (cb_) cb_(u);
        SherpaOnnxDestroyOfflineStream(os);
        if (r) SherpaOnnxDestroyOfflineRecognizerResult(r);
        SherpaOnnxVoiceActivityDetectorPop(d_->vad);
        SherpaOnnxDestroySpeechSegment(seg);

        if (localEnd > 0 && localEnd <= static_cast<long long>(allSamples_.size())) {
            allSamples_.erase(allSamples_.begin(), allSamples_.begin() + localEnd);
            consumed_ += localEnd;
        }
    }
}

} // namespace rcp::asr
