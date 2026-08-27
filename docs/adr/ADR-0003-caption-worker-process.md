# ADR-0003 — 播放器与字幕 worker 分进程

- **状态：** Accepted
- **日期：** 2026-08-27
- **决策人：** 架构 Agent
- **相关任务：** T0005

## 背景

ASR 推理（模型加载、ONNX 线程、CPU 峰值）若在主进程会阻塞或拖垮播放器 UI；
崩溃隔离、可独立重启、可替换后端都需要进程边界。

## 决策驱动约束

- worker 崩溃不得导致视频停止或主窗口崩溃。
- 媒体被 mpv 与 FFmpeg 各打开一次（双开），因此 rclone 挂载必须用 VFS `full` 读缓存。

## 候选方案

- A（采用）：独立 `caption-worker.exe`，`QLocalSocket` 长度前缀 JSON IPC。
  - 优点：崩溃隔离、可重启、可替换后端、测试可直接喂媒体与命令。
  - 缺点：同一媒体双开；需处理 IPC 与代际隔离。
- B（否决）：worker 合回 UI 线程 / 同进程线程。违反隔离与稳定性硬指标。

## 决定

采用方案 A。worker 负责模型加载/校验/预热、FFmpeg 解码重采样、VAD、识别、段落组装、
指标与结构化错误；不访问 SQLite、不操作 UI、不联网。主进程负责生命周期、libmpv、UI、
worker 启停/IPC/心跳/恢复、字幕选择/叠加/同步、SQLite、SRT、文件关联、诊断。

## 影响

- IPC 帧格式：uint32 LE 长度前缀 + UTF-8 JSON；最大 1 MiB；信封含 `v/type/id/generation/payload`。
- 代际（generation）隔离：seek/换媒体/换音轨/换模型语言/重开字幕均 +1。
- 不通过 IPC 传 PCM（避免大流量）；worker 自己读媒体。

## 兼容性

`IRecognizerBackend` 接口允许后续替换云 ASR / GPU 后端而不改 UI/播放核心。

## 许可证

IPC 使用 Qt `QLocalSocket`（Qt 许可）；worker 不引入额外网络/云依赖。

## 回滚

如改为同进程：丧失崩溃隔离与可重启性，违反稳定性硬指标，不可取。

## 验证

集成测试：杀掉 worker 进程 → 视频继续 → 自动重启（≤3 次）→ 恢复当前媒体/音轨/语言/档位/playhead；
主播放器不中断、不崩。
