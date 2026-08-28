# SBOM — Realtime Caption Player v0.1.0（软件物料清单）

生成日期：2026-08-28。权威来源：`dependencies.lock.json`（含完整 SHA-256 与来源 URL）。
本文件为发布摘要；安装包内运行时 DLL 与模型的哈希另见 `release-manifest-v0.1.0.txt`。

| 组件 | 版本 | 许可证 | 用途 | 集成方式 |
|---|---|---|---|---|
| Qt6 (Core/Gui/Widgets/OpenGL/OpenGLWidgets/Network/Svg) | 6.8.1 | GPL-3.0（开源合规路径） | UI 框架 | 动态链接，windeployqt 随包 |
| libmpv (zhongfly mpv-winbuild mpv-dev) | v0.41.0 系（2026-08-26 构建） | GPL-3.0 | 播放内核/渲染/字幕叠加 | 动态链接 mpv-2.dll + 现场生成 mpv.lib |
| FFmpeg (avformat/avcodec/avutil/swresample/swscale) | 63/63/61/7/10 系列（LGPL 配置） | LGPL-2.1+ | 音轨抽取解码（worker） | 动态链接 |
| sherpa-onnx | 1.13.6 | Apache-2.0 | ASR 推理运行时（C API） | 动态链接 |
| onnxruntime | 1.27.1（随 sherpa-onnx 分发） | MIT | 张量推理后端 | 动态链接 |
| WiX Toolset | .tools/wix（v3 系） | MS-RL（仅构建期，不分发） | MSI 打包 | 构建工具 |
| MSVC 14.44 运行时 | VC Redist 14.x | 微软专有许可 | CRT | 随包分发对应 DLL |

## 模型

| 模型 | 文件 | 许可证 | 用途 |
|---|---|---|---|
| Zipformer2-CTC 中文流式 int8（2025-06-30） | models/zipformer-ctc/model.int8.onnx + tokens.txt | Apache-2.0（sherpa-onnx 发布） | 实时逐字 partial |
| SenseVoice Small int8 | models/sensevoice/model.int8.onnx + tokens.txt | Apache-2.0（FunAudioLLM，经 sherpa-onnx 重发布） | 分句精准终稿（ITN） |
| Silero VAD | models/silero/silero_vad.onnx | MIT（silero-vad） | 语音活动检测分句 |
| Paraformer（离线备用，2023-02） | models/paraformer/ | Apache-2.0 | 未进入流式链路（BLOCKER-2） |

## 自有代码

Realtime Caption Player 源码（src/ tools/ spikes/）：GPL-3.0-or-later，见 LICENSE。

## 合规说明

- 整体分发基于 GPL-3.0-or-later：libmpv（GPL）+ Qt（GPL 路径）+ 自有代码，兼容。
- FFmpeg 以 LGPL 配置构建/获取，动态链接，满足 LGPL 义务（提供组件来源与许可文本）。
- Apache-2.0/MIT 组件与 GPL-3.0 汇聚（非衍生合并），分发时附带其许可与声明文本。
- 无遥测、无网络回传；模型推理全部本地。
