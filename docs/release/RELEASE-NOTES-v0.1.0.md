# Realtime Caption Player v0.1.0 — 发布说明（MVP）

日期：2026-08-28 ｜ 许可证：GPL-3.0-or-later ｜ 目标平台：Windows 10/11 x64

##这是什么

**实时字幕播放器**：本地离线的 Windows 视频播放器。打开视频即自动启动独立的
caption-worker 进程，用本地 ASR（Zipformer2-CTC 实时逐字 + Silero VAD 分句 +
SenseVoice 精准终稿）为**没有字幕的视频**实时生成中文字幕并叠加显示，支持导出 SRT。
**默认完全离线：无遥测、无云上传、模型全部内置安装包。**

## 安装即用

- 安装包：`RealtimeCaptionPlayer-0.1.0.msi`（≈677MB，全部依赖与模型内嵌，终用户零下载）
- 安装：双击 MSI，选择安装目录（默认 Program Files\RealtimeCaptionPlayer）
- 卸载：系统"设置→应用"或 msiexec /x；卸载保留用户字幕/播放记录数据

## 本版本能力（MVP 范围）

- libmpv 播放内核：常见容器/编码本地播放，倍速 0.5x–2.0x、进度拖动、±10s、
  音量、上一/下一（播放列表）、全屏（F）、空格暂停、Ctrl+O 打开、拖拽打开
- 实时字幕：打开媒体即全速预识别，播放头对齐叠加显示（mpv osd-overlay）
- 字幕转写面板：partial 实时滚动 + final 带时间戳落定 + 字幕行数统计
- SRT 导出：识别完成即可导出（含数字与标点规整，SenseVoice ITN）
- 暗色主题 UI（复刻前端参考稿布局：标题栏/播放列表/转写面板/状态卡/控制台）

## 真机验证记录（2026-08-28）

- 中文 TTS 固定语料 4 句端到端：65 个实时 partial、4 个 final **逐字正确**、SRT 正确
- 完整播放 21.5s 测试视频：613 个进度事件、正常 EOF、字幕叠加命令 0 报错
- MSI 静默安装（UAC）→ 安装目录一键运行（自动拉起 worker）→ 卸载 → 目录清理，exit 0
- QtTest 21/21 通过（含 BoundedQueue 容量稳定性）

## 已知限制

1. **代码签名**：安装包未签名（无私钥证书）；发布以 SHA-256 清单（release-manifest）供校验。
   首次运行如触发 SmartScreen，请选择"仍要运行"或自行校验哈希。
2. **文件关联**：暂未实现"设为默认播放器"（UserChoice 注册与引导为后续版本）。
3. **rclone/网盘挂载路径**：架构支持（ADR-0005 VFS full 策略），本版未做断网回归测试。
4. **实时字幕同步模型**：当前为"全速预识别 + 播放头对齐展示"；逐帧跟随抽音的
   流水线同步在后续版本调优（ASR 首字延迟等性能门槛亦留待该版本标定）。
5. **Intel 显卡**：个别驱动上 mpv 首帧建纹理报一次 INVALID_ENUM，非致命、不影响播放。
6. 设置页、说话人分离、云端 ASR 等能力不在 MVP 范围（UI 无对应假入口）。

## 构件校验

见 `docs/release/release-manifest-v0.1.0.txt`（MSI、双 exe、模型 SHA-256）。

## 第三方组件

FFmpeg（LGPL）、libmpv（GPL）、sherpa-onnx + onnxruntime（Apache-2.0）、Qt 6.8.1（GPL）、
模型：Zipformer2-CTC zh-int8 / SenseVoice / Silero VAD —— 版本与来源见
`dependencies.lock.json` 与 `docs/release/SBOM.md`。
