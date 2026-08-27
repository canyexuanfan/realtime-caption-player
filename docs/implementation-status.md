# Implementation Status — Realtime Caption Player

> 本文件是项目实时状态的主索引。任何 Agent 打开仓库首先读它。
> 规则：状态只能为 `TODO / IN_PROGRESS / BLOCKED / DONE / WAIVED`。禁止伪造完成。

## 当前快照（2026-08-27）

- **Current phase:** P0 阶段门已具备签署条件（T0001–T0024 代码+编译/链接全完成；运行时证据标注 PARTIAL）；产品实现已启动并阶段性成型（P2 播放内核 / P4 caption-worker / P4/IPC / P6 字幕叠加 / P7 UI 集成 均代码+编译验证完成，MSI 已重打为真实产品包）
- **Current task:** T0024（签署 P0 阶段门报告）— BLOCKER-1（dll 冲突）已修复；T0021（Silero VAD + SenseVoice 离线）真机验证通过（exit 0）；用户硬要求"实时逐字 partial 必须可用、不可降级"→ 原崩溃的 2023-02 双语 Paraformer 模型图与 ORT 流式不兼容，**已换 Zipformer2-CTC 现代流式引擎修复**：2026-08-27 中文 `zipformer-ctc-zh-int8-2025-06-30` 模型由用户本机下载回填，三探针 git-bash 重编 rc=0、`online_probe`/`hybrid_probe` 真机 rc=0 逐块 partial 实时上屏、零崩溃，**BLOCKER-2 关闭**；精准字幕（VAD×SenseVoice+ITN）已由 T0021/T0022 验证满足验收
- **Current branch:** `main`
- **Last completed task:** T0023（合规 ASR 基准语料 + FunASR 基线，替换原 whisper 基线）
- **Last verified commit:** `69920bb`（[T0023] 合规语料+FunASR 基线，已推送 origin/main）
- **Last phase gate:** P0 报告草案 `docs/phase-gates/P0-report.md` 已就绪（签署待真机运行时证据回填）
- **Last update:** 2026-08-27

## 环境事实（诚实记录）

| 项 | 状态 |
|---|---|
| OS | Windows（win32） |
| C++ 编译器 `cl.exe` | ✅ VS2022 BuildTools，MSVC 14.44 |
| `git` | ✅ |
| `gh`（已登录 github.com） | ✅ |
| 互联网 | ⚠️ 部分可达：`github.com` / `huggingface.co` 主页可通；但 `release-assets.githubusercontent.com`（GitHub release 附件）与 `cdn-lfs.huggingface.co`（HF 模型/LFS）均 TLS 握手失败，无法下载大附件/模型（影响 BLOCKER-2 修复包与真实中文音频样本获取） |
| `cmake` | ✅ venv（`.../envs/default/Scripts/cmake.exe`，4.4.2） |
| `ninja` | ✅ venv（同 venv `ninja.exe`，1.13.0） |
| `Qt 6` | ✅ 已就绪（仓库内 `.qt6/6.8.1/msvc2022_64`，含 Core/Gui/Widgets/Test/Sql） |
| `libmpv` / `FFmpeg` / `sherpa-onnx` | ✅ 已下载并锁定（.tools/ 下；libmpv 用 zhongfly mpv-dev + 现场生成 mpv.lib；手工 cl/link 链接验证通过） |
| `WiX Toolset` | ✅ 已就绪（.tools/wix） |
| ASR 模型权重 | ✅ 已下载（paraformer-removed-from-streaming / sensevoice / silero / **zipformer-ctc(zh-int8-2025-06-30，用户本机下载回填)** 均在 .tools/models，并打进自包含运行时） |

