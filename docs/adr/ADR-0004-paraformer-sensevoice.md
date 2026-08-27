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

采用方案 A（Balanced 双层），**但 partial 引擎在 P0 spike 阶段确定为 Zipformer2-CTC（非 Paraformer）**：
- partial（实时逐字）：**Online Zipformer2-CTC**，每 200ms 更新，相同文本不重复发。
- final（精准终稿）：VAD 完整片段送 SenseVoice，启用 ITN/标点；通过相同 `segment_id` 替换 partial。
- 句尾后 2.5s 内 SenseVoice 未完成，先提交标点化 Zipformer2-CTC 作为 revision 1；SenseVoice 后到提交 revision 2。
- Lite 档：无 SenseVoice，终稿来源标记为 `zipformer2_ctc_punctuated`。

> 历史说明：原 ADR 草拟时 partial 层写的是 Online Paraformer。P0 spike 证实该 2023-02 双语 Paraformer int8 模型图与 ORT 流式路径不兼容（1.13.6 与 1.12.1 均崩），故 partial 引擎改为现代 ORT-1.27 兼容的 Zipformer2-CTC。

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

- **运行时发现**：在 sherpa-onnx 1.13.6 / ORT 1.27.1 下，原 partial 层（Online Paraformer int8）在 `SherpaOnnxCreateOnlineRecognizer` 硬崩溃（零 stderr，exit 127）；官方 `sherpa-onnx-vad-with-online-asr.exe` 用相同模型+正确参数复现。
- **根因（已锁定）**：崩溃来自 **2023-02 双语 Paraformer int8 模型图本身与 ORT 流式路径不兼容**，**非 ORT 版本 bug、非代码缺陷**。决定性证据：用官方 `sherpa-onnx.exe`（**1.12.1 / 旧 ORT**）跑**同一模型** → 同样在识别器创建处硬崩溃 → 降 sherpa/ORT 版本无效。已否定方向：①降版 sherpa/ORT（1.12.1 同崩）；②fp32 encoder（双语 Paraformer fp32 包 998MB、MSI >1GB，非产品方向）。
- **final 层（SenseVoice 离线 + Silero VAD）在 1.13.6/ORT 1.27.1 下完整跑通**（`sensevoice_probe` exit=0、`hybrid_probe` 精准路径 RC=0），中文识别强、ITN 开启，满足"准确到可替代观看"的验收标准。
- **修复（用户硬要求：实时逐字必须可用、不可降级）**：**partial 引擎改为现代 ORT-1.27 兼容的 Zipformer2-CTC 中文流式模型**（CLI `--zipformer2-ctc-model`，C-API `config.model_config.zipformer2_ctc.model`，单文件 `model.int8.onnx`+`tokens.txt`，cjkchar 字符级）。推荐 `sherpa-onnx-streaming-zipformer-ctc-zh-int8-2025-06-30`（生产级）或 `...-multi-zh-hans-2023-12-13`（备选）。`OnlineProbe`/`HybridProbe` 已改写为使用 `zipformer2_ctc` 字段；`HybridProbe` = VAD 分句 → 每段①Zipformer2-CTC 实时 partial ②SenseVoice 精准终稿。待中文 Zipformer2-CTC 模型下载后真机验证。
- **沙箱限制（已修正误判）**：此前记"TLS 握手失败无法下载"有误；实测 `curl -k` 可下载小文件（已拉取 1.12.1/1.11.1 shared 包与便携 exe）。真正限制是代理对**大文件(>~100MB)中途丢连接**（多次在 ~46MB 处断）。小体积中文流式模型（`zipformer-ctc-small-2024-03-18` ~30MB）可 `curl -k`+续传拉取先在沙箱验证；生产级大模型用户本机拉取。真实中文准确率验证仍因沙箱无显示器/语料/网盘挂载需用户本机回填。
- **目标架构**：方案 A（Balanced 双层）仍是产品目标形态；partial 引擎由"崩溃的 Paraformer"替换为"Zipformer2-CTC"，实时逐字 + 精准终稿双引擎互补，两者都交付。
