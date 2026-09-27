# todolist — Realtime Caption Player（重新基线版 2026-09-27）

> 本文件是项目唯一权威执行 Todo（配合 `03_Detailed_Development_TODO.md` 任务数据库使用）。
> 2026-09-27 起按外部全面审查（`docs/review/2026-09-25-全面代码审查与后续规划.md`）重新基线：
> 此前状态文档中 "RELEASE READY / T0001–T0272 完成" 的声明**撤回**，一切以真实验证状态为准。

## 当前执行状态

- 项目阶段：**阶段 A 止血与重新基线——已完成**（A0–A5 全部落地并通过验证）；下一步阶段 B1
- 当前任务编号：无（等待下一会话/用户指令启动阶段 B）
- 当前允许动作：阶段 B1 协议加固（审查 §5 阶段 B）
- 当前禁止动作：物理删除 CaptionController/CaptionStateMachine/AssEscaper（需用户单独授权）；force-push/历史改写（需用户单独授权）；跳过阶段门
- 项目规则状态/更新时间：AGENTS.md 2026-09-27（内部路径泛化）
- 已读取公用指南：`F:\questions\_索引.md` + 00/02/07/08（2026-09-27）
- 最近验证结果：`tools/build-release.sh` 全新 configure + 全目标构建 rc=0（manifest 默认嵌入）；ctest **21/21 PASS**；`cmake -P cmake/run_bundle.cmake` 白名单打包实证（runtime/models 仅 zipformer-ctc/silero/sensevoice）；4 个比对脚本 py_compile OK
- 最近 Git 存档：阶段 A 共 6 个 commit（`git log --grep '^\[A'`，本次会话产出）
- 当前阻塞：无

## 阶段 A（2026-09-27 完成）

- [x] **A0** 归档审查至 `docs/review/`（含项目侧实码复核勘误附录）+ 建立本 todolist + `docs/adr/ADR-0007-字幕渲染单一路径.md` + 03 TODO 顶部基线注记
- [x] **A1** 重写 `docs/implementation-status.md` 与 `docs/agent-handoff.md`：撤回 RELEASE READY、远程改 PUBLIC、HEAD 说明（filter-repo 后旧哈希失效）、真实课程名/G 盘信息脱敏、venv 绝对路径泛化为占位符
- [x] **A2** 公开仓库路径脱敏：tools 像素比对脚本参数化（`Path(__file__)` 相对 + argv 覆盖）、`cmake/run_bundle.cmake` 仓库相对化、`docs/evidence/`+`docs/phase-gates/`+`questions/` 本机路径与课程名脱敏（Python 字节级替换，保 BOM/换行）、AGENTS.md 内部路径泛化、删一次性工具 `tools/fix_blocker2_zh_streaming.bat`
- [x] **A3** 构建固化：CMakePresets 增 `windows-ninja-release`（Ninja generator）；新增 `tools/build-release.sh`（显式 cl/rc/mt 绝对路径传参，不污染 PATH；自动拷 Qt DLL 到测试目录）；`/MANIFEST:NO` 改 `RCP_SANDBOX_NO_MANIFEST` 选项门控（正式构建默认嵌入 manifest）；`RcpBundle` 模型白名单。验证：全新 configure + 全目标构建（119 targets）rc=0、`ctest --preset windows-ninja-release` **21/21 PASS**、白名单打包实证、py_compile 4 脚本 OK
- [x] **A4** 清理：删根目录重复文档（`01_PRD.md`/`02_Technical_Implementation_Plan.md`/`PRD.md`/`docs/development-todo.md`，与 docs/ 正本逐字节一致；根 `03_Detailed_Development_TODO.md` 保留为任务数据库）、`README.txt`、`SHA256SUMS.txt`、未引用的 `packaging/wix/EnsureAppSubfolder.js/.vbs`；`RELEASE-NOTES-v0.1.0` 标注预览版并修正「mpv osd-overlay」失实描述；README/NOTICE 的 Online Paraformer 陈旧描述更正为 Zipformer2-CTC
- [x] **A5** Git 存档：按任务分 6 个中文 commit → 按 08 指南完成上传内容审计 → push origin main（常规 push，无历史改写）

### A3 踩坑记录（已固化进脚本）

- MSYS bash 的 PATH 只认 POSIX 风格，Windows 版 cmake 只认 Windows 风格 → 编译器/工具链显式绝对路径传参
- `CMAKE_MT-NOTFOUND`：mt.exe 由 Windows SDK 提供，MSVC bin 目录没有
- 中文仓库路径下 MSYS→Windows 的 PATH 转换乱码 → Qt DLL 直接拷到测试 exe 旁（0xc0000135 根因）
- bash → `python -c` 传参把 `\\` 折叠成 `\`，`\r` 被当转义 → 用 `chr(92)` 构造反斜杠

## 阶段 A 验收核对（退出门）

- [x] 已跟踪文件 `git grep -E "F:[/\\]|考点1|提干"` 除 `docs/review/`（审查原文引文）外零命中；`wzm33` 零命中
- [x] `tools/build-release.sh` 干净构建目录 configure+build rc=0
- [x] runtime 模型目录仅含白名单三套模型（paraformer 已从 runtime 移除）
- [x] implementation-status / agent-handoff / todolist 三者状态一致，且与 03 TODO 勾选一致（291 项未勾选 = 真实状态）
- [x] 工作树干净，全部 commit 已 push

## 后续阶段队列（来自审查 §5，未获用户授权不启动实质编码）

- **阶段 B 字幕管线重做**：B1 协议加固（FrameDecoder 两端统一/Unknown/ack/generation 隔离）→ B2 worker 重构（seek/PTS sample clock/有界队列/SegmentAssembler）→ B3 主进程 CaptionCoordinator（状态机/唯一时间线）→ B4 生命周期（心跳超时/重启退避/Job Object）→ B5 档位/语言/过载降级
- **阶段 C 前端纠偏**：C1 假数据/假开关/focus-visible 立即项（可与 B 并行）；C2 MainWindow 结构拆分；C3 播放器基线功能；C4 字幕 UI（依赖 B3）；C5 快捷键编辑器（依赖 D2）
- **阶段 D 持久化与基础设施**：SQLite/WAL + 迁移 + 仓库层；ActionRegistry + Keymap；设置迁移收敛；日志脱敏 + 单实例 + 崩溃转储；GitHub Actions CI
- **阶段 E Windows 集成与发布**：文件关联、版本资源、代码签名、Lite/Offline 双 MSI、恢复 ICE、干净 VM smoke、升级/卸载矩阵

## 已知待用户拍板

1. **历史残留**：阶段 A 仅清洗当前 HEAD；GitHub 可达历史仍残留旧路径（无密钥，2026-09-25 已扫）。是否第二次 filter-repo（会再次变更全部哈希并需 force-push）由用户决定。
2. **死代码删除**：ADR-0007 已定性 mpv osd-overlay 路径四件套为待删死代码，物理删除需用户明确授权（2026-09-04 用户曾拒绝删除）。
