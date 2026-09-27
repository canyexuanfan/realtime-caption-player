# Agent Handoff

- **Last updated:** 2026-09-27（阶段 A 止血与重新基线，依据 `docs/review/2026-09-25-全面代码审查与后续规划.md`）
- **Branch:** `main`
- **HEAD:** 以 `git log` 实时为准（2026-09-25 filter-repo 转公开后全部旧提交哈希失效；历史文档中的哈希仅作历史标记）
- **远程：** origin = github.com/canyexuanfan/realtime-caption-player，**PUBLIC**（2026-09-25 经用户授权转公开）。
  任何提交严禁引入 Windows 用户名、本机绝对路径、真实媒体文件名与密钥/Token；公开内容按 08 指南最小披露。

## Current Phase

**阶段 A 止血与重新基线（进行中）**。⚠️ 此前「MVP v0.1.0 RELEASE READY」声明已于 2026-09-27 **撤回**：
当前真实状态是「能打开视频播放并叠加显示实时生成字幕」的演示级预览——worker 全速预识别模型，
seek/倍速/暂停/切轨不通知 worker；持久化、CI、文件关联、日志脱敏未实现。缺口清单见审查文档 §0–§3。

## Last Completed Task（阶段 A，2026-09-27）

- **A0** 归档外部审查（`docs/review/`，含项目侧实码复核勘误附录）+ 建立 `todolist.md` +
  `docs/adr/ADR-0007-字幕渲染单一路径.md` + 03 TODO 基线注记。
- **A1** 状态文档重基线（本文件 + `docs/implementation-status.md`：撤回 RELEASE READY、远程改 PUBLIC、
  HEAD 说明、真实媒体信息脱敏）。
- **A2** 公开仓库路径脱敏：tools 像素比对脚本参数化（`Path(__file__)` 相对）、`cmake/run_bundle.cmake`
  仓库相对化、evidence/phase-gates/questions 路径与课程名脱敏、AGENTS.md 内部路径泛化、
  删一次性工具 `tools/fix_blocker2_zh_streaming.bat`。
- **A3** 构建固化：CMakePresets 增 `windows-ninja-release`、新增 `tools/build-release.sh`、
  `/MANIFEST:NO` 改 `RCP_SANDBOX_NO_MANIFEST` 门控（正式构建默认嵌入 manifest）、
  RcpBundle 模型白名单（zipformer-ctc/silero/sensevoice）。
  **验证：全新 configure + 全目标构建 rc=0；ctest 21/21 PASS；白名单打包实证。**
- **A4** 清理：删根目录重复文档（01_PRD/02_Technical/PRD.md/docs/development-todo.md）、README.txt、
  SHA256SUMS.txt、未引用的 EnsureAppSubfolder.*；RELEASE-NOTES 标注预览版并修正 osd-overlay 失实描述；
  README/NOTICE 的 Paraformer 陈旧描述更正为 Zipformer2-CTC。

（逐项勾选以根目录 `todolist.md` 为准）

## Current Task

见根目录 `todolist.md`「当前执行状态」。

## Next Exact Action

阶段 A 收口（A5 Git 存档 + push）后，进入**阶段 B1 协议加固**（审查 §5）：
worker 端改用 `src/ipc/FrameDecoder` 安全分帧、协议增加 Unknown 类型与必填字段校验、
事件保留原始 generation 并两端丢弃旧代。动手前先读 `todolist.md` 与审查文档阶段 B 章节。

## Important Decisions

- **ADR-0007（2026-09-27）**：唯一字幕渲染路径 = Qt CaptionLabel；mpv osd-overlay 路径
  （CaptionController overlay 输出 / MpvPlayer::showSubtitleOverlay / CaptionStateMachine / AssEscaper）
  为死代码——授权前不接线、不维护、不得新增调用者；物理删除需用户明确授权（2026-09-04 用户曾拒绝）。
