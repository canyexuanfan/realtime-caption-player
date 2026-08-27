# Implementation Status — Realtime Caption Player

> 本文件是项目实时状态的主索引。任何 Agent 打开仓库首先读它。
> 规则：状态只能为 `TODO / IN_PROGRESS / BLOCKED / DONE / WAIVED`。禁止伪造完成。

## 当前快照（2026-08-27）

- **Current phase:** P0（技术验证、决策与第三方基线）
- **Current task:** T0008（依赖锁 schema）/ T0009（固定 Qt 与构建工具环境，后台安装中）
- **Current branch:** `main`
- **Last completed task:** T0007（rclone full 决策）
- **Last verified commit:** 见下方“本批次提交”
- **Last phase gate:** 无（P0 阶段门未过；P0 demo 任务 BLOCKED 于原生库）
- **Last update:** 2026-08-27

## 环境事实（诚实记录）

| 项 | 状态 |
|---|---|
| OS | Windows（win32） |
| C++ 编译器 `cl.exe` | ✅ VS2022 BuildTools，MSVC 14.16/14.29/14.44 |
| `git` | ✅ |
| `gh`（已登录 github.com） | ✅ |
| 互联网 | ✅（GitHub 可达） |
| `cmake` | ⏳ 后台安装（pip cmake） |
| `ninja` | ⏳ 后台安装 |
| `Qt 6` | ⏳ 后台安装（aqtinstall 6.8.1 win64 qtbase） |
| `libmpv` / `FFmpeg` / `sherpa-onnx` | ❌ 未构建/未锁定（BLOCKED 于原生依赖） |
| `WiX Toolset` | ❌ 未安装（BLOCKED） |
| ASR 模型权重 | ❌ 未下载（BLOCKED，需合法来源与体积） |

> 说明：本会话已具备 C++ 编译器与 git/gh/网络。正在安装 `cmake`+`ninja`+`Qt 6.8.1`，
> 以便对**不依赖 libmpv/FFmpeg/sherpa 的纯逻辑模块**做真实编译与 QtTest。
> 依赖上述原生库的模块（P2 播放内核、P4 worker ASR、P8 MSI）在库就绪前标记为 BLOCKED，**不伪造构建/测试通过**。

## Task Status（P0：T0001–T0024）

