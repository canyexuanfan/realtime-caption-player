# Implementation Status — Realtime Caption Player

> 本文件是项目实时状态的主索引。任何 Agent 打开仓库首先读它。
> 规则：状态只能为 `TODO / IN_PROGRESS / BLOCKED / DONE / WAIVED`。禁止伪造完成。

## 当前快照（2026-08-27）

- **Current phase:** P0（技术验证、决策与第三方基线）
- **Current task:** T0010（自建并锁定 FFmpeg，**BLOCKED** 于原生依赖）/ T0011–T0024 同理 BLOCKED
- **Current branch:** `main`
- **Last completed task:** T0009（纯逻辑核心模块 + 构建系统 + 21 项单元测试全绿）
- **Last verified commit:** `d8a34a0`（[T0009] 真实构建 + 21 测试 PASS）；资产提交 `69736c5`
- **Last phase gate:** 无（P0 阶段门未过；P0 demo 任务 BLOCKED 于原生库）
- **Last update:** 2026-08-27

## 环境事实（诚实记录）

| 项 | 状态 |
|---|---|
| OS | Windows（win32） |
| C++ 编译器 `cl.exe` | ✅ VS2022 BuildTools，MSVC 14.44（14.16/14.29 亦在） |
| `git` | ✅ |
| `gh`（已登录 github.com） | ✅ |
| 互联网 | ✅（GitHub 可达） |
| `cmake` | ✅ venv（`.../envs/default/Scripts/cmake.exe`，4.4.2） |
| `ninja` | ✅ venv（同 venv `ninja.exe`，1.13.0） |
| `Qt 6` | ✅ 已就绪（仓库内 `.qt6/6.8.1/msvc2022_64`，含 Core/Gui/Widgets/Test/Sql；runtime 用 Release DLL） |
| `libmpv` / `FFmpeg` / `sherpa-onnx` | ❌ 未构建/未锁定（BLOCKED 于原生依赖） |
| `WiX Toolset` | ❌ 未安装（BLOCKED） |
| ASR 模型权重 | ❌ 未下载（BLOCKED，需合法来源与体积） |

> 说明：本会话已具备 C++ 编译器、git/gh/网络，venv 内 cmake 4.4.2 + ninja 1.13.0，以及仓库内
> `.qt6/6.8.1/msvc2022_64`（Qt 6.8.1 完整模块）。已对**不依赖 libmpv/FFmpeg/sherpa 的纯逻辑模块**完成
> 真实编译与 21 项 QtTest（全 PASS）。依赖上述原生库的模块（P2 播放内核、P4 worker ASR、P8 MSI）在库就绪前
> 标记为 BLOCKED，**不伪造构建/测试通过**。
>
> **沙箱构建须知**：本沙箱中 `Visual Studio 17 2022` generator（CMakePresets 默认）在 `project()` 查询
> `VCTargetsPath` 时崩溃（Access violation，已知 MSBuild 沙箱问题），故实际构建改用 `Ninja` generator 直调
> `cl.exe`。标准开发机仍可用 CMakePresets。

## Task Status（P0：T0001–T0024）

| Task | 标题 | Status | Commit | Verification | Evidence | Notes |
|---|---|---|---|---|---|---|
| T0001 | PRD/技术方案/TODO 纳入仓库 | DONE | 016b3c7 | 三文件可打开；git diff --check | docs/product/PRD.md, docs/architecture/技术实现方案.md, docs/development-todo.md | — |
| T0002 | ADR 模板 | DONE | 124ca9c | 模板可生成完整 ADR | docs/adr/ADR-template.md, docs/adr/README.md | — |
| T0003 | GPL 分发决定 | DONE | 07d4247 | LICENSE 与 ADR 一致；NOTICE 占位 | LICENSE, NOTICE, docs/adr/ADR-0001-gpl-distribution.md | — |
| T0004 | Qt Widgets 边界 | DONE | 951ee1c | ADR 含被否决方案 | docs/adr/ADR-0002-qt-widgets.md | — |
| T0005 | worker 分进程 | DONE | 558662f | ADR 含双开代价/IPC 不传 PCM | docs/adr/ADR-0003-caption-worker-process.md | — |
| T0006 | Paraformer+SenseVoice | DONE | 79cb563 | ADR 明确 SenseVoice≠streaming partial | docs/adr/ADR-0004-paraformer-sensevoice.md, docs/adr/ADR-0006-offline-models.md | — |
| T0007 | rclone full 修正 | DONE | 5626342 | ADR 禁止 writes 作为读缓存 | docs/adr/ADR-0005-rclone-full.md, docs/user/rclone.md | — |
| T0008 | 依赖锁 schema | DONE | 73a7e81 | schema 可解析；工具链/第三方项合规 | dependencies.lock.json | 原生库字段预留，待 T0010–T0013 补全 |
| T0009 | 固定 Qt 与构建工具环境 | DONE | d8a34a0 | 真实构建 + 21 项 QtTest 全 PASS（9.20s） | src/ tests/ CMakeLists.txt（commit d8a34a0） | 沙箱需 Ninja（见 Known Issues）；原生库模块仍 BLOCKED |
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
   处理：在具备源码与构建环境后按 T0010–T0013 自建并写 lock；模型按 ADR-0006 离线分发。
