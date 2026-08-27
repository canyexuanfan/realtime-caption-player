# ADR-0001 — GPL-3.0-or-later 开源分发

- **状态：** Accepted
- **日期：** 2026-08-27
- **决策人：** 架构 Agent（依据 PRD 与技术实现方案 0.3）
- **相关任务：** T0003

## 背景

MVP 使用 libmpv（GPL-2.0-or-later）作为播放内核。采用 GPL 兼容路线分发整体产品，
是最保守、可执行且合规的默认选择。闭源/专有分发在替换播放内核前不可行。

## 决策驱动约束

- 产品整体以 **GPL-3.0-or-later** 许可分发。
- 使用 libmpv 时不得声称“仅放 DLL 即不受 GPL 约束”。
- 依赖许可证、NOTICE、源码来源、hash、构建参数必须可追溯。

## 候选方案

- A（采用）：整体 GPL-3.0-or-later 聚合分发。优点：合规清晰、对 libmpv 友好；缺点：限制专有再分发。
- B：闭源分发。必须先用非 GPL 播放内核替换 libmpv，且需法律审查。当前不可行。

## 决定

采用方案 A：MVP 整体 GPL-3.0-or-later。Qt 动态链接并保留 LGPLv3/重链接说明；
FFmpeg 按实际 configure flags 记录 LGPL/GPL 成分；sherpa-onnx（Apache-2.0）保留 NOTICE。

## 影响

- `LICENSE` 必须为完整 GPL-3.0 文本；`NOTICE` 列出第三方与许可证。
- 发布门禁包含“完整 GPL 源码获取方式、NOTICE、依赖许可证”。
- 任何闭源路线变更必须先新增 ADR 并做法律审查。

## 兼容性

后续若需专有分发，须替换 libmpv（见 ADR-0003 的回滚路径），不影响核心逻辑模块。

## 许可证

GPL-3.0-or-later（项目）；各依赖许可证见 `dependencies.lock.json` 与 `packaging/licenses/`。

## 回滚

如改为闭源：先替换播放内核 → 新 ADR 记录 → 移除 GPL 聚合声明。

## 验证

许可证检查脚本能找到 `LICENSE` 与 `NOTICE`；Release 产物含源码包与 SBOM。
