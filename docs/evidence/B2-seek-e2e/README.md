# B2 seek E2E（worker 层）— 2026-09-27

## 方法
1. `tools/gen_seek_e2e_media.ps1 -SeekSec 1800`：SAPI 中文 TTS 语音 + ffmpeg 拼接
   30 分钟静音（[0,1800s)）+ 语音（~10s）+ 5s 尾巴 → `out/e2e-verify/seek-e2e.mp4`（AAC 16k mono）。
2. `spikes/seek-e2e/SeekE2EProbe.cpp`（`-DBUILD_SPIKES=ON` 构建）：
   打开媒体 → `seekToSec(1800)` → 0.1s 分块解码并 `engine.feed(s, n, ptsMs)` → flush。
3. PASS 判定：存在非空字幕事件 startMs 落在 [1795s, 1845s]。

## 结果
- `probe-output.txt`：partial 区间 [1809464ms → 1814364ms]，识别文本
  「在第三十分钟的位置我们验证字幕能够跟随播放头定位实时语音识别一切正常」→ **SEEK-E2E-PASS (exit 0)**。
- 证明：seek 生效、PTS 锚定正确（块时间戳来自媒体 PTS 而非累计样本）、
  partial 携带正确的绝对时间区间。

## 边界（诚实标注）
- 本探针验证 worker 层管线（解码/引擎/时间戳）。"播放中 seek 后 3s 内出字幕"的
  实时窗口行为依赖真机播放器联动（mpv 播放头驱动 SetPlayhead），需带显示器的
  真机回填；沙箱无 GPU/显示。
