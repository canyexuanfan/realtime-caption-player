# Implementation Status — Realtime Caption Player

> 本文件是项目实时状态的主索引。任何 Agent 打开仓库首先读它。
> 规则：状态只能为 `TODO / IN_PROGRESS / BLOCKED / DONE / WAIVED`。禁止伪造完成。

## 当前快照（2026-08-27）

- **Current phase:** P0（技术验证、决策与第三方基线）
- **Current task:** T0010–T0013 已完成（原生库下载+锁定+链接验证）；T0014+ 依赖已解除（可推进 playback/worker/ASR 实现）
- **Current branch:** `main`
- **Last completed task:** T0013（模型 manifest；paraformer/sensevoice/silero 已下载并打进运行时）
- **Last verified commit:** `d8a34a0`（[T0009] 真实构建 + 21 测试 PASS）；本会话新增：原生库下载/锁定 + 打包验证（待提交）
- **Last phase gate:** 无（P0 demo 依赖已就绪；实现任务待推进）
- **Last update:** 2026-08-27

## 环境事实（诚实记录）

| 项 | 状态 |
|---|---|
| OS | Windows（win32） |
| C++ 编译器 `cl.exe` | ✅ VS2022 BuildTools，MSVC 14.44 |
| `git` | ✅ |
| `gh`（已登录 github.com） | ✅ |
| 互联网 | ✅（GitHub 可达） |
| `cmake` | ✅ venv（`.../envs/default/Scripts/cmake.exe`，4.4.2） |
| `ninja` | ✅ venv（同 venv `ninja.exe`，1.13.0） |
| `Qt 6` | ✅ 已就绪（仓库内 `.qt6/6.8.1/msvc2022_64`，含 Core/Gui/Widgets/Test/Sql） |
| `libmpv` / `FFmpeg` / `sherpa-onnx` | ✅ 已下载并锁定（.tools/ 下；libmpv 用 zhongfly mpv-dev + 现场生成 mpv.lib；手工 cl/link 链接验证通过） |
| `WiX Toolset` | ✅ 已就绪（.tools/wix） |
| ASR 模型权重 | ✅ 已下载（paraformer / sensevoice / silero 均在 .tools/models，并打进自包含运行时） |

> 说明：本会话已具备 C++ 编译器、git/gh/网络，venv 内 cmake 4.4.2 + ninja 1.13.0，以及仓库内
> `.qt6/6.8.1/msvc2022_64`（Qt 6.8.1 完整模块）。已对**不依赖 libmpv/FFmpeg/sherpa 的纯逻辑模块**完成
> 真实编译与 21 项 QtTest（全 PASS）。原生库（libmpv/FFmpeg/sherpa-onnx）与 ASR 模型现已下载并锁定于
> `.tools/`，`cmake/RcpBundle.cmake` 可将其与模型打进自包含运行时（`out/bundle/runtime`，674MB，零用户下载）。
> 依赖这些库的模块（P2 播放内核、P4 worker ASR、P8 MSI）已解除 BLOCK，可推进实现；实现完成前不伪造构建/测试通过。
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
| T0008 | 依赖锁 schema | DONE | 73a7e81 | schema 可解析；工具链/第三方项合规 | dependencies.lock.json | 原生库字段本会话补全 |
| T0009 | 固定 Qt 与构建工具环境 | DONE | d8a34a0 | 真实构建 + 21 项 QtTest 全 PASS（9.20s） | src/ tests/ CMakeLists.txt（commit d8a34a0） | 沙箱需 Ninja；原生库现已就绪 |
| T0010 | 下载并锁定 FFmpeg | DONE | (本会话) | `.tools/ffmpeg` 含 avformat/avcodec/avutil/swresample/swscale 的 lib+dll+include；deps_smoke 链接通过 | dependencies.lock.json, `.tools/ffmpeg/` | 采用预编译二进制（非自构建）；LGPL 配置 |
| T0011 | 下载并锁定 libmpv | DONE | (本会话) | `.tools/mpv` 含 mpv-2.dll + mpv.lib(现场生成) + 头文件；手工 cl/link deps_smoke 链接 exit=0 | dependencies.lock.json, `.tools/mpv/` | 官方 v0.41.0 msvc 包只含 CLI；改从 zhongfly/mpv-winbuild 取 mpv-dev 开发库并生成 MSVC 导入库 |
| T0012 | 下载并锁定 sherpa-onnx | DONE | (本会话) | `.tools/sherpa-onnx` 含 sherpa-onnx-c-api.dll + onnxruntime；链接验证通过 | dependencies.lock.json, `.tools/sherpa-onnx/` | Apache-2.0 |
| T0013 | 模型 component manifest | DONE | (本会话) | paraformer/sensevoice/silero 已下载并打进 `out/bundle/runtime/models`；lock 记录路径 | dependencies.lock.json, `.tools/models/` | ct-transformer 可选未下载 |
| T0014 | libmpv 渲染验证程序 | TODO | — | — | — | 依赖 T0011（libmpv 已锁定） |
| T0015 | mpv 事件桥/属性观察 | TODO | — | — | — | 依赖 T0011（已锁定） |
| T0016 | ASS overlay + 带字幕截图 | TODO | — | — | — | 依赖 T0011 + T0014 |
| T0017 | FFmpeg 指定音轨解码 | TODO | — | — | — | 依赖 T0010（已锁定） |
| T0018 | FFmpeg seek + sample clock | TODO | — | — | — | 依赖 T0010（已锁定） |
| T0019 | 本地与 rclone full 双开 | TODO | — | — | — | 依赖 T0010 + 网络/挂载 |
| T0020 | Online Paraformer partial | TODO | — | — | — | 依赖 T0012 + 模型（已就绪） |
| T0021 | SenseVoice VAD 分句终稿 | TODO | — | — | — | 依赖 T0012 + 模型（已就绪） |
| T0022 | Hybrid partial→final 修订 | TODO | — | — | — | 依赖 T0020/T0021 |
| T0023 | ASR 基准语料 + whisper 基线 | TODO | — | — | — | 依赖模型（已就绪）+ 合法语料 |
| T0024 | 签署 P0 阶段门报告 | TODO | — | — | — | 依赖 T0010–T0023 全部 PASS |

