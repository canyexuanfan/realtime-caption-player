# Realtime Caption Player / 实时字幕播放器

本地优先、离线可用的 Windows x64 实时字幕播放器。基于 C++20 + Qt 6 Widgets + libmpv，
使用独立 `caption-worker.exe` 进程运行本地 ASR（Online Paraformer + SenseVoice + Silero VAD，
由 sherpa-onnx 提供运行时），实时生成中文优先的字幕并支持 SRT 导出。

> 分发许可：**GPL-3.0-or-later**（见 `LICENSE` 与 `NOTICE`）。任何闭源分发路线必须先替换 libmpv 播放内核。

## 当前状态（诚实记录）

- 本仓库初始仅包含产品/技术/任务文档（资料包）。
- 已建立工程骨架、Agent 接力文档、`reference/` 开源调研、P0 ADR 决策集与核心可测模块骨架。
- **构建工具链缺口（环境相关，非代码缺陷）：** 本机已具备 MSVC `cl.exe`（VS2022 BuildTools）与 `git`/`gh`，
  但 `cmake`、Qt 6、libmpv、FFmpeg、sherpa-onnx、WiX 尚未就绪。正在后台安装 `cmake`+`ninja`+`Qt 6.8.1`，
  以便对**不依赖原生媒体库的纯逻辑模块**进行真实编译与 QtTest 运行。
- 依赖 libmpv/FFmpeg/sherpa-onnx 的模块（播放内核、worker ASR、IPC 真机联调、MSI 打包）在工具链补全前标记为 `BLOCKED`，
  **不会伪造构建/测试通过**。详见 `docs/implementation-status.md`。

## 目录结构（要点）

```
app/player          主程序入口
app/caption-worker  字幕 worker 入口
src/                core / playback / captions / ipc / storage / settings / playlist / platform / diagnostics / ui / worker / app
resources/          默认设置、快捷键、模型 manifest
migrations/         SQLite 迁移
packaging/          WiX / licenses / 第三方声明
third_party/        依赖构建元数据
tools/              bootstrap / build / verify / e2e / package 脚本
tests/              unit / integration / e2e / fixtures / benchmarks
docs/               product / architecture / adr / phase-gates / benchmarks / test-plans / release / implementation-status.md / agent-handoff.md
reference/          开源调研与许可证分析
```

## 构建（目标工具链就绪后）

```powershell
pwsh ./tools/bootstrap.ps1
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug --parallel
ctest --preset windows-msvc-debug --output-on-failure

cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --target package --parallel
```

## 文档优先级（冲突裁决）

`PRD > 技术实现方案 > 详细开发 TODO > ADR > 测试 > 实现 > UI 参考`。

详见 `docs/architecture/技术实现方案.md` 与 `03_Detailed_Development_TODO.md`。
