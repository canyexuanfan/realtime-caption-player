# ADR-0004 — Online Paraformer + SenseVoice 双层识别

- **状态：** Accepted
- **日期：** 2026-08-27
- **决策人：** 架构 Agent
- **相关任务：** T0006

## 背景

需要低延迟的灰色“部分结果”与高准确度的白色“终稿”，且必须离线。

## 决策驱动约束

- **SenseVoice 不是原生在线 partial 模型**，不得误当成 continuous partial recognizer。
- partial 必须由 Online Paraformer 负责；final 由 SenseVoice（分句后）负责。

## 候选方案

- A（采用）：Balanced = `PCM → Online Paraformer(partial 灰字) ‖ VAD 分句 → SenseVoice(final 白字)`；
  Lite = `PCM → Online Paraformer → endpoint → 标点 → final`（无 SenseVoice）。
- B（否决）：把 SenseVoice 当 streaming partial 引擎。违反模型特性，终稿质量与延迟不可控。

## 决定

采用方案 A。
- partial：Online Paraformer，每 200ms 更新，相同文本不重复发。
- final：VAD 完整片段送 SenseVoice，启用 ITN/标点；通过相同 `segment_id` 替换 partial。
- 句尾后 2.5s 内 SenseVoice 未完成，先提交标点化 Paraformer 作为 revision 1；SenseVoice 后到提交 revision 2。
- Lite 档：无 SenseVoice，终稿来源标记为 `paraformer_punctuated`。

## 影响

- `SegmentAssembler` 维护有序 timeline，revision 递增覆盖。
- 过载时 Balanced 暂停 SenseVoice 新任务、回退 Lite 终稿（见技术实现方案 8.6）。
- partial 不写数据库；final/revision 在 storage 线程事务写入。

## 兼容性

`IRecognizerBackend` 允许后续加入方言/云 ASR 后端。

## 许可证

sherpa-onnx（Apache-2.0，保留 NOTICE）提供 Paraformer / SenseVoice / Silero VAD / CT-Transformer 标点。

## 回滚

如改用单一模型：失去平衡档的准确度与延迟权衡，需新 ADR。

## 验证

T0020（Paraformer partial）、T0021（SenseVoice 分句终稿）、T0022（Hybrid revision）演示与基准。
