# ADR-0005 — rclone 必须使用 `--vfs-cache-mode full`

- **状态：** Accepted
- **日期：** 2026-08-27
- **决策人：** 架构 Agent
- **相关任务：** T0007

## 背景

网盘挂载盘上的视频由 mpv 与 worker(FFmpeg) 同时打开（双开架构，见 ADR-0003）。
只读视频的随机 seek 与双进程读需要可靠的磁盘读缓存。

## 决策驱动约束

- `writes` 主要缓存写入，**不能保证只读视频的读缓存**；不可作为本产品视频读缓存推荐值。
- 双开架构下，`full` 让 mpv 与 FFmpeg 复用 VFS 磁盘缓存。

## 候选方案

- A（采用）：`--vfs-cache-mode full` + `--vfs-cache-max-size` + `--vfs-cache-max-age` + `--vfs-read-ahead` + `--buffer-size`。
- B（否决）：`--vfs-cache-mode writes`。理由：不满足只读视频随机 seek 的读缓存需求。

## 决定

采用方案 A。`RcloneDiagnostics` 仅做非侵入式诊断：检测路径类型、可选解析用户主动提供的挂载命令是否含 `full`、
检查缓存目录存在/可写/剩余空间、输出建议命令与复制按钮；不扫描/修改所有 `.bat`、不保存密钥、不停止挂载、不删缓存。

## 影响

- 推荐挂载参数写入用户文档与诊断建议。
- MVP 不提供“清空 rclone VFS 缓存”热按钮（无通用安全接口）。

## 兼容性

仅影响网盘播放路径；本地/UNC 播放不受影响。

## 许可证

rclone 为独立用户侧工具，不在本仓库分发；仅提供配置建议。

## 回滚

如改为 `writes`：双开 seek 高延迟、重复远端读取，违反设计约束，禁止。

## 验证

T0019（本地与 rclone full 双开读取）演示；发布门禁明确“rclone 文档仍推荐 writes 作为只读视频缓存”则不得发布。
