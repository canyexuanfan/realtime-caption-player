# Phase P0 Gate Report — 技术验证、决策与第三方基线

- **Status:** ⏳ IN PROGRESS（文档/决策 DONE；第三方 demo 因原生依赖缺失 BLOCKED）
- **Commit:** 见 `git log`（foundation 批次）
- **Tag:** 未创建（阶段门未 PASS）
- **Date:** 2026-08-27

## 阶段目标

先消除最可能导致全盘返工的许可、渲染、字幕截图、音轨映射、网盘双开和 ASR 流式路线风险。

## 阶段门条件（含 P0-report 每项 PASS 才能进入产品代码）

| 验证项 | 要求 | 当前状态 | 说明 |
|---|---|---|---|
| GPL 分发合规 | LICENSE/NOTICE/ADR 一致 | ✅ DONE | ADR-0001 + 真实 GPL-3.0 |
| Qt 跨平台边界 | ADR 明确边界与被否决方案 | ✅ DONE | ADR-0002 |
| worker 分进程 | ADR 含双开代价/IPC 不传 PCM | ✅ DONE | ADR-0003 |
| Paraformer+SenseVoice 路线 | SenseVoice≠streaming partial | ✅ DONE | ADR-0004 |
| rclone full | 禁止 writes 作读缓存 | ✅ DONE | ADR-0005 |
| 依赖锁 schema | 可解析、示例项合规 | 🔶 IN_PROGRESS | T0008 待写 |
| 构建工具环境 | cmake/Qt 版本固定 | 🔶 IN_PROGRESS | T0009 后台安装中 |
| libmpv 渲染 demo | 源码+素材+命令+结果 | ⛔ BLOCKED | 缺 libmpv（T0011/T0014） |
| mpv 事件桥 demo | — | ⛔ BLOCKED | 缺 libmpv |
| ASS overlay + 带字幕截图 demo | — | ⛔ BLOCKED | 缺 libmpv |
| FFmpeg 指定音轨解码 demo | — | ⛔ BLOCKED | 缺 FFmpeg |
| FFmpeg seek + sample clock demo | — | ⛔ BLOCKED | 缺 FFmpeg |
| 本地与 rclone full 双开 demo | — | ⛔ BLOCKED | 缺 FFmpeg+挂载 |
| Online Paraformer partial demo | — | ⛔ BLOCKED | 缺 sherpa+模型 |
| SenseVoice VAD 分句终稿 demo | — | ⛔ BLOCKED | 缺 sherpa+模型 |
| Hybrid partial→final 修订 demo | — | ⛔ BLOCKED | 缺模型 |
| ASR 基准语料 + whisper 基线 | — | ⛔ BLOCKED | 缺模型+语料 |
| P0 阶段门签署 | 全部 PASS | ⛔ BLOCKED | 依赖上述 demo |

## Build

- Debug / Release：待 cmake+Qt 安装后，纯逻辑模块可构建；原生媒体模块暂不可构建。

## Tests

- 单元测试（纯逻辑）：待工具链就绪后 `ctest` 验证。
- 集成/E2E/真实模型测试：BLOCKED（缺原生库与模型）。

## Evidence

- `reference/`：开源调研与许可证分析。
- `docs/adr/`：6 份已接受 ADR。
- `.tools/install_step1.log`：工具链安装日志。

## Known Issues

- 原生依赖（libmpv/FFmpeg/sherpa-onnx/模型/WiX）缺失，导致 P0 所有 demo 与后续阶段 BLOCKED。
- 解决路径：工具链就绪 → T0010–T0013 自建并锁定 → T0014–T0023 demo → T0024 签署阶段门。

## Gate Result

⛔ **FAIL / 未通过**（因原生依赖 BLOCKED，非代码缺陷）。不得进入产品 UI 代码（P1）直至 demo 通过。
