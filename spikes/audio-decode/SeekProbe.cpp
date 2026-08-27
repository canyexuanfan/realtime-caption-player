// SeekProbe — T0018 探针：FFmpeg seek + 采样时钟映射。
//
// 目的（编译/链接 + 沙箱内真实运行验证）：
//   1. 对指定音轨 av_seek_frame 到目标秒数（AVSEEK_FLAG_BACKWARD）。
//   2. 解码 seek 后的首帧，用 stream->time_base 把 frame->pts 换算成秒。
//   3. 采样时钟：seconds -> sample_offset = pts_sec * sample_rate * channels，
//      即 ASR 流水线把"字幕时间戳"对齐到"PCM 样本下标"的换算依据。
//
// 用法（真机）：
//   seek_probe.exe <media.mkv> [target_seconds]
//
// 链接目标：rcp::ffmpeg

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "tests/fixtures/multitrack.mkv";
    double target_sec = (argc > 2) ? std::atof(argv[2]) : 2.0;

    AVFormatContext* fmt = nullptr;
    if (avformat_open_input(&fmt, path, nullptr, nullptr) < 0) { std::printf("seek: open failed\n"); return 1; }
    avformat_find_stream_info(fmt, nullptr);

    int stream_idx = -1;
    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { stream_idx = (int)i; break; }
    }
    if (stream_idx < 0) { std::printf("seek: no audio\n"); avformat_close_input(&fmt); return 1; }
    AVStream* st = fmt->streams[stream_idx];

    const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
    AVCodecContext* ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(ctx, st->codecpar);
    avcodec_open2(ctx, dec, nullptr);

    // seek：把目标秒数换算到 stream time_base 单位
    int64_t seek_ts = av_rescale_q((int64_t)(target_sec * 1000), AVRational{1, 1000}, st->time_base);
    int sr = av_seek_frame(fmt, stream_idx, seek_ts, AVSEEK_FLAG_BACKWARD);
    std::printf("seek: target=%.3fs seek_ts=%lld ret=%d\n", target_sec, (long long)seek_ts, sr);
    avcodec_flush_buffers(ctx);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    double first_pts_sec = -1.0;
    int64_t sample_offset = -1;
    while (av_read_frame(fmt, pkt) >= 0) {
        if (pkt->stream_index != stream_idx) { av_packet_unref(pkt); continue; }
        avcodec_send_packet(ctx, pkt);
        if (avcodec_receive_frame(ctx, frame) >= 0) {
            if (frame->pts != AV_NOPTS_VALUE) {
                first_pts_sec = frame->pts * av_q2d(st->time_base);
                int channels = ctx->ch_layout.nb_channels ? ctx->ch_layout.nb_channels : 1;
                sample_offset = (int64_t)(first_pts_sec * ctx->sample_rate * channels);
                break;
            }
        }
        av_packet_unref(pkt);
    }

    if (first_pts_sec >= 0) {
        std::printf("seek: first decoded frame pts=%.4fs (delta=%.4fs) sample_offset=%lld @%dHz/%dch\n",
                    first_pts_sec, first_pts_sec - target_sec,
                    (long long)sample_offset, ctx->sample_rate,
                    ctx->ch_layout.nb_channels ? ctx->ch_layout.nb_channels : 1);
    } else {
        std::printf("seek: no frame with valid pts after seek\n");
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
    return 0;
}
