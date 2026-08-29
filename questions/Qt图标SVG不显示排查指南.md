# Qt SVG 图标/Logo 不显示排查指南

## 问题描述

复刻前端后，UI 中所有按钮图标（SVG）、标题栏 logo、窗口图标全部不显示：
控制台按钮是空圆、播放大圆无 ▶、设置导航无图标。用户三次反馈"没有变化"。

## 已尝试的修复方法及失败原因（严格时序记录）

### ❌ 尝试 1：qt_add_resources 挂在静态库 rcp_player 上（初始实现）
qrc 初始化对象（qrc_*_init.cpp.obj）无外部符号引用，被 MSVC 链接器丢弃，
运行时 :/ 资源全部读取失败。
→ 修复动作：qt_add_resources 移到 player_app 可执行目标（提交 4a022fe）。
→ 结果：构建通过、obj 存在，但用户仍报"没有变化"。不充分。

### ❌ 尝试 2：qt_add_resources 的 FILES 语法不支持 alias（独立 bug，叠加）
资源落在 :/logo128.png，而代码读 :/logo.png → logo 必然 404。
（与尝试 1 无关的独立缺陷，即使 1 修好 logo 也不显示。）

### ❌ 尝试 3：移到 exe 后仍不可靠/未验证视觉（提交 4a022fe 的状态）
我没有视觉自验证手段（截屏通道故障），只能让用户当眼睛——
违反了"验证闭环"原则，反复让用户失望。

### ⚠️ 尝试 4（当前，待视觉确认）：AUTORCC ON + 手写 rcp.qrc（提交 53196e3）
- 放弃 qt_add_resources，改经典 AUTORCC + 手写 qrc（alias logo.png 正确）。
- main.cpp 加**启动资源自检**：resource-check.txt 记录 :/ 资源可达性。
- 运行时实测（我本机跑 exe 读取自检文件）：
  `:/logo.png=OK, :/icons/play.svg=OK, :/icons/settings.svg=OK,
  :/icons/subtitle.svg=OK, svg-bytes=227, svg-render(play)=OK`
  → **资源加载与 SVG 解析层已证实通过**。
- ⚠️ 但用户仍报"没有变化" → 说明还存在**渲染输出层**问题未排除：
  a) svgIcon 的颜色替换 `currentColor → QColor::name(HexArgb)`（#AARRGGBB）
     可能被 QSvgHandler 拒绝 → 描边回落黑色 → 深色 UI 里"看不见"；
  b) QPixmap/DPR/其他绘制环节；
  c) 用户运行的 exe 不是最新 bundle（无法排除）。
- **教训**：只有 isValid() 不够，必须验证"渲染出的像素非空/非纯背景"。

## ❌ 流程性失败（用户点名批评）

违反 AGENTS.md 问题修复流程：未在每次失败后先 ❌ 记录并重读全部失败经验，
而是"猜→构建→让用户验收→再猜"，把用户当验证工具。本指南即为补立档案。
后续任何修复必须先过：**离屏快照自验证**（见下），我亲眼确认渲染像素后才交付。

## 下一步排查策略（已定，按序执行）

1. **离屏快照自验证**：main.cpp 加 RCP_SNAPSHOT=<png路径> 支持——启动后
   QTimer 单发 grab() 存 PNG 并退出；用 QT_QPA_PLATFORM=offscreen 跑，
   不弹窗、不扰用户，我直接 Read PNG 亲眼看渲染结果。
2. 据快照逐项修（图标颜色/布局/logo），每修一轮重拍快照对比。
3. 快照与参考稿结构一致后 → 重打 MSI → 提交 → 请用户终验。

## 调试工具

- `RCP_SNAPSHOT=<path>` + `QT_QPA_PLATFORM=offscreen`：离屏视觉自验证（本轮新增）。
- `resource-check.txt`（exe 旁/AppData）：资源可达性 + QSvgRenderer 有效性（常驻）。
- 构建产物 obj 检查：player_app.dir/.qt/rcc/qrc_*.obj 存在性。

## 注意事项

- 静态库里的 qrc 初始化对象会被 MSVC 链接器丢弃；资源必须挂最终 exe。
- qt_add_resources 的 FILES 不支持 alias；需要别名用 AUTORCC + 手写 .qrc。
- QSvgRenderer.isValid() ≠ 渲染像素正确；必须像素级快照确认。
- 验证闭环原则：改 UI 必须自带"我能亲眼看到"的手段，不得让用户当验证工具。

## 更新记录

- 2026-08-29 首次建档：4 次尝试全记录（3❌ 1⚠️待确认），确定离屏快照自验证路线。

## ✅ 成功方法（2026-08-29 终版，运行时+像素双验证）

1. 资源：AUTORCC ON + 手写 resources/rcp.qrc（alias logo.png / icons/*.svg 正确），
   qrc 作为 player_app 源文件（可执行目标）。弃用 qt_add_resources（两大坑见上）。
2. 图标颜色：svgIcon 中 currentColor 替换必须用 **6 位 #RRGGBB**（color.name()），
   8 位 #AARRGGBB 是 Qt 私有格式，QSvgHandler 不认 → 描边回落黑色 → 深色背景隐形。
3. **视觉自验证闭环（关键流程改进）**：main.cpp 支持 RCP_SNAPSHOT=<png>：
   以 WA_DontShowOnScreen + show()（布局正常激活、屏幕无窗口）→ 2.5s 后 grab() 存 PNG
   退出。配合 resource-check.txt（资源可达性+QSvgRenderer 有效性）构成双层自检。
   此前隐藏窗口直接 grab 会因布局未激活产生伪影（列表行 "e 0"、ASR 卡空白）——
   WA_DontShowOnScreen 版本消除伪影。
4. 实证：snap3.png 像素级确认——logo、30 类图标、列表行「▸ e2e-test.mp4 00:00」、
   tabs、设置面板 7 页导航图标、控制台全部按钮图标全部渲染。

### 流程更正（用户指出）
违反 AGENTS.md 问题修复流程：失败后未先 ❌ 记录、未重读失败经验、把用户当验证工具。
本指南即为补档；后续 UI 问题一律先快照自证、后交付。

### 本轮自检修复（快照闭环首次实战）
- ASR 卡高度从未显式确定（靠运行时 polish）→ buildAsrCard 末尾 adjustSize()，
  快照实证完整渲染（快照模式与真机一致）。
- 播放列表行时长脏数据（durationChanged 早于 m_currentPath 赋值）→
  openFile 先设 m_currentPath；显示阈值 d>999。
- 引擎值文案过长被 195px 卡截断 → 「本地（双引擎）」。
- snap4/snap5 像素级确认以上全部生效。
