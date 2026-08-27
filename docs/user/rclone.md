# 使用 rclone 挂载网盘播放视频

本产品支持直接播放 rclone 挂载盘（如 Google Drive、OneDrive、S3 等）上的视频。
由于播放器与字幕 worker 会**同时**打开同一个视频文件（见 ADR-0003 双开架构），
只读视频的随机 seek 与双进程读取都需要可靠的磁盘读缓存。

## 推荐挂载参数（只读视频）

```bat
rclone mount remote:/media X: ^
  --vfs-cache-mode full ^
  --vfs-cache-max-size 50G ^
  --vfs-cache-max-age 72h ^
  --vfs-read-ahead 256M ^
  --buffer-size 64M ^
  --dir-cache-time 72h ^
  --poll-interval 1m
```

| 参数 | 作用 |
|---|---|
| `--vfs-cache-mode full` | 把远端文件完整缓存到本地磁盘，支持随机 seek 与双进程复用同一份缓存。**此为只读视频的唯一推荐值。** |
| `--vfs-cache-max-size` | VFS 缓存上限，按本机磁盘空间调整。 |
| `--vfs-cache-max-age` | 缓存文件最长保留时间，到期自动清理。 |
| `--vfs-read-ahead` | 顺序预读窗口，降低连续播放卡顿。 |
| `--buffer-size` | 内存读缓冲，提升顺序读取吞吐。 |

## 为什么不能用 `--vfs-cache-mode writes`

`writes` 主要缓存**写入**操作，**不能保证只读视频的读缓存**。
在本产品的双开架构下，使用 `writes` 会导致：

- 随机 seek 高延迟，反复触发远端读取；
- mpv 与 FFmpeg 无法复用同一份 VFS 磁盘缓存；
- 违反 ADR-0005 的设计约束，禁止作为视频读缓存推荐值。

## 兼容性

- 仅影响网盘挂载盘播放路径；本地文件、UNC 路径（`\\server\share`）播放不受影响。
- rclone 是用户侧独立工具，本仓库**不分发** rclone，仅提供上述配置建议。

## 诊断与限制

- 应用内的 `RcloneDiagnostics` 仅做非侵入式诊断：检测路径类型、可选解析用户主动提供的挂载命令是否含 `full`、
  检查缓存目录是否存在/可写/剩余空间，并输出建议命令。
- 不扫描或修改任何 `.bat`、不保存密钥、不停止挂载、不删除缓存。
- MVP **不提供**“清空 rclone VFS 缓存”热按钮（无通用安全接口）。
