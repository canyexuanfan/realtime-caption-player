# T0017 观察文档：FFmpeg 指定音轨解码

**探针**：`spikes/audio-decode/main.cpp`（构建目标 `audio_decode_probe`，链接 `rcp::ffmpeg`）
**夹具**：`tests/fixtures/multitrack.mkv`（工作空间 FFmpeg 生成，2 条 AAC mono/48000 音轨，73KB）

## 构建验证

`BUILD_SPIKES=ON` 下 cmake/Ninja 真实构建通过（`BUILD_EXIT=0`），`dumpbin /dependents` 确认导入
`avformat-63.dll` / `avcodec-63.dll` / `avutil-61.dll` / `swresample-7.dll`。

## 运行时验证（沙箱内真实运行，非仅编译链接）

```
audio_decode_probe.exe tests/fixtures/multitrack.mkv 0
audio_decode_probe.exe tests/fixtures/multitrack.mkv 1
```

输出（节选）：

```
[audio #0] stream=0 codec=aac ch=mono rate=48000 lang=?
[audio #1] stream=1 codec=aac ch=mono rate=48000 lang=?
audio_decode: decoding stream=0 (PCM S16)
audio_decode: frames=188 total_samples=192512 (~4.01s) checksum=3395679301
audio_decode: decoding stream=1 (PCM S16)
audio_decode: frames=188 total_samples=192512 (~4.01s) checksum=3349846617
```

**关键判据通过**：两条音轨帧数/样本数相同（同为 4s 正弦），但 **checksum 不同** → 证明探针确实按
`stream_index` 取到了不同音轨的 PCM，而非同一条数据。指定音轨解码链路（解封装→选流→解码→重采样 S16）端到端成立。

## 夹具生成命令（如需重新生成）

```bat
set PATH=%PATH%;.tools\ffmpeg\bin
ffmpeg -y -hide_banner -loglevel error ^
  -f lavfi -i "sine=frequency=440:duration=4:sample_rate=48000" ^
  -f lavfi -i "sine=frequency=880:duration=4:sample_rate=48000" ^
  -map 0:a -map 1:a -c:a aac tests/fixtures/multitrack.mkv
```

> 注：工作空间 FFmpeg 构建含 `--disable-whisper`，与项目"禁用 Whisper、用 FunASR"的长期规则一致。

## 真机回填清单

| 项 | 预期 | 结果 |
|---|---|---|
| 音轨映射枚举 | 列出每条 audio 流的 codec/声道/采样率/语言 | ✅ 已真机式验证 |
| 指定流解码 | 按 stream_index 取对应音轨 | ✅ 已验证（checksum 区分） |
| S16 PCM 输出 | 帧数/时长与源一致 | ✅ 已验证（4.01s） |
| 多语言/多编解码器 | 真实媒体含不同语言/编码时仍正确 | ⏳ 建议真机用真实剧集样本复核 |
