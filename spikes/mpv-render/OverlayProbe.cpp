// OverlayProbe — T0016 探针：ASS 字幕 overlay + 带字幕截图。
//
// 目的（编译/链接级验证；真正出图需真机显示器 + GL/WL 上下文）：
//   1. 通过 --sub-files 把外部 .ass 作为字幕轨挂入（ASS overlay 的核心接入点）。
//   2. 初始化 mpv render 上下文（MPV_RENDER_API_TYPE_OPENGL + mpv_opengl_init_params），
//      登记 render update 回调；headless 下 GL 加载器为 NULL，运行期 create 会失败，属预期。
//   3. 发出 screenshot 命令（option=subtitles）以"带字幕"方式截图。
//   4. 跑短暂事件循环，证明命令/事件链路接通。
//
// 用法（真机）：
//   overlay_probe.exe <video.mkv> [subtitle.ass]
//
// 链接目标：rcp::mpv -> mpv-2.dll

#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

static void wakeup_cb(void* d) { (void)d; }
static void update_cb(void* d) { (void)d; /* 渲染线程：需要重绘 */ }

int main(int argc, char** argv) {
    const char* media = (argc > 1) ? argv[1] : "tests/fixtures/multitrack.mkv";
    const char* ass   = (argc > 2) ? argv[2] : "tests/fixtures/subtitle.ass";

    mpv_handle* ctx = mpv_create();
    if (!ctx) { std::printf("overlay: mpv_create failed\n"); return 1; }

    // headless-safe：沙箱无显示器时强制 vo=null，避免默认开窗口导致崩溃。
    // 真机运行时注释掉这行即可接真实 VO / GL 上下文。
    if (mpv_set_option_string(ctx, "vo", "null") < 0)
        std::printf("overlay: set vo=null failed (ignored)\n");

    // 1) ASS overlay 接入：把外部 .ass 作为字幕轨挂入
    if (mpv_set_option_string(ctx, "sub-files", ass) < 0)
        std::printf("overlay: set sub-files failed\n");
    mpv_set_wakeup_callback(ctx, wakeup_cb, nullptr);

    // 2) 初始化 render 上下文。
    //    渲染上下文必须有真实的 GL/Vulkan 表面才能创建；headless 下 GL 加载器为 NULL，
    //    跳过创建（仅在提供真实加载器时才会走到 mpv_render_context_create，链接验证仍覆盖该符号）。
    mpv_opengl_init_params gl_init{};
    gl_init.get_proc_address = nullptr;       // 真机需填真实 GL 函数加载器
    gl_init.get_proc_address_ctx = nullptr;
    mpv_render_context* rctx = nullptr;
    if (gl_init.get_proc_address != nullptr) {
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, (void*)(intptr_t)MPV_RENDER_API_TYPE_OPENGL},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        int rc = mpv_render_context_create(&rctx, ctx, params);
        std::printf("overlay: render_context_create rc=%d (0=ok)\n", rc);
        if (rc == 0) mpv_render_context_set_update_callback(rctx, update_cb, nullptr);
    } else {
        std::printf("overlay: headless => skip render context (需要提供 GL 加载器才能在真机创建)\n");
    }

    if (mpv_initialize(ctx) < 0)
        std::printf("overlay: mpv_initialize <0 (no display in sandbox is expected)\n");

    // 3) 加载媒体
    if (argc > 1) {
        const char* load[] = {"loadfile", media, nullptr};
        if (mpv_command(ctx, load) < 0) std::printf("overlay: loadfile failed\n");
    }

    // 4) 事件循环 + 截图命令（option=subtitles => 把字幕烧录进截图）
    bool quit = false;
    for (int i = 0; i < 60 && !quit; i++) {
        mpv_event* ev = mpv_wait_event(ctx, 0.1);
        if (ev->event_id == MPV_EVENT_SHUTDOWN) quit = true;
        else if (ev->event_id == MPV_EVENT_FILE_LOADED) {
            // 文件已载入 -> 触发一次带字幕截图
            const char* shot[] = {"screenshot", "out_sub", "subtitles", nullptr};
            if (mpv_command(ctx, shot) == 0)
                std::printf("overlay: screenshot(subtitles) issued\n");
            else
                std::printf("overlay: screenshot command rejected (no VO in headless)\n");
        }
    }

    if (rctx) mpv_render_context_free(rctx);
    mpv_destroy(ctx);
    std::printf("overlay: done\n");
    return 0;
}
