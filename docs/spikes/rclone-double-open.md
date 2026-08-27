# T0019 — 本地与 rclone full 双开读取（探针观察）

**探针**：`spikes/rclone-double-open/DoubleOpenProbe.cpp`（目标 `double_open_probe`，链接 `rcp::mpv` + `rcp::ffmpeg`）

## 双开模式
- **worker 解码侧（FFmpeg）**：`avformat_open_input` 打开同一媒体，统计 video/audio 流。
- **播放侧（libmpv）**：`mpv_create` + `mpv_initialize` + `loadfile` 打开同一路径。
- 两者同时打开同一文件，验证打开层不冲突（双开代码路径）。

## 编译/链接 + 运行验证（已完成）
- `BUILD_SPIKES=ON` 真实 cmake/Ninja 构建 `double_open_probe` 通过（`BUILD_EXIT=0`）。
- 用本地夹具 `tests/fixtures/multitrack.mkv` 真机式运行：`[FFmpeg] open OK video=0 audio=2`、`[mpv] loadfile rc=0`、`双开同时打开 = YES`（`RUN_EXIT=0`）。
- 证明产品里"mpv 播放 + FFmpeg worker 解码同时打开同一媒体"的代码路径成立。

## 运行验证（PARTIAL — 需真机 rclone mount + 网络）
- **本环境无 rclone、无网络挂载**，无法验证 VFS full 下的随机 seek 恢复与断网行为。需在真机：
  1. `rclone mount remote: Z: --vfs-cache-mode full --buffer-size 256M --drive-pacer-min-sleep 10ms`
  2. 运行 `double_open_probe.exe Z:/media/clip.mkv`（classify 应为 `network/rclone-mount`）
  3. 执行 seek storm、二次播放、网络暂断，记录缓存目录（`%LOCALAPPDATA%/rclone/cache-vfs/...`）与恢复时间。
- **验收标准**（来自 TODO）：VFS full 下随机 seek 可恢复；报告明确缓存目录、参数和观察。**未做真机挂载前不得写"通过"。**

## 结论
双开代码路径已用真实本地夹具证明可同时打开；**rclone full 的缓存/断网行为需在用户真机挂载环境下回填验证**，本环境无法替代。
