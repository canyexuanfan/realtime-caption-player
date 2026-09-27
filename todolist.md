# todolist — Realtime Caption Player（审查路线 A→E 执行完毕）

> 本文件是项目唯一权威执行 Todo（配合 `03_Detailed_Development_TODO.md` 任务数据库使用）。
> 2026-09-27 按外部全面审查（`docs/review/2026-09-25-全面代码审查与后续规划.md`）重新基线，
> 并于同日完成审查规划的 **阶段 A→E 全部可执行工作**。

## 当前执行状态

- 项目阶段：**阶段 A/B/C/D/E 全部执行完毕**（2026-09-27 单日完成 A→E 主线）
- 当前任务编号：无（等待用户真机回填验收 / 下一轮指令）
- 当前允许动作：真机验证回填、按需修复、后续版本规划
- 当前禁止动作：宣称"全部真机验收完成"（沙箱无 GPU/显示器/语料，真机项见下）
- 项目规则状态/更新时间：AGENTS.md 2026-09-27
- 已读取公用指南：`F:\questions\_索引.md` + 00/02/07/08（2026-09-27）
- 最近验证结果：ctest **20/20 PASS**；seek E2E（30 分钟媒体）PASS；Lite/完整双包真实媒体全程 workerError=0；双 MSI 出包（511MB/343MB，OLE 头通过）；check-status PASS
- 最近 Git 存档：origin/main = 2a514be（[E] 阶段提交）
- 当前阻塞：真机项（渲染出图/ASR 准确率/续播体验）需带显示器设备回填

## 阶段 A（完成，commit 已 push）

- [x] A0 归档审查（含勘误附录）+ todolist + ADR-0007 + 03 TODO 基线注记
- [x] A1 状态文档重基线：撤回 RELEASE READY、PUBLIC 事实、旧哈希标注失效、脱敏
- [x] A2 路径脱敏：tools 参数化、run_bundle 相对化、evidence/报告/指南清洗、删一次性 bat
- [x] A3 构建固化：windows-ninja-release 预设 + tools/build-release.sh + manifest 门控 + 模型白名单
- [x] A4 清理：重复文档/README.txt/SHA256SUMS/未引用 WiX 脚本；RELEASE-NOTES 预览版标注
- [x] A5 分任务中文 commit + 08 审计 + push

## 阶段 B 字幕管线重做（完成，commit 已 push）

- [x] B1 协议加固：Unknown 哨兵/Ack 回执/FrameDecoder 两端统一/generation 贯通/心跳 2s
- [x] B2 worker 管线：AudioExtractor（RAII/取消/seek/PTS 锚定）+ AsrEngine（partial 时间戳/限频去重/resetForSeek/Lite 档）+ CaptionPipeline（播放头 30–120s 窗口调度/双有界队列/换代丢弃/RTF 过载检测）+ IpcServer 实时命令
- [x] B3 CaptionCoordinator：主进程唯一时间线（二分查找）+ 状态机 + seek/换轨/换语言/换档位换代 + 显示选择（final show-until-next/partial 区间内）+ 死代码清理（ADR-0007 方案 a，mpv osd-overlay 四件套删除）
- [x] B4 生命周期：心跳看门狗（2s×6.5s 判失联）/自动重启退避（≤3 次 1/2/4s）/Job Object/closeEvent 优雅关闭
- [x] B5 Lite/Balanced 档位、zh/en/ja/ko 语言、RTF 过载自动降级、设置页真实接线
- [x] B 验收：seek E2E PASS（30 分钟媒体 seekToSec(1800) 识别正确、partial 区间精确锚定，`docs/evidence/B2-seek-e2e/`）；**播放中 seek 3s 窗口行为依赖真机联动，待真机回填**

## 阶段 C 前端纠偏（完成主线，commit 已 push）