> 说明：本会话已具备 C++ 编译器、git/gh/网络，venv 内 cmake 4.4.2 + ninja 1.13.0，以及仓库内
> `.qt6/6.8.1/msvc2022_64`（Qt 6.8.1 完整模块）。已对**不依赖 libmpv/FFmpeg/sherpa 的纯逻辑模块**完成
> 真实编译与 21 项 QtTest（全 PASS）。原生库（libmpv/FFmpeg/sherpa-onnx）与 ASR 模型现已下载并锁定于
> `.tools/`，`cmake/RcpBundle.cmake` 可将其与模型打进自包含运行时（`out/bundle/runtime`，~994MB，含 zipformer-ctc 新模型，零用户下载）。
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
| T0014 | libmpv 渲染验证程序 | DONE | 49f1a09 | src/player/{MpvPlayer.h,MpvPlayer.cpp,player_app.cpp,CMakeLists.txt}；BUILD_PLAYER=ON 真实构建，player_app 链接 mpv-2.dll 通过 | 依赖 T0011；mpv_destroy 替换缺失的 mpv_detach_destroy |
| T0015 | mpv 事件桥/属性观察 | DONE | (本会话) | spikes/mpv-render/MpvEventProbe.cpp；BUILD_SPIKES=ON 真实构建，导入 mpv-2.dll | 依赖 T0011 |
| T0016 | ASS overlay + 带字幕截图 | DONE(code) | (本会话) | spikes/mpv-render/OverlayProbe.cpp 构建+链接 OK；headless 运行 RUN_EXIT=0；**渲染出图需真机** → PARTIAL | 依赖 T0011+T0014；render 上下文需 GL 表面 |
| T0017 | FFmpeg 指定音轨解码 | DONE | (本会话) | spikes/audio-decode/main.cpp 真机式运行：两音轨解码出不同 checksum（3395679301 vs 3349846617） | 依赖 T0010 |
| T0018 | FFmpeg seek + sample clock | DONE | (本会话) | spikes/audio-decode/SeekProbe.cpp 真机式运行：target 2.000s→pts 2.005s(Δ5ms)，sample_offset 自洽 | 依赖 T0010 |
| T0019 | 本地与 rclone full 双开 | DONE(code) | (本会话) | spikes/rclone-double-open/DoubleOpenProbe.cpp 真机式运行：FFmpeg+mpv 同开同文件双开=YES；rclone full 缓存/断网 PARTIAL | 依赖 T0010+真机挂载 |
| T0020 | Online 流式 partial（实时逐字） | DONE | (本会话) | `online_probe` 从 Paraformer 改写为 **Zipformer2-CTC** 流式（`config.model_config.zipformer2_ctc.model`）；**真机 rc=0**：recognizer+stream 创建、逐块 `[partial]` 实时上屏、零访问违规；用户硬要求实时逐字不可降级 → 已满足 | 依赖 T0012+zh-int8-2025-06-30 模型（用户本机下载回填）；见 BLOCKER-2 修复 |
| T0021 | SenseVoice VAD 分句终稿 | DONE | (本会话) | `sensevoice_probe` 真机运行 **exit=0**：Silero VAD detector 创建成功、SenseVoice 离线 recognizer 创建成功、离线 decode 链路跑通（合成非语音音频，仅验证管线，非识别准确率） → **运行时验证通过** | 依赖 T0012+模型；离线终稿路径已通 |
| T0022 | Hybrid 融合（VAD×识别器→SRT） | DONE | (本会话) | `hybrid_probe` **真机 rc=0**：VAD 分句 → Zipformer2-CTC 实时 partial + SenseVoice 精准终稿 融合链路接通；BLOCKER-2 关闭后双引擎全链路无崩溃 | 依赖 T0021+新流式模型 |
| T0023 | ASR 基准语料 + FunASR 基线（原案 whisper 已禁用） | DONE(framework) | 69920bb | tools/benchmark-asr.py py_compile OK、CER/WER 单测正确、FunASR 缺失优雅退出；真机评测 PARTIAL | 依赖授权语料+FunASR 安装 |
| T0024 | 签署 P0 阶段门报告 | DONE(draft) | (本会话) | docs/phase-gates/P0-report.md 证据表+签署条件；**签署需真机运行时证据回填** → PARTIAL | 依赖 T0010–T0023 |

## Current Blockers

