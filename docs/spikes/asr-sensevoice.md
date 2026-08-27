# T0021 — Silero VAD 分句 + SenseVoice 终稿（探针观察）

**探针**：`spikes/asr-hybrid/SenseVoiceProbe.cpp`（目标 `sensevoice_probe`，链接 `rcp::sherpa-onnx`）

## 接入点（sherpa-onnx C-API）
- **Silero VAD 分句**：专用 `SherpaOnnxVoiceActivityDetector` API
  （`SherpaOnnxVadModelConfig.silero_vad` + `CreateVoiceActivityDetector` →
  `AcceptWaveform` → `Detected` → `Front`/`Pop`/`DestroySpeechSegment` 取分段）。
  **注意**：VAD 走独立 detector，**不是**离线 recognizer 的子字段（离线 config 无 `silero_vad`）。
- **SenseVoice 终稿**：`SherpaOnnxOfflineRecognizer`（`model_config.sense_voice.{model,language,use_itn}`），
  离线 `AcceptWaveformOffline` → `DecodeOfflineStream` → `GetOfflineStreamResult`。
  **注意**：SenseVoice 在 sherpa-onnx 中是*离线*模型，在线 config 无 `sense_voice` 字段。

## 编译/链接验证（已完成）
- `BUILD_SPIKES=ON` 真实 cmake/Ninja 构建 `sensevoice_probe` 通过（`BUILD_EXIT=0`）。
- 过程中修正了两处 API 误用（离线喂音频函数名、VAD 走 detector 而非 recognizer），
  证明探针代码对真实 sherpa-onnx API 的调用正确。

## 运行验证（BLOCKED — 环境级模型/ORT 版本错配，同 T0020）
- 运行时加载 `.tools/models/` 报与 T0020 相同的 `version [27] not supported` 错误并崩溃。
- **根因一致**：bundled sherpa-onnx 1.13.6 的 ORT 不支持模型 opset 27。非代码缺陷。
- **修复方向**：同 T0020（升级 sherpa-onnx+ORT，或换配套模型）。
- **结论**：VAD 分句 + SenseVoice 终稿的代码路径已验证编译/链接/API 调用；真正分句与识别
  正确性需模型/ORT 版本对齐后在真机回填。

## 下一步
版本问题解决后，用真实语音跑 `sensevoice_probe <silero_dir> <sensevoice_dir>` 回填
VAD 段数 / SenseVoice 文本。
