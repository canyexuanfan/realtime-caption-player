// src/asr/AsrEngine.h
// 双引擎 ASR 封装（复用 hybrid spike 验证过的链路）：
//   实时逐字 partial = Zipformer2-CTC 流式
//   精准终稿         = Silero VAD 分句 × SenseVoice 离线（带 ITN）
// 输入 16k mono float PCM（由 AudioExtractor 提供），通过回调输出逐句结果。
#pragma once

#include <QString>
#include <functional>
#include <vector>
#include "captions/CaptionTypes.h"

namespace rcp::asr {

struct CaptionUtterance {
    QString   text;
    long long startMs = 0;
    long long endMs = 0;
    bool      isPartial = false;
};

class AsrEngine {
public:
    AsrEngine();
    ~AsrEngine();

    bool load(const QString& zipformerDir, const QString& sileroDir,
              const QString& sensevoiceDir, const QString& modelBase = "model.int8.onnx");

    // 喂入 16k mono float PCM（任意长度）。立即触发 partial/final 回调。
    void feed(const float* samples, int count);

    // 处理缓冲尾部（VAD flush 未决段）。
    void flush();

    using CaptionCallback = std::function<void(const CaptionUtterance&)>;
    void setCaptionCallback(const CaptionCallback& cb) { cb_ = cb; }
    QString lastError() const { return error_; }

private:
    void drainVad();  // 排泄 VAD 检测到的语音段 -> SenseVoice 终稿

    struct Impl;
    Impl* d_ = nullptr;
    CaptionCallback cb_;
    QString error_;
    std::vector<float> allSamples_;
    long long consumed_ = 0;  // 已丢弃的前缀样本数（VAD 段下标隔离）
};

} // namespace rcp::asr