- UI 视觉与布局以 `05_Single_HTML_Frontend_Reference.html` 为唯一参考稿复刻。
- 诊断 trace `RCP_TRACE=<file>`（src/player/MpvTrace.h）保留为诊断能力。
- 已知非致命问题：Intel Iris Xe 上 mpv 首帧建纹理 INVALID_ENUM 一次（不阻塞播放）。
- 文件关联（T0219–T0227）与设置页（T0202+）未实装，UI 中不放假入口。
- `/MANIFEST:NO` 已改 `RCP_SANDBOX_NO_MANIFEST` 门控：正式构建默认嵌入 manifest；
  仅当沙箱受限 TEMP 报 CVT1108/LNK1123 时加 `-DRCP_SANDBOX_NO_MANIFEST=ON`。

## Build & Test（已固化，统一走脚本）

```bash
# 一键 Release（git-bash；自动定位 VS/SDK/ninja，可用 RCP_VS_ROOT/RCP_SDK_ROOT/RCP_NINJA/RCP_CMAKE 覆盖）
bash tools/build-release.sh                                 # configure(preset windows-ninja-release) + 全目标构建 + 部署 Qt DLL
bash tools/build-release.sh -DRCP_SANDBOX_NO_MANIFEST=ON    # 沙箱 cvtres workaround（勿用于正式发布）

# 测试（Qt DLL 已被脚本自动拷到测试 exe 旁，无需设 PATH）
"C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/ctest.exe" --preset windows-ninja-release

# 自包含运行时（模型白名单）+ MSI
cmake -P cmake/run_bundle.cmake                             # → out/bundle/runtime（zipformer-ctc/silero/sensevoice）
ninja -C out/build/windows-ninja-release package_msi        # → out/package/RealtimeCaptionPlayer-0.1.0.msi
```

脚本要点（踩坑固化）：不把 MSVC/SDK 塞进 PATH（MSYS 与 Windows 版 cmake 对 PATH 风格要求相反），
显式传 cl/rc/mt 绝对路径给 CMake；mt.exe 由 **Windows SDK** 提供（MSVC bin 目录没有）；
中文仓库路径下 MSYS→Windows 的 PATH 转换会乱码，Qt DLL 必须拷到测试 exe 旁（脚本已自动做）。

## 真实视频字幕验证方法

`out\bundle\runtime\player_app.exe "<视频路径>"` + `RCP_SNAPSHOT=<png>` 双帧截图后退出；
**不得设 `RCP_NO_CAPTION`**（会跳过 worker）。运行/错误细节先看 `RCP_TRACE` 日志。

## 公开仓库提交前自查

```bash
git grep -nE "F:[/\\\\]|考点1|提干" -- . | grep -v "docs/review/"   # 应无输出（占位符 <你的用户名> 除外）
```

## 本会话新增坑位（详见 questions/）

- MSYS bash 的 PATH 只认 POSIX 风格条目（`/c/...`），Windows 版 cmake 只认 Windows 风格——
  两者不可共用 PATH，编译器/工具链一律显式绝对路径传参（已固化进 build-release.sh）。
- bash → python -c 传参会把 `\\` 折叠成 `\`，Python 字符串里 `\r` `\w` 等会被当转义；
  跨层传反斜杠用 `chr(92)` 构造，或写字幕脚本文件执行。
- sherpa-onnx VAD：`Detected()`≠"有已完成分句"，排空队列必须用 `Empty()`；`Front()` 队列空时返回 NULL。
- 时间戳用 `seg->start`（绝对采样索引）；`seg->start - consumed_` 只是缓冲区局部索引。
- libmpv：`MPV_FORMAT_FLAG` 必须传 `int*`（0/1）；osd-overlay 无 add/remove 子命令，移除用 format=none。
- PowerShell 5.1 无 BOM UTF-8 中文脚本乱码：脚本需加 BOM；PowerShell 内联 stdout 本会话不显示，写文件后 Read。