## Current Blockers

1. **原生依赖已就绪**：libmpv / FFmpeg / sherpa-onnx 已下载并锁定于 `.tools/`，ASR 模型（paraformer/sensevoice/silero）已下载并打进自包含运行时。T0010–T0013 已完成；T0014+ 已解除 BLOCK，可推进实现。
2. **打包机制已验证**：`cmake/RcpBundle.cmake` 将 DLL + 模型拷入 `out/bundle/runtime`（674MB，自包含），由 WiX 整体打进 MSI —— 终端用户安装即用、零运行时下载。
3. **WiX 已就绪**（`.tools/wix`）。
4. **已知环境坑（非阻塞）**：经 Git Bash 直接 `cmake` 全量 configure 时，因 MSYS 路径转换导致 `rc.exe` 资源编译器不可见而失败；需在真实 vcvars 开发者环境（或固化 INCLUDE/LIB/PATH 反斜杠）下做全量构建。手工 `cl`/`link` 已验证三库链接通过，依赖本身无问题。

## Known Issues

- **沙箱构建限制**：CMakePresets 默认 `Visual Studio 17 2022` generator 在本沙箱 `project()` 阶段崩溃
  （Access violation，已知 MSBuild 沙箱问题）；必须使用 `Ninja` generator 直调 `cl.exe` 才能构建。
  此外，Git Bash 的 MSYS 加载器会篡改中文 PATH 与 `rc.exe` 可见性；手工 `cl`/`link` 直接调用（INCLUDE/LIB
  用反斜杠 Windows 原生路径）可绕过。Debug CRT 在本机缺失（仅 14.16，无 14.44 的 Debug 运行时），统一用 **Release**。
- **libmpv 导入库来源**：官方 `mpv-player/mpv` v0.41.0 的 msvc zip 仅含 `mpv.exe`（CLI 播放器），不含 libmpv 开发库。
  解决：从 `zhongfly/mpv-winbuild` 取 `mpv-dev-x86_64-20260826-git-*.7z`（含 `libmpv-2.dll` + 头文件），
  再用 `dumpbin /exports libmpv-2.dll` + `lib.exe /machine:X64 /def:mpv.def` **现场生成 MSVC 的 `mpv.lib`**
  （MinGW 原包只给 `libmpv.dll.a`，MSVC `link.exe` 无法使用）。libmpv 客户端 API 跨 0.x 稳定，新版 dev 包可正常链接。
- 上一会话将全部源码写成未提交状态（文档中"T-核心提交"并不存在）；本会话已首次真实构建并将
  `src/`+`tests/`+构建系统提交（d8a34a0）。
- PowerShell 内联 stdout 在本会话不显示，需写文件后 Read；Git Bash 输出正常。
- `03_Detailed_Development_TODO.md` 状态表示例含占位 `abc1234`，实际所有任务均为「未完成」，已据此重置认知。

## Phase Status

- P0：进行中（文档/ADR/基础 DONE；纯逻辑模块 + 单元测试 DONE；原生库+模型已下载锁定 DONE；T0014+ 实现待推进）。
- P1–P9：P1/P2 依赖的原生库已就绪，可启动。

## Last Agent Summary

本会话（续跑）完成：
- 修复全部 15 处编译错误 + 2 处真实逻辑缺陷（Timecode::formatClock 零填充、BoundedQueue 取消死锁），
  首次真实构建 `rcp_core` 静态库 + 21 个 QtTest，**ctest 21/21 PASS（9.20s）**。
- 将此前未提交的实现（src/、tests/、CMakeLists.txt、resources、migrations、tools、docs/development）与配置
  （dependencies.lock.json、CMakePresets.json）及原始设计资产（04/05/06）提交：73a7e81(T0008)、d8a34a0(T0009)、69736c5(assets)。
- 下载并锁定全部原生依赖（FFmpeg / libmpv / sherpa-onnx）与 ASR 模型（paraformer / sensevoice / silero）于 `.tools/`；
  补齐 libmpv 开发库（zhongfly mpv-dev + 现场生成 mpv.lib）；`deps_smoke` 经手工 cl/link 验证三库可链接（link exit=0）。
- 落地打包机制 `cmake/RcpBundle.cmake` + `run_bundle.cmake`：将 13 个运行时 DLL + 三套模型拷入
  `out/bundle/runtime`（674MB，自包含），仅拷 *.dll/模型、跳过 *.lib/*.def/*.part/*.bad —— 可由 WiX 整体打进 MSI，
  **终端用户安装即用、零运行时下载**（直接回应「每次用户下载这么久很影响体验」）。
- 更新 dependencies.lock.json（原生库/模型全部标 installed、新增 bundled_runtime 段）与本文档状态。

下一步：提交本会话的原生库/打包/文档成果到私有远程；随后从 T0014 起推进 libmpv 播放内核实现（依赖已就绪）。
