// src/audio/AudioExtractor.cpp
#include "AudioExtractor.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/timestamp.h>
}

#include <QByteArray>
#include <cmath>

namespace rcp::audio {

struct AudioExtractor::Private {
    AVFormatContext* fmt = nullptr;
    AVCodecContext*  ctx = nullptr;
    SwrContext*      swr = nullptr;
    int              chosen = -1;
    int              outRate = 16000;
    int              outChannels = 1;  // mono
    QString          error;
};

AudioExtractor::AudioExtractor() : d_(new Private) {}
AudioExtractor::~AudioExtractor() { close(); }

void AudioExtractor::close() {
    if (!d_) return;
    if (d_->swr) swr_free(&d_->swr);
    if (d_->ctx) avcodec_free_context(&d_->ctx);
    if (d_->fmt) avformat_close_input(&d_->fmt);
    d_->chosen = -1;
    m_acc.clear();
    m_totalOut = 0;
    m_chosen = -1;
}

bool AudioExtractor::open(const QString& path, int audioStreamIndex) {
    // RAII：重复 open 先完整释放旧上下文（原先 delete d_ 只释放壳，FFmpeg
    // 上下文泄漏，审查 §1.1-7）。
    close();
    if (!d_) d_ = new Private;

    const QByteArray p = path.toUtf8();
    if (avformat_open_input(&d_->fmt, p.constData(), nullptr, nullptr) < 0) {
        d_->error = QStringLiteral("avformat_open_input failed: ") + path;
        return false;
    }
    if (avformat_find_stream_info(d_->fmt, nullptr) < 0) {
        d_->error = QStringLiteral("avformat_find_stream_info failed");
        return false;
    }

    // 选定音轨
    int audioCount = 0;
    for (unsigned i = 0; i < d_->fmt->nb_streams; i++) {
        if (d_->fmt->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
        if (audioStreamIndex < 0 && d_->chosen < 0) d_->chosen = static_cast<int>(i);
        if (audioStreamIndex >= 0 && static_cast<int>(i) == audioStreamIndex) d_->chosen = audioStreamIndex;
        audioCount++;
    }
    if (audioCount == 0) { d_->error = QStringLiteral("no audio tracks"); return false; }
    if (d_->chosen < 0)  { d_->error = QStringLiteral("wanted audio stream not found"); return false; }

    AVStream* st = d_->fmt->streams[d_->chosen];
    const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) { d_->error = QStringLiteral("no decoder for codec"); return false; }
    d_->ctx = avcodec_alloc_context3(dec);
    if (avcodec_parameters_to_context(d_->ctx, st->codecpar) < 0) { d_->error = QStringLiteral("params->ctx failed"); return false; }
    if (avcodec_open2(d_->ctx, dec, nullptr) < 0) { d_->error = QStringLiteral("avcodec_open2 failed"); return false; }

    // 重采样到 16k mono float
    AVChannelLayout outLayout;
    av_channel_layout_from_mask(&outLayout, AV_CH_LAYOUT_MONO);
    if (swr_alloc_set_opts2(&d_->swr, &outLayout, AV_SAMPLE_FMT_FLT, d_->outRate,
                            &d_->ctx->ch_layout, d_->ctx->sample_fmt, d_->ctx->sample_rate, 0, nullptr) < 0) {
        d_->error = QStringLiteral("swr_alloc_set_opts2 failed"); return false;
    }
    if (swr_init(d_->swr) < 0) { d_->error = QStringLiteral("swr_init failed"); return false; }

    m_chosen = d_->chosen;
    return true;
}

double AudioExtractor::durationSec() const {
    if (!d_ || !d_->fmt) return -1.0;
    if (d_->fmt->duration > 0) return static_cast<double>(d_->fmt->duration) / AV_TIME_BASE;
    if (d_->chosen >= 0 && d_->fmt->streams[d_->chosen] && d_->fmt->streams[d_->chosen]->duration > 0) {
        const AVStream* st = d_->fmt->streams[d_->chosen];
        return static_cast<double>(st->duration) * av_q2d(st->time_base);
    }
    return -1.0;
}

bool AudioExtractor::seekToSec(double sec) {
    if (!d_ || !d_->fmt || !d_->ctx || d_->chosen < 0) return false;
    AVStream* st = d_->fmt->streams[d_->chosen];
    const int64_t target = static_cast<int64_t>(sec / av_q2d(st->time_base));
    // 后向 seek 到目标之前的关键帧，保证解码器有参考帧；实际锚点由首帧 PTS 校正。
    if (avformat_seek_file(d_->fmt, d_->chosen, INT64_MIN, target, target, 0) < 0) {
        d_->error = QStringLiteral("avformat_seek_file failed");
        return false;
    }
    avcodec_flush_buffers(d_->ctx);
    m_acc.clear();
    m_totalOut = 0;
    m_anchorSec = -1.0;  // 负值 = 未锚定，extract 收到首个有效 PTS 时重锚
    return true;
}

long long AudioExtractor::extract(double chunkSec, const ChunkCallback& cb,
                                  const rcp::CancellationToken& tok) {
    if (!d_ || !d_->fmt || !d_->ctx) return 0;
    // 参数防御：chunkSec*rate<=0 会导致分块死循环（审查 §1.1-9）。
    if (chunkSec <= 0.0) chunkSec = 0.1;
    const int chunkSamples = static_cast<int>(chunkSec * d_->outRate);
    if (chunkSamples <= 0) return 0;

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    m_totalOut = 0;
    m_acc.clear();

    auto processFrame = [&](AVFrame* f) -> bool {
        // PTS 锚定：块时间 = 锚点 + 累计输出/采样率。首帧或 PTS 跳变 >0.5s 时重锚，
        // 修正"时间戳=累计样本数、不看 PTS"的偏差（审查 §1.1-6）。
        if (f->pts != AV_NOPTS_VALUE) {
            const double ptsSec = static_cast<double>(f->pts) * av_q2d(d_->fmt->streams[d_->chosen]->time_base);
            const double expected = m_anchorSec + static_cast<double>(m_totalOut) / d_->outRate;
            if (m_anchorSec < 0.0 || std::abs(ptsSec - expected) > 0.5) {
                m_anchorSec = ptsSec - static_cast<double>(m_totalOut) / d_->outRate;
            }
        }
        int outEst = swr_get_out_samples(d_->swr, f->nb_samples);
        if (outEst <= 0) return true;
        std::vector<uint8_t> ob(static_cast<size_t>(outEst) * d_->outChannels * sizeof(float));
        uint8_t* op = ob.data();
        int converted = swr_convert(d_->swr, &op, outEst,
                                    (const uint8_t**)f->data, f->nb_samples);
        if (converted > 0) {
            const float* fdata = reinterpret_cast<const float*>(ob.data());
            m_acc.insert(m_acc.end(), fdata, fdata + static_cast<size_t>(converted) * d_->outChannels);
            while (static_cast<int>(m_acc.size()) >= chunkSamples * d_->outChannels) {
                if (tok.isCanceled()) return false;
                if (cb) cb(m_acc.data(), chunkSamples,
                           m_anchorSec + static_cast<double>(m_totalOut) / d_->outRate);
                m_totalOut += chunkSamples;
                m_acc.erase(m_acc.begin(), m_acc.begin() + chunkSamples * d_->outChannels);
            }
        }
        return true;
    };

    while (av_read_frame(d_->fmt, pkt) >= 0) {
        if (tok.isCanceled()) { av_packet_unref(pkt); break; }
        if (pkt->stream_index != d_->chosen) { av_packet_unref(pkt); continue; }
        if (avcodec_send_packet(d_->ctx, pkt) < 0) { av_packet_unref(pkt); continue; }
        bool canceled = false;
        while (avcodec_receive_frame(d_->ctx, frame) >= 0) {
            if (!processFrame(frame)) { canceled = true; break; }
        }
        av_packet_unref(pkt);
        if (canceled) break;
    }
    if (!tok.isCanceled()) {
        // flush
        avcodec_send_packet(d_->ctx, nullptr);
        while (avcodec_receive_frame(d_->ctx, frame) >= 0) {
            if (!processFrame(frame)) break;
        }
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);
    return m_totalOut;
}

} // namespace rcp::audio