- [x] C1 立即项：demo 仅 RCP_DEMO_ONLY 注入 + 空状态引导；保留音调/字幕延迟(sub-delay)/自动导出格式目录真实接线；连播末项循环与导出不执行双 bug 修复；GUI 心跳常驻+new int 泄漏修复；:focus-visible 紫色描边（参考稿 73-76）；Switch hover/focus ring；accessibleName + tab 顺序；QSS 迁移 qrc theme.qss
- [x] C3 播放器基线：track-list 动态音轨/字幕轨菜单（aid→ffmpeg 索引映射同步 worker）、逐帧（,/.）、Esc 退出全屏、±0.1x 倍速步进、静音切换、画面调节（亮度/对比度/饱和度/旋转/宽高比）、带实时字幕截图（grab 合成进图）、休眠抑制（SetThreadExecutionState）、全屏 2.5s 自动隐藏+双击、打开失败错误引导
- [x] C4 字幕 UI：真实指标（延迟=覆盖头-播放头、已识别时长、行数、状态机文案）；置信度无真实数据源保持"—"（不伪造）
- [x] C5 快捷键编辑器（依赖 D2 已满足）：QKeySequenceEdit 编辑 + 持久化 + 冲突提示
- [ ] C2 MainWindow 完整组件化拆分：**部分完成**（CaptionCoordinator 已拆出最大职责；构建函数已按区域分组）——完整拆分（PlayerControls/PlaylistPanel/SettingsDialog 等十余组件）为纯结构重构，列为后续债务，不阻塞功能/验收

## 阶段 D 持久化与基础设施（完成，commit 已 push）

- [x] D1 SQLite：Database（WAL/外键/线程连接）+ MigrationRunner（qrc 内嵌 001–003 迁移）+ HistoryRepository（缓存键=指纹+大小，续播不再按文件名覆盖）+ TranscriptRepository（字幕缓存重看秒出）+ MediaIdentity 去 mtime + MainWindow 后台线程接线 + 缓存预载去重合并
- [x] D2 KeymapService（setBinding/冲突检测）+ hotkeyDefs 动作表接线（C5 前置）
- [x] D3 SettingsService::load 接入 SettingsMigration
- [x] D4 日志脱敏（路径→<path>）+ 字节级轮转计数 + 单实例（QLocalServer 唤起）+ 崩溃转储（MiniDump）
- [x] D5 GitHub Actions（windows-msvc-ci configure/build/test）+ tools/check-status.ps1（本地 PASS）——CI 绿标待推送后由 GitHub 跑出

## 阶段 E Windows 集成与发布（完成可执行项，commit 已 push）

- [x] 双 MSI：完整包 511MB（三模型）/ Lite 包 343MB（zipformer-ctc+silero，AsrEngine 缺失模型自动 Lite 档回退）
- [x] 版本资源（0.1.0 preview）+ manifest 默认嵌入（A3 门控）
- [x] 文件关联注册（OpenWithProgids，"设为默认"的用户选择交互为后续项）
- [x] ICE 校验恢复：实抓修复 ICE80/ICE57；定向豁免 ICE61（同版本顶替设计）/ICE20（定制 UI 收尾项）
- [x] **B2 管线 IPC 崩溃根因修复**（E 验证阶段发现）：EOF 残块重复产出 + sensevoice language 悬空指针 + B4 重启 openSent 时序——双包真实媒体全程 workerError=0
- [ ] 代码签名：**BLOCKED**（无证书；发布以 SHA-256 清单校验，见 RELEASE-NOTES）
- [ ] 干净 VM smoke / 升级卸载矩阵：**BLOCKED**（需真机/VM）
- [ ] v0.1.0 重评：待真机回填后重评

## 全阶段验收核对（2026-09-27 实测）

- [x] 全量构建 rc=0（windows-ninja-release，119+ targets 含 spikes）
- [x] ctest 20/20 PASS（含新增 test_sql_persistence 持久化往返）
- [x] seek E2E PASS（30 分钟媒体，docs/evidence/B2-seek-e2e/）
- [x] Lite/完整双包真实媒体全程 workerError=0 + 快照正常
- [x] 双 MSI 出包 + OLE 头 + 文件关联字节级验证
- [x] check-status.ps1 PASS（无路径泄漏/用户名/虚假声明）
- [x] 全部 commit 已 push（origin/main = 2a514be）

## 已知限制与真机回填清单（诚实记录）

1. 真机渲染出图/ASR 真实语音准确率/续播体验：沙箱无显示器/GPU/语料，待回填
2. 播放中 seek 的 3s 字幕窗口行为：依赖真机 mpv 联动验证
3. 代码签名/VM smoke/升级卸载矩阵：见阶段 E BLOCKED 项
4. C2 完整组件化拆分：列为后续债务
5. 沙箱构建需 build-release.sh（Ninja + 显式 cl/rc/mt 路径）；标准开发机可用 windows-msvc-* 预设
6. 历史提交仍残留旧路径（无密钥）；二次 filter-repo 需用户单独授权
