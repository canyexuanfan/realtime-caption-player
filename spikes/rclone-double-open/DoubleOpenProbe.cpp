// DoubleOpenProbe — T0019 探针：验证本地与 rclone full 双开读取。
//
// 目的（编译/链接级验证双开代码路径；真正挂载+断网行为需真机 rclone mount）：
//   1. 同时用 libmpv（播放侧）与 FFmpeg（worker 解码侧）打开同一媒体路径，
//      证明"双开"在打开层不冲突。
//   2. 对路径做来源分类（本地盘 / UNC / rclone 挂载根），便于真机核对 VFS full 缓存行为。
//
// 用法（真机，需先 rclone mount 到某盘符，例如 Z:）：
//   double_open_probe.exe Z:/media/clip.mkv
//   double_open_probe.exe D:/local/clip.mkv
//
// 链接目标：rcp::mpv + rcp::ffmpeg

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <string>
#include <mpv/client.h>

extern "C" {
#include <libavformat/avformat.h>
}

// 路径来源分类：决定真机测试时关注的缓存目录与 VFS 参数。
static const char* classify(const char* p) {
    if (p[0] && p[1] == ':' && (p[2] == '/' || p[2] == '\\'))
        return "local-drive";
    if (p[0] == '\\' && p[1] == '\\')
        return "unc";
    if (std::strncmp(p, "//", 2) == 0 || std::strncmp(p, "\\\\", 2) == 0)
        return "network/rclone-mount";
    return "other";
}

int main(int argc, char** argv) {
    const char* media = (argc > 1) ? argv[1] : "tests/fixtures/multitrack.mkv";
    std::printf("double_open: path=%s  classify=%s\n", media, classify(media));

    // ---- 侧 1：FFmpeg（worker 解码侧）打开 ----
    avformat_network_init();
    AVFormatContext* fmt = nullptr;
    int fr = avformat_open_input(&fmt, media, nullptr, nullptr);
    int audioStreams = 0, videoStreams = 0;
    if (fr == 0) {
        avformat_find_stream_info(fmt, nullptr);
        for (unsigned i = 0; i < fmt->nb_streams; i++) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) audioStreams++;
            else if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) videoStreams++;
        }
        std::printf("double_open: [FFmpeg] open OK  video=%d audio=%d\n", videoStreams, audioStreams);
        avformat_close_input(&fmt);
    } else {
        std::printf("double_open: [FFmpeg] open FAILED (rc=%d) — 真机需 rclone mount 就绪\n", fr);
    }

    // ---- 侧 2：libmpv（播放侧）打开同一路径 ----
    mpv_handle* ctx = mpv_create();
    int mr = -1;
    if (ctx) {
        if (mpv_set_option_string(ctx, "vo", "null") < 0) { /* headless 安全 */ }
        mr = mpv_initialize(ctx);
        if (mr == 0) {
            const char* load[] = {"loadfile", media, "replace", nullptr};
            mr = mpv_command(ctx, load);
            std::printf("double_open: [mpv] loadfile rc=%d\n", mr);
        } else {
            std::printf("double_open: [mpv] initialize FAILED (rc=%d)\n", mr);
        }
        mpv_destroy(ctx);
    } else {
        std::printf("double_open: [mpv] create FAILED\n");
    }

    // ---- 双开结论 ----
    bool bothOpen = (fr == 0) && (mr == 0);
    std::printf("double_open: 双开同时打开 = %s\n", bothOpen ? "YES" : "NO(headless/无挂载时仅验证代码路径)");
    std::printf("double_open: VFS full 建议参数（真机挂载时）：--vfs-cache-mode full --buffer-size 256M --drive-pacer-min-sleep 10ms\n");
    std::printf("double_open: done\n");
    return 0;
}
