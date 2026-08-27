# Architecture Decision Records (ADR)

已接受的 ADR 是本仓库的**硬约束**，代码实现不得静默违背。任何技术方案变更必须先新增 ADR 并通过评审。

| ADR | 标题 | 状态 |
|---|---|---|
| [ADR-0001](ADR-0001-gpl-distribution.md) | GPL-3.0-or-later 开源分发 | Accepted |
| [ADR-0002](ADR-0002-qt-widgets.md) | Qt 6 Widgets 与跨平台边界 | Accepted |
| [ADR-0003](ADR-0003-caption-worker-process.md) | 播放器与字幕 worker 分进程 | Accepted |
| [ADR-0004](ADR-0004-paraformer-sensevoice.md) | Online Paraformer + SenseVoice 双层识别 | Accepted |
| [ADR-0005](ADR-0005-rclone-full.md) | rclone 必须使用 `--vfs-cache-mode full` | Accepted |
| [ADR-0006](ADR-0006-offline-models.md) | 默认模型随离线安装包提供 | Accepted |

新增 ADR 使用 `ADR-template.md`，编号递增（ADR-0007 起）。
