# 参考：mpv / libmpv

- **仓库：** https://github.com/mpv-player/mpv
- **Stars：** 36,690（gh api，2026-08-27）
- **许可证：** GPL-2.0-or-later（GitHub API 显示 `NOASSERTION`，因 mpv 的 `Copyright` 文件非标准 SPDX；上游实际以 GPL-2.0-or-later 发布）。
  - 注意：`libmpv` 的客户端头文件 `libmpv/client.h` 以 **MIT-0** 许可，允许任意许可证的客户端链接；但 mpv 本体（我们若修改/打包）仍受 GPL-2.0-or-later 约束。
- **角色：** 播放内核（解码、渲染、字幕轨、音轨、滤镜、倍速、HDR、截图）。
- **版本基线（技术实现方案 3.1）：** mpv 0.41.0（实际锁版本以 `dependencies.lock.json` 为准）。

## 我们怎么用（复用程度：链接库 + 参考官方示例模式）

1. **链接 libmpv 静态/动态库**，不直接修改 mpv 源码（除非打补丁并通过 ADR 记录）。
2. **渲染桥接参考官方示例** `mpv-player/mpv-examples/libmpv/qt_opengl/mpvwidget.cpp`（Stars 283，许可证随 mpv GPL-2.0-or-later）：
   - `MpvWidget : public QOpenGLWidget`，`initializeGL()` 创建 `mpv_render_context`（OpenGL），`paintGL()` 调用 `mpv_render_context_render`。
   - `get_proc_address` 从 `QOpenGLContext::currentContext()->getProcAddress()` 取 GL 函数。
   - `mpv_set_wakeup_callback` → `QMetaObject::invokeMethod(..., QueuedConnection)` 唤醒事件处理。
   - 这与我们的 `src/playback/MpvRenderWidget` + `MpvEventBridge` 设计一致（技术方案 6.3–6.4）。
3. **命令/属性**：通过 `mpv_command_async` / `mpv_observe_property` 操作，属性变更转为强类型 Qt signal（技术方案 6.3）。
4. **初始化选项**（技术方案 6.2）：`terminal=no`、`input-default-bindings=no`、`osc=no`、`hwdec=auto-safe`、`audio-pitch-correction=yes`、`sub-auto=fuzzy`、`screenshot-format=png` 等。

## 风险 / 注意

- **GPL 传染**：整体产品以 GPL-3.0-or-later 聚合分发（ADR-0001），与 mpv GPL-2.0-or-later 兼容（GPL-3.0 是 GPL-2.0 的超集兼容方式需谨慎；聚合分发下各自按自身许可证）。
- **client.h MIT-0** 允许我们链接；但不得把“仅放 DLL”当作规避 GPL 的理由（ADR-0001）。
- 若需修改 mpv 行为（如截图含 overlay 的特定 flag），应作为补丁记录在 `third_party/build-metadata/mpv.txt`，并保留上游版权。

## 合规

- 发布含 mpv 源码获取方式（GPL 要求）或明确指向 mpv 上游源码。
- `NOTICE` / `packaging/licenses/` 包含 mpv 许可证文本。
