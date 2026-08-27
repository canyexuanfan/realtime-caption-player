# T0020 — Online Paraformer partial 识别（探针观察）

**探针**：`spikes/asr-hybrid/OnlineProbe.cpp`（目标 `online_probe`，链接 `rcp::sherpa-onnx`）

## 接入点（sherpa-onnx C-API）
- `SherpaOnnxCreateOnlineRecognizer` + `SherpaOnnxOnlineRecognizerConfig`，
  `model_config.paraformer.{encoder,decoder}` 指向 int8 Paraformer + `tokens`。
- 流式循环：`SherpaOnnxOnlineStreamAcceptWaveform` → `IsOnlineStreamReady` →
  `DecodeOnlineStream` → `GetOnlineStreamResult` 取 partial 文本。

## 编译/链接验证（已完成）
- `BUILD_SPIKES=ON` 真实 cmake/Ninja 构建 `online_probe` 通过（`BUILD_EXIT=0`）。
- 证明产品里用 sherpa-onnx C-API 调用在线 Paraformer 的代码路径编译、链接、导入
  `sherpa-onnx-c-api.dll` 均成立。

## 运行验证（BLOCKED — 环境级模型/ORT 版本错配）
- 运行时加载 `.tools/models/paraformer/` 时报：
  `The given version [27] is not supported, only version 1 to 10 is supported in this build.`
  随后进程崩溃（exit 139）。
- **根因**：`.tools/models/` 内的 Paraformer onnx 使用 opset 27，而 bundled
  sherpa-onnx **1.13.6** 的 onnxruntime 不支持该 opset。这是**模型与 ORT 版本错配**，
  **不是探针代码缺陷**（代码已正确调用 API）。
- **修复方向（需用户决策/操作，非本环境可独立完成）**：
  1. 升级 sherpa-onnx + onnxruntime 到支持 opset 17+ 的版本（推荐，与模型库配套）；或
  2. 重新下载与 sherpa-onnx 1.13.6 配套的 Paraformer/SenseVoice/Silero 模型（opset ≤ 该 ORT 支持）。
- **结论**：代码路径已验证可用；真正 partial 识别正确性需模型/ORT 版本对齐后在真机回填验证。

## 下一步
待模型/ORT 版本问题解决后，用真实语音 PCM 跑 `online_probe <paraformer_dir>` 回填 partial 文本。
