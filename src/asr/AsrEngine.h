// src/asr/AsrEngine.h
// 双引擎 ASR 封装（复用 hybrid spike 验证过的链路）：
//   实时逐字 partial = Zipformer2-CTC 流式
//   精准终稿         = Silero VAD 分句 × SenseVoice 离线（带 ITN）
// 输入 16k mono float PCM（由 AudioDecoder 提供），通过回调输出逐句结果。
//
// B2 升级：partial 携带绝对时间区间（以喂入块的 PTS 起始为锚）、200ms 限频
// 与重复文本去重、seek 后可重置流状态（generation 切换）、Lite 档位
// （跳过 SenseVoice，终稿取分句结束时的 partial 快照）、SenseVoice 语言可选。
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
    QString   language;          // 产出语言（"zh" 固定为流式引擎；终稿按配置）
};

class AsrEngine {
public:
    enum class Profile { Balanced, Lite };

    AsrEngine();
    ~AsrEngine();
    AsrEngine(const AsrEngine&) = delete;
    AsrEngine& operator=(const AsrEngine&) = delete;

    // language: "auto"/"zh"/"en"（传给 SenseVoice；流式 Zipformer2-CTC 为中文模型）。
    bool load(const QString& zipformerDir, const QString& sileroDir,
              const QString& sensevoiceDir, const QString& modelBase = "model.int8.onnx");

    // 喂入 16k mono float PCM（任意长度）。startMs = 该块起始的媒体绝对毫秒
    // （PTS 锚定）；<0 时退化为累计样本时钟（文件模式兼容旧调用）。
    // 立即触发 partial/final 回调。
    void feed(const float* samples, int count, long long startMs = -1);

    // 处理缓冲尾部（VAD flush 未决段）。
    void flush();

    // seek/换会话：丢弃流式状态与未决 VAD 段，清空缓冲（generation 切换时调用）。
    void resetForSeek();

    // 档位：Lite 跳过 SenseVoice 终稿，取分句结束时的 partial 快照作终稿。
    void setProfile(Profile p) { m_profile = p; }

    using CaptionCallback = std::function<void(const CaptionUtterance&)>;
    void setCaptionCallback(const CaptionCallback& cb) { cb_ = cb; }
    QString lastError() const { return error_; }

private:
    void drainVad();  // 排泄 VAD 检测到的语音段 -> 终稿（Balanced=SenseVoice / Lite=partial 快照）
    void emitPartialIfDue();  // 200ms 限频 + 重复文本去重

    struct Impl;
    Impl* d_ = nullptr;
    CaptionCallback cb_;
    QString error_;

    std::vector<float> allSamples_;
    long long consumed_ = 0;       // 已丢弃的前缀样本数（VAD 段下标隔离）
    long long m_streamPosMs = -1;  // 已喂入音频的媒体绝对位置（ms；PTS 锚定）
    long long m_lastFinalEndMs = 0;
    long long m_partialStartMs = -1;   // 当前未决语句开始点（partial 区间起点）
    long long m_lastPartialEmitPosMs = 0;
    QString   m_lastPartialText;
    QString   m_currentPartialText;   // 当前未决语句的 partial 快照（Lite 终稿用）
    QString   m_language = QStringLiteral("auto");
    Profile   m_profile = Profile::Balanced;
};

} // namespace rcp::asr
