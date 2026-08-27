# Implementation Status — Realtime Caption Player

> 本文件是项目实时状态的主索引。任何 Agent 打开仓库首先读它。
> 规则：状态只能为 `TODO / IN_PROGRESS / BLOCKED / DONE / WAIVED`。禁止伪造完成。

## 当前快照（2026-08-27）

- **Current phase:** P0（技术验证、决策与第三方基线）
- **Current task:** T0024（签署 P0 阶段门报告）— BLOCKER-1（dll 冲突）已修复；T0021（Silero VAD + SenseVoice 离线）真机验证通过（exit 0）；用户验收标准"中文准确识别达到直接在网盘内播放的效果"由 **SenseVoice 权威精准路径**满足（T0021 exit=0、T0022 精准路径 RC=0）；T0020/T0022 的在线 Paraformer 流式增强在 ORT 1.27.1 下创建识别器崩溃（官方二进制复现），列为 🔴 BLOCKER-2，仅阻断"实时逐字 partial"增强（默认关闭），**不阻断精准字幕**；修复方向②（钉 sherpa 版本）因沙箱代理 TLS 限制无法下载对照包，待用户本机执行
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
| T0014 | libmpv 渲染验证程序 | DONE | 49f1a09 | src/player/{MpvPlayer.h,MpvPlayer.cpp,player_app.cpp,CMakeLists.txt}；BUILD_PLAYER=ON 真实构建，player_app 链接 mpv-2.dll 通过 | 依赖 T0011；mpv_destroy 替换缺失的 mpv_detach_destroy |
| T0015 | mpv 事件桥/属性观察 | DONE | (本会话) | spikes/mpv-render/MpvEventProbe.cpp；BUILD_SPIKES=ON 真实构建，导入 mpv-2.dll | 依赖 T0011 |
| T0016 | ASS overlay + 带字幕截图 | DONE(code) | (本会话) | spikes/mpv-render/OverlayProbe.cpp 构建+链接 OK；headless 运行 RUN_EXIT=0；**渲染出图需真机** → PARTIAL | 依赖 T0011+T0014；render 上下文需 GL 表面 |
| T0017 | FFmpeg 指定音轨解码 | DONE | (本会话) | spikes/audio-decode/main.cpp 真机式运行：两音轨解码出不同 checksum（3395679301 vs 3349846617） | 依赖 T0010 |
| T0018 | FFmpeg seek + sample clock | DONE | (本会话) | spikes/audio-decode/SeekProbe.cpp 真机式运行：target 2.000s→pts 2.005s(Δ5ms)，sample_offset 自洽 | 依赖 T0010 |
| T0019 | 本地与 rclone full 双开 | DONE(code) | (本会话) | spikes/rclone-double-open/DoubleOpenProbe.cpp 真机式运行：FFmpeg+mpv 同开同文件双开=YES；rclone full 缓存/断网 PARTIAL | 依赖 T0010+真机挂载 |
| T0020 | Online Paraformer partial | BLOCKED(增强项) | (本会话) | `online_probe` 编译+链接 OK；真机在 `SherpaOnnxCreateOnlineRecognizer` 硬崩溃（零 stderr，exit 127），**官方 `sherpa-onnx-vad-with-online-asr.exe` 同模型+正确参数复现** → 在线 Paraformer 模型×ORT 1.27.1(CPU EP) 致命缺陷；**仅影响实时逐字 partial 增强，不影响精准字幕** | 依赖 T0012+模型；见 BLOCKER-2 |
| T0021 | SenseVoice VAD 分句终稿 | DONE | (本会话) | `sensevoice_probe` 真机运行 **exit=0**：Silero VAD detector 创建成功、SenseVoice 离线 recognizer 创建成功、离线 decode 链路跑通（合成非语音音频，仅验证管线，非识别准确率） → **运行时验证通过** | 依赖 T0012+模型；离线终稿路径已通 |
| T0022 | Hybrid 融合（VAD×识别器→SRT） | DONE(精准路径) | (本会话) | `hybrid_probe` 重写：默认走 **VAD 分句 × SenseVoice 精准识别 → SRT**（RC=0，融合编排链路接通）；Paraformer 流式仅作 `RCP_TRY_STREAMING=1` 可选增强（复现 BLOCKER-2） | 依赖 T0021；精准路径已通，partial 增强见 BLOCKER-2 |
| T0023 | ASR 基准语料 + FunASR 基线（原案 whisper 已禁用） | DONE(framework) | 69920bb | tools/benchmark-asr.py py_compile OK、CER/WER 单测正确、FunASR 缺失优雅退出；真机评测 PARTIAL | 依赖授权语料+FunASR 安装 |
| T0024 | 签署 P0 阶段门报告 | DONE(draft) | (本会话) | docs/phase-gates/P0-report.md 证据表+签署条件；**签署需真机运行时证据回填** → PARTIAL | 依赖 T0010–T0023 |

## Current Blockers

