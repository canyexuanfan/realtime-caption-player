# 开源参考与许可证分析（reference/）

本目录是 **开发前强制调研** 的成果（依据 `/goal` 与 ADR 流程）。目标是避免重复造轮子，
并对每个拟参考/依赖的上游项目做许可证与“可参考程度”评估。

> 星标（stars）数据采集自 `gh api repos/<owner>/<repo> --jq .stargazers_count`（采集日 2026-08-27）。
> 许可证以各仓库 `LICENSE` / SPDX 字段为准；GitHub 显示 `NOASSERTION`/`null` 的，已在各报告正文标注真实许可证。

## 索引

| 报告 | 上游 | Stars | 许可证 | 我们怎么用 |
|---|---|---:|---|---|
| [mpv-libmpv](mpv-libmpv.md) | mpv-player/mpv | 36,690 | GPL-2.0-or-later | 播放内核（libmpv 库） |
| [qt6-integration](qt6-integration.md) | Qt 6 / mpv-examples / mpc-qt | — | LGPLv3 / GPL-2.0 | Qt Widgets + OpenGL 渲染桥接 |
| [sherpa-onnx](sherpa-onnx.md) | k2-fsa/sherpa-onnx | 14,411 | Apache-2.0 | VAD / Online Paraformer / SenseVoice / 标点 |
| [ffmpeg-audio-decode](ffmpeg-audio-decode.md) | FFmpeg/FFmpeg | 63,666 | LGPL-2.1/GPL-2.0 | worker 音频解码 → PCM |
| [ipc-qlocalsocket](ipc-qlocalsocket.md) | Qt（QLocalSocket） | — | Qt 许可 | worker↔player IPC |
| [wix-msi](wix-msi.md) | WiX Toolset | — | MS-RL | MSI 打包（仅工具） |
| [srt-ass](srt-ass.md) | 自实现 + libass/srt 参考 | — | LGPL-2.1 / GPL | SRT 导出 / 字幕解析参考 |
| [license-matrix](license-matrix.md) | 汇总 | — | — | 合规总览 |

## 结论

- **不重复造轮子**：播放用 libmpv；ASR 运行时用 sherpa-onnx；音频解码用 FFmpeg；IPC 用 Qt QLocalSocket；打包用 WiX。
- **合规基线**：项目整体 GPL-3.0-or-later；Qt 动态链接（LGPLv3 重链接说明）；mpv GPL-2.0-or-later；sherpa-onnx Apache-2.0（保留 NOTICE）；FFmpeg 按实际 configure flags 记录；WiX 仅打包工具（MS-RL）。
- **禁止**：将 libmpv 仅当 DLL 即声称不受 GPL；将 SenseVoice 当 streaming partial；将 rclone `writes` 当只读视频缓存；未经锁定的随机 DLL。
