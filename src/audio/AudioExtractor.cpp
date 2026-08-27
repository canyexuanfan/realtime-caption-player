// src/audio/AudioExtractor.cpp
#include "AudioExtractor.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
}

#include <QByteArray>

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

AudioExtractor::~AudioExtractor() {
    if (d_) {
        swr_free(&d_->swr);
        avcodec_free_context(&d_->ctx);
        avformat_close_input(&d_->fmt);
        delete d_;
        d_ = nullptr;
    }
}

bool AudioExtractor::open(const QString& path, int audioStreamIndex) {
    delete d_;
    d_ = new Private;

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

long long AudioExtractor::extract(double chunkSec, const ChunkCallback& cb) {
    if (!d_ || !d_->fmt) return 0;

    const int chunkSamples = static_cast<int>(chunkSec * d_->outRate);  // mono
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    m_totalOut = 0;
    m_acc.clear();

    auto processFrame = [&](AVFrame* f) {
        int outEst = swr_get_out_samples(d_->swr, f->nb_samples);
        if (outEst <= 0) return;
        std::vector<uint8_t> ob(static_cast<size_t>(outEst) * d_->outChannels * sizeof(float));
        uint8_t* op = ob.data();
        int converted = swr_convert(d_->swr, &op, outEst,
                                    (const uint8_t**)f->data, f->nb_samples);
        if (converted > 0) {
            const float* fdata = reinterpret_cast<const float*>(ob.data());
            m_acc.insert(m_acc.end(), fdata, fdata + static_cast<size_t>(converted) * d_->outChannels);
            while (static_cast<int>(m_acc.size()) >= chunkSamples * d_->outChannels) {
                if (cb) cb(m_acc.data(), chunkSamples, static_cast<double>(m_totalOut) / d_->outRate);
                m_totalOut += chunkSamples;
                m_acc.erase(m_acc.begin(), m_acc.begin() + chunkSamples * d_->outChannels);
            }
        }
    };

    while (av_read_frame(d_->fmt, pkt) >= 0) {
        if (pkt->stream_index != d_->chosen) { av_packet_unref(pkt); continue; }
        if (avcodec_send_packet(d_->ctx, pkt) < 0) { av_packet_unref(pkt); continue; }
        while (avcodec_receive_frame(d_->ctx, frame) >= 0) {
            processFrame(frame);
        }
        av_packet_unref(pkt);
    }
    // flush
    avcodec_send_packet(d_->ctx, nullptr);
    while (avcodec_receive_frame(d_->ctx, frame) >= 0) {
        processFrame(frame);
    }

    // 余量
    if (!m_acc.empty() && cb) {
        cb(m_acc.data(), static_cast<int>(m_acc.size() / d_->outChannels),
           static_cast<double>(m_totalOut) / d_->outRate);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    return m_totalOut;
}

} // namespace rcp::audio
