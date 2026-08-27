// src/player/MpvPlayer.h
// libmpv 客户端封装（T0014：libmpv 渲染验证程序）。
// 仅依赖 libmpv 客户端/渲染 API + Qt（QString）。不引入 GUI 工具包，
// 便于在沙箱中以"编译 + 链接"确认 libmpv 可嵌入。
#pragma once

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
// render_gl.h 会把 mpv_render_context_create 等宏重定向为运行期函数指针
// （需先经 mpv_render_context_create_fn 取址）。本工程以 mpv.lib 静态链接，
// 希望保留 render.h 中真正导出的符号，故撤销该宏重定向，仅借用其中
// mpv_opengl_init_params 结构体定义。
#undef mpv_render_context_create

#include <QString>

class MpvPlayer {
public:
    MpvPlayer() = default;
    ~MpvPlayer();

    MpvPlayer(const MpvPlayer&) = delete;
    MpvPlayer& operator=(const MpvPlayer&) = delete;

    // 客户端 API 版本（高 16 位主版本，低 16 位次版本）。验证 libmpv 可链接可调用。
    static unsigned long clientApiVersion();

    // 创建底层 mpv_handle；失败返回 false。
    bool create();

    // 设置字符串选项（如 "vo"="libmpv"）。需在 initialize() 之前调用。
    int setOption(const QString& name, const QString& value);

    // 初始化播放核心（创建句柄后、加载媒体前调用）。
    int initialize();

    // 异步加载媒体文件（loadfile 命令）。
    int loadFile(const QString& path, bool replace = true);

    // 创建 OpenGL 渲染上下文。
    // 需要上层提供真实 GL 上下文（get_proc_address + glCtx），沙箱无显示设备故不调用。
    int createRenderContext(mpv_render_context** outCtx,
                            void* (*getProcAddr)(void* ctx, const char* name),
                            void* glCtx);

    mpv_handle* handle() const { return m_handle; }

private:
    mpv_handle* m_handle = nullptr;
};
