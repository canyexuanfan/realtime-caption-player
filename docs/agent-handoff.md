# Agent Handoff

- **Last updated:** 2026-08-27
- **Branch:** `main`
- **HEAD:** 见 `git log`（本批次 foundation 提交）

## Current Phase

P0（技术验证、决策与第三方基线）

## Last Completed Task

T0007 — rclone full 缓存修正（ADR-0005）

## Current Task

T0008（依赖锁 schema，IN_PROGRESS）/ T0009（固定 Qt 与构建工具环境，后台安装中）

## What Was Just Done

1. `git init` 于 `main`；创建完整目录骨架（src/、docs/、resources/、migrations/、packaging/、third_party/、tools/、tests/、reference/、app/）。
2. 获取真实 GPL-3.0 全文写入 `LICENSE`；编写 `NOTICE`（第三方占位清单）。
3. 编写 `.gitignore`（排除 .tools/、构建产物、模型/媒体大文件）。
4. 编写 `AGENTS.md`（Agent 协议入口）。
5. 将 PRD/技术方案/TODO 复制进 `docs/product/`、`docs/architecture/`、`docs/development-todo.md`。
6. 编写 6 份 P0 ADR（ADR-0001 GPL / ADR-0002 Qt / ADR-0003 worker / ADR-0004 Paraformer+SenseVoice / ADR-0005 rclone full / ADR-0006 离线模型）及 ADR 模板与索引。
7. 后台启动 `cmake`+`ninja`+`Qt 6.8.1` 安装（任务 70qabc，输出 `.tools/install_step1.log`）。
8. 完成 `reference/` 开源调研与许可证分析（见下）。
9. 实现核心纯逻辑模块与 QtTest 单元测试骨架（见 T-核心提交）。

## Files Changed

- 新增：`LICENSE`、`NOTICE`、`.gitignore`、`README.md`、`AGENTS.md`
- 新增：`docs/product/PRD.md`、`docs/architecture/技术实现方案.md`、`docs/development-todo.md`
- 新增：`docs/adr/ADR-template.md`、`docs/adr/README.md`、`docs/adr/ADR-0001..0006`
- 新增：`docs/implementation-status.md`、`docs/agent-handoff.md`、`docs/phase-gates/P0-report.md`
- 新增：`reference/*`（开源调研）
- 新增：`src/core/*`、`src/ipc/*`、`src/captions/*`、`src/settings/*`、`src/rclone/*`、`tests/unit/*`
- 新增：`CMakeLists.txt`、`CMakePresets.json`、`dependencies.lock.json`、`resources/defaults/*`、`resources/models/model-bundles.json`、`migrations/001_initial.sql`

## Verification Performed

- `git status` / `git branch` 确认仓库状态。
- 工具链探测：cl.exe 存在；cmake/Qt 缺失；gh 已登录；互联网可达。
- LICENSE 为完整 GPL-3.0 文本（35KB）。
- 纯逻辑模块：待 cmake+Qt 安装完成后运行 `ctest --preset windows-msvc-debug` 验证（当前 BLOCKED 于安装）。

## Current Known Problems

- libmpv / FFmpeg / sherpa-onnx / 模型权重 / WiX 缺失 → P0 demo 任务（T0010–T0024）BLOCKED。
- Qt/cmake 安装进行中；完成后需确认 `F:/workbuddy/视频播放器/.tools/Qt/6.8.1/msvc2022_64/lib/cmake/Qt6` 可用。
- 真实模型基准（T0023）需合法语料与模型下载，暂不可行。

## Next Exact Action

1. 检查后台安装结果：`cat F:/workbuddy/视频播放器/.tools/install_step1.log`（确认 cmake/Qt 安装成功）。
2. 若成功：编写 `tools/bootstrap.ps1` 校验 Qt/cmake；执行
   `cmake --preset windows-msvc-debug && cmake --build --preset windows-msvc-debug --parallel && ctest --preset windows-msvc-debug --output-on-failure`
   验证纯逻辑模块（ipc FrameCodec、captions SrtExporter/AssEscaper/CaptionStateMachine/SegmentAssembler、settings KeymapService、rclone 解析、Timecode、Error/Result、BoundedQueue）。
3. 修复测试至全 PASS；将 T0008/T0009 标 DONE 并提交。
4. 继续 T0010–T0013：在具备源码与构建环境后自建并锁定 FFmpeg/libmpv/sherpa-onnx，写 `dependencies.lock.json` 与 `third_party/build-metadata/*`。
5. 每完成一个任务：更新 TODO、implementation-status、agent-handoff；`git commit -m "[Txxxx] ..."`；`git push`。

## Important Decisions

- 工作环境为 Windows + MSVC；Qt 版本采用实际可获取的 **6.8.1**（技术实现方案建议 6.11.2 为基线建议，真实可用版本在 lock 中记录并说明）。
- 纯逻辑模块用 Qt 类型（QString 等）实现以便真实编译与 QtTest；依赖 libmpv/FFmpeg/sherpa 的模块在库就绪前仅保留接口/头文件，不实装，不伪造。
- 所有模型/媒体大文件不进 Git；经 `.gitignore` 排除。

## Commands To Resume

```powershell
cd F:\workbuddy\视频播放器
git status
cat .tools/install_step1.log        # 确认 cmake/Qt 安装结果
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug --parallel
ctest --preset windows-msvc-debug --output-on-failure
git push
```
