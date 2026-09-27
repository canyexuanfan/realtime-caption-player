# Realtime Caption Player / 实时字幕播放器

本地优先、离线可用的 Windows x64 实时字幕播放器。基于 C++20 + Qt 6 Widgets + libmpv，
使用独立 `caption-worker.exe` 进程运行本地 ASR（Zipformer2-CTC 流式 + SenseVoice + Silero VAD，
由 sherpa-onnx 提供运行时），实时生成中文优先的字幕并支持 SRT 导出。

> 分发许可：**GPL-3.0-or-later**（见 `LICENSE` 与 `NOTICE`）。任何闭源分发路线必须先替换 libmpv 播放内核。

## 当前状态（诚实记录，2026-09-27 重新基线）

- **预览版**：当前是「能打开视频播放并叠加显示实时生成字幕」的演示级实现，距 PRD/技术方案的 MVP 验收
  尚有明确缺口（字幕不随 seek/倍速/暂停/切轨同步、持久化、CI、文件关联、日志脱敏等未实现）。
  完整清单与修复路线见 `docs/review/2026-09-25-全面代码审查与后续规划.md` 与根目录 `todolist.md`（阶段 A→E）。
- 已验证（2026-09-27 复验）：`tools/build-release.sh` 全新 configure + 全目标构建 rc=0、
  QtTest 21/21 PASS、模型白名单自包含运行时、MSI 打包流水线。
- 项目规则与开发协议见 `AGENTS.md`；状态详见 `docs/implementation-status.md`。

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
