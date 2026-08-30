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

## 前端像素级复现（进行中，2026-08-30）

用户要求：以 05_Single_HTML_Frontend_Reference.html 源码为规格像素级复现前端，
示例数据也同源。已建 Edge 同宽比对回路（--headless --screenshot vs
RCP_DEMO_ONLY=1 + RCP_SNAPSHOT + RCP_SNAP_T1/T2，统一 1280x800 逻辑尺寸，
注意用户屏幕 150% 缩放 → 快照物理像素 1.5x）。

本轮已落地（未提交部分见 git status）：
- 示例数据模式：fillDemoData/demoTick（demoPlaylist 5 项/historyPlaylist 3 项/
  transcriptSegments 7 段/state 24:18、49:12、1.50x、音量 72、字幕行数 128、
  延迟 1.2s），activeSegmentForTime 同源算法；开真实媒体整体退出示例态
- 历史记录 tab 无反应根修：checkable 按钮无互斥组 → QButtonGroup exclusive
- 字幕按视频画面矩形锚定（video-params contain-fit，repositionOverlays）
- @media(max-width:1320px) 断点：左栏 218/设置 322/标题钮只留图标
- CaptionLabel 对齐参考 CSS（字体族/字距/四向描边+模糊阴影缓存）
- mpv sub-auto=no+sid=no 根除同目录 srt 双层字幕；手动加载 sub-add select

### 下一轮清单（按此继续）
1. 播放列表行绘制重叠：rowName setFixedWidth(132) 后仍溢出（怀疑 QLabel
   stylesheet 字体与 fm12 不一致/行容器宽于 sizeHint 触发横向滚动条），需
   row 级布局重做：play 18 + name elided(stretch) + dur 右对齐，行宽锁 218。
2. 设置面板"实时字幕"页补参考稿缺失行：字幕字体 select(思源黑体)、字号
   slider、字幕颜色 白/黄 segmented、描边/阴影 select、字幕位置 select、
   预览结果颜色 chip、导出设置(自动导出 switch/导出格式 SRT/保存目录)。
3. 字幕定位 trace 验证（reposition trace 行已加：RCP_TRACE 看 content/box 矩形）。
4. Waveform 对照（36 根 2px 渐变条 #a79cff→#6555ef 动画）。
5. demo 海报：参考稿 stage 有 cover 图片（航拍中国剧照），demo 态放等价渐变。
6. a11y 实测历史 tab 点击 + 全量 ctest + MSI 重打。

### 已完成（2026-08-30 第二轮）
- 播放列表行重影根除：demo 项把名字写进 QListWidgetItem 本身，默认委托把
  条目文字画在透明行控件底下（全名+省略名叠加+横向滚动条）；条目文字留空、
  只设 UserRole 后干净。rowName 定宽 110 裁剪 + 时长完整右对齐（49:12）。
- 设置面板实时字幕页对齐参考稿：识别引擎/语言/模型大小/计算设备选项串逐字
  同源；新增 显示设置（字幕字体/字幕颜色白黄segmented/描边阴影/预览结果颜色
  chip）与 导出设置（自动导出/导出格式/保存目录），全部接入真实 settings 并
  由 applyCaptionStyle 生效（黄=#ffe66d；思源黑体=Source Han Sans SC 族）。
- 字号步长 ±1（参考 22..54）。

### 仍余
- 字幕定位 reposition trace 复验（行已加）；Waveform 细节对照；demo 海报图；
  a11y 实测历史 tab 点击；MSI 重打。

### 已完成（2026-08-30 第三轮）
- 字幕画面锚定 trace 实证：host=740x644, video=1280x720 → content=(0,114
  740x416), 字幕盒 (52,295 636x180)（content.y+87%-180）。mediaLoaded 时
  video-params 常未就绪 → positionChanged 里 m_videoRectValid 未就绪即重试。
- MSI 重打 06307143…（含目录记忆/示例数据/设置页对齐），manifest 同步。

### 仍余
- Waveform 细节对照（36 根 2px 渐变动画——现有实现结构已同，仅微调观感）；
  demo 海报图（参考稿为外部剧照资产，暂用黑底，可用渐变替代）；历史 tab 点击
  a11y 实测（QButtonGroup 修复后逻辑确定，未做真机点击验证）。
- a11y 实测历史 tab 点击通过（标题联动+真实历史列表切换）；Waveform 对照通过
  （36 根/2px/间距2/渐变/动画同源）；无媒体底色改参考 .video-surface #10151d。
- 第三处同类重影根除：loadHistory 也把文件名写进 QListWidgetItem 本身（用户
  截图历史行重影），已改为条目文字留空+UserRole（与 demo 行同款修复）。
- final 字幕改单行省略（参考稿 final 恒单行；2 行会顶压 partial 行），
  a11y 实测 live 播放中 partial/final 均单行省略。
- Waveform 峰值 14→11、衰减 0.92→0.85：说话时常满格导致的"连成实块"消除。