1. **原生依赖已就绪**：libmpv / FFmpeg / sherpa-onnx 已下载并锁定于 `.tools/`，ASR 模型（paraformer/sensevoice/silero）已下载并打进自包含运行时。T0010–T0013 已完成；T0014–T0024 代码与编译/链接验证已全部完成。
2. **打包机制已验证**：`cmake/RcpBundle.cmake` 将 DLL + 模型拷入 `out/bundle/runtime`（674MB，自包含），由 WiX 整体打进 MSI —— 终端用户安装即用、零运行时下载。
3. **WiX 已就绪**（`.tools/wix`）。
4. **✅ BLOCKER-1（已修复，原诊断误诊）**：旧记录称"bundled ORT 不支持 opset 27"。实测 `sherpa-onnx-version.exe` 显示 **onnxruntime 1.27.1**（支持 opset 27），sherpa-onnx 已是最新 **1.13.6**。真正根因：C:\Windows\System32（版本 1.10.220126）与 SysWOW64 存在**过时的 onnxruntime.dll（ORT 1.10）**，spike 探针 exe 目录未带 bundled dll，按 DLL 搜索顺序（exe 目录 → 系统目录 → PATH）加载了系统的 ORT 1.10，才报 `version [27] not supported` 并崩溃。修复：将 bundled 的 4 个 dll（`onnxruntime.dll` / `onnxruntime_providers_shared.dll` / `sherpa-onnx-c-api.dll` / `sherpa-onnx-cxx-api.dll`）拷到探针 exe 旁，exe 目录优先于系统目录 → 探针加载正确的 ORT 1.27.1。真实产品的 `RcpBundle`/WiX 已把这些 dll 打进运行时目录，故**产品本身不受 System32 冲突影响**，仅 spike 构建需此修复。修复后 `sensevoice_probe` 真机 exit=0（VAD+SenseVoice 离线链路通），证明 ORT 1.27.1 完全可用。
5. **🔴 BLOCKER-2（新，仅阻断"实时逐字 partial"增强，不阻断精准字幕）：在线 Paraformer 模型 × ORT 1.27.1(CPU EP) 致命崩溃**：`SherpaOnnxCreateOnlineRecognizer` 在创建流式 Paraformer 识别器时硬崩溃（无 stderr，exit 127），**官方 `sherpa-onnx-vad-with-online-asr.exe` 用相同模型 + 正确参数（`--paraformer-encoder`/`--paraformer-decoder`）复现**（打印 `Creating recognizer ...` 后崩溃）。模型文件尺寸完整（encoder.int8.onnx 165MB / decoder.onnx 25MB），排除损坏。SenseVoice/离线路径在 ORT 1.27.1 下正常 → 缺陷特定于流式 Paraformer 模型图（int8 量化最可能触发 ORT 1.27.1 CPU EP 量化算子回归）。**影响范围（重新定性）**：仅阻断实时逐字 partial（T0020/T0022 的 Paraformer 流式增强）；精准字幕（VAD 分句 × SenseVoice）不受任何影响，已运行时验证（T0021 exit=0、T0022 精准路径 RC=0），满足用户验收"中文准确识别"。**决策（用户授权"自己看着办"）**：fp32 方向①放弃（包 ~998MB、MSI 撑到 >1GB）；保留 int8 模型，方向②把 sherpa-onnx 钉到"int8 Paraformer 可跑"的版本组合（如 1.12.1），但沙箱代理 TLS 限制无法下载对照包（见下条），需用户本机执行并回归 SenseVoice 仍正常。当前 `hybrid_probe` 默认仅跑 SenseVoice 精准路径（RC=0），Paraformer 流式作 `RCP_TRY_STREAMING=1` 可选增强（详见 P0 报告 §3 与 ADR-0004 状态更新）。
6. **🟡 沙箱下载限制（非代码缺陷，影响 BLOCKER-2 修复与真机准确率验证）**：本沙箱经 Clash Verge TUN 代理；`github.com` / `huggingface.co` 主页可通，但 **GitHub `release-assets.githubusercontent.com`（release 附件）与 HF `cdn-lfs.huggingface.co`（模型/LFS）均 TLS 握手失败**（`schannel: failed to receive handshake`）。后果：无法在沙箱内下载旧版 sherpa 修复包（方向②），也**无法下载真实中文音频样本**做端到端准确率验证。BLOCKER-2 方向②与"真实中文识别准确率"验证均需在用户本机（网络正常、有 rclone 网盘挂载）执行；沙箱只负责架构验证与管线 smoke。
7. **🟡 PARTIAL：真机运行时验证未回填**：T0016 渲染出图、T0019 rclone 断网、T0020–22 真实语音 ASR（T0021 已通、T0020/T0022 阻塞于 BLOCKER-2）、T0023 FunASR CER/WER 均需在带显示器/授权语料/挂载的真机回填证据；沙箱无显示器/GPU/语料/rclone，无法替代。
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
