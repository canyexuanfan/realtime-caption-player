# 参考：FFmpeg（worker 音频解码）

- **仓库：** https://github.com/FFmpeg/FFmpeg
- **Stars：** 63,666（gh api，2026-08-27）
- **许可证：** LGPL-2.1-or-later（若以默认 configure 构建）；若启用 GPL 组件则为 GPL-2.0。
  - 我们**以 LGPL 配置构建**用于分发的运行库，并在 `third_party/build-metadata/ffmpeg.txt` 记录完整 configure flags。
- **角色：** caption-worker 用 FFmpeg 再次打开当前媒体，按主进程给出的 `audio_ff_index` 定位音轨，解码并重采样为模型要求的**单声道 float32 PCM**（技术方案 4.2、7.5）。

## 我们怎么用（复用程度：链接 libav* + 标准解码流程）

### 使用的库
- `libavformat`（打开/探测/seek）、`libavcodec`（解码）、`libavutil`（帧/采样率/时间基）、`libswresample`（重采样到目标采样率/单声道/float32）。

### 标准解码流程（对应 `src/worker/AudioDecoder` + `AudioResampler` + `DecodeScheduler`）
1. `avformat_open_input`（UTF-8/宽字符路径，不经 shell）。
2. `avformat_find_stream_info`。
3. 校验 `audio_ff_index` 为音频流；否则返回 `ASR-AUDIO-STREAM-NOT-FOUND`（技术方案 6.6）。
4. 找到 decoder，`avcodec_alloc_context3` + `avcodec_open2`。
5. `swr_alloc_set_opts`（目标：模型采样率 / 单声道 / float32 / 规范化的 [-1,1]）。
6. 解码循环 `av_read_frame` → `avcodec_send_packet`/`receive_frame` → `swr_convert` → 输出 `PcmChunk`（含 `generation/start_ms/samples/discontinuity/end_of_stream`）。
7. 时间戳：从解码帧 `pts` + 累计样本数构造**单调 sample clock**，不依赖 packet PTS 直接当样本时间（技术方案 7.5）。

### seek（技术方案 7.6）
- `avformat_seek_file` 定位到 `target_ms - 800ms` 关键帧附近；`avcodec_flush_buffers`；重置 resampler/sample clock/VAD；解码并丢弃目标前音频，仅保留 ≤300ms pre-roll；首个 chunk `discontinuity=true`；不把 pre-roll 文本显示为目标前字幕。
- 不支持 seek 的媒体返回 `ASR-MEDIA-NOT-SEEKABLE`。

### 解码提前量调度（技术方案 7.7）
- 以“媒体时间”而非“墙钟时间”调度：`target_lead_ms = clamp(30000*speed, 30000, 90000)`；`max_lead_ms = min(target_lead_ms+30000, 120000)`；暂停时最多到 `max_lead_ms` 后休眠。

## 风险 / 注意

- **双开**：同一媒体被 mpv 与 FFmpeg 各打开一次 → rclone 挂载必须 `vfs-cache-mode full`（ADR-0005）。
- **音轨映射**：必须用 `ff-index` 与主进程 `track-list` 对齐，禁止“第一个音轨”（多语言 MKV 会错，技术方案 6.6）。
- **LGPL vs GPL**：分发构建保持 LGPL 配置；若确需 GPL 组件，须在 `dependencies.lock.json` 标注并整体合规。

## 合规

- `packaging/licenses/` 包含 FFmpeg LICENSE（LGPL-2.1 + GPL-2.0 混合说明）与使用的第三方库（如 zlib/Libav 等）声明。
- 记录完整 configure flags 与库许可证（技术方案 3.1）。