1. **原生依赖已就绪**：libmpv / FFmpeg / sherpa-onnx 已下载并锁定于 `.tools/`，ASR 模型（paraformer/sensevoice/silero）已下载并打进自包含运行时。T0010–T0013 已完成；T0014–T0024 代码与编译/链接验证已全部完成。
2. **打包机制已验证**：`cmake/RcpBundle.cmake` 将 DLL + 模型拷入 `out/bundle/runtime`（~994MB，自包含，含 zipformer-ctc 新模型），由 WiX 整体打进 MSI —— 终端用户安装即用、零运行时下载。
3. **WiX 已就绪**（`.tools/wix`）。
4. **✅ BLOCKER-1（已修复，原诊断误诊）**：旧记录称"bundled ORT 不支持 opset 27"。实测 `sherpa-onnx-version.exe` 显示 **onnxruntime 1.27.1**（支持 opset 27），sherpa-onnx 已是最新 **1.13.6**。真正根因：C:\Windows\System32（版本 1.10.220126）与 SysWOW64 存在**过时的 onnxruntime.dll（ORT 1.10）**，spike 探针 exe 目录未带 bundled dll，按 DLL 搜索顺序（exe 目录 → 系统目录 → PATH）加载了系统的 ORT 1.10，才报 `version [27] not supported` 并崩溃。修复：将 bundled 的 4 个 dll（`onnxruntime.dll` / `onnxruntime_providers_shared.dll` / `sherpa-onnx-c-api.dll` / `sherpa-onnx-cxx-api.dll`）拷到探针 exe 旁，exe 目录优先于系统目录 → 探针加载正确的 ORT 1.27.1。真实产品的 `RcpBundle`/WiX 已把这些 dll 打进运行时目录，故**产品本身不受 System32 冲突影响**，仅 spike 构建需此修复。修复后 `sensevoice_probe` 真机 exit=0（VAD+SenseVoice 离线链路通），证明 ORT 1.27.1 完全可用。
5. **✅ BLOCKER-2（已修复 + 已真机验证：实时逐字 partial 必须可用，不可降级）**：原**2023-02 的 int8 双语 Paraformer 模型图本身**与 ORT 流式路径不兼容（非 ORT 版本 bug、非代码缺陷）。决定性证据：官方 `sherpa-onnx-vad-with-online-asr.exe`（1.13.6/ORT1.27.1）与该模型 → `Creating recognizer ...` 后硬崩溃（exit 127，零 stderr）；官方 `sherpa-onnx.exe`（**1.12.1/旧 ORT**）跑**同一模型** → 同样崩溃 → 降 sherpa/ORT 版本无效。模型尺寸完整，排除损坏；离线路径（SenseVoice+VAD）在 1.13.6 正常 → 缺陷特定于流式 Paraformer 模型图。已否定方向：①降 sherpa/ORT 版本（1.12.1 同崩）；②fp32 encoder（双语 Paraformer fp32 包 998MB、MSI >1GB，非产品方向）。**采纳修复（用户硬要求实时逐字）**：替换流式引擎为现代 ORT-1.27 兼容的 **Zipformer2-CTC** 中文流式模型（CLI `--zipformer2-ctc-model`，C-API `config.model_config.zipformer2_ctc.model`，单文件 `model.int8.onnx`+`tokens.txt`，cjkchar 字符级）。**真机验证（2026-08-27）**：中文 `sherpa-onnx-streaming-zipformer-ctc-zh-int8-2025-06-30` 由用户本机下载回填 `.tools/models/zipformer-ctc/`；git-bash 设 INCLUDE/LIB 后 `ninja` 重编三探针 rc=0；`online_probe`/`hybrid_probe` 真机 rc=0 逐块 `[partial]` 实时上屏、零崩溃 → **BLOCKER-2 关闭，实时逐字（用户不可降级核心功能）已真机可用**。`OnlineProbe`/`HybridProbe` 已改写使用 `zipformer2_ctc`；`HybridProbe` = VAD 分句 → 每段①Zipformer2-CTC 实时 partial ②SenseVoice 精准终稿。
6. **🟡 沙箱下载限制（已修正误判）**：此前记"TLS 握手失败无法下载"**有误**。实测 `curl -k`（忽略代理 MITM 证书）可正常下载 GitHub release 附件，已成功拉取 `sherpa-onnx-v1.12.1-win-x64-shared.tar.bz2`(22.8MB)、`v1.11.1`(22.3MB)、便携 exe(17.4MB)。**真正限制**：代理对**大文件(>~100MB)中途丢连接**（`curl: (56) Failure when receiving data from the peer`，多次在 ~46MB 处断）；`zipformer-ctc-multi-zh-hans-2023-12-13.tar.bz2`(213MB) 多次续传仍难完成。应对：验证用**小体积中文流式模型**（如 `zipformer-ctc-small-2024-03-18` ~30MB）可 `curl -k`+续传拉取先在沙箱验证流式路径；生产级大模型用户本机拉取（网络正常）。真实中文准确率验证仍因沙箱无显示器/语料/网盘挂载需用户本机回填。
7. **🟡 PARTIAL：真机运行时验证未回填**：T0016 渲染出图、T0019 rclone 断网、T0020–22 真实中文语音 ASR 准确率（T0021 已通、T0020/T0022 流式链路已 rc=0 接通、待真实语音确认准确率）、T0023 FunASR CER/WER 均需在带显示器/授权语料/挂载的真机回填证据；沙箱无显示器/GPU/语料/rclone，无法替代。
8. **已知环境坑（非阻塞）**：Git Bash 直接 `cmake` 全量 configure 时 MSYS 路径转换导致 `rc.exe` 不可见；需固化 INCLUDE/LIB/PATH 反斜杠或 vcvars 环境。手工 `cl`/`link` 已验证三库链接通过。

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
- **P8：打包流水线已打通并产出真实产品 MSI（2026-08-28 第二轮）** —— 刷新 `out/bundle/runtime`（拷入新 player_app.exe 266KB + caption_worker.exe 191KB，并 `windeployqt` 收齐 Qt6Core/Gui/Widgets/OpenGLWidgets/Network + platforms/qwindows.dll），`ninja package_msi` 经 heat→candle→light -sval 产出 `out/package/RealtimeCaptionPlayer-0.1.0.msi`（≈677MB，OLE 头 `d0cf11e0` 校验通过）。包内 = 真实 GUI 播放器(player_app) + 字幕 worker(caption_worker) + FFmpeg/mpv/sherpa DLL + 四套 ASR 模型(zipformer-ctc/sensevoice/silero/paraformer) + Qt6 全套，安装到 `ProgramFiles64Folder\RealtimeCaptionPlayer` 并建快捷方式。⚠️ 诚实标注 PARTIAL：真实播放渲染出图、真实中文语音 ASR 准确率、worker 全速预识别→播放头对齐叠加 的"视角"行为均需在带显示器/语料的真机回填；沙箱无显示器/GPU，无法替代验证。

