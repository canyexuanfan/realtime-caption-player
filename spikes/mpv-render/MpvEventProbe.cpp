// MpvEventProbe — T0015 探针：验证 libmpv 的"事件桥 + 属性观察"机制。
//
// 目的（仅做编译/链接级验证；真实运行需真机 + 视频文件）：
//   1. 创建 mpv_handle，注册 wakeup 回调（事件桥：其他线程有事件时通知主循环）；
//   2. 观察 time-pos / duration / pause 属性，属性变化时收到 MPV_EVENT_PROPERTY_CHANGE；
//   3. 在事件主循环里 drain 所有事件（SHUTDOWN / FILE_LOADED / PROPERTY_CHANGE / LOG_MESSAGE）。
//
// 沙箱无显示器/GPU，不会真正初始化播放；真机上带视频路径参数即可运行：
//   mpv_event_probe.exe <video.mkv>
//
// 链接目标：rcp::mpv （mpv.lib -> libmpv-2.dll）

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <mpv/client.h>

// 事件桥：mpv 在内部线程产生事件时回调，仅作计数以证明回调被触发。
static volatile int g_wakeups = 0;
static void wakeup_cb(void* d) {
    (void)d;
    g_wakeups++;
}

// 处理单个 mpv 事件；返回 true 表示主循环应退出。
static bool handle_event(mpv_event* ev) {
    switch (ev->event_id) {
        case MPV_EVENT_SHUTDOWN:
            std::printf("[event] SHUTDOWN\n");
            return true;
        case MPV_EVENT_FILE_LOADED:
            std::printf("[event] FILE_LOADED  (userdata=%llu)\n",
                        static_cast<unsigned long long>(ev->reply_userdata));
            break;
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto* p = static_cast<mpv_event_property*>(ev->data);
            if (!p || !p->name) break;
            if (std::strcmp(p->name, "time-pos") == 0 && p->format == MPV_FORMAT_DOUBLE) {
                std::printf("[prop] time-pos = %.3f\n", *static_cast<double*>(p->data));
            } else if (std::strcmp(p->name, "duration") == 0 && p->format == MPV_FORMAT_DOUBLE) {
                std::printf("[prop] duration = %.3f\n", *static_cast<double*>(p->data));
            } else if (std::strcmp(p->name, "pause") == 0 && p->format == MPV_FORMAT_FLAG) {
                std::printf("[prop] pause    = %d\n", *static_cast<int*>(p->data));
            } else {
                std::printf("[prop] %s changed (fmt=%d)\n", p->name, static_cast<int>(p->format));
            }
            break;
        }
        case MPV_EVENT_LOG_MESSAGE: {
            auto* m = static_cast<mpv_event_log_message*>(ev->data);
            if (m) std::printf("[log:%s] %s: %s", m->prefix, m->level, m->text);
            break;
        }
        default:
            std::printf("[event] id=%d\n", static_cast<int>(ev->event_id));
            break;
    }
    return false;
}

int main(int argc, char** argv) {
    std::printf("MpvEventProbe: client API %#x\n", mpv_client_api_version());

    mpv_handle* ctx = mpv_create();
    if (!ctx) {
        std::printf("mpv_create failed\n");
        return 1;
    }

    // 事件桥：注册 wakeup 回调
    mpv_set_wakeup_callback(ctx, wakeup_cb, nullptr);

    // 属性观察（事件桥核心：属性变化 -> MPV_EVENT_PROPERTY_CHANGE）
    mpv_observe_property(ctx, 1, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(ctx, 2, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(ctx, 3, "pause", MPV_FORMAT_FLAG);
    mpv_request_log_messages(ctx, "info");

    if (mpv_initialize(ctx) < 0) {
        // 沙箱无显示器时预期失败；真机上有显示则成功。不致命，继续 drain 事件证明结构可用。
        std::printf("mpv_initialize returned <0 (no display in sandbox is expected)\n");
    }

    // 若传入视频路径则加载
    if (argc > 1) {
        const char* cmd[] = {"loadfile", argv[1], nullptr};
        if (mpv_command(ctx, cmd) < 0) {
            std::printf("loadfile failed\n");
        }
    }

    // 事件主循环：drain 事件，最多 200 次或遇到 SHUTDOWN
    bool quit = false;
    for (int i = 0; i < 200 && !quit; i++) {
        mpv_event* ev = mpv_wait_event(ctx, 0.1);
        quit = handle_event(ev);
    }

    std::printf("MpvEventProbe: wakeups=%d, done\n", static_cast<int>(g_wakeups));

    mpv_unobserve_property(ctx, 1);
    mpv_unobserve_property(ctx, 2);
    mpv_unobserve_property(ctx, 3);
    mpv_destroy(ctx);
    return 0;
}
