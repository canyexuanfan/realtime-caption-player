# 许可证合规矩阵（License Matrix）

项目整体分发许可：**GPL-3.0-or-later**（见 `LICENSE` / ADR-0001）。
下游依赖按各自许可证合规；聚合分发下各自保留其许可证义务。

| 组件 | 用途 | 许可证 | 我们如何合规 | 锁文件位置 |
|---|---|---|---|---|
| 本项目代码 | 应用本体 | GPL-3.0-or-later | 源码全量提供；GPL 文本随包 | — |
| Qt 6（Widgets/GUI/Network/Sql/Test） | UI/IPC/DB/测试 | LGPLv3 / GPLv3 | **动态链接**；保留 LGPLv3 文本 + 重链接说明 | dependencies.lock.json |
| mpv / libmpv | 播放内核 | GPL-2.0-or-later | 聚合 GPL 分发；client.h 为 MIT-0 允许链接；提供 mpv 源码获取方式 | dependencies.lock.json + third_party/build-metadata/mpv.txt |
| FFmpeg（libav*） | worker 音频解码 | LGPL-2.1-or-later（默认）/ GPL-2.0 | 以 LGPL 配置构建；记录完整 configure flags；提供源码 | dependencies.lock.json + third_party/build-metadata/ffmpeg.txt |
| sherpa-onnx | VAD/Paraformer/SenseVoice/标点 | Apache-2.0 | 保留 NOTICE；Apache-2.0 文本随包 | dependencies.lock.json + third_party/build-metadata/sherpa-onnx.txt |
| Silero VAD 权重 | VAD 模型 | MIT（权重） | 模型 manifest 记录；NOTICE | model manifest |
| Paraformer / SenseVoice / CT-Transformer 权重 | ASR 模型 | Apache-2.0 / 上游声明 | 模型 manifest 记录；随离线包提供 NOTICE | model manifest |
| rclone | 网盘挂载（仅诊断，不捆绑） | MIT | 不分发；仅提供配置建议 | — |
| WiX Toolset | MSI 打包（仅工具） | MS-RL | 构建期使用，不随运行时分发 | — |
| CMake | 构建工具 | BSD-3-Clause | 构建期使用 | — |
| QtTest / GoogleTest | 测试 | Qt 许可 / BSD-3-Clause | 仅测试依赖 | — |
| ONNX Runtime | sherpa-onnx 内部 | MIT | 随 sherpa-onnx 分发并保留其声明 | third_party/build-metadata/sherpa-onnx.txt |

## 合规红线（违反即不得发布，技术方案 15.3）

- 未附完整 GPL 源码获取方式、NOTICE、依赖许可证。
- 把 libmpv 仅当 DLL 即声称不受 GPL。
- 默认应用按钮试图绕过系统确认（修改 `UserChoice`）。
- 离线模式出现未解释外连。
- rclone 文档仍推荐 `writes` 作只读视频缓存（必须为 `full`，ADR-0005）。
- SRT 时间重叠或乱码、seek 后出现旧字幕、worker 崩溃拖垮主进程。

## 产物要求（技术方案 20 / 49）

- 源码 + 可复现脚本、dependencies.lock.json、第三方构建日志、模型 manifest/hash。
- Offline + Lite MSI、PDB/符号、GPL 源码包、LICENSE、NOTICE、SBOM。
- 测试/基准/隐私/安全报告、安装升级卸载报告、用户与开发文档、release notes、Git tag。

## 审计脚本

- `tools/check-licenses.ps1`（规划）：确认 `LICENSE`/`NOTICE`/`packaging/licenses/` 齐全、依赖锁与第三方声明一致、无未声明许可证的二进制进入分发。
