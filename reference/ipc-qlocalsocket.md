# 参考：QLocalSocket IPC（player ↔ caption-worker）

- **来源：** Qt 6 自带 `QLocalSocket` / `QLocalServer`（Qt 许可，动态链接 LGPLv3）。
- **角色：** 主进程与独立 `caption-worker.exe` 之间的进程间通信（技术方案 4.1、7.2–7.3）。

## 我们怎么用（复用程度：自实现协议 + Qt 传输）

### 传输
- 主进程建立当前用户专属 `QLocalServer`（命名管道，仅当前用户可访问）。
- worker 以 `--pipe-name <随机名>` 连接；不通过 shell 拼接命令（`QProcess::setProgram/setArguments`）。

### 帧格式（长度前缀 JSON，技术方案 7.2）
```
uint32 LE  payload_length
payload_length 字节  UTF-8 JSON
```
- 最大帧 1 MiB；超限立即断开并记录 `IPC-FRAME-TOO-LARGE`。
- JSON 必须是对象，含 `v/type/id/generation/payload`；未知类型返回错误不崩溃。
- **不发送 PCM**（避免 IPC 大流量）；worker 自己读媒体。
- 每个命令返回 `ack` 或 `error`；事件可无请求 ID。

### 消息清单（技术方案 7.3）
- 主→worker：`command.hello/load_model_bundle/open_media/close_media/set_playhead/seek/set_paused/set_speed/set_audio_track/set_language/set_profile/start_captioning/stop_captioning/shutdown`
- worker→主：`event.ready/model_progress/media_opened/caption_partial/caption_final/caption_revision/coverage_progress/overload/metrics/heartbeat/error`

### 代际隔离（技术方案 7.4）
- 打开新媒体 / seek / 切音轨 / 切模型语言 / 重开字幕 → `generation+1`；旧代事件丢弃，绝不写入当前会话。

### 安全（技术方案 14.5、19）
- token 不写入普通日志；worker 每 2s 心跳，连续 3 次丢失判失联；主进程关闭先 `command.shutdown` 超时后终止；worker 监控 parent PID 自行退出。
- 恶意超长帧 / 坏 JSON / 路径注入在 `FrameCodec` 层拒绝。

## 参考实现要点
- 参考 Qt 官方 `QLocalSocket` 文档的“按长度读取”模式（`waitForReadyRead` + 缓存未完整帧）以正确处理**粘包/拆包**。
- 对应 `src/ipc/FrameCodec.h/.cpp`（长度前缀编解码）、`JsonMessageCodec.h/.cpp`（JSON 信封校验）、`WorkerClient.h/.cpp`、`WorkerSupervisor.h/.cpp`。

## 风险 / 注意
- Windows 命名管道 ACL：确保仅当前用户可访问（随机名 + 当前用户会话）。
- 不得把 Partial 写入最终 SRT（技术方案 19）。

## 合规
- 纯 Qt 能力，无额外第三方许可；遵守 Qt LGPLv3 动态链接要求。
