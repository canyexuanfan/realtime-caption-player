# 参考：sherpa-onnx（ASR 运行时）

- **仓库：** https://github.com/k2-fsa/sherpa-onnx
- **Stars：** 14,411（gh api，2026-08-27）
- **许可证：** Apache-2.0（保留 NOTICE）
- **角色：** 提供 VAD（Silero）、Online Paraformer（partial）、SenseVoice（final 分句终稿）、CT-Transformer（标点）。
  - 基于 ONNX Runtime，纯 CPU 推理，无需 PyTorch，跨平台（Windows/Linux/macOS/嵌入式）。
- **版本基线（技术实现方案 3.1）：** sherpa-onnx 1.13.6（实际锁版本以 `dependencies.lock.json` 为准）。

## 我们怎么用（复用程度：链接 C API + 复用官方示例模式）

### Online Paraformer（partial 灰字）—— 对应 `src/worker/OnlineParaformerBackend`
官方 C API 示例 `c-api-examples/streaming-paraformer-c-api.c` 给出标准循环（与我们的后端一一对应）：
```c
SherpaOnnxOnlineParaformerModelConfig paraformer_config; memset(...);
paraformer_config.encoder = "encoder.int8.onnx";
paraformer_config.decoder = "decoder.int8.onnx";
SherpaOnnxOnlineModelConfig m; m.tokens="tokens.txt"; m.paraformer=paraformer_config; m.num_threads=...; m.provider="cpu";
SherpaOnnxOnlineRecognizer *rec = SherpaOnnxCreateOnlineRecognizer(&cfg);
SherpaOnnxOnlineStream *s = SherpaOnnxCreateOnlineStream(rec);
// 逐块喂 PCM（如每 100ms）
SherpaOnnxOnlineStreamAcceptWaveform(s, sample_rate, samples, n);
while (SherpaOnnxIsOnlineStreamReady(rec, s)) SherpaOnnxDecodeOnlineStream(rec, s);
const SherpaOnnxOnlineRecognizerResult *r = SherpaOnnxGetOnlineStreamResult(rec, s);
// r->text 为 partial；SherpaOnnxOnlineStreamIsEndpoint 判定句尾
if (SherpaOnnxOnlineStreamIsEndpoint(rec, s)) { /* 句尾 */ SherpaOnnxOnlineStreamReset(rec, s); }
```
- 模型：`sherpa-onnx-streaming-paraformer-bilingual-zh-en`（中英双语，int8 约 encoder 158M + decoder 68M；fp32 更大）。**注意：该模型不支持词级时间戳** → 片段时间用音频 sample clock + VAD/endpoint 边界（技术方案 8.2 Lite 链路）。
- 采样率 16 kHz 单声道 float32（与 worker 重采样目标一致，技术方案 7.5）。

### SenseVoice（final 白字）—— 对应 `src/worker/SenseVoiceBackend`
- 模型：`sherpa-onnx-sense-voice-zh-en-ja-ko-yue-2024-07-17`（int8）。支持中/英/日/韩/粤，含 ITN/标点。
- 用法：VAD 完整分句后整段送入 SenseVoice 非流式识别，得到高质量终稿；通过相同 `segment_id` 替换 partial（技术方案 8.2 Balanced 链路、ADR-0004）。

### Silero VAD —— 对应 `src/worker/VadSegmenter`
- 模型：`silero_vad.onnx`（MIT 权重）。
- 参数（技术方案 8.3）：最短语音 250ms、开始/结束 padding 200/300ms、静音断句 550ms、单段最大 25s、最短有效片段 400ms。

### CT-Transformer 标点 —— 对应 `src/worker/PunctuationBackend`
- 模型：`sherpa-onnx-ct-transformer-zh-en`（int8）。Lite 链路终稿标点化（技术方案 8.2）。

## 风险 / 注意

- **SenseVoice ≠ streaming partial**（ADR-0004 硬约束）：只用它做分句后的 final，不把它当连续 partial 引擎。
- **模型体积**：Balanced 包含 Paraformer+SenseVoice+VAD+标点，离线 MSI 较大 → 提供 Lite/Offline 两包（技术方案 11.4，ADR-0006）。
- **线程数/RSS**：需在 manifest 记录推荐线程数；过载降级见技术方案 8.6。
- **Apache-2.0 NOTICE** 必须随分发保留。

## 合规

- `packaging/third-party-notices/` 包含 sherpa-onnx NOTICE 与 Apache-2.0 文本。
- 模型权重许可证（Paraformer/SenseVoice/Silero/CT-Transformer 多为 Apache-2.0/MIT）在 `dependencies.lock.json` 与模型 manifest 记录，随包并提供 NOTICE。
