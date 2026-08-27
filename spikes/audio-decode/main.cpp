// AudioDecodeProbe — T0017 探针：FFmpeg 指定音轨解码。
//
// 目的（编译/链接级验证；真实运行需真机 + 多音轨媒体）：
//   1. 打开媒体，枚举所有 AVMEDIA_TYPE_AUDIO 流，打印"音轨映射表"
//      （流索引 -> 编解码器 -> 声道布局 -> 采样率 -> 语言）。
//   2. 按传入的音轨流索引，打开对应解码器，解码该音轨的全部 packet -> frame。
//   3. 用 swresample 转成 S16 交错 PCM，累计样本数 + 简易校验和。
//
// 用法（真机）：
//   audio_decode_probe.exe <media.mkv> [audio_stream_index]
// 默认输入回退到 tests/fixtures/multitrack.mkv（见 docs/spikes/audio-track-map.md 的生成命令）。
//
// 链接目标：rcp::ffmpeg （avformat avcodec avutil swresample avfilter）

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "tests/fixtures/multitrack.mkv";
    int wanted = (argc > 2) ? std::atoi(argv[2]) : -1;  // -1 = 第一个音轨

    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, path, nullptr, nullptr) < 0) {
        std::printf("audio_decode: cannot open %s\n", path);
        return 1;
    }
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        std::printf("audio_decode: find_stream_info failed\n");
        avformat_close_input(&fmt);
        return 1;
    }

    // 1) 枚举音轨映射
    std::printf("audio_decode: tracks in %s\n", path);
    int audio_count = 0;
    int chosen = -1;
    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        AVStream* s = fmt->streams[i];
        if (s->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
        char layout[64] = {0};
        if (av_channel_layout_describe(&s->codecpar->ch_layout, layout, sizeof(layout)) < 0)
            std::strncpy(layout, "?", sizeof(layout) - 1);
        const char* lang = "?";
        AVDictionaryEntry* t = av_dict_get(s->metadata, "language", nullptr, 0);
        if (t) lang = t->value;
        std::printf("  [audio #%d] stream=%u codec=%s ch=%s rate=%d lang=%s\n",
                    audio_count, i,
                    avcodec_get_name(s->codecpar->codec_id),
                    layout, s->codecpar->sample_rate, lang);
        if (wanted < 0 && chosen < 0) chosen = static_cast<int>(i);  // 默认第一个
        if (wanted >= 0 && static_cast<int>(i) == wanted) chosen = wanted;
        audio_count++;
    }
    if (audio_count == 0) {
        std::printf("audio_decode: no audio tracks found\n");
        avformat_close_input(&fmt);
        return 1;
    }
    if (chosen < 0) {
        std::printf("audio_decode: wanted stream %d not an audio track\n", wanted);
        avformat_close_input(&fmt);
        return 1;
    }
    std::printf("audio_decode: decoding stream=%d (PCM S16)\n", chosen);

    // 2) 打开解码器
    AVStream* st = fmt->streams[chosen];
    const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) { std::printf("audio_decode: no decoder for %s\n", avcodec_get_name(st->codecpar->codec_id)); avformat_close_input(&fmt); return 1; }
    AVCodecContext* ctx = avcodec_alloc_context3(dec);
    if (avcodec_parameters_to_context(ctx, st->codecpar) < 0) { std::printf("audio_decode: params->ctx failed\n"); return 1; }
    if (avcodec_open2(ctx, dec, nullptr) < 0) { std::printf("audio_decode: avcodec_open2 failed\n"); return 1; }

    // 3) 重采样到 S16 交错
    SwrContext* swr = nullptr;
    AVChannelLayout out_layout;
    av_channel_layout_copy(&out_layout, &ctx->ch_layout);  // 保持原始声道布局
    if (swr_alloc_set_opts2(&swr, &out_layout, AV_SAMPLE_FMT_S16, ctx->sample_rate,
                            &ctx->ch_layout, ctx->sample_fmt, ctx->sample_rate, 0, nullptr) < 0) {
        std::printf("audio_decode: swr alloc failed\n");
        return 1;
    }
    swr_init(swr);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    int64_t total_samples = 0;
    uint64_t checksum = 0;
    int frame_count = 0;

    while (av_read_frame(fmt, pkt) >= 0) {
        if (pkt->stream_index != chosen) { av_packet_unref(pkt); continue; }
        if (avcodec_send_packet(ctx, pkt) < 0) { av_packet_unref(pkt); continue; }
        while (avcodec_receive_frame(ctx, frame) >= 0) {
            frame_count++;
            // 计算该帧 S16 PCM 样本数
            int out_samples = swr_get_out_samples(swr, frame->nb_samples);
            if (out_samples <= 0) { av_packet_unref(pkt); continue; }
            uint8_t* out_buf = nullptr;
            int ret = av_samples_alloc(&out_buf, nullptr, out_layout.nb_channels,
                                       out_samples, AV_SAMPLE_FMT_S16, 0);
            if (ret < 0) { av_packet_unref(pkt); continue; }
            int converted = swr_convert(swr, &out_buf, out_samples,
                                        (const uint8_t**)frame->data, frame->nb_samples);
            if (converted > 0) {
                int bytes = converted * out_layout.nb_channels * (int)sizeof(int16_t);
                total_samples += converted;
                for (int b = 0; b < bytes; b++) checksum = (checksum * 31 + out_buf[b]) & 0xFFFFFFFFull;
            }
            av_freep(&out_buf);
        }
        av_packet_unref(pkt);
    }
    // flush
    avcodec_send_packet(ctx, nullptr);
    while (avcodec_receive_frame(ctx, frame) >= 0) {
        frame_count++;
        int out_samples = swr_get_out_samples(swr, frame->nb_samples);
        if (out_samples <= 0) continue;
        uint8_t* out_buf = nullptr;
        av_samples_alloc(&out_buf, nullptr, out_layout.nb_channels, out_samples, AV_SAMPLE_FMT_S16, 0);
        int converted = swr_convert(swr, &out_buf, out_samples, (const uint8_t**)frame->data, frame->nb_samples);
        if (converted > 0) total_samples += converted;
        av_freep(&out_buf);
    }

    std::printf("audio_decode: frames=%d total_samples=%lld (~%.2fs) checksum=%llu\n",
                frame_count, (long long)total_samples,
                (double)total_samples / (ctx->sample_rate * (out_layout.nb_channels ? out_layout.nb_channels : 1)),
                (unsigned long long)checksum);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
    return 0;
}