2. **Qt 已就绪**（仓库内 `.qt6/6.8.1/msvc2022_64`）——纯逻辑模块已因此完成真实编译/测试。
3. **WiX 未安装**：MSI 打包（P8）BLOCKED，待安装。

## Known Issues

- **沙箱构建限制**：CMakePresets 默认 `Visual Studio 17 2022` generator 在本沙箱 `project()` 阶段崩溃
  （Access violation，已知 MSBuild 沙箱问题）；必须使用 `Ninja` generator 直调 `cl.exe` 才能构建。
  构建命令（单条内联全部 MSVC 环境，Bash 状态不跨调用持久）：
  `cmake -G Ninja -B <build> -S <src> -DBUILD_TESTS=ON -DBUILD_PLAYER=OFF -DBUILD_WORKER=OFF -DCMAKE_BUILD_TYPE=Release ...`
  后 `cmake --build <build>`。
  此外 Debug CRT 在本机缺失（仅 14.16，无 14.44 的 Debug 运行时），且 Git Bash 的 MSYS 加载器会篡改中文 PATH
  导致运行时找不到 Qt6Core.dll，故统一用 **Release** 构建并把 `Qt6Core.dll/Qt6Test.dll/Qt6Gui.dll` 复制到
  ASCII 构建目录的 `tests/` 下紧邻测试 EXE 再跑 `ctest`。
- 上一会话将全部源码写成未提交状态（文档中"T-核心提交"并不存在）；本会话已首次真实构建并将
  `src/`+`tests/`+构建系统提交（d8a34a0）。
- PowerShell 内联 stdout 在本会话不显示，需写文件后 Read；Git Bash 输出正常。
- `03_Detailed_Development_TODO.md` 状态表示例含占位 `abc1234`，实际所有任务均为「未完成」，已据此重置认知。

## Phase Status

- P0：进行中（文档/ADR/基础 DONE；纯逻辑模块 + 单元测试 DONE；demo 因原生库 BLOCKED）。
- P1–P9：未开始（依赖 P0 通过）。

## Last Agent Summary

本会话（DevAgent 续跑）完成：
- 修复全部 15 处编译错误 + 2 处真实逻辑缺陷（Timecode::formatClock 零填充、BoundedQueue 取消死锁），
  首次真实构建 `rcp_core` 静态库 + 21 个 QtTest，**ctest 21/21 PASS（9.20s）**。
- 将此前未提交的实现（src/、tests/、CMakeLists.txt、resources、migrations、tools、docs/development）与配置
  （dependencies.lock.json、CMakePresets.json）及原始设计资产（04/05/06）提交：73a7e81(T0008)、d8a34a0(T0009)、69736c5(assets)。
- 标记 T0008/T0009 为 DONE。**P0 demo 任务（T0010–T0024）仍 BLOCKED 于缺失的原生库（libmpv/FFmpeg/sherpa-onnx）与模型权重、WiX 未安装。**
下一步：在具备原生库源码与构建环境（及合法模型权重、WiX）后推进 T0010–T0013；
在此之前不伪造任何构建/测试通过，持续维护本状态文件与 agent-handoff。
