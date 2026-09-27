// src/audio/AudioExtractor.h
// FFmpeg 音频抽取：打开媒体 -> 选音轨 -> 解码 -> 重采样为 16kHz mono float
// （sherpa-onnx 双引擎所需格式），按固定时长分块回调给下游 ASR。
//
// B2 升级：RAII 释放（重复 open 不再泄漏 FFmpeg 上下文）、协作式取消、
// seek（解码器 flush + 时间锚重置）、以首帧 PTS 为锚的绝对时间戳
// （chunk 时间 = 锚点 + 累计输出样本/采样率，PTS 跳变 >0.5s 自动重锚）。
#pragma once

#include <QString>
#include <functional>
#include <vector>
#include "core/Cancellation.h"

namespace rcp::audio {

class AudioExtractor {
public:
    // samples: 16k mono float；tStartSec: 该块起始在媒体中的绝对秒位置（PTS 锚定）。
    using ChunkCallback = std::function<void(const float* samples, int count, double tStartSec)>;

    AudioExtractor();
    ~AudioExtractor();
    AudioExtractor(const AudioExtractor&) = delete;
    AudioExtractor& operator=(const AudioExtractor&) = delete;

    // 打开媒体并选定音轨（audioStreamIndex<0 表示第一个音轨）。重复打开安全。
    bool open(const QString& path, int audioStreamIndex = -1);
    void close();

    int sampleRate() const { return 16000; }
    int channels() const { return 1; }
    int audioStreamIndex() const { return m_chosen; }
    QString lastError() const { return m_error; }

    // 媒体时长（秒）；未知返回负值。
    double durationSec() const;

    // 抽取 PCM，按 chunkSec（默认 0.1s）分块回调。返回总样本数。
    // tok 取消后提前返回（协作式：循环内逐块检查）。
    long long extract(double chunkSec, const ChunkCallback& cb,
                      const rcp::CancellationToken& tok = {});

    // seek 到指定秒（avformat seek + 解码器 flush + 输出锚重置）。
    // 之后 extract 从新位置继续，块时间戳以新 PTS 锚定。
    bool seekToSec(double sec);

private:
    struct Private;
    Private* d_ = nullptr;

    int      m_chosen = -1;
    QString  m_error;
    std::vector<float> m_acc;
    long long m_totalOut = 0;
    double   m_anchorSec = 0.0;   // 当前输出流起点对应的媒体绝对秒（首帧 PTS 锚）
};

} // namespace rcp::audio
