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
- 完成的任务立即 `git push` 到远程 origin。仓库已于 2026-09-25 经用户明确授权转为 **PUBLIC**（canyexuanfan/realtime-caption-player），历史已用 git-filter-repo 清除个人路径；此后任何提交严禁引入 Windows 用户名、本机绝对路径与密钥/Token。
- 外部阻塞（缺工具链/模型/签名/网络）如实记录 `BLOCKED` / `PUSH_PENDING`，
  继续不依赖该阻塞的可执行工作，**不得假装成功**。
- 隐私：默认离线、无遥测、无云上传；日志默认不记录完整字幕正文与完整路径。

## 桌面自动化（本项目运行在这台 Windows 电脑上时）

- 当用户要求操作这台电脑上的内容时，不要因为目标不在内置浏览器里就停在“我不能”。
- 先寻找可行的本地路线：已有 MCP 工具、Python 桌面自动化、pyautogui/pygetwindow、PowerShell/UIAutomation、截图坐标、应用专用脚本，或轻量的临时辅助工具。
- 优先采用轻量、可逆的操作，并持续向用户同步进展。
- 涉及密码、API Key、付款、删除、权限变更、敏感表单最终提交时，必须暂停并确认具体动作。

## 文件保存位置

- 禁止把新生成、下载或另存的文件保存到 C 盘（包括用户桌面、用户目录、C 盘临时目录等任何 C 盘路径）。
- 产出文件一律保存到非 C 盘位置：默认保存到 F 盘工作区（`F:\zcode\workspace\default\`），其次 D/E 等数据盘；用户明确指定路径时以用户指定为准。
- 读取、分析 C 盘上已有的文件不受限制；限制仅针对“保存/写出新文件”。
- 系统或工具必须使用 C 盘路径的临时文件（如 COM 交互）应尽量避免；无法避免时用完立即清理，且不作为成果交付物。

## 问题修复流程（强制循环）

- 每次遇到一类新的技术问题，先检查本项目 `questions` 文件夹中是否已有类似排查文档（若 `questions` 文件夹不存在，则先创建）。
- 如果没有，创建 `questions/[问题类型]排查指南.md`。
- 每次修复失败后，必须先把失败尝试写入对应文档，并用 `❌` 标记，然后重新阅读所有失败经验，避免重复同一条路。
- 按“记录 → 阅读 → 修复 → 验证”的循环持续推进，直到问题解决。
- 修复成功后，必须把成功方法写入文档，并用 `✅` 标记，总结关键经验。
- 排查指南应包含：问题描述、已尝试的修复方法及失败原因、深层问题分析、下一步排查策略、调试工具、注意事项、更新记录。
- 启动开发服务器前必须检查目标端口是否被占用；如果被占用，提示用户并询问是否改用新端口，不要静默冲突，也不要静默换端口。

## 前端设计协作（本项目 UI 相关工作）

- 前端视觉设计、配色方向、布局风格和关键交互方案一律由 Antigravity CLI 产出；Agent 不自行拍板或手搓前端设计方向。
- 给 Antigravity CLI 的提示只包含产品目标、目标用户、核心页面/功能、期望气质等基本描述，不要加太多限制，让它自由发挥。
- 如果用户已提供参考 HTML、现成页面、设计稿或明确可执行的前端参考，则不要重复 Antigravity 前置设计流程；直接基于用户提供的参考进行分析、归档、实现或改造。
- 当用户要求由 Antigravity CLI 生成前端参考 HTML、设计文档或其他文件时，让 Antigravity CLI 直接在目标目录创建文件；Agent 不把终端输出手动转抄成文件，除非 Antigravity 无法写入且用户同意 fallback。
- 若 Antigravity CLI 不可用或调用失败，按「问题修复流程」记录失败原因，再用当前可用方式（文本描述或现有参考）推进，不要让前端工作完全阻塞；但不要假装生成了设计，也不要自行手搓视觉方案充当 Antigravity 的产出。
- 前端设计阶段产生的图像、参考 HTML、视觉方向说明和对比索引，统一保存到 `reference/前端` 文件夹。
- 开发出来的前端组件必须具备清晰的鼠标悬停反馈；按钮、导航项、卡片、表格行、列表项、可点击标签、输入框和工具栏控件都应提供 hover/focus-visible 状态，并避免只在静态状态下完成视觉设计。

## Skill 安装

- 未经完整测试的 Skill 禁止安装或复制到 C 盘全局目录（例如 `C:\Users\<你的用户名>\.agents\skills`）。
- 新 Skill 在未完成真实任务验证前，只能保存在当前项目目录或临时工作区中。
- `quick_validate.py` 只算格式校验，不算完整测试；只有经过至少一次真实场景试运行、关键流程验证和失败经验记录后，才能视为可安装到全局目录。
- 安装全局 Skill 前必须明确告知用户测试状态和目标路径，未经确认不得擅自全局安装。

## 开发复用与架构精简（覆盖整个开发过程）

- 动手新写任何功能、组件、工具、逻辑之前，必须先查找项目中是否已有可复用的实现（代码、模块、脚本、依赖）；已有则直接复用或小幅扩展，禁止另造重复轮子。
- 同一职责只允许一个实现入口：出现第二处相似逻辑时必须当场合并收敛到单一模块/组件/函数，而不是并行维护两份。
- 跨模块共享的元素（工具函数、UI 组件、常量、数据结构）统一收口到唯一位置，修改时全局同步生效。
- 架构保持精简：能复用标准库或已有依赖就不引入新依赖；能复用现有模块就不新增文件；重复代码、废弃分支、一次性临时工具要及时清理。
- 精简的前提是不损失功能与性能：确需重写替代旧实现时，旧实现必须同步移除，禁止新旧两套并存。
- 引入外部库或第三方方案前，先确认项目内无等价实现；引入后应优先用其能力替掉项目内的手写重复实现，而不是叠加。

## 当前环境事实（首次接管时记录）

- OS：Windows（win32）。编译器：`cl.exe`（VS2022 BuildTools，MSVC 14.44 为主）可用。
- `cmake`(4.4.2) / `ninja`(1.13.0) / `Qt 6.8.1`（仓库内 `.qt6/6.8.1/msvc2022_64`，含 Core/Gui/Widgets/Test/Sql）已就绪。
- `libmpv` / `FFmpeg` / `sherpa-onnx` / `WiX` 缺失（BLOCKED）；原生库就绪前依赖它们的模块不实装、不伪造通过。
- 沙箱构建必须用 `Ninja` generator（CMakePresets 默认 VS2022 generator 在 `project()` 阶段崩溃），详见 `docs/implementation-status.md` → Known Issues。
- `git` / `gh`（已登录 github.com）/ 互联网 可用。
- PowerShell 内联 stdout 在本会话不显示，需写入文件后 Read；Git Bash 输出正常。

详见 `docs/implementation-status.md` 与 `docs/agent-handoff.md`。
