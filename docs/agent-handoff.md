# Agent Handoff

- **Last updated:** 2026-08-28（真机回填 + UI 复刻 + MSI 闭环轮）
- **Branch:** `main`
- **HEAD:** 本会话最终提交（948f86d 之后的发布提交，见 git log）
- **远程：** origin/main 已同步（github.com/canyexuanfan/realtime-caption-player，私有）

## Current Phase

**MVP v0.1.0 RELEASE READY**。P0 已签署、P8 报告 PASS、P9 发布产物齐（发布说明/SBOM/SHA-256 manifest）。tag：`v0.1.0`。

## Last Completed Task（本会话主线）

1. **真实中文 ASR 端到端真机验证**：TTS 固定语料（Microsoft Huihui，4 句）→ FFmpeg 合成
   21.5s mp4 → caption_worker 全链路：65 个实时 partial、4 个 final **逐字正确**、SRT 正确。
   修复 4 个真实缺陷：VAD `Empty()/Detected()` API 误用（真实语音首句必崩）、
   字幕时间戳误用缓冲区局部索引、drainVad use-after-free、BoundedQueue 容量失效
   （ctest 21/21 稳定通过）。commit d5059cd。
2. **按 HTML 前端参考复刻 UI**（用户指出未用参考稿后重做）：自绘无边框标题栏、
   238px 左侧栏（播放列表+实时字幕转写面板）、浮动 ASR 状态卡、控制台
   （±10s/上下曲/倍速/音量/字幕开关/全屏/SRT 导出）、暗色 QSS 主题、快捷键。
   无假按钮（截图/AB 循环/设置页未实装即不放）。commit 948f86d。
3. **mpv 真机三缺陷修复**：`MPV_FORMAT_FLAG` 传 char* 导致 play() 实为暂停
   （播放停滞 0.00 的根因，verbose 日志 `video=playing (paused)` 证据）、
   osd-overlay 正确语法 `osd-overlay <id:int> <format> <data>`（移除用 none）、
   事件端 pause 按 int 读。修复后真机 613 进度事件完整播放至 EOF、osd 0 报错。
4. **MSI 真机闭环**：`ninja package_msi` 出包（677MB）→ UAC 静默安装到
   `F:\rcp-app-test`（exit 0）→ 安装目录一键运行（player_app+caption_worker 自动
   从安装目录启动、模型解析 `<appDir>/models` 修复生效、转写 4 句正确）→
   卸载（exit 0、目录清理）。
5. **阶段门/发布产物**：P0 报告签署（T0016 渲染/T0020/T0022 真实语音回填完成，
   T0019 rclone 断网、T0023 FunASR CER/WER 维持 PARTIAL——外部依赖）；
   P8 报告 PASS；`docs/release/`（RELEASE-NOTES-v0.1.0.md、SBOM.md、
   release-manifest-v0.1.0.txt）。

## Current Task

**NONE（MVP v0.1.0 完成并打 tag）**。

## Next Exact Action

MVP v0.1.0 已 Release Ready 并打 tag。后续版本（T0500+）需用户/PRD 明确授权后启动；
候选方向（均在 docs 中有记录）：文件关联 UserChoice（T0219–T0227）、代码签名、
rclone 断网回归（T0019）、FunASR CER/WER 评测（T0023）、播放头跟随抽音的流水线同步调优、
设置页（T0202+）。

## 本会话新增坑位（详见 questions/）

- sherpa-onnx VAD：`Detected()`≠"有已完成分句"，排空队列必须用 `Empty()`；
  `Front()` 队列空时返回 NULL。
- 时间戳用 `seg->start`（绝对采样索引）；`seg->start - consumed_` 只是缓冲区局部索引。
- libmpv：`MPV_FORMAT_FLAG` 必须传 `int*`（0/1）；osd-overlay 无 add/remove 子命令，
  移除用 format=none。
- 本环境 shell PATH export 不生效：ninja/cl/cmake/ctest 全部用绝对路径调用
  （cmake/ninja 在 `C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/`）。
- PowerShell 5.1 无 BOM UTF-8 中文脚本乱码：脚本需加 BOM。
- 全量构建需同一条命令内联 INCLUDE/LIB/MSYS_NO_PATHCONV（详见下文命令块）。
- cmake 重新配置 spikes2 必须带
  `-DCMAKE_MAKE_PROGRAM="C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/ninja.exe"`。

## Important Decisions

- UI 视觉与布局以 `05_Single_HTML_Frontend_Reference.html` 为唯一参考稿复刻（QSS 令牌对应 :root 变量）。
- 诊断 trace `RCP_TRACE=<file>`（src/player/MpvTrace.h）保留为 P7 诊断能力的一部分。
- 已知非致命问题：Intel Iris Xe 上 mpv 首帧建纹理 INVALID_ENUM 一次（不阻塞播放）；
  `/MANIFEST:NO` 为沙箱链接 workaround，正式发布建议恢复嵌入。
- 文件关联（T0219–T0227）与设置页（T0202+）未实装，UI 中不放假入口。

## MVP Build & Package（2026-08-28，已验证）

```bash
# 1) 编译（内联 MSVC 环境，绝对路径调用）
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'
export VS="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207"
export SDK="C:/Program Files (x86)/Windows Kits/10"
export INCLUDE="$VS/include;$SDK/Include/10.0.26100.0/ucrt;$SDK/Include/10.0.26100.0/um;$SDK/Include/10.0.26100.0/shared"
export LIB="$VS/lib/x64;$SDK/Lib/10.0.26100.0/ucrt/x64;$SDK/Lib/10.0.26100.0/um/x64"
"C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/ninja.exe" -C out/build/spikes2 player_app caption_worker
cp out/build/spikes2/bin/*.exe out/bundle/runtime/

# 2) MSI
"C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/ninja.exe" -C out/build/spikes2 package_msi

# 3) 安装/卸载（UAC 提权）
# msiexec /i out/package/RealtimeCaptionPlayer-0.1.0.msi INSTALLFOLDER=F:\rcp-app-test /qn
# msiexec /x out/package/RealtimeCaptionPlayer-0.1.0.msi /qn
```

## Commands To Resume

- 构建/打包见上节；测试：在 `out/build/spikes2` 下运行
  `"C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/ctest.exe"`（tests 目录需 Qt6 DLL，构建后已拷入）。
- E2E 语料：`out/e2e-verify/`（TTS 生成脚本 gen_tts.ps1、e2e-test.mp4、语料文本）。

## 追加（2026-08-28 晚）：MSI 交互式「修改文件夹」2812 修复

- 根因与修复：见 `questions/Wix安装器错误排查指南.md`（三次错误事件名试错记录 +
  官方 BrowseDlg.wxs 对照正解）。product.wxs 已改用
  `DirectoryListUp`/`DirectoryListNew`/`Subscribe IgnoreChange`/`SetTargetPath`/`Reset`。
- 交互验证：非提权向导 a11y 驱动，Browse→Up 导航成功无 2812（旧包同操作必现）。
- MSI 已重打，manifest 哈希已更新（tag v0.1.0 指向修复前状态，修复为 tag 后提交）。
- 待用户重装复验完整链（Up/NewFolder/选目录/OK/安装）。
