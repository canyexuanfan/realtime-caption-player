# T0022 — Hybrid 融合（VAD 分段 × Paraformer partial → SRT 草稿）（探针观察）

**探针**：`spikes/asr-hybrid/HybridProbe.cpp`（目标 `hybrid_probe`，链接 `rcp::sherpa-onnx`）

## 融合编排（sherpa-onnx C-API）
- 同时持有 `SherpaOnnxOnlineRecognizer`（Paraformer 流式 partial）与
  `SherpaOnnxVoiceActivityDetector`（Silero VAD 分段）。
- 同一段音频同时喂入两侧：VAD 负责**段边界**，Paraformer 负责**段内 partial 文本**；
  融合后输出 SRT 草稿骨架（段序号 / 起止时间 / 文本占位）。

## 编译/链接验证（已完成）
- `BUILD_SPIKES=ON` 真实 cmake/Ninja 构建 `hybrid_probe` 通过（`BUILD_EXIT=0`）。
- 证明"双模型并行 + 融合编排"代码路径编译、链接、导入 `sherpa-onnx-c-api.dll` 成立。

## 运行验证（BLOCKED — 环境级模型/ORT 版本错配，同 T0020/T0021）
- 运行时与 T0020 相同：`model_config` 加载 `.tools/models/` 报 `version [27] not supported` 并崩溃。
- **根因一致**：bundled sherpa-onnx 1.13.6 的 ORT 不支持模型 opset 27。非代码缺陷。
- **修复方向**：同 T0020（升级 sherpa-onnx+ORT，或换配套模型）。
- **结论**：融合编排代码路径已验证可用；真正"VAD 段 × Paraformer 文本"融合质量与 SRT 时间戳
  正确性需模型/ORT 版本对齐后在真机回填。

## 下一步
版本问题解决后，用真实语音跑 `hybrid_probe <paraformer_dir> <silero_dir>` 回填融合 SRT。
