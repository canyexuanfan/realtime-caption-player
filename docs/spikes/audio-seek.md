# T0018 观察文档：FFmpeg seek + 采样时钟

**探针**：`spikes/audio-decode/SeekProbe.cpp`（构建目标 `seek_probe`，链接 `rcp::ffmpeg`）

## 构建验证

`BUILD_SPIKES=ON` 下 cmake/Ninja 真实构建通过（`BUILD_EXIT=0`）。

## 运行时验证（沙箱内真实运行）

```
seek_probe.exe tests/fixtures/multitrack.mkv 2.0
```

输出：

```
seek: target=2.000s seek_ts=2000 ret=0
seek: first decoded frame pts=2.0050s (delta=0.0050s) sample_offset=96240 @48000Hz/1ch
```

**关键判据通过**：
- `av_seek_frame(..., AVSEEK_FLAG_BACKWARD)` 返回 0（成功），seek 误差仅 5ms（约一帧），满足字幕/转写对齐精度。
- 采样时钟映射正确：`sample_offset = pts_sec * sample_rate * channels = 2.005 * 48000 * 1 = 96240`，与打印值一致。
  这正是 ASR 流水线把"字幕时间戳"对齐到"PCM 样本下标"的换算依据。

## 真机回填清单

| 项 | 预期 | 结果 |
|---|---|---|
| seek 到目标秒 | ret=0，首帧 pts≈目标 | ✅ 已真机式验证（delta 5ms） |
| 采样时钟映射 | sample_offset = sec·rate·ch | ✅ 已验证（96240） |
| 非关键帧 seek | 真实媒体任意 seek 点 | ⏳ 建议真机用长视频复核 |
