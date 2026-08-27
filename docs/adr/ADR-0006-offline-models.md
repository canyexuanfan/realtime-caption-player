# ADR-0006 — 默认模型随离线安装包提供

- **状态：** Accepted
- **日期：** 2026-08-27
- **决策人：** 架构 Agent
- **相关任务：** T0003/T0013/T0019（离线模型分发决策）

## 背景

MVP 必须在断网状态完整工作（PRD 1.1.13、技术实现方案 0.3 明确“云端 ASR 不进入 MVP”）。

## 决策驱动约束

- 默认不联网；无遥测、无云上传、无自动模型下载。
- 模型更新不自动联网；所有默认模型随离线安装包提供。

## 候选方案

- A（采用）：Offline MSI 含 Lite + Balanced 默认模型；Lite MSI 仅含 Lite 模型。模型经 SHA-256 校验后随包分发。
- B（否决）：首次启动强制联网下载模型。违反离线硬约束与隐私要求。

## 决定

采用方案 A。模型 component manifest 记录文件清单、字节数、SHA-256、类型、采样率、语言、sherpa-onnx 兼容版本、
来源、许可证、int8/fp32、推荐线程数、是否支持 partial/标点/语言识别/时间戳、发布前基准。
模型损坏 → 禁用实时字幕 + “重新安装模型”提示，基础播放继续。

## 影响

- 打包顺序（技术实现方案 11.4）第 5 步：复制并校验模型。
- `resources/models/model-bundles.json` 描述 Lite / Balanced 两个 bundle。
- `verify-models.ps1` 在启动/安装时校验 hash。

## 兼容性

`IModelSource` 接口保留后续“用户手动导入 / 在线更新”扩展。

## 许可证

模型权重（Paraformer/SenseVoice/Silero/CT-Transformer）按各自许可证（多为 Apache-2.0 / MIT）随包并提供 NOTICE。

## 回滚

如改为联网下载：违反离线约束，禁止，除非新 ADR 明确且用户显式开启。

## 验证

干净机断网安装 → 启动 → 开启字幕 → 识别可用；网络审计确认无外连。
