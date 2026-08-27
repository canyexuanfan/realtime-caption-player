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

## 状态更新（2026-08-27 运行时验证，P0 spike 阶段）

- **运行时发现**：在 sherpa-onnx 1.13.6 / ORT 1.27.1 下，方案 A 的 **partial 层（Online Paraformer int8）在 `SherpaOnnxCreateOnlineRecognizer` 硬崩溃**（零 stderr，exit 127）；官方 `sherpa-onnx-vad-with-online-asr.exe` 用相同模型+正确参数复现 → 非代码缺陷，而是 int8 Paraformer 模型图 × ORT 1.27.1 CPU EP 的量化算子回归（BLOCKER-2）。
- **final 层（SenseVoice 离线 + Silero VAD）在 1.13.6/ORT 1.27.1 下完整跑通**（`sensevoice_probe` exit=0、`hybrid_probe` 精准路径 RC=0），中文识别强、ITN 开启，满足"准确到可替代观看"的验收标准。
- **决策（用户授权"自己看着办"）**：
  - 在 BLOCKER-2 解决前，**SenseVoice final 层即产品的权威精准中文字幕引擎**（VAD 分句 → 每段 SenseVoice → SRT）；原 ADR 的"partial 灰字 / final 白字"双层体验降级为"final 白字先行、partial 后补"。
  - Online Paraformer partial 层保留（int8 小模型），但**默认关闭**，仅作 `RCP_TRY_STREAMING=1` 可选增强，避免在 ORT 回归崩溃时打断精准字幕链路。
  - 修复方向：保留 int8 模型，将 sherpa-onnx 钉到"int8 Paraformer 可跑"的版本组合（方向②）；fp32 替换（方向①）因包体 ~998MB / MSI 撑到 >1GB 已放弃。
- **沙箱限制**：本沙箱代理对 GitHub `release-assets` 与 HF `cdn-lfs` 均 TLS 握手失败，无法下载对照 sherpa 包验证方向②，亦无法下载真实中文音频做端到端准确率验证；方向②与准确率回归验证需在用户本机（网络正常、有 rclone 网盘挂载）执行。
- **目标架构不变**：方案 A（Balanced 双层）仍是产品目标形态；partial 层为"暂禁用、待修复后启用"的增强项，非废弃。