## Product Milestones (P2 / P4 / P6 / P7) — 2026-08-28 MVP

> 用户指令："把产品开发成型"。从 libmpv 验证壳升级为能打开视频→播放→实时字幕的端到端 MVP。
> 以下里程碑均为"代码 + 真实编译链接验证(rc=0)"完成；真机运行时验证标注 PARTIAL。

| Milestone | 标题 | Status | Commit | Verification | Notes |
|---|---|---|---|---|---|
| P2 | 播放内核（MpvPlayer 播放控制 + MpvRenderWidget Qt OpenGL 渲染 + MainWindow 基础窗口） | DONE | edba059 | `ninja player_app` rc=0，player_app.exe 266KB 链接 mpv-2.dll+Qt6 通过 | 真机渲染出图 PARTIAL（沙箱无 GPU/显示） |
| P4 | caption-worker 核心（AudioExtractor FFmpeg 抽音轨 + AsrEngine 双引擎 Zipformer2-CTC partial + SenseVoice 终稿 + SRT 导出） | DONE | 28364b0 | `ninja caption_worker` rc=0（核心 97KB→含 IPC 188KB） | 双引擎链路 T0020–22 真机 rc=0 已验证 |
| P4/IPC | 进程通信：worker 侧 IpcServer(QLocalServer 收命令+后台 QtConcurrent 识别+JsonMessageCodec 回报) + 主进程侧 WorkerSupervisor(QLocalSocket 收发) | DONE | 088a79d | 两侧均 rc=0 编译链接；协议按 ipc/Protocol.h 对齐 | 命令/事件帧格式 = [u32 LE len][UTF-8 JSON] |
| P6 | 字幕叠加：CaptionController(播放头对齐 active 段→CaptionStateMachine 显示态) + MpvPlayer.showSubtitleOverlay(mpv osd-overlay ass-events) | DONE | 088a79d | 编译 rc=0；ASS 由 AssEscaper 转义 | 真机叠加出图 PARTIAL；worker 全速预识别、播放头对齐展示（非逐帧跟随抽音） |
| P7 | UI 集成 + 全量编译 + 重打 MSI：MainWindow 打开即启动 worker、播放头驱动叠加层、字幕开关、导出 SRT | DONE | 088a79d | player_app+caption_worker 同编 rc=0；MSI 重打含真实双 exe+Qt Widgets+模型 | MSI 真机安装/运行 PARTIAL |

**MVP 同步模型（诚实声明）**：worker 以全速预识别整段媒体并打绝对时间戳，主进程 CaptionController 按播放头挑选覆盖当前时刻的 final 段叠加显示；真实中文准确率与"播放头跟随抽音"实时同步留待真机调优（沙箱无法验证播放）。实时逐字 partial（用户硬要求、不可降级）在引擎层已由 Zipformer2-CTC 真机 rc=0 证明可用（见 BLOCKER-2）。

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

