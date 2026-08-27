# ADR-0002 — Qt 6 Widgets 与跨平台边界

- **状态：** Accepted
- **日期：** 2026-08-27
- **决策人：** 架构 Agent
- **相关任务：** T0004

## 背景

首发平台为 Windows x64。需要一套可维护、能复用核心逻辑到 macOS 的 UI 技术。

## 决策驱动约束

- Windows 优先；核心层不写死 Windows API。
- macOS 后续通过接口复用，不在 MVP 正式版本。

## 候选方案

- A（采用）：C++20 + Qt 6 Widgets。优点：跨平台核心复用、生态成熟、OpenGL 渲染成熟。
- B（否决）：WinUI + SwiftUI 双壳。优点：各平台原生感；缺点：两套 UI/逻辑、首版成本过高、与“核心层复用”冲突。

## 决定

采用 Qt 6 Widgets。Windows API 只允许出现在 `src/platform/windows` 与有 ADR 的极少量构建适配层；
`ui` 可依赖业务接口但不得依赖 worker 内部；`core` 不依赖任何业务模块。

## 影响

- 目录边界见技术实现方案 5.1。
- 后续 macOS 通过 `IPlatformIntegration` / `IVideoSurface` 接口接入。

## 兼容性

核心/字幕/IPC/存储模块保持平台无关，可直接被 macOS 复用。

## 许可证

Qt 6 动态链接（LGPLv3 / GPL-3.0；保留重链接说明）。

## 回滚

若放弃 Qt：需替换整套 UI 与渲染层，影响面大，须新 ADR。

## 验证

静态检查确认 Windows API 调用仅位于 `src/platform/windows`；
CI 对核心库做平台无关编译检查。
