// src/audio/AudioExtractor.h
// FFmpeg 音频抽取：打开媒体 -> 选音轨 -> 解码 -> 重采样为 16kHz mono float
// （sherpa-onnx 双引擎所需格式），按固定时长分块回调给下游 ASR。
#pragma once

#include <QString>
#include <functional>
#include <vector>

namespace rcp::audio {

class AudioExtractor {
public:
    // samples: 16k mono float；tStartSec: 该块起始在音频中的绝对秒位置。
    using ChunkCallback = std::function<void(const float* samples, int count, double tStartSec)>;

    AudioExtractor() = default;
    ~AudioExtractor();

    // 打开媒体并选定音轨（audioStreamIndex<0 表示第一个音轨）。
    bool open(const QString& path, int audioStreamIndex = -1);

    int sampleRate() const { return 16000; }
    int channels() const { return 1; }
    int audioStreamIndex() const { return m_chosen; }
    QString lastError() const { return m_error; }

    // 抽取全部 PCM，按 chunkSec（默认 0.1s）分块回调。返回总样本数。
    long long extract(double chunkSec = 0.1, const ChunkCallback& cb = nullptr);

private:
    struct Private;
    Private* d_ = nullptr;

    int      m_chosen = -1;
    QString  m_error;
    std::vector<float> m_acc;
    long long m_totalOut = 0;
};

} // namespace rcp::audio