| Task | 标题 | Status | Commit | Verification | Evidence | Notes |
|---|---|---|---|---|---|---|
| T0001 | PRD/技术方案/TODO 纳入仓库 | DONE | 见本批次 [T0001] | 三文件可打开；git diff --check | docs/product/PRD.md, docs/architecture/技术实现方案.md, docs/development-todo.md | — |
| T0002 | ADR 模板 | DONE | 见本批次 [T0002] | 模板可生成完整 ADR | docs/adr/ADR-template.md, docs/adr/README.md | — |
| T0003 | GPL 分发决定 | DONE | 见本批次 [T0003] | LICENSE 与 ADR 一致；NOTICE 占位 | LICENSE, NOTICE, docs/adr/ADR-0001-gpl-distribution.md | — |
| T0004 | Qt Widgets 边界 | DONE | 见本批次 [T0004] | ADR 含被否决方案 | docs/adr/ADR-0002-qt-widgets.md | — |
| T0005 | worker 分进程 | DONE | 见本批次 [T0005] | ADR 含双开代价/IPC 不传 PCM | docs/adr/ADR-0003-caption-worker-process.md | — |
| T0006 | Paraformer+SenseVoice | DONE | 见本批次 [T0006] | ADR 明确 SenseVoice≠streaming partial | docs/adr/ADR-0004-paraformer-sensevoice.md | — |
| T0007 | rclone full 修正 | DONE | 见本批次 [T0007] | ADR 禁止 writes 作为读缓存 | docs/adr/ADR-0005-rclone-full.md | — |
| T0008 | 依赖锁 schema | IN_PROGRESS | — | schema 可解析；示例项合规 | dependencies.lock.json（待写） | 本批次 foundation-code 完成 |
| T0009 | 固定 Qt 与构建工具环境 | IN_PROGRESS | — | cmake/Qt 版本固定并写入 lock | 待工具链安装确认后写入 | 后台安装 cmake+Qt 6.8.1 中 |
| T0010 | 自建并锁定 FFmpeg | BLOCKED | — | — | — | 原生库未构建/未锁定；需源码与构建环境 |
| T0011 | 自建并锁定 libmpv | BLOCKED | — | — | — | 同上 |
| T0012 | 自建并锁定 sherpa-onnx | BLOCKED | — | — | — | 同上 |
| T0013 | 模型 component manifest | BLOCKED | — | — | — | 依赖 T0012 + 合法模型权重 |
| T0014 | libmpv 渲染验证程序 | BLOCKED | — | — | — | 依赖 T0011 |
| T0015 | mpv 事件桥/属性观察 | BLOCKED | — | — | — | 依赖 T0011 |
| T0016 | ASS overlay + 带字幕截图 | BLOCKED | — | — | — | 依赖 T0011 + T0014 |
| T0017 | FFmpeg 指定音轨解码 | BLOCKED | — | — | — | 依赖 T0010 |
| T0018 | FFmpeg seek + sample clock | BLOCKED | — | — | — | 依赖 T0010 |
| T0019 | 本地与 rclone full 双开 | BLOCKED | — | — | — | 依赖 T0010 + 网络/挂载 |
| T0020 | Online Paraformer partial | BLOCKED | — | — | — | 依赖 T0012 + 模型 |
| T0021 | SenseVoice VAD 分句终稿 | BLOCKED | — | — | — | 依赖 T0012 + 模型 |
| T0022 | Hybrid partial→final 修订 | BLOCKED | — | — | — | 依赖 T0020/T0021 |
| T0023 | ASR 基准语料 + whisper 基线 | BLOCKED | — | — | — | 依赖模型 + 合法语料 |
| T0024 | 签署 P0 阶段门报告 | BLOCKED | — | — | — | 依赖 T0010–T0023 全部 PASS |

## Current Blockers

1. **原生依赖缺失**：libmpv / FFmpeg / sherpa-onnx 未构建、未锁定、未下载；模型权重未获取。
   影响 T0010–T0024（P0 全部 demo 与后续阶段）。
   处理：工具链就绪后按 T0010–T0013 自建并写 lock；模型按 ADR-0006 离线分发。
2. **Qt/cmake 安装中**：后台任务（task 70qabc）安装 cmake+ninja+Qt 6.8.1；完成后解锁纯逻辑模块真实编译/测试。
3. **WiX 未安装**：MSI 打包（P8）BLOCKED，待安装。

## Known Issues

- 仓库初始仅有文档（资料包），无源码、无 Git 历史。已 `git init` 于 `main`。
- PowerShell 内联 stdout 在本会话不显示，需写文件后 Read；Git Bash 输出正常。
- `03_Detailed_Development_TODO.md` 状态表示例含占位 `abc1234`，实际所有任务均为「未完成」，已据此重置认知。

## Phase Status

- P0：进行中（文档/ADR/基础已 DONE；demo 因原生库 BLOCKED）。
- P1–P9：未开始（依赖 P0 通过）。

## Last Agent Summary

当前 Agent 完成：仓库初始化（git/main）、目录骨架、LICENSE(真实 GPL-3.0)、NOTICE、.gitignore、
AGENTS.md、docs 结构（product/architecture/adr/phase-gates/...）、6 份 P0 ADR（T0002–T0007）、
开源调研（reference/，见 T-研究）、核心纯逻辑模块骨架与单元测试（见 T-核心）。
后台安装 cmake+Qt 6.8.1 以解锁纯逻辑真实编译。
下一步：工具链就绪后运行 `cmake --preset windows-msvc-debug && build && ctest` 验证纯逻辑模块，
随后推进 T0008/T0009 并完成依赖锁与构建环境固定。