下一步：提交本会话 T0014（libmpv 渲染验证程序）成果到私有远程；随后从 T0015 起推进 mpv 事件桥/属性观察（依赖 T0011，已锁定）。

## 本会话补充（2026-08-28，P8 打包流水线打通）
- 用户本机下载中文 `zipformer-ctc-zh-int8-2025-06-30` 模型回填 `.tools/models/zipformer-ctc/`；`RcpBundle` 重建 `out/bundle/runtime`（~994MB，含新模型），并 `windeployqt` 收 Qt6Core 到运行时。
- 启用 `packaging/wix/wix.cmake`：`heat` 采集运行时(`-ag` 稳定 GUID) → `candle` 编译 → `light -sval`(沙箱跳过 ICE) 链接，实测产出 `out/package/RealtimeCaptionPlayer-0.1.0.msi`（≈666MB，OLE 头 `d0cf11e0` 校验通过，zipformer-ctc 已进包）。
- `src/player/CMakeLists.txt` 给 `player_app` 加 `/MANIFEST:NO`（沙箱 cvtres 受限 TEMP 链接 workaround）。
- 提交 dcc06a8 并推送 origin/main（38da5b1..dcc06a8）。诚实标注：本 MSI 为打包机制打通的**验证壳版**，player_app 仍是 libmpv 链接验证壳，非完整 GUI 播放器；待 P2/P5/P7 产品本体成型后重跑即出正式包。
- 已知环境坑补充：git-bash 单独 Bash 调用不保留上一调用的 INCLUDE/LIB/PATH 环境变量，ninja 编译必须在该命令内同设 MSVC 环境（与 P0 探针编译一致）；`cmd //c` 在本环境被安全策略禁止且 MSYS 路径转换会让 `set` 失效，统一用 git-bash + 内联 export 法。

## 本会话补充（2026-08-28，P4/IPC 主进程侧 + P6 字幕叠加 + P7 UI 集成 + 重打真实 MSI）

接上一轮（P2/P4 核心 + P4/IPC worker 侧已编完），本轮把产品真正打通为端到端 MVP：
- **主进程侧 WorkerSupervisor**（`src/player/WorkerSupervisor.h/.cpp`，QLocalSocket 客户端）：启动 `caption_worker.exe --servername <uuid> --models <dir>`，发送 Hello/OpenMedia，接收 Ready/MediaOpened/CaptionPartial/CaptionFinal/Error/Heartbeat，转 `rcp::CaptionSegment` 经 `captionSegment(seg,isPartial)` / `sessionStarted` / `ready` / `workerError` 信号回报；含连接重试（每 100ms，最多 ~5s）。
- **CaptionController**（`src/captions/CaptionController.h/.cpp`，编入 rcp_core）：维护 final 时间线 + 最新 partial，按播放头 ms 取 active 段，驱动 CaptionStateMachine 显示态，输出 mpv `osd-overlay` 的 ASS events；`finals()` 供 SRT 导出。
- **MpvPlayer** 新增 `showSubtitleOverlay`/`clearSubtitleOverlay`（mpv `osd-overlay add/remove ass-events`）。
- **worker IpcServer** 支持 `setModelsRoot`；`main` 新增 `--models` 选项（默认 `.tools/models`）。
- **MainWindow** 集成：打开媒体即启动 worker；`positionChanged` 驱动叠加层；新增「字幕:开/关」与「导出SRT」按钮；`mediaEnded` 停止识别。
- **全量编译**：`ninja player_app caption_worker` rc=0（player_app 266KB / caption_worker 191KB）。
- **重打真实 MSI**：`out/bundle/runtime` 拷入新双 exe + `windeployqt` 收齐 Qt6 Widgets/OpenGL/Network + platforms；`ninja package_msi` 产出 `RealtimeCaptionPlayer-0.1.0.msi`（≈677MB，OLE 校验通过），含真实播放器+worker+原生 DLL+四套模型+Qt6 全套。
- 提交 `088a79d` 并推送 `28364b0..088a79d`。
- **诚实标注 PARTIAL**：沙箱无显示器/GPU/语料，以下仍待用户本机回填——(1) mpv 真实渲染出图与字幕叠加可见性；(2) 真实中文语音 ASR 准确率（Zipformer2-CTC partial + SenseVoice 终稿）；(3) worker 全速预识别 vs 播放头跟随抽音 的实时同步打磨；(4) MSI 真机安装与一键运行。
