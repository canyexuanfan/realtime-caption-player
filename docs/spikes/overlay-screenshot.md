# T0016 — ASS 字幕 overlay + 带字幕截图（探针观察）

**探针**：`spikes/mpv-render/OverlayProbe.cpp`（目标 `overlay_probe`，链接 `rcp::mpv` → `mpv-2.dll`）
**夹具**：`tests/fixtures/subtitle.ass`（ASS 字幕样本）

## 接入点
- **ASS overlay 核心**：`mpv_set_option_string(ctx, "sub-files", <ass>)` 把外部 `.ass` 作为字幕轨挂入，mpv 内部完成 ASS 解析与渲染合成。
- **渲染上下文**：`mpv_opengl_init_params` + `mpv_render_context_create(...)`；登记 `mpv_render_context_set_update_callback` 作为"需要重绘"信号。
- **带字幕截图**：`screenshot <outfile> subtitles` 命令，`option=subtitles` 会把字幕烧录进截图。

## 编译/链接验证（已完成）
- `BUILD_SPIKES=ON` 真实 cmake/Ninja 构建 `overlay_probe` 通过（`BUILD_EXIT=0`）。
- `dumpbin /dependents` 确认导入 `mpv-2.dll`。
- `mpv_render_context_create` 本版本为 **3 参数**（`mpv_render_context_create(&rctx, ctx, params)`，数组以 `MPV_RENDER_PARAM_INVALID` 收尾），已修正早前 4 参数误写。

## 运行验证（PARTIAL — 需真机显示器）
- **沙箱 headless 实测**：程序运行 `RUN_EXIT=0`，但无 GL 表面，渲染上下文按设计跳过（`headless => skip render context`）。`vo=null` 下无窗口可截图，FILE_LOADED 截图命令路径未在此环境触发（预期）。
- **真机待回填**：在带显示器/GL 的 Windows 上，把 `gl_init.get_proc_address` 换成真实加载器（如 Qt `QOpenGLContext::getProcAddress`）、移除 `vo=null`，预期：
  1. `mpv_render_context_create` 返回 0；
  2. 加载含 `sub-files` 的媒体后画面叠加 ASS 字幕；
  3. `screenshot ... subtitles` 产出带字幕的 PNG。

## 结论
代码与链接已证明可在产品里接入 ASS overlay 与截图命令；**真正的"画面叠加字幕 + 出图"需在真机（带 GPU/显示器）回填验证**，本环境无法替代。
