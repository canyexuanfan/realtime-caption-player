# Agent Handoff

- **Last updated:** 2026-09-27（审查阶段 A→E 全线收口）
- **Branch:** `main`
- **HEAD:** 以 `git log` 实时为准；origin/main = 2a514be（[E] 阶段提交）
- **远程：** origin = github.com/canyexuanfan/realtime-caption-player，**PUBLIC**。
  任何提交严禁引入 Windows 用户名、本机绝对路径、真实媒体文件名与密钥/Token；公开内容按 08 指南最小披露。

## Current Phase

**审查阶段 A→E 全部可执行工作完成**（2026-09-27 单日完成，逐阶段明细与验收见根目录 `todolist.md`）。
产品形态：本地离线实时字幕播放器——worker 按播放头窗口实时解码+双引擎识别（B2 起），
主进程 CaptionCoordinator 唯一时间线驱动叠加显示，SQLite 持久化历史/续播/字幕缓存，
双 MSI 分发（完整 511MB / Lite 343MB）。

## Last Completed Task（阶段 A→E 主线）

按依赖顺序执行（每步均有 commit + 构建/测试证据，commit 链见 git log [A0]..[E]）：
A0-A5 重新基线与脱敏 → B1 协议加固 → B2 worker 实时管线 → B3 CaptionCoordinator+死代码清理 →
B4 生命周期 → B5 档位/语言/过载 → C1/C3/C4/C5 前端 → D1-D5 基础设施 → E 双 MSI/版本资源/文件关联/ICE。
E 验证期发现并修复 **B2 管线 IPC 崩溃三根因**（EOF 残块重复产出 / language 悬空指针 / 重启 openSent 时序）。

## Current Task

**NONE（等待用户真机回填验收）**。

## Next Exact Action

用户在带显示器的真机执行验收：
1. 安装 `out/package/RealtimeCaptionPlayer-0.1.0.msi`（或免安装跑 `out/bundle/runtime/player_app.exe`）
2. 打开无字幕视频：确认字幕随播放/seek/倍速/暂停正确跟随（B 阶段核心验收）
3. 重启程序确认续播/历史/字幕缓存（D1"重看秒出"）
4. 异常场景：强杀 player_app 确认 worker 随 Job Object 退出；杀 worker 确认自动重启
5. 结果回填 `docs/implementation-status.md`；问题按 questions/ 闭环

## Important Decisions

- **ADR-0007**：唯一字幕渲染路径 = Qt CaptionLabel（mpv osd-overlay 四件套已按授权删除）。
- **字幕管线**（B2）：解码按播放头 30–120s 窗口调度，generation 以主进程为权威，两端丢弃旧代。
- **构建**：沙箱用 `bash tools/build-release.sh`（Ninja+显式 cl/rc/mt 绝对路径，勿把 MSVC 塞 PATH）；
  标准开发机可用 `windows-msvc-*` 预设；`-DRCP_SANDBOX_NO_MANIFEST=ON` 仅为沙箱 workaround。
- **打包**：`cmake -P cmake/run_bundle.cmake`（完整）/ `RCP_BUNDLE_LITE=1`（Lite）→
  `ninja package_msi [package_msi-Lite]`；ICE 校验默认开启（豁免仅 ICE61/ICE20 定向，理由见 wix.cmake）。
- **诊断设施**：`RCP_TRACE`（事件流）/ `RCP_SNAPSHOT`（双帧快照）/ `RCP_CRASH_DIR`（worker SEH+terminate
  钩子 + MiniDump + 生命周期文件日志，默认零开销）/ `RCP_DEMO_ONLY`（像素比对示例态）。

## Build & Test（统一走脚本）

```bash
bash tools/build-release.sh                       # configure + 全目标构建 + Qt DLL 部署
ctest --preset windows-ninja-release              # 20/20（Qt DLL 已由脚本旁挂）
cmake -P cmake/run_bundle.cmake                   # 完整运行时
RCP_BUNDLE_LITE=1 cmake -P cmake/run_bundle.cmake # Lite 运行时
ninja -C out/build/windows-ninja-release package_msi package_msi-Lite
```

## 公开仓库提交前自查

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File tools/check-status.ps1   # 应 CHECK-STATUS-PASS
git grep -nE "F:[/\\\\]|wzm33" -- . | grep -v "docs/review/"                 # 应无输出
```

## 已知限制与真机回填清单

见根目录 `todolist.md`「已知限制与真机回填清单」（渲染出图/ASR 准确率/续播体验/签名/VM smoke/C2 拆分）。

## 本会话新增坑位（详见 questions/ 与 commit 记录）

- **MSYS/Windows 双环境**：bash PATH 只认 POSIX 风格、cmake 认 Windows 风格 → 编译器一律显式绝对路径
  传参（build-release.sh）；mt.exe 在 Windows SDK 不在 MSVC bin；中文路径下 MSYS→Windows PATH 乱码
  → Qt DLL 旁挂测试目录。
- **bash→python -c 传参**：`\\` 折叠为 `\`，`\n` 落盘成真实换行 → C++ 字面量被污染（本次多次踩），
  反斜杠一律用 chr(92) 或 Edit 工具落盘；**静默管道 `cmd | grep -c` 会吞构建失败**——验证性构建必须看尾部输出。
- **sherpa C-API**：config 字符串成员指向的缓冲必须活过 Create 调用——`toUtf8().constData()` 临时对象
  即悬空（sensevoice language 崩溃根因之一）；模型缺失用 QFile::exists 预判跳过（Lite 包路径）。
- **FFmpeg extract 复用**：av_read_frame EOF 后 flush 段每次调用都会重复产出最后残块——EOF 必须
  状态化（m_eof），否则下游 ASR/VAD 被反复喂同块。
- **QProcess kill() 表现为 errorOccurred(Crashed)**——区分"真崩溃"与"watchdog 击杀"需 trace 佐证；
  自动重启检查所用状态不得在 finished 处理中提前清除（B4 openSent 时序 bug）。
- **CMake script 模式**：`-D` 变量不总可见 → 开关走环境变量双通道；cache 变量在函数内 set 需调用方 FORCE。
- PS1 中文匹配必须 UTF-8 BOM（check-status.ps1 自踩）；Qt 资源编进静态库需 Q_INIT_RESOURCE 显式拉入。
