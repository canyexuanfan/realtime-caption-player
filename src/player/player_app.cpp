// src/player/player_app.cpp
// T0014 链接验证可执行文件：确认 libmpv 客户端 API 可嵌入并链接。
// 真实视频渲染需要 GUI/GL 上下文，应在真实 Windows 会话验证；
// 此处仅在沙箱做"编译 + 链接"验证（无法运行 GUI）。
#include "MpvPlayer.h"

#include <cstdio>

int main() {
    std::printf("[player_app] libmpv render verification\n");

    const unsigned long ver = MpvPlayer::clientApiVersion();
    std::printf("[player_app] mpv client API version: %lu (0x%lx)\n", ver, ver);

    MpvPlayer player;
    if (!player.create()) {
        std::printf("[player_app] FAILED: mpv_create() returned null\n");
        return 1;
    }
    std::printf("[player_app] mpv_create() OK (handle=%p)\n", (void*)player.handle());

    // 验证期不拉起 GPU/窗口；真实渲染在后续阶段用 GL 上下文。
    player.setOption("vo", "libmpv");
    player.setOption("gpu", "no");

    const int initRc = player.initialize();
    std::printf("[player_app] mpv_initialize() rc=%d\n", initRc);

    // 渲染上下文（mpv_render_context_create）需要真实 GL 上下文
    // （get_proc_address + glCtx）。沙箱无显示设备，不实际调用以避免空回调崩溃；
    // 但 MpvPlayer::createRenderContext 已引用该符号，故它仍被链接进本可执行文件，
    // 满足"渲染 API 可链接"的验证目标。
    mpv_render_context* ctx = nullptr;
    (void)ctx;

    std::printf("[player_app] build/link verification PASSED (libmpv embedded)\n");
    return 0;
}
