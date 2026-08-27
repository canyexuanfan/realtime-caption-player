# Agent Development Protocol — Realtime Caption Player

本文件是所有未来开发 Agent 的入口。无论上下文是否完整，打开仓库后按本文件行动。

## Read First（必读顺序）

1. `docs/product/PRD.md`
2. `docs/architecture/技术实现方案.md`
3. `docs/implementation-status.md`
4. `docs/agent-handoff.md`
5. `03_Detailed_Development_TODO.md`（详细开发 TODO，任务数据库）
6. `docs/adr/`（已接受的架构决策，不得被代码静默违背）
7. `reference/`（开源调研与许可证分析）

## Source of Truth（冲突裁决）

```
PRD > 技术实现方案 > 详细开发 TODO > ADR > 自动化测试/阶段验收 > 当前实现 > UI 参考 HTML/设计图 > 聊天上下文
```

## Resume Procedure（恢复流程）

1. `git status` / `git log --oneline -20` / `git remote -v`
2. 阅读 `docs/implementation-status.md`（当前阶段、当前任务、已知阻塞）
3. 阅读 `docs/agent-handoff.md`（下一步精确动作）
4. 读取 `03_Detailed_Development_TODO.md` 中当前任务的验收标准
5. 交叉验证：TODO 勾选状态 必须与 实际代码 / 测试 / 提交 / evidence 一致
6. 选取「当前阶段内、所有前置任务 DONE 的第一项未完成任务」执行

## 阶段（Phase Gates）

```
P0  技术验证、决策与第三方基线
P1  仓库骨架与基础设施
P2  libmpv 播放内核
P3  设置、播放列表、历史、持久化
P4  IPC、worker 生命周期、FFmpeg 音频
P5  Lite / Balanced ASR
P6  字幕状态机、Overlay、缓存、SRT
P7  完整 UI、设置、可访问性、诊断
P8  Windows 集成、文件关联、MSI
P9  全量 QA、安全、性能、合规、Release
```

## Rules（铁律）

- **绝不伪造完成状态**：没有真实构建/测试/证据，不得勾选 `[x]`。
- **绝不跳过依赖或阶段门**。
- **绝不静默更换技术路线**；需变更必须先写 `docs/adr/ADR-xxxx-*.md`。
- 每个任务都有验证；任务级验收先于勾选。
- 实时回填 TODO、implementation-status、agent-handoff。
- 每个任务一个独立 Git commit，标题格式 `[Txxxx] 中文动作描述`。
- 完成的任务立即 `git push` 到私有远程（PRIVATE，禁止公开仓库）。
- 外部阻塞（缺工具链/模型/签名/网络）如实记录 `BLOCKED` / `PUSH_PENDING`，
  继续不依赖该阻塞的可执行工作，**不得假装成功**。
- 隐私：默认离线、无遥测、无云上传；日志默认不记录完整字幕正文与完整路径。

## 当前环境事实（首次接管时记录）

- OS：Windows（win32）。编译器：`cl.exe`（VS2022 BuildTools，MSVC 14.44 为主）可用。
- `cmake`(4.4.2) / `ninja`(1.13.0) / `Qt 6.8.1`（仓库内 `.qt6/6.8.1/msvc2022_64`，含 Core/Gui/Widgets/Test/Sql）已就绪。
- `libmpv` / `FFmpeg` / `sherpa-onnx` / `WiX` 缺失（BLOCKED）；原生库就绪前依赖它们的模块不实装、不伪造通过。
- 沙箱构建必须用 `Ninja` generator（CMakePresets 默认 VS2022 generator 在 `project()` 阶段崩溃），详见 `docs/implementation-status.md` → Known Issues。
- `git` / `gh`（已登录 github.com）/ 互联网 可用。
- PowerShell 内联 stdout 在本会话不显示，需写入文件后 Read；Git Bash 输出正常。

详见 `docs/implementation-status.md` 与 `docs/agent-handoff.md`。
