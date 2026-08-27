# 参考：Qt 6 Widgets 集成与 libmpv 渲染桥接

- **Qt 6：** https://www.qt.io （源码镜像 qt/qtbase，Stars 3,067；许可证 Qt 商业 / GPLv3 / LGPLv3）
- **官方 mpv Qt 示例：** https://github.com/mpv-player/mpv-examples （Stars 283，随 mpv GPL-2.0-or-later）
- **mpc-qt（另一 Qt+mpv 播放器参考）：** https://github.com/mpc-qt/mpc-qt （许可证 GPL-2.0；注意 OmarAssadi/mpc-qt 仅为已删除仓库的镜像，Stars 1）

## 我们怎么用（复用程度：参考模式 + 自实现 Qt 封装）

### Qt 6 技术栈（技术方案 3.1）
- Qt 6.11.2 为文档建议基线；本环境将使用实际可获取的 **Qt 6.8.1**（通过 aqtinstall 安装，版本在 `dependencies.lock.json` 记录并说明差异）。
- 模块：Widgets、GUI/OpenGL、Network（QLocalSocket IPC）、SQL（SQLite）、Test（QtTest）。
- 动态链接，保留 LGPLv3 文本与重链接说明（ADR-0002）。

### 渲染桥接（参考 mpv-examples/libmpv/qt_opengl）
- `MpvRenderWidget : QOpenGLWidget`：
  - `initializeGL()`：`mpv_render_context_create` with `MPV_RENDER_API_TYPE_OPENGL`，`get_proc_address` 取 Qt GL 函数。
  - `paintGL()`：`mpv_render_context_render` with `MPV_RENDER_PARAM_OPENGL_FBO`（默认 framebuffer）+ `FLIP_Y`。
  - render update callback：`QMetaObject::invokeMethod(widget,"update",QueuedConnection)`。
  - 高 DPI：传入实际 framebuffer 尺寸而非逻辑像素；多显示器/全屏后校验 viewport。
  - 释放顺序：render context → mpv handle → OpenGL widget。
- 这些与 `src/playback/MpvRenderWidget.h/.cpp` + `MpvEventBridge.h/.cpp` 对应（技术方案 6.3–6.4）。

### 其它 Qt 参考点
- `MpvController`/`MpvHandle` 的“纯 C++ RAII 管 mpv_handle* + 后台线程串行消费命令队列”模式（见 CSDN 博客描述的四层隔离模型）与我们的 `MpvHandle` RAII + `PendingCommandRegistry` 一致。
- 事件桥：`mpv_set_wakeup_callback` 只做线程安全排队唤醒；GUI 线程 `drainEvents()` 循环 `mpv_wait_event(handle,0)`。

## 风险 / 注意

- **多显示器/休眠/驱动重置** 后需重建 render context，失败显示可恢复错误页（技术方案 6.4）。
- **Qt 版本差异**：6.8 与 6.11 在 OpenGL widget API 上基本一致；若升级到建议版本需回归 `paintGL`/DPI 行为。
- **LGPL 合规**：动态链接 + 提供 Qt 开源版本获取方式 + 保留 NOTICE；不得静态链接 Qt 而不公开相应目标文件/重链接能力。

## 合规

- `packaging/licenses/` 包含 Qt LGPLv3 + GPLv3 文本与重链接说明。
- 仅在 `src/platform/windows` 与少量构建适配层出现 Windows API（ADR-0002 边界）。
