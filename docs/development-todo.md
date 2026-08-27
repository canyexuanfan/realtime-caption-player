# 实时字幕播放器——详细开发 TODO List

> 文档版本：v1.0  
> 配套文档：`实时字幕播放器_技术实现方案.md`  
> 目标：把完整项目拆成可独立实施、可验证、可提交的小任务，使初级开发者或代码 Agent 不需要自行猜架构、顺序和完成标准。  

---

## 一、执行协议（每个开发 Agent 必须遵守）

1. **只领取当前阶段中“所有前置任务已完成”的第一项任务。** 不得跳过阶段门。
2. 开始前阅读该任务列出的文件以及配套技术方案对应章节；不要遍历和改动无关模块。
3. 一个任务只做任务中写明的内容。发现额外问题，记录到 `docs/implementation-status.md`，不要顺手大改。
4. 不允许提交占位函数、永远返回成功的 mock、假进度、假设置开关、无实现菜单或未说明的 `TODO/FIXME`。
5. 先写或更新测试，再完成实现；至少执行任务验收项。测试失败不得勾选。
6. 每个任务一个独立提交，提交标题：`[Txxxx] 中文动作描述`。
7. 完成后更新 `docs/implementation-status.md`：状态、commit、测试命令、结果/报告位置、遗留说明。
8. UI 任务必须保存截图或 UI 自动化证据；性能任务必须保存原始 JSON/CSV；安装任务必须保存包 SHA-256。
9. 遇到方案无法执行时停止该任务，新增 ADR 草案并把状态标为 `BLOCKED`；不得私自换技术路线。
10. 所有路径、文件名、命令行、IPC、字幕文本都视为不可信输入；不得通过 shell 拼接执行。
11. 主线程不得执行模型加载、FFmpeg 解码、数据库大写入、目录扫描或网络访问。
12. 每次 seek、换媒体、换音轨、换模型/语言都必须通过 `generation` 隔离旧任务。
13. 只有验收证据齐全后，才把本文件对应复选框改为 `[x]`。

### 状态表格式

`docs/implementation-status.md` 必须至少包含：

```markdown
| Task | Status | Commit | Verification | Evidence | Notes |
|---|---|---|---|---|---|
| T0001 | DONE | abc1234 | `git diff --check` | docs/... | 无 |
```

允许状态只有：`TODO / IN_PROGRESS / BLOCKED / DONE / WAIVED`。`WAIVED` 只允许出现在非 P0 任务，并必须链接书面批准。

### 每项任务的通用完成检查

```powershell
# 只运行与任务相关的最小测试后，还必须执行：
git diff --check
cmake --build --preset windows-msvc-debug --parallel
ctest --preset windows-msvc-debug --output-on-failure
pwsh ./tools/check-status.ps1
```

阶段门任务还必须执行 Release 构建、真实依赖/模型测试以及该阶段列出的 E2E。

---

## 阶段 0：技术验证、决策与第三方基线

**阶段目标：** 先消除最可能导致全盘返工的许可、渲染、字幕截图、音轨映射、网盘双开和 ASR 流式路线风险。  
**阶段门：** T0001–T0024 全部完成；`docs/phase-gates/P0-report.md` 的每个验证项均为 PASS，才能进入产品代码。

### T0001 — 把 PRD、技术方案和 TODO 纳入仓库

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** 无
- **修改/新增文件：** docs/product/PRD.md；docs/architecture/技术实现方案.md；docs/implementation-status.md
- **执行步骤：**
  1. 复制三份文档到固定目录。
  2. 在状态文件写明文档版本、来源和当前阶段。
  3. 添加文档变更规则。
- **验收标准：**
  - 三个文件可从仓库直接打开。
  - 状态文件包含 P0、负责人占位、最近更新时间。
  - `git diff --check` 通过。
- **完成证据：** `[<commit>] T0001` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0002 — 建立 ADR 模板

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0001
- **修改/新增文件：** docs/adr/ADR-template.md；docs/adr/README.md
- **执行步骤：**
  1. 定义状态、背景、约束、候选、决定、影响、回滚、验证字段。
  2. 规定 ADR 编号和审批方式。
- **验收标准：**
  - 复制模板可形成完整 ADR。
  - README 明确已接受 ADR 不得被代码静默违背。
- **完成证据：** `[<commit>] T0002` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0003 — 记录 GPL 开源分发决定

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0002
- **修改/新增文件：** docs/adr/ADR-0001-gpl-distribution.md；LICENSE；NOTICE
- **执行步骤：**
  1. 写明 MVP 整体 GPL-3.0-or-later。
  2. 列出 Qt、mpv、FFmpeg、sherpa-onnx 和模型的处理。
  3. 写明闭源前必须替换 libmpv。
- **验收标准：**
  - LICENSE 与 ADR 一致。
  - NOTICE 有第三方占位清单。
  - 许可证检查脚本能找到这些文件。
- **完成证据：** `[<commit>] T0003` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0004 — 记录 Qt Widgets 与跨平台边界决定

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0002
- **修改/新增文件：** docs/adr/ADR-0002-qt-widgets.md
- **执行步骤：**
  1. 写明 Windows x64 首发、Qt Widgets、核心层不写 Windows API、macOS 后续通过接口复用。
- **验收标准：**
  - ADR 包含被否决的 WinUI+SwiftUI 双壳方案及原因。
  - 目录边界与技术方案一致。
- **完成证据：** `[<commit>] T0004` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0005 — 记录播放器与字幕 worker 分进程决定

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0002
- **修改/新增文件：** docs/adr/ADR-0003-caption-worker-process.md
- **执行步骤：**
  1. 写明 worker 直接用 FFmpeg 二次打开媒体、QLocalSocket、崩溃隔离、rclone full 依赖。
- **验收标准：**
  - ADR 包含双开代价、IPC 不传 PCM 的原因、回退方式。
- **完成证据：** `[<commit>] T0005` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0006 — 记录 Online Paraformer + SenseVoice 双层识别决定

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0002
- **修改/新增文件：** docs/adr/ADR-0004-hybrid-asr.md
- **执行步骤：**
  1. 明确 Online Paraformer 只负责 partial。
  2. SenseVoice 只负责 VAD 分句终稿。
  3. Lite 和 Balanced 两档。
- **验收标准：**
  - ADR 禁止把 SenseVoice 当原生 continuous partial。
  - 列出超时降级规则。
- **完成证据：** `[<commit>] T0006` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0007 — 记录 rclone full 缓存修正

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0002
- **修改/新增文件：** docs/adr/ADR-0005-rclone-vfs-full.md；docs/user/rclone.md
- **执行步骤：**
  1. 把只读视频推荐值固定为 `--vfs-cache-mode full`。
  2. 说明 `writes` 不满足读取缓存。
  3. 移除热清理承诺。
- **验收标准：**
  - 全文搜索不再把 `writes` 写成视频推荐值。
  - 文档有完整示例参数。
- **完成证据：** `[<commit>] T0007` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0008 — 建立第三方依赖锁文件 schema

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0003
- **修改/新增文件：** dependencies.lock.json；tools/verify-dependencies.ps1；third_party/build-metadata/.gitkeep
- **执行步骤：**
  1. 为每个依赖记录版本、tag、commit、源码 hash、二进制 hash、许可证和构建参数文件。
  2. 脚本校验必填字段。
- **验收标准：**
  - 脚本对完整 lock 返回 0。
  - 删掉任一 hash 后返回非零并指出依赖名。
- **完成证据：** `[<commit>] T0008` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0009 — 固定 Qt 与构建工具环境

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0008
- **修改/新增文件：** tools/bootstrap.ps1；CMakePresets.json；docs/development/setup-windows.md
- **执行步骤：**
  1. 检测 MSVC、Windows SDK、CMake、Qt。
  2. 只接受锁定范围。
  3. 打印缺失安装项。
  4. 不得静默改系统配置。
- **验收标准：**
  - 干净开发机按文档可完成检测。
  - 重复运行 bootstrap 结果一致。
  - 缺 Qt 时明确失败。
- **完成证据：** `[<commit>] T0009` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0010 — 自建并锁定 FFmpeg 运行库

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0008,T0009
- **修改/新增文件：** tools/build-third-party.ps1；third_party/build-metadata/ffmpeg.txt；dependencies.lock.json
- **执行步骤：**
  1. 从锁定源码构建 shared FFmpeg。
  2. 记录 configure flags。
  3. 复制头文件、import lib、DLL 到 artifact。
  4. 计算 SHA-256。
- **验收标准：**
  - worker 测试程序可链接并打印版本。
  - 所有产物 hash 与 lock 一致。
  - 构建日志可复现。
- **完成证据：** `[<commit>] T0010` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0011 — 自建并锁定 mpv/libmpv

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0008,T0010
- **修改/新增文件：** tools/build-third-party.ps1；third_party/build-metadata/mpv.txt；dependencies.lock.json
- **执行步骤：**
  1. 使用锁定源码和 FFmpeg 构建 libmpv。
  2. 保存 feature/config 列表。
  3. 生成 import lib、DLL、头文件和许可证。
- **验收标准：**
  - 最小控制台程序创建并销毁 mpv_handle。
  - 版本、commit、hash 可追溯。
- **完成证据：** `[<commit>] T0011` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0012 — 自建并锁定 sherpa-onnx

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0008,T0009
- **修改/新增文件：** tools/build-third-party.ps1；third_party/build-metadata/sherpa-onnx.txt；dependencies.lock.json
- **执行步骤：**
  1. 构建 C++ shared/static 依赖。
  2. 关闭未使用示例。
  3. 锁定 ONNX Runtime 方式。
  4. 记录 CPU 指令要求。
- **验收标准：**
  - 最小程序可加载空 recognizer 配置并返回预期错误。
  - DLL 依赖在干净机可解析。
- **完成证据：** `[<commit>] T0012` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0013 — 建立模型 component manifest

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0012
- **修改/新增文件：** resources/models/model-bundles.json；models/manifests/*.json；tools/verify-models.ps1
- **执行步骤：**
  1. 为 Paraformer、SenseVoice、标点、VAD 记录文件、尺寸、SHA-256、采样率、语言、许可证、兼容版本。
- **验收标准：**
  - 脚本验证完整模型返回 0。
  - 篡改 1 字节后返回非零。
  - 不在日志输出模型来源凭据。
- **完成证据：** `[<commit>] T0013` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0014 — 建立 P0 libmpv 渲染验证程序

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0011
- **修改/新增文件：** spikes/mpv-render/CMakeLists.txt；spikes/mpv-render/main.cpp
- **执行步骤：**
  1. 用 QOpenGLWidget + mpv render API 打开测试视频。
  2. 实现 resize、暂停、seek。
  3. 记录显卡和 mpv 日志。
- **验收标准：**
  - 连续打开/关闭 20 次不崩溃。
  - 100% 和 200% DPI 画面比例正确。
  - 提供运行命令。
- **完成证据：** `[<commit>] T0014` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0015 — 验证 mpv 事件桥和属性观察

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0014
- **修改/新增文件：** spikes/mpv-render/MpvEventProbe.*；docs/spikes/mpv-events.md
- **执行步骤：**
  1. 接入 wakeup callback。
  2. 观察 time-pos、pause、speed、track-list、aid、seeking。
  3. 禁止 callback 内做 UI 工作。
- **验收标准：**
  - 事件在 GUI 线程消费。
  - seek/切音轨后日志顺序可解释。
  - Thread Sanitizer 不适用时至少加线程断言。
- **完成证据：** `[<commit>] T0015` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0016 — 验证 ASS overlay 与带实时字幕截图

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0014,T0015
- **修改/新增文件：** spikes/mpv-render/OverlayProbe.*；docs/spikes/overlay-screenshot.md
- **执行步骤：**
  1. 用 `osd-overlay` 显示灰/白两种字幕。
  2. 测试 mpv 截图 flag。
  3. 若截图不含 overlay，完成 framebuffer 合成验证。
- **验收标准：**
  - 输出 3 张样例：无字幕、原字幕、实时字幕。
  - 带字幕图不含播放器控制栏。
  - 中文无乱码。
- **完成证据：** `[<commit>] T0016` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0017 — 验证 FFmpeg 指定音轨解码

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0010
- **修改/新增文件：** spikes/audio-decode/main.cpp；tests/fixtures/multitrack.mkv；docs/spikes/audio-track-map.md
- **执行步骤：**
  1. 读取 mpv track-list 的 ff-index。
  2. 让 FFmpeg 按 index 解码。
  3. 输出 codec、channels、首尾 PTS 和 PCM hash。
- **验收标准：**
  - 多音轨 fixture 选择不同轨道得到不同 PCM hash。
  - 不允许总是取第一音轨。
- **完成证据：** `[<commit>] T0017` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0018 — 验证 FFmpeg seek 与 sample clock

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0017
- **修改/新增文件：** spikes/audio-decode/SeekProbe.*；docs/spikes/audio-seek.md
- **执行步骤：**
  1. 用 avformat_seek_file 跳到多个位置。
  2. flush decoder/resampler。
  3. 构造单调毫秒时钟。
  4. 丢弃 target 前音频。
- **验收标准：**
  - 每个 seek 首块时间在允许误差内。
  - 长音频时间不倒退。
  - 连续 seek 无旧数据。
- **完成证据：** `[<commit>] T0018` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0019 — 验证本地与 rclone full 双开读取

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0014,T0017,T0007
- **修改/新增文件：** spikes/rclone-double-open/；docs/spikes/rclone-double-open.md
- **执行步骤：**
  1. mpv 与 FFmpeg 同时打开同一挂载视频。
  2. 记录首次播放、seek、二次播放的读缓存行为。
  3. 测试网络暂断。
- **验收标准：**
  - VFS full 下随机 seek 可恢复。
  - 报告明确缓存目录、参数和观察。
  - 任何数据不支持时不写“通过”。
- **完成证据：** `[<commit>] T0019` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0020 — 验证 Online Paraformer partial

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0012,T0013
- **修改/新增文件：** spikes/asr-hybrid/OnlineProbe.*；docs/spikes/asr-online.md
- **执行步骤：**
  1. 加载 int8 在线模型。
  2. 按 100 ms chunk 输入 fixture。
  3. 每 200 ms 读取 partial。
  4. 在 endpoint 取 final。
- **验收标准：**
  - 连续语音能产生非空 partial。
  - 相同 partial 被去重。
  - 记录首 partial、RTF、峰值内存。
- **完成证据：** `[<commit>] T0020` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0021 — 验证 SenseVoice VAD 分句终稿

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0012,T0013
- **修改/新增文件：** spikes/asr-hybrid/SenseVoiceProbe.*；docs/spikes/asr-sensevoice.md
- **执行步骤：**
  1. 用 VAD 切分。
  2. 添加前后 padding。
  3. SenseVoice 识别完整段。
  4. 启用模型支持的 ITN/标点选项。
- **验收标准：**
  - 普通话 fixture 输出可读终稿。
  - 每段有媒体起止时间。
  - 记录句尾到 final 延迟。
- **完成证据：** `[<commit>] T0021` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0022 — 验证 Hybrid partial→final 修订

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0020,T0021
- **修改/新增文件：** spikes/asr-hybrid/HybridProbe.*；docs/spikes/asr-hybrid.md
- **执行步骤：**
  1. 同一 PCM 同时驱动在线 partial 与 VAD。
  2. 为每段分配 segment_id。
  3. SenseVoice 以 revision 覆盖。
  4. 超时用标点化在线结果。
- **验收标准：**
  - 事件序列符合 partial*→final/revision。
  - 无重叠 segment_id。
  - 超时路径可强制复现。
- **完成证据：** `[<commit>] T0022` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0023 — 建立合规 ASR 基准语料与 whisper 基线

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0020,T0021
- **修改/新增文件：** tests/fixtures/asr/manifest.json；tools/benchmark-asr.ps1；docs/benchmarks/baseline.md
- **执行步骤：**
  1. 只放有授权/自录素材。
  2. 保存参考文本、区间和语言。
  3. 运行项目内选定 whisper.cpp 基线并记录参数。
- **验收标准：**
  - manifest 每条有许可说明。
  - 脚本输出 CER/WER、延迟、RTF、RSS。
  - 结果可重复。
- **完成证据：** `[<commit>] T0023` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0024 — 签署 P0 阶段门报告

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0003-T0023
- **修改/新增文件：** docs/phase-gates/P0-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 逐项链接源码、命令、日志、截图和基准。
  2. 对失败项写结论，不得以“后续再看”判 PASS。
  3. 更新状态。
- **验收标准：**
  - 报告所有必选项 PASS。
  - 依赖/模型 hash 固定。
  - 架构无未决阻塞。
  - 状态进入 P1。
- **完成证据：** `[<commit>] T0024` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 1：仓库骨架、公共基础设施与可构建空应用

**阶段目标：** 建立可复现构建、错误模型、日志、路径、设置、测试和两个可执行程序的稳定骨架。  
**阶段门：** T0025–T0049 完成；Debug/Release/ASan 预设可构建，空播放器与 worker 可启动、握手并退出。

### T0025 — 创建根 CMake 工程与目标

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0024
- **修改/新增文件：** CMakeLists.txt；app/player/CMakeLists.txt；app/caption-worker/CMakeLists.txt；src/CMakeLists.txt；tests/CMakeLists.txt
- **执行步骤：**
  1. 创建 `RealtimeCaptionPlayer`、`caption-worker`、共享静态库和测试目标。
  2. 设置 C++20。
  3. 禁止全局 include/link 污染。
- **验收标准：**
  - 四个预设至少 Debug/Release 可生成。
  - 两个 exe 启动返回 0。
  - compile_commands 可生成。
- **完成证据：** `[<commit>] T0025` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0026 — 完善 CMakePresets

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025
- **修改/新增文件：** CMakePresets.json
- **执行步骤：**
  1. 定义 debug、release、asan、ci configure/build/test presets。
  2. 统一输出到 `out/<preset>`。
  3. 注入依赖根目录。
- **验收标准：**
  - 每个 preset 名与技术方案一致。
  - 错误依赖路径时生成阶段明确失败。
- **完成证据：** `[<commit>] T0026` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0027 — 启用严格编译选项

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025
- **修改/新增文件：** cmake/CompilerWarnings.cmake；cmake/Sanitizers.cmake
- **执行步骤：**
  1. MSVC 启用 `/W4 /permissive- /utf-8` 等。
  2. 项目代码 warning-as-error。
  3. 第三方头标 SYSTEM。
- **验收标准：**
  - 故意加入未使用局部变量会让 CI 失败。
  - 第三方 warning 不淹没构建。
- **完成证据：** `[<commit>] T0027` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0028 — 生成版本与构建元数据

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0025
- **修改/新增文件：** src/core/AppVersion.h.in；cmake/Version.cmake；resources/version.json.in
- **执行步骤：**
  1. 从项目版本和 Git commit 生成版本。
  2. dirty build 标记。
  3. 提供 protocol/schema 版本常量。
- **验收标准：**
  - `--version` 输出产品、commit、build type、IPC 版本。
  - Release 不含空 commit。
- **完成证据：** `[<commit>] T0028` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0029 — 实现 AppPaths

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025
- **修改/新增文件：** src/core/AppPaths.h；src/core/AppPaths.cpp；tests/unit/test_app_paths.cpp
- **执行步骤：**
  1. 用 Qt/Known Folder API 计算配置、数据、日志、模型、截图和导出路径。
  2. 创建目录。
  3. 提供测试 override。
- **验收标准：**
  - 中文用户名测试通过。
  - 测试可重定向到临时目录。
  - 不硬编码盘符/用户名。
- **完成证据：** `[<commit>] T0029` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0030 — 实现结构化日志与轮转

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0029
- **修改/新增文件：** src/core/Logging.h；src/core/Logging.cpp；tests/unit/test_logging.cpp
- **执行步骤：**
  1. 区分 player/worker。
  2. 加入 UTC、PID、TID、category、correlation ID。
  3. 路径和字幕默认脱敏。
  4. 按大小/保留天数轮转。
- **验收标准：**
  - 连续写大日志会轮转且总量受限。
  - 默认日志搜索不到完整 fixture 路径和字幕正文。
- **完成证据：** `[<commit>] T0030` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0031 — 实现 Result 与 AppError

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025
- **修改/新增文件：** src/core/Result.h；src/core/Error.h；src/core/Error.cpp；tests/unit/test_error.cpp
- **执行步骤：**
  1. 定义错误域、code、用户消息、技术消息、retryable、脱敏 context。
  2. 提供从 Qt/mpv/FFmpeg 错误转换器。
- **验收标准：**
  - 错误可被单元测试比较和序列化。
  - 技术信息不直接显示给用户。
- **完成证据：** `[<commit>] T0031` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0032 — 实现 Timecode 工具

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0025
- **修改/新增文件：** src/core/Timecode.h；src/core/Timecode.cpp；tests/unit/test_timecode.cpp
- **执行步骤：**
  1. 实现毫秒转换、SRT 格式、显示格式、范围夹取和安全加减。
- **验收标准：**
  - 覆盖负值、0、99/100 小时、最大 int64 边界。
  - 无溢出。
- **完成证据：** `[<commit>] T0032` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0033 — 实现取消令牌与 generation 类型

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025
- **修改/新增文件：** src/core/Cancellation.h；src/core/Generation.h；tests/unit/test_cancellation.cpp
- **执行步骤：**
  1. 用原子状态实现 cooperative cancel。
  2. generation 使用强类型避免和普通整数混用。
- **验收标准：**
  - 多线程取消测试无数据竞争。
  - 旧 generation 比较明确失败。
- **完成证据：** `[<commit>] T0033` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0034 — 建立 QtTest 单元测试框架

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025
- **修改/新增文件：** tests/unit/TestMain.cpp；cmake/AddQtTest.cmake
- **执行步骤：**
  1. 提供临时目录、事件循环等待、日志捕获、fixture 查找 helper。
  2. 每个测试独立。
- **验收标准：**
  - `ctest --preset windows-msvc-debug` 能发现并执行测试。
  - 失败输出包含测试名。
- **完成证据：** `[<commit>] T0034` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0035 — 建立 CI 基础流水线

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0026,T0027,T0034
- **修改/新增文件：** .github/workflows/ci.yml 或实际 CI 配置；tools/ci.ps1
- **执行步骤：**
  1. 执行依赖验证、configure、build、unit test、diff check。
  2. 缓存只按 lock hash 命中。
- **验收标准：**
  - 空 PR 流水线通过。
  - 破坏测试时失败。
  - 日志不暴露 secret。
- **完成证据：** `[<commit>] T0035` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0036 — 实现依赖运行时校验

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0008,T0025
- **修改/新增文件：** src/diagnostics/DependencyProbe.h；src/diagnostics/DependencyProbe.cpp；tests/unit/test_dependency_probe.cpp
- **执行步骤：**
  1. 启动时读取编译期/运行时版本。
  2. 检测 DLL 版本不一致。
  3. 生成脱敏摘要。
- **验收标准：**
  - 替换为错误版本 mock 时返回 `APP-DEPENDENCY-MISMATCH`。
  - 正常版本显示在诊断中。
- **完成证据：** `[<commit>] T0036` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0037 — 实现许可证清单生成脚本

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0003,T0008
- **修改/新增文件：** tools/generate-notices.ps1；packaging/licenses/manifest.json
- **执行步骤：**
  1. 从 lock 和模型 manifest 生成 NOTICE/SBOM 输入。
  2. 缺许可证文本或来源时失败。
- **验收标准：**
  - 当前依赖生成成功。
  - 删除任一 license 文件后脚本非零退出。
- **完成证据：** `[<commit>] T0037` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0038 — 建立 Qt 资源与中文翻译框架

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0025
- **修改/新增文件：** resources/resources.qrc；resources/translations/app_zh_CN.ts；src/ui/Tr.h
- **执行步骤：**
  1. 将图标、默认 JSON、翻译纳入资源。
  2. 所有用户字符串走 `tr()`。
- **验收标准：**
  - 运行时加载中文。
  - 源代码扫描不允许新增明显硬编码用户文案（测试/日志除外）。
- **完成证据：** `[<commit>] T0038` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0039 — 创建默认 settings.json

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0038
- **修改/新增文件：** resources/defaults/settings.json；tests/unit/test_default_settings.cpp
- **执行步骤：**
  1. 按技术方案写完整默认值和 schema_version。
  2. 为数值范围写测试。
- **验收标准：**
  - JSON 可解析。
  - 默认关闭联网/遥测。
  - speed、字号、delay 在合法范围。
- **完成证据：** `[<commit>] T0039` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0040 — 创建默认 keymap.json

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0038
- **修改/新增文件：** resources/defaults/keymap.json；tests/unit/test_default_keymap.cpp
- **执行步骤：**
  1. 写动作 ID 与默认快捷键。
  2. 保证无冲突。
  3. 为无法解析的键提供测试。
- **验收标准：**
  - 所有动作 ID 唯一。
  - Qt 能解析每个组合。
  - 无重复绑定。
- **完成证据：** `[<commit>] T0040` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0041 — 实现命令行解析

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0028,T0031
- **修改/新增文件：** src/app/CommandLine.h；src/app/CommandLine.cpp；tests/unit/test_command_line.cpp
- **执行步骤：**
  1. 支持 `--version --enqueue --play-now --fullscreen <media...>`。
  2. 未知选项报错。
  3. 路径转 QUrl。
- **验收标准：**
  - 覆盖空参数、多文件、`--`、中文路径、未知参数。
  - 不调用 shell。
- **完成证据：** `[<commit>] T0041` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0042 — 实现单实例消息协议

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0033,T0041
- **修改/新增文件：** src/app/SingleInstance.h；src/app/SingleInstance.cpp；tests/integration/test_single_instance.cpp
- **执行步骤：**
  1. 首实例建当前用户 pipe。
  2. 后续实例发送 URL 和打开模式。
  3. 加入长度限制与 ack。
- **验收标准：**
  - 启动第二实例后首实例收到一次请求，第二实例退出 0。
  - 坏帧不崩溃。
- **完成证据：** `[<commit>] T0042` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0043 — 实现 Application 初始化骨架

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0029-T0042
- **修改/新增文件：** src/app/Application.h；src/app/Application.cpp；app/player/main.cpp
- **执行步骤：**
  1. 按技术方案顺序初始化路径、日志、单实例、设置占位、数据库占位、窗口占位、worker 占位。
  2. 每步 Result 化。
- **验收标准：**
  - 日志能看到顺序。
  - 任一注入失败有降级/退出策略。
  - 主窗口可显示。
- **完成证据：** `[<commit>] T0043` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0044 — 实现 worker 命令行与空进程

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0028-T0033
- **修改/新增文件：** src/worker/CaptionWorkerApplication.h；src/worker/CaptionWorkerApplication.cpp；app/caption-worker/main.cpp
- **执行步骤：**
  1. 解析 pipe、token、parent PID、log dir。
  2. 初始化 worker 日志。
  3. 无参数时明确失败。
- **验收标准：**
  - 合法参数启动。
  - 缺 token/pipe 返回非零。
  - token 不出现在日志。
- **完成证据：** `[<commit>] T0044` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0045 — 实现 Windows minidump/崩溃边界

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0029,T0030
- **修改/新增文件：** src/platform/windows/MiniDumpWriter.h；src/platform/windows/MiniDumpWriter.cpp；tests/integration/test_crash_dump.ps1
- **执行步骤：**
  1. 在未处理异常时写最小 dump 和 metadata。
  2. player/worker 分目录。
  3. 不在 signal handler 做复杂 Qt 操作。
- **验收标准：**
  - 测试子进程故意崩溃后生成 dump。
  - metadata 不含完整路径/字幕正文。
- **完成证据：** `[<commit>] T0045` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0046 — 实现统一优雅退出顺序

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0043,T0044
- **修改/新增文件：** src/app/Application.cpp；src/worker/CaptionWorkerApplication.cpp；tests/integration/test_clean_shutdown.cpp
- **执行步骤：**
  1. 停止新命令、保存状态、关闭 worker、关闭 DB/日志、销毁 mpv 占位。
  2. 为超时进程留强制终止路径。
- **验收标准：**
  - 正常退出无挂起。
  - 重复 close 安全。
  - 测试中无残留 worker 进程。
- **完成证据：** `[<commit>] T0046` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0047 — 添加格式化与静态检查入口

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0025
- **修改/新增文件：** .clang-format；.clang-tidy；tools/lint.ps1
- **执行步骤：**
  1. 固定格式。
  2. 启用基础 bugprone/performance 检查。
  3. 第三方排除。
- **验收标准：**
  - `tools/lint.ps1` 在当前代码通过。
  - 故意格式错误会失败。
- **完成证据：** `[<commit>] T0047` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0048 — 建立 implementation-status 自动校验

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0001,T0035
- **修改/新增文件：** tools/check-status.ps1；docs/implementation-status.md
- **执行步骤：**
  1. 状态表字段固定为 ID、状态、commit、测试、备注。
  2. 禁止已完成任务缺证据。
- **验收标准：**
  - 当前未完成状态可通过。
  - 将任务标完成但无测试/commit 时脚本失败。
- **完成证据：** `[<commit>] T0048` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0049 — 签署 P1 骨架阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0025-T0048
- **修改/新增文件：** docs/phase-gates/P1-foundation-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 运行四预设中的适用构建、测试、lint、依赖/状态校验。
  2. 附日志。
  3. 更新阶段。
- **验收标准：**
  - Debug/Release 构建通过。
  - player 与 worker 能启动退出。
  - CI 绿色。
  - 进入播放内核阶段。
- **完成证据：** `[<commit>] T0049` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 2：libmpv 播放内核与播放器基线功能

**阶段目标：** 完成 PRD 要求的基础播放器能力，使实时字幕尚未接入时也已是稳定可用的本地播放器。  
**阶段门：** T0050–T0085 完成；基础播放 E2E、轨道/字幕、倍速、截图、全屏、休眠恢复全部通过。

### T0050 — 实现 MpvHandle RAII

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0049,T0011
- **修改/新增文件：** src/playback/MpvHandle.h；src/playback/MpvHandle.cpp；tests/unit/test_mpv_handle.cpp
- **执行步骤：**
  1. 封装 create/initialize/destroy。
  2. 禁止复制。
  3. 把 mpv_error 转 AppError。
- **验收标准：**
  - 创建销毁 100 次无泄漏/崩溃。
  - 错误 option 能返回具体错误。
- **完成证据：** `[<commit>] T0050` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0051 — 集中设置 mpv 初始化选项

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0050
- **修改/新增文件：** src/playback/MpvOptions.h；src/playback/MpvOptions.cpp；tests/unit/test_mpv_options.cpp
- **执行步骤：**
  1. 设置 idle、keep-open、hwdec、pitch correction、sub-auto、截图、禁用内置 OSC/键盘。
  2. 区分启动前后 option。
- **验收标准：**
  - 每个必需 option 有测试。
  - 失败 option 被日志记录且能决定是否致命。
- **完成证据：** `[<commit>] T0051` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0052 — 实现 MpvEventBridge

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0050
- **修改/新增文件：** src/playback/MpvEventBridge.h；src/playback/MpvEventBridge.cpp；tests/integration/test_mpv_event_bridge.cpp
- **执行步骤：**
  1. wakeup callback 只排队。
  2. GUI 线程 drain。
  3. 分发 property/event/reply。
  4. 销毁时撤 callback。
- **验收标准：**
  - 线程断言通过。
  - 快速属性事件不丢 shutdown/file-loaded。
  - 销毁后无回调 UAF。
- **完成证据：** `[<commit>] T0052` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0053 — 实现 MpvRenderWidget

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0050,T0052
- **修改/新增文件：** src/playback/MpvRenderWidget.h；src/playback/MpvRenderWidget.cpp；tests/integration/test_mpv_render_widget.cpp
- **执行步骤：**
  1. 在 initializeGL 创建 render context。
  2. paintGL 渲染。
  3. update callback 排队。
  4. 正确销毁。
- **验收标准：**
  - fixture 可见。
  - 窗口 resize/最小化/恢复无黑屏。
  - 连续创建销毁通过。
- **完成证据：** `[<commit>] T0053` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0054 — 处理高 DPI framebuffer 尺寸

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0053
- **修改/新增文件：** src/playback/MpvRenderWidget.cpp；tests/e2e/dpi_render.md
- **执行步骤：**
  1. 使用 devicePixelRatioF 计算实际尺寸。
  2. 响应屏幕 DPI 变化。
- **验收标准：**
  - 100/150/200% 下不拉伸、不裁切。
  - 跨屏后比例正确。
- **完成证据：** `[<commit>] T0054` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0055 — 实现 render context 恢复

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0053
- **修改/新增文件：** src/playback/MpvRenderWidget.cpp；src/playback/MpvPlayer.cpp；tests/integration/test_render_recovery.cpp
- **执行步骤：**
  1. OpenGL context 丢失时暂停渲染、重建。
  2. 失败切错误页且音频可停止。
- **验收标准：**
  - 模拟 context 重建成功。
  - 失败路径无死循环/崩溃，有重试按钮。
- **完成证据：** `[<commit>] T0055` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0056 — 定义 IMediaPlayer 与领域快照

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0049
- **修改/新增文件：** src/playback/IMediaPlayer.h；src/playback/PlaybackState.h；src/playback/TrackInfo.h
- **执行步骤：**
  1. 定义开播、控制、seek、speed、track、snapshot 和 signals。
  2. 不暴露 mpv 类型给 UI。
- **验收标准：**
  - 编译期 mock 可替代 MpvPlayer。
  - UI 测试不需要链接 libmpv。
- **完成证据：** `[<commit>] T0056` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0057 — 实现 MpvPlayer 基础生命周期

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0050-T0056
- **修改/新增文件：** src/playback/MpvPlayer.h；src/playback/MpvPlayer.cpp
- **执行步骤：**
  1. 组合 handle/event/render。
  2. 观察必需属性。
  3. 维护 Idle/Loading/Playing/Paused/Error。
- **验收标准：**
  - 状态转换测试覆盖打开成功、失败、结束、关闭。
  - 无互相矛盾布尔值。
- **完成证据：** `[<commit>] T0057` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0058 — 实现异步打开媒体

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0057
- **修改/新增文件：** src/playback/MpvPlayer.cpp；tests/integration/test_open_media.cpp
- **执行步骤：**
  1. 校验 QUrl。
  2. 发送 async loadfile。
  3. 处理 start-file/file-loaded/end-file。
  4. 建立 media correlation ID。
- **验收标准：**
  - 本地、中文、UNC fixture 可打开。
  - 不存在文件进入可恢复错误。
  - UI 不阻塞。
- **完成证据：** `[<commit>] T0058` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0059 — 实现播放/暂停/停止

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0057
- **修改/新增文件：** src/playback/MpvPlayer.cpp；tests/integration/test_play_pause.cpp
- **执行步骤：**
  1. 映射 pause 属性。
  2. 停止关闭当前媒体并清理状态。
  3. 重复命令幂等。
- **验收标准：**
  - 状态和实际属性一致。
  - 无媒体调用不崩溃。
  - 快捷连续点击通过。
- **完成证据：** `[<commit>] T0059` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0060 — 实现绝对/相对 seek

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；tests/integration/test_seek.cpp
- **执行步骤：**
  1. 范围夹取。
  2. 使用 absolute+exact 和 relative。
  3. 监听 seeking。
  4. 命令有 reply 追踪。
- **验收标准：**
  - 0、结尾、中间、负相对 seek 正确。
  - 失败时 slider 回真实位置。
- **完成证据：** `[<commit>] T0060` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0061 — 实现进度条拖动节流

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0060
- **修改/新增文件：** src/ui/PlayerControls.cpp；src/playback/MpvPlayer.cpp；tests/unit/test_seek_throttle.cpp
- **执行步骤：**
  1. 拖动中最多固定频率预览 seek。
  2. 释放发一次精确 seek。
  3. 媒体切换取消旧请求。
- **验收标准：**
  - 拖动 5 秒命令数量受限。
  - 最终位置与释放位置一致。
  - 旧媒体 reply 被忽略。
- **完成证据：** `[<commit>] T0061` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0062 — 实现音量和静音

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；tests/integration/test_volume.cpp
- **执行步骤：**
  1. 设置/观察 volume、mute。
  2. 保存用户值。
  3. 静音不覆盖原音量。
- **验收标准：**
  - 边界值夹取。
  - 重启恢复。
  - 静音切换后音量不跳变。
- **完成证据：** `[<commit>] T0062` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0063 — 实现 0.5–4.0x 倍速和保音调

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/PlayerControls.cpp；tests/integration/test_speed.cpp
- **执行步骤：**
  1. 0.1 步进。
  2. 设置 speed。
  3. 确保 audio-pitch-correction。
  4. 显示一位小数。
  5. 重置 1.0x。
- **验收标准：**
  - 0.5/1/2/4x 播放。
  - 超界拒绝。
  - 音调人工样例通过。
  - 字幕接口收到 speed signal。
- **完成证据：** `[<commit>] T0063` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0064 — 实现前后帧

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0059
- **修改/新增文件：** src/playback/MpvPlayer.cpp；tests/integration/test_frame_step.cpp
- **执行步骤：**
  1. 暂停后调用 frame-step/frame-back-step。
  2. 播放中先暂停。
  3. 无视频轨提示。
- **验收标准：**
  - 前进单帧稳定。
  - 后退按 mpv 能力执行且错误可理解。
  - 音频文件不崩溃。
- **完成证据：** `[<commit>] T0064` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0065 — 实现 AB 循环

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/PlayerControls.cpp；tests/integration/test_ab_loop.cpp
- **执行步骤：**
  1. 第一次设 A、第二次设 B、第三次清除。
  2. B 必须大于 A。
  3. 媒体切换清除。
- **验收标准：**
  - 状态提示正确。
  - 循环播放发生。
  - 非法顺序自动纠正/拒绝。
- **完成证据：** `[<commit>] T0065` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0066 — 解析 track-list

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0057
- **修改/新增文件：** src/playback/TrackInfo.h；src/playback/MpvPlayer.cpp；tests/unit/test_track_parser.cpp
- **执行步骤：**
  1. 解析 type/id/ff-index/lang/title/codec/channels/selected/external。
  2. 缺字段安全处理。
- **验收标准：**
  - 用真实/缺字段 JSON 节点测试。
  - ff-index 保留。
  - 排序稳定。
- **完成证据：** `[<commit>] T0066` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0067 — 实现音轨选择

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0066
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/TrackMenuBuilder.cpp；tests/integration/test_audio_track.cpp
- **执行步骤：**
  1. 按 mpv track id 设置 aid。
  2. 观察实际选择。
  3. 发送 ff-index 与元数据 signal。
- **验收标准：**
  - 多音轨 fixture 切换声音和 ff-index。
  - 无效 id 返回错误。
  - worker 接口可消费。
- **完成证据：** `[<commit>] T0067` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0068 — 实现内嵌字幕轨选择

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0066
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/TrackMenuBuilder.cpp；tests/integration/test_subtitle_track.cpp
- **执行步骤：**
  1. 设置 sid/no。
  2. 菜单展示语言/标题/格式。
  3. 选择结果以 mpv 属性为准。
- **验收标准：**
  - 多字幕 fixture 正确切换/关闭。
  - 媒体变化菜单刷新。
- **完成证据：** `[<commit>] T0068` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0069 — 实现外挂字幕加载/移除/重载

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0068
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/MainWindow.cpp；tests/integration/test_external_subtitle.cpp
- **执行步骤：**
  1. 支持 SRT/ASS/SSA/VTT。
  2. 调用 sub-add select。
  3. 记录 external track id。
  4. 提供 reload/remove。
- **验收标准：**
  - 四格式样例可加载。
  - 不存在/坏编码有提示。
  - 移除后菜单同步。
- **完成证据：** `[<commit>] T0069` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0070 — 实现字幕字号、位置、编码和延迟

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0068,T0069
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/settings/Settings.h；tests/integration/test_subtitle_options.cpp
- **执行步骤：**
  1. 映射 sub-font-size/sub-pos/sub-delay/sub-codepage。
  2. 输入范围校验。
  3. 媒体间保留。
- **验收标准：**
  - 延迟正负值生效。
  - 字号位置可见。
  - 坏编码设置可恢复默认。
- **完成证据：** `[<commit>] T0070` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0071 — 实现亮度/对比度/饱和度

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/settings/VideoSettingsPage.cpp；tests/integration/test_video_eq.cpp
- **执行步骤：**
  1. 映射属性。
  2. 预设和复位。
  3. 不在 Qt 像素层重复处理。
- **验收标准：**
  - 属性读回一致。
  - 复位恢复 0。
  - 无视频时控件禁用。
- **完成证据：** `[<commit>] T0071` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0072 — 实现旋转和宽高比

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/settings/VideoSettingsPage.cpp；tests/integration/test_video_transform.cpp
- **执行步骤：**
  1. 旋转 0/90/180/270。
  2. 宽高比 auto/16:9/4:3/original。
  3. 媒体切换规则明确。
- **验收标准：**
  - 画面变化可验证。
  - 非法值不发送。
  - 复位正确。
- **完成证据：** `[<commit>] T0072` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0073 — 实现硬件解码档位

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0051,T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/settings/VideoSettingsPage.cpp；tests/integration/test_hwdec.cpp
- **执行步骤：**
  1. 支持 auto-safe/auto/no。
  2. 改变后按 mpv 要求重载。
  3. 失败回退 no 并提示。
- **验收标准：**
  - 诊断显示实际 hwdec。
  - 不支持 GPU 上可播放。
  - 回退不循环。
- **完成证据：** `[<commit>] T0073` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0074 — 实现 HDR/tone mapping 预设入口

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/ui/settings/VideoSettingsPage.cpp；docs/user/hdr.md
- **执行步骤：**
  1. 提供 Auto/SDR 映射/原样等少量预设。
  2. 集中映射 mpv 属性。
  3. 记录实际 video-params。
- **验收标准：**
  - HDR fixture 在 SDR 显示器可见。
  - 预设切换不崩溃。
  - 文档不夸大颜色准确性。
- **完成证据：** `[<commit>] T0074` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0075 — 实现全屏与控制栏自动隐藏

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0053,T0058
- **修改/新增文件：** src/ui/MainWindow.cpp；src/ui/PlayerControls.cpp；tests/e2e/fullscreen.md
- **执行步骤：**
  1. F11/双击切换。
  2. 全屏无边框。
  3. 鼠标静止隐藏控制栏。
  4. 移动/键盘再显示。
- **验收标准：**
  - 多屏进出位置正确。
  - Esc 退出。
  - 字幕位置随控制栏变化稳定。
- **完成证据：** `[<commit>] T0075` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0076 — 实现纯画面截图

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0058
- **修改/新增文件：** src/playback/ScreenshotService.h；src/playback/ScreenshotService.cpp；tests/integration/test_screenshot.cpp
- **执行步骤：**
  1. 生成安全文件名。
  2. 让 mpv 先写入目标目录中的唯一临时文件。
  3. 校验 PNG 后再原子重命名，并处理同名冲突。
  4. 通知结果。
- **验收标准：**
  - PNG 可打开、尺寸正确。
  - 中文文件名。
  - 只读目录回退并提示。
- **完成证据：** `[<commit>] T0076` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0077 — 实现带实时字幕截图后备合成接口

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0016,T0076
- **修改/新增文件：** src/playback/ScreenshotService.cpp；src/captions/ICaptionPainter.h；tests/integration/test_caption_screenshot.cpp
- **执行步骤：**
  1. 先实现 P0 选定的 mpv flag。
  2. 保留统一 caption painter fallback。
  3. 截图服务不依赖具体 UI widget。
- **验收标准：**
  - golden 图包含实时字幕。
  - 不包含控制栏。
  - 高 DPI 不模糊/偏移。
- **完成证据：** `[<commit>] T0077` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0078 — 实现拖放打开

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/ui/MainWindow.cpp；tests/e2e/drag_drop.md
- **执行步骤：**
  1. 接受文件/多个文件/目录。
  2. 文件首个播放其余入列表。
  3. 目录交后台枚举。
  4. 拒绝 URL 文本执行。
- **验收标准：**
  - 中文多文件拖放正确。
  - 不支持扩展有提示。
  - UI 不冻结。
- **完成证据：** `[<commit>] T0078` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0079 — 实现文件打开对话框

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0058
- **修改/新增文件：** src/ui/MainWindow.cpp；src/core/SupportedFormats.h；tests/unit/test_supported_formats.cpp
- **执行步骤：**
  1. 集中维护扩展名和过滤器。
  2. 记住上次目录。
  3. 支持多选。
- **验收标准：**
  - 过滤器含 PRD 格式。
  - 扩展名大小写不敏感。
  - 取消无副作用。
- **完成证据：** `[<commit>] T0079` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0080 — 实现播放错误页与恢复动作

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0057
- **修改/新增文件：** src/ui/PlaybackErrorWidget.h；src/ui/PlaybackErrorWidget.cpp；src/ui/MainWindow.cpp
- **执行步骤：**
  1. 根据 AppError 显示重试、移除、复制详情、打开日志。
  2. 不弹无限 modal。
- **验收标准：**
  - 坏文件显示可行动提示。
  - 重试成功可回视频。
  - 技术详情已脱敏。
- **完成证据：** `[<commit>] T0080` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0081 — 实现播放结束与连续播放

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0057
- **修改/新增文件：** src/playback/MpvPlayer.cpp；src/playlist/PlaylistModel.cpp；tests/integration/test_auto_next.cpp
- **执行步骤：**
  1. 正常 EOF 播下一项。
  2. 用户 stop 不自动下一项。
  3. 列表末尾按设置停止/循环。
- **验收标准：**
  - 三项列表顺序正确。
  - 错误项跳过一次并提示。
  - 不会无限循环坏文件。
- **完成证据：** `[<commit>] T0081` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0082 — 实现播放期间系统休眠抑制

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0059
- **修改/新增文件：** src/platform/windows/PowerManagement.h；src/platform/windows/PowerManagement.cpp；tests/unit/test_power_state.cpp
- **执行步骤：**
  1. 仅播放视频时请求系统保持显示/系统。
  2. 暂停或退出释放。
  3. RAII 防泄漏。
- **验收标准：**
  - 状态切换调用成对。
  - 崩溃恢复不永久改变系统。
  - 单元测试 mock 通过。
- **完成证据：** `[<commit>] T0082` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0083 — 处理系统休眠/唤醒

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0057,T0055
- **修改/新增文件：** src/platform/windows/WindowsIntegration.cpp；src/playback/MpvPlayer.cpp；tests/e2e/sleep_resume.md
- **执行步骤：**
  1. 监听 power event。
  2. 休眠前暂停状态/flush。
  3. 唤醒后重建必要资源并保留位置。
- **验收标准：**
  - 睡眠唤醒后可继续。
  - worker 后续可重连。
  - 无重复音频/黑屏死锁。
- **完成证据：** `[<commit>] T0083` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0084 — 补齐播放内核自动化测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0050-T0083
- **修改/新增文件：** tests/unit/playback/；tests/integration/playback/；tools/run-playback-tests.ps1
- **执行步骤：**
  1. 覆盖所有属性映射、状态、轨道、seek、截图、错误。
  2. 固定 fixture 和超时。
- **验收标准：**
  - 测试可单命令运行。
  - 连续运行 3 次无随机失败。
  - CI 上传失败日志/截图。
- **完成证据：** `[<commit>] T0084` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0085 — 签署基础播放器阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0050-T0084
- **修改/新增文件：** docs/phase-gates/P2-playback-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 按格式/路径/DPI/睡眠矩阵执行。
  2. 记录人工音调和 HDR 结果。
  3. 清零阻塞缺陷。
- **验收标准：**
  - 播放器在无字幕 worker 情况下完整可用。
  - 所有 P0/P1 验收有证据。
  - 进入数据/worker 阶段。
- **完成证据：** `[<commit>] T0085` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 3：设置、播放列表、历史与持久化基础

**阶段目标：** 把播放器状态、用户偏好和历史从 UI 中分离，并建立线程安全 SQLite 迁移与存储层。  
**阶段门：** T0086–T0104 完成；设置损坏可恢复、数据库可迁移、历史和快捷键重启后正确保存。

### T0086 — 实现后台文件夹枚举器

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0085
- **修改/新增文件：** src/playlist/FolderEnumerator.h；src/playlist/FolderEnumerator.cpp；tests/unit/test_folder_enumerator.cpp
- **执行步骤：**
  1. 在工作线程枚举支持格式。
  2. 支持取消、权限错误和上限。
  3. 不递归默认，递归为显式选项。
- **验收标准：**
  - 大目录期间 GUI 响应。
  - 取消及时。
  - 不支持文件被过滤。
  - 错误目录有结果对象。
- **完成证据：** `[<commit>] T0086` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0087 — 实现自然排序与稳定去重

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0086
- **修改/新增文件：** src/playlist/NaturalSort.h；src/playlist/NaturalSort.cpp；tests/unit/test_natural_sort.cpp
- **执行步骤：**
  1. 按数字、大小写、中文 locale 做稳定排序。
  2. 规范化 URL 去重但不合并不同文件。
- **验收标准：**
  - `1,2,10` 顺序正确。
  - 同路径不同表示只保留一次。
  - 测试不依赖机器 locale。
- **完成证据：** `[<commit>] T0087` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0088 — 实现 PlaylistModel

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0086,T0087
- **修改/新增文件：** src/playlist/PlaylistModel.h；src/playlist/PlaylistModel.cpp；tests/unit/test_playlist_model.cpp
- **执行步骤：**
  1. 支持 add/remove/move/clear/current/next/previous。
  2. 线程结果回 GUI 后更新。
  3. 不在 model 里播放。
- **验收标准：**
  - 模型通知范围正确。
  - 当前项删除后选择规则明确。
  - 重复项策略通过测试。
- **完成证据：** `[<commit>] T0088` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0089 — 实现命名播放列表持久化

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0088
- **修改/新增文件：** migrations/001_initial.sql；src/storage/PlaylistRepository.h；src/storage/PlaylistRepository.cpp；tests/integration/test_playlist_repository.cpp
- **执行步骤：**
  1. 保存 playlist/item 顺序。
  2. 事务替换。
  3. 读取缺失文件仍保留 URL。
- **验收标准：**
  - 重启后顺序一致。
  - 事务中断不出现半列表。
  - 删除 playlist 级联 items。
- **完成证据：** `[<commit>] T0089` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0090 — 实现 Database RAII 与 WAL 配置

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0085
- **修改/新增文件：** src/storage/Database.h；src/storage/Database.cpp；tests/integration/test_database.cpp
- **执行步骤：**
  1. 只在 storage 线程创建连接。
  2. 启用 foreign_keys/WAL/busy_timeout。
  3. 检查错误。
- **验收标准：**
  - 错误线程访问触发断言/错误。
  - WAL 生效。
  - 只读路径返回可降级错误。
- **完成证据：** `[<commit>] T0090` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0091 — 实现 MigrationRunner

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0090
- **修改/新增文件：** src/storage/MigrationRunner.h；src/storage/MigrationRunner.cpp；migrations/001_initial.sql；tests/integration/test_migrations.cpp
- **执行步骤：**
  1. 按版本顺序事务执行。
  2. 已发布迁移不可重写。
  3. 失败回滚并备份 DB。
- **验收标准：**
  - 空库到最新成功。
  - 模拟第二步失败后版本/表未半迁移。
  - 重复运行幂等。
- **完成证据：** `[<commit>] T0091` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0092 — 实现 StorageWorker 线程队列

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0090
- **修改/新增文件：** src/storage/StorageWorker.h；src/storage/StorageWorker.cpp；tests/integration/test_storage_worker.cpp
- **执行步骤：**
  1. 用专用 QThread 接受异步任务。
  2. 返回 future/signal。
  3. 退出 drain 或取消。
  4. 禁止 GUI 直接用连接。
- **验收标准：**
  - 大量写入时 UI 测试事件循环仍响应。
  - 关闭无丢关键事务/死锁。
- **完成证据：** `[<commit>] T0092` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0093 — 实现 HistoryRepository

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0091,T0092
- **修改/新增文件：** src/storage/HistoryRepository.h；src/storage/HistoryRepository.cpp；tests/integration/test_history_repository.cpp
- **执行步骤：**
  1. upsert media identity、last_position、last_played、track。
  2. 最近 200 条查询。
  3. 清空/删除。
- **验收标准：**
  - 同媒体更新不重复。
  - 排序正确。
  - 清空事务成功。
  - 不存在路径仍能显示。
- **完成证据：** `[<commit>] T0093` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0094 — 实现周期保存播放位置

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0093,T0058
- **修改/新增文件：** src/app/Application.cpp；src/storage/HistoryRepository.cpp；tests/integration/test_position_checkpoint.cpp
- **执行步骤：**
  1. 播放时每 10 秒异步保存。
  2. 暂停、换媒体、退出强制保存。
  3. 过密写入合并。
- **验收标准：**
  - 模拟播放 31 秒写入次数受控。
  - 退出值正确。
  - DB 慢不阻塞 GUI。
- **完成证据：** `[<commit>] T0094` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0095 — 实现继续播放规则

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0093,T0058
- **修改/新增文件：** src/ui/ResumePrompt.cpp；src/app/Application.cpp；tests/unit/test_resume_rules.cpp
- **执行步骤：**
  1. 少于 5 分钟/距结尾 30 秒内不提示。
  2. 其余提示继续/从头。
  3. 记忆本次选择。
- **验收标准：**
  - 边界用例通过。
  - URL 变化不错误复用。
  - 提示不阻塞自动打开队列。
- **完成证据：** `[<commit>] T0095` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0096 — 实现 Settings 领域结构与验证

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0039
- **修改/新增文件：** src/settings/Settings.h；src/settings/SettingsValidation.cpp；tests/unit/test_settings_validation.cpp
- **执行步骤：**
  1. 为每字段定义类型、默认、范围。
  2. 未知字段保留/忽略策略固定。
  3. 非法值裁剪并记录。
- **验收标准：**
  - 随机坏值不会崩溃。
  - 网络/遥测默认 false。
  - 验证输出指出被修复字段。
- **完成证据：** `[<commit>] T0096` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0097 — 实现 SettingsService 原子读写

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0096,T0029
- **修改/新增文件：** src/settings/SettingsService.h；src/settings/SettingsService.cpp；tests/integration/test_settings_service.cpp
- **执行步骤：**
  1. 从资源默认+用户文件合并。
  2. QSaveFile 原子保存。
  3. 损坏文件备份。
  4. 失败保留内存有效值。
- **验收标准：**
  - 截断 JSON 后恢复默认并生成备份。
  - 只读目录有提示。
  - 保存后重启一致。
- **完成证据：** `[<commit>] T0097` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0098 — 实现设置 schema 迁移

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0097
- **修改/新增文件：** src/settings/SettingsMigration.h；src/settings/SettingsMigration.cpp；tests/unit/test_settings_migration.cpp
- **执行步骤：**
  1. 按 schema_version 逐级迁移。
  2. 禁止跳步。
  3. 保留用户可识别字段。
  4. 备份旧版本。
- **验收标准：**
  - v0 fixture 升 v1 golden 一致。
  - 未知未来版本拒绝降写。
  - 迁移幂等。
- **完成证据：** `[<commit>] T0098` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0099 — 实现 KeymapService

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0040,T0097
- **修改/新增文件：** src/settings/KeymapService.h；src/settings/KeymapService.cpp；tests/unit/test_keymap_service.cpp
- **执行步骤：**
  1. 加载默认与用户覆盖。
  2. 解析 QKeySequence。
  3. 提供 action→binding 和重置。
- **验收标准：**
  - 坏 JSON/坏键恢复默认。
  - 重启保存。
  - 所有默认动作可解析。
- **完成证据：** `[<commit>] T0099` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0100 — 实现快捷键冲突检测

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0099
- **修改/新增文件：** src/settings/KeymapService.cpp；tests/unit/test_keymap_conflicts.cpp
- **执行步骤：**
  1. 规范化键序列。
  2. 检测同 context 冲突。
  3. 拒绝系统保留/空组合。
  4. 返回冲突动作列表。
- **验收标准：**
  - 冲突保存失败且原配置不变。
  - 大小写/等价表示能识别。
- **完成证据：** `[<commit>] T0100` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0101 — 建立 ActionRegistry

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0056,T0099
- **修改/新增文件：** src/app/ActionRegistry.h；src/app/ActionRegistry.cpp；tests/unit/test_action_registry.cpp
- **执行步骤：**
  1. 集中注册 action ID、文案、默认键、enabled 条件和回调。
  2. 菜单/按钮/快捷键复用 QAction。
- **验收标准：**
  - 不存在 action 获取失败。
  - 状态随媒体/字幕变化。
  - 无重复 ID。
- **完成证据：** `[<commit>] T0101` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0102 — 绑定默认与用户快捷键

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0101
- **修改/新增文件：** src/app/Application.cpp；src/ui/MainWindow.cpp；tests/e2e/shortcuts.md
- **执行步骤：**
  1. 从 KeymapService 更新 QAction。
  2. 更改后即时生效。
  3. 全屏/输入框 context 正确。
- **验收标准：**
  - 播放、seek、speed、字幕、导出、全屏键可用。
  - 文本输入时不误触全局字符键。
- **完成证据：** `[<commit>] T0102` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0103 — 补齐设置/存储/列表测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0086-T0102
- **修改/新增文件：** tests/unit/settings/；tests/integration/storage/；tools/run-storage-tests.ps1
- **执行步骤：**
  1. 增加并发关闭、DB busy、损坏文件、迁移、自然排序、快捷键冲突测试。
- **验收标准：**
  - 连续运行无 flaky。
  - 测试结束无残留 DB/WAL 文件句柄。
- **完成证据：** `[<commit>] T0103` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0104 — 签署持久化阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0086-T0103
- **修改/新增文件：** docs/phase-gates/P3-storage-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 在干净配置、旧配置、损坏配置、只读目录和 DB busy 场景验收。
  2. 更新状态。
- **验收标准：**
  - 播放器重启后位置/设置/列表正确。
  - 故障均可降级。
  - 进入 worker 实现。
- **完成证据：** `[<commit>] T0104` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 4：IPC、worker 生命周期与 FFmpeg 音频解码

**阶段目标：** 完成不含真实 ASR 的字幕基础设施：主进程可稳定控制 worker，worker 可按当前音轨与媒体时间解码、seek、预读并输出模拟事件。  
**阶段门：** T0105–T0135 完成；反复 seek、切音轨、杀 worker 后主播放器继续工作且无旧代事件。

### T0105 — 定义强类型 IPC 协议常量

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0104,T0005
- **修改/新增文件：** src/ipc/Protocol.h；tests/unit/test_protocol_constants.cpp
- **执行步骤：**
  1. 定义 protocol v1、message type、字段名、最大帧、错误码。
  2. 两进程只引用此文件。
- **验收标准：**
  - 编译期/测试确保 type 唯一。
  - 协议版本写入 `--version` 和握手。
- **完成证据：** `[<commit>] T0105` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0106 — 实现长度前缀 FrameCodec

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0105
- **修改/新增文件：** src/ipc/FrameCodec.h；src/ipc/FrameCodec.cpp；tests/unit/test_frame_codec.cpp
- **执行步骤：**
  1. 支持拆包、粘包、部分 header、0 长、超大帧。
  2. uint32 little-endian。
  3. 有输入缓冲上限。
- **验收标准：**
  - 随机分片 round-trip。
  - 超 1 MiB 立即错误。
  - 不无限增长内存。
- **完成证据：** `[<commit>] T0106` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0107 — 实现 JSON 消息编解码与 schema 校验

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0105,T0106
- **修改/新增文件：** src/ipc/JsonMessageCodec.h；src/ipc/JsonMessageCodec.cpp；tests/unit/test_json_message_codec.cpp
- **执行步骤：**
  1. 校验 v/type/id/generation/payload。
  2. 时间必须整数毫秒。
  3. 未知字段策略固定。
  4. 错误脱敏。
- **验收标准：**
  - 坏 UTF-8/JSON/字段类型返回错误。
  - 合法中文文本 round-trip。
- **完成证据：** `[<commit>] T0107` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0108 — 实现主进程 QLocalServer 接受连接

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0106,T0107
- **修改/新增文件：** src/ipc/WorkerClient.h；src/ipc/WorkerClient.cpp；tests/integration/test_local_ipc.cpp
- **执行步骤：**
  1. 创建随机当前用户 pipe。
  2. 只接受一个 worker。
  3. 限制权限。
  4. 读取帧并分发。
- **验收标准：**
  - 第二连接被拒。
  - 其他用户不可访问的策略有说明/测试。
  - 断开有 signal。
- **完成证据：** `[<commit>] T0108` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0109 — 实现 WorkerSupervisor 安全启动

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0108,T0044
- **修改/新增文件：** src/ipc/WorkerSupervisor.h；src/ipc/WorkerSupervisor.cpp；tests/integration/test_worker_launch.cpp
- **执行步骤：**
  1. 用 QProcess 参数启动。
  2. 生成 256-bit token。
  3. 传 parent PID。
  4. 捕获 stdout/stderr。
  5. 不经 shell。
- **验收标准：**
  - 带命令字符路径仍安全。
  - token 不在日志。
  - 启动失败返回可行动错误。
- **完成证据：** `[<commit>] T0109` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0110 — 实现握手与协议版本检查

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0108,T0109
- **修改/新增文件：** src/ipc/WorkerClient.cpp；src/worker/CaptionWorkerApplication.cpp；tests/integration/test_worker_handshake.cpp
- **执行步骤：**
  1. worker 连接后发送 token/protocol/build。
  2. 主进程验证后发 ack。
  3. 超时/错 token 断开。
- **验收标准：**
  - 正确握手 ready。
  - 错 token/版本被拒。
  - 错误不会进入重启风暴。
- **完成证据：** `[<commit>] T0110` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0111 — 实现 heartbeat 与失联判定

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0110
- **修改/新增文件：** src/ipc/WorkerClient.cpp；src/worker/CaptionWorkerApplication.cpp；tests/integration/test_worker_heartbeat.cpp
- **执行步骤：**
  1. worker 每 2 秒发 heartbeat。
  2. 主进程连续 3 次未收判失联。
  3. 记录 last_seen。
- **验收标准：**
  - 暂停事件循环测试阈值正确。
  - 正常繁忙推理仍能 heartbeat（独立控制线程）。
- **完成证据：** `[<commit>] T0111` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0112 — 实现 worker 自动重启与退避

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0111
- **修改/新增文件：** src/ipc/WorkerSupervisor.cpp；tests/integration/test_worker_restart.cpp
- **执行步骤：**
  1. 单会话最多 3 次。
  2. 保存重启次数。
  3. 崩溃后重新握手。
  4. 超过次数停止并提示。
- **验收标准：**
  - 杀进程 1 次自动恢复。
  - 连续 4 次停止。
  - 视频测试仍播放。
- **完成证据：** `[<commit>] T0112` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0113 — 实现 worker 优雅关闭与父进程监控

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0110
- **修改/新增文件：** src/ipc/WorkerSupervisor.cpp；src/worker/CaptionWorkerApplication.cpp；tests/integration/test_worker_shutdown.cpp
- **执行步骤：**
  1. shutdown ack 后退出。
  2. 超时 terminate/kill。
  3. worker 定期检查 parent PID。
  4. pipe 断开也退出。
- **验收标准：**
  - 主进程正常退出无孤儿。
  - 强杀父进程后 worker 自退。
  - 重复 shutdown 幂等。
- **完成证据：** `[<commit>] T0113` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0114 — 实现 WorkerSession 状态机

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0110
- **修改/新增文件：** src/worker/WorkerSession.h；src/worker/WorkerSession.cpp；tests/unit/test_worker_session.cpp
- **执行步骤：**
  1. 状态 Idle/ModelLoading/Ready/MediaOpen/Captioning/Seeking/Error/ShuttingDown。
  2. 定义命令许可。
- **验收标准：**
  - 非法状态命令返回错误不崩溃。
  - 所有状态转换有测试。
- **完成证据：** `[<commit>] T0114` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0115 — 实现线程安全 BoundedQueue

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0033
- **修改/新增文件：** src/worker/BoundedQueue.h；tests/unit/test_bounded_queue.cpp
- **执行步骤：**
  1. 支持 push 阻塞/取消、try_pop、close、clear generation。
  2. 无复制大对象要求。
- **验收标准：**
  - 多生产/消费压力测试。
  - close 唤醒等待者。
  - 容量永不超限。
- **完成证据：** `[<commit>] T0115` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0116 — 定义 PcmChunk 与 AudioStreamInfo

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0115
- **修改/新增文件：** src/worker/PcmChunk.h；src/worker/AudioStreamInfo.h；tests/unit/test_pcm_types.cpp
- **执行步骤：**
  1. 字段含 generation/start_ms/sample_rate/samples/discontinuity/eof。
  2. stream info 含 ff-index/timebase/codec/lang/channels。
- **验收标准：**
  - 构造校验拒绝负采样率/非单调时间。
  - 类型可移动不可意外大复制。
- **完成证据：** `[<commit>] T0116` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0117 — 实现 FFmpeg RAII 包装

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0010,T0031
- **修改/新增文件：** src/worker/FfmpegHandles.h；src/worker/FfmpegError.cpp；tests/unit/test_ffmpeg_raii.cpp
- **执行步骤：**
  1. 封装 format/codec/frame/packet/swr。
  2. 所有 free 路径唯一。
  3. 错误转 AppError。
- **验收标准：**
  - 故意在各阶段失败无泄漏。
  - ASan 预设通过。
- **完成证据：** `[<commit>] T0117` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0118 — 实现 AudioDecoder 打开媒体

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0116,T0117
- **修改/新增文件：** src/worker/AudioDecoder.h；src/worker/AudioDecoder.cpp；tests/integration/test_audio_open.cpp
- **执行步骤：**
  1. avformat_open_input/find_stream_info。
  2. 支持 Unicode/UNC。
  3. 提取 duration/start。
  4. 不记录完整路径。
- **验收标准：**
  - 容器矩阵可打开。
  - 不存在/权限/损坏文件错误分类正确。
- **完成证据：** `[<commit>] T0118` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0119 — 实现按 audio_ff_index 校验音轨

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0067,T0118
- **修改/新增文件：** src/worker/AudioDecoder.cpp；tests/integration/test_audio_stream_selection.cpp
- **执行步骤：**
  1. 按 index 查 streams。
  2. 必须是 audio。
  3. 返回 codec/lang/channels。
  4. 失败不自动选第一轨。
- **验收标准：**
  - 多音轨 fixture 每轨 PCM 不同。
  - 视频 stream index 被拒。
  - 错误码固定。
- **完成证据：** `[<commit>] T0119` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0120 — 实现 AudioResampler

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0117,T0119
- **修改/新增文件：** src/worker/AudioResampler.h；src/worker/AudioResampler.cpp；tests/unit/test_resampler.cpp
- **执行步骤：**
  1. 任意常见采样格式/布局转 16k mono float32 [-1,1]。
  2. 正确 drain。
  3. 处理 5.1 downmix。
- **验收标准：**
  - 16/44.1/48/96k、mono/stereo/5.1 golden 长度在误差内。
  - 无 NaN/越界。
- **完成证据：** `[<commit>] T0120` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0121 — 实现媒体 sample clock

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0119,T0120
- **修改/新增文件：** src/worker/MediaSampleClock.h；src/worker/MediaSampleClock.cpp；tests/unit/test_sample_clock.cpp
- **执行步骤：**
  1. 优先 best_effort_timestamp。
  2. 缺 PTS 时按累计样本。
  3. 处理 start_time、discontinuity。
  4. 强制单调。
- **验收标准：**
  - 带缺失/倒退 PTS fixture 仍单调。
  - 一小时误差在定义范围内。
- **完成证据：** `[<commit>] T0121` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0122 — 实现 100ms PCM chunker

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0120,T0121
- **修改/新增文件：** src/worker/AudioDecoder.cpp；tests/unit/test_pcm_chunker.cpp
- **执行步骤：**
  1. 跨 frame 聚合为固定 100 ms。
  2. 尾块允许短。
  3. 每块 start_ms 精确。
  4. eof 单独标记。
- **验收标准：**
  - 不同 frame size 下总样本守恒。
  - chunk 时间连续。
  - 尾部不丢。
- **完成证据：** `[<commit>] T0122` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0123 — 实现 AudioDecoder 主循环

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0115,T0118-T0122
- **修改/新增文件：** src/worker/AudioDecoder.cpp；tests/integration/test_decode_loop.cpp
- **执行步骤：**
  1. read packet→send/receive frame→resample→chunk→bounded queue。
  2. 只处理选中流。
  3. 响应 cancel。
- **验收标准：**
  - 完整解码样本数正确。
  - 取消快速。
  - 坏 packet 可跳过/报错策略明确。
- **完成证据：** `[<commit>] T0123` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0124 — 实现 FFmpeg seek/flush/pre-roll

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0018,T0123
- **修改/新增文件：** src/worker/AudioDecoder.cpp；tests/integration/test_decoder_seek.cpp
- **执行步骤：**
  1. avformat_seek_file(target-800ms)。
  2. flush codec/swr/clock。
  3. 丢弃 target 前。
  4. 首块 discontinuity。
- **验收标准：**
  - 10 个随机 seek 后首块时间正确。
  - 旧 queue 清空。
  - 不可 seek 媒体返回固定错误。
- **完成证据：** `[<commit>] T0124` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0125 — 实现 generation 取消贯穿 decoder

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0033,T0124
- **修改/新增文件：** src/worker/WorkerSession.cpp；src/worker/AudioDecoder.cpp；tests/integration/test_decoder_generation.cpp
- **执行步骤：**
  1. open/seek/track/profile 增 generation。
  2. 取消旧 loop。
  3. 所有 chunk 带 generation。
  4. 消费者过滤。
- **验收标准：**
  - 快速发 20 次 seek 后只出现最后 generation。
  - 无线程泄漏。
- **完成证据：** `[<commit>] T0125` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0126 — 实现 EOF、媒体消失和解码错误处理

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0123
- **修改/新增文件：** src/worker/AudioDecoder.cpp；tests/integration/test_decode_errors.cpp
- **执行步骤：**
  1. 正常 EOF 发一次。
  2. 中途文件删除/网络断开返回 retryable 分类。
  3. 避免无限 retry。
- **验收标准：**
  - EOF 无重复。
  - 删除 fixture 后 worker 存活。
  - 错误事件有媒体位置。
- **完成证据：** `[<commit>] T0126` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0127 — 实现 DecodeScheduler 提前量算法

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0123
- **修改/新增文件：** src/worker/DecodeScheduler.h；src/worker/DecodeScheduler.cpp；tests/unit/test_decode_scheduler.cpp
- **执行步骤：**
  1. 按 speed 计算 target/max lead。
  2. lead 过高等待，过低唤醒。
  3. 暂停规则。
  4. 数值范围固定。
- **验收标准：**
  - 0.5/1/2/4x 表格与公式一致。
  - 负 lead 立即运行。
  - 暂停达到上限后停。
- **完成证据：** `[<commit>] T0127` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0128 — 实现 playhead/pause/speed 命令处理

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0114,T0127
- **修改/新增文件：** src/worker/WorkerSession.cpp；tests/integration/test_worker_playback_commands.cpp
- **执行步骤：**
  1. 接收 set_playhead/set_paused/set_speed。
  2. 校验 generation。
  3. 更新 scheduler。
  4. 过旧命令忽略。
- **验收标准：**
  - 乱序命令不会回退 playhead。
  - 非法 speed 报错。
  - heartbeat 不受影响。
- **完成证据：** `[<commit>] T0128` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0129 — 实现解码队列背压

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0115,T0127
- **修改/新增文件：** src/worker/WorkerSession.cpp；tests/integration/test_decode_backpressure.cpp
- **执行步骤：**
  1. PCM 上限 300 chunk。
  2. 满时 decoder 阻塞可取消。
  3. 记录 queue depth。
  4. 不丢音频。
- **验收标准：**
  - 慢消费者时容量不超。
  - seek 能打断阻塞。
  - 样本顺序完整。
- **完成证据：** `[<commit>] T0129` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0130 — 实现 worker metrics 事件

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0127-T0129
- **修改/新增文件：** src/worker/WorkerMetrics.h；src/worker/WorkerMetrics.cpp；tests/unit/test_worker_metrics.cpp
- **执行步骤：**
  1. 每 2 秒汇总 decoded_until、queue、decode RTF、RSS、generation。
  2. 高频字段聚合。
- **验收标准：**
  - metrics JSON schema 固定。
  - 无媒体时合理。
  - 不含完整路径/文本。
- **完成证据：** `[<commit>] T0130` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0131 — 实现模拟字幕输出 backend

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0123,T0128
- **修改/新增文件：** src/worker/FakeRecognizerBackend.h；src/worker/FakeRecognizerBackend.cpp；tests/integration/test_fake_caption_flow.cpp
- **执行步骤：**
  1. 按 PCM 时间每秒输出固定 partial/final，专用于主进程集成。
  2. Release 默认不可启用。
- **验收标准：**
  - 测试能稳定复现事件。
  - 生产命令行无法误开 fake。
  - generation 正确。
- **完成证据：** `[<commit>] T0131` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0132 — 补齐 IPC fuzz/异常测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0105-T0113
- **修改/新增文件：** tests/unit/ipc/；tests/fuzz/ipc_corpus/；tools/run-ipc-fuzz.ps1
- **执行步骤：**
  1. 覆盖随机分片、坏长度、深 JSON、未知 type、断开重连、过快消息。
- **验收标准：**
  - 限定运行轮次无崩溃/无限内存。
  - 错误日志限速。
- **完成证据：** `[<commit>] T0132` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0133 — 补齐 FFmpeg 容器/音轨/seek 集成测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0117-T0129
- **修改/新增文件：** tests/integration/worker/；tests/fixtures/media/manifest.json
- **执行步骤：**
  1. 生成容器、codec、采样率、多音轨、坏 packet、不可 seek fixture。
  2. 自动校验 PCM hash/时间。
- **验收标准：**
  - 矩阵全部通过。
  - 测试 fixture 可由脚本重建。
  - 不依赖受版权保护素材。
- **完成证据：** `[<commit>] T0133` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0134 — 完成杀 worker 后自动恢复 E2E

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0112,T0131
- **修改/新增文件：** tests/e2e/worker_recovery.ps1；docs/test-plans/worker-recovery.md
- **执行步骤：**
  1. 播放媒体并启动 fake 字幕。
  2. 杀 worker。
  3. 验证视频继续、状态提示、重启、从 playhead 恢复。
- **验收标准：**
  - 旧 generation 不再显示。
  - 最多 3 次规则生效。
  - 无孤儿进程。
- **完成证据：** `[<commit>] T0134` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0135 — 签署 IPC/解码阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0105-T0134
- **修改/新增文件：** docs/phase-gates/P4-worker-decode-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 执行 seek storm、track switch、worker crash、rclone full 双开、内存背压。
  2. 附日志和结果。
- **验收标准：**
  - 全部 PASS。
  - 主进程可用 fake 字幕走完整链路。
  - 进入真实 ASR。
- **完成证据：** `[<commit>] T0135` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 5：模型管理、Lite 与 Balanced ASR

**阶段目标：** 接入真实离线模型，形成低延迟 partial、带标点终稿、SenseVoice 修订、硬件自适应与过载降级。  
**阶段门：** T0136–T0161 完成；真实模型在断网环境通过延迟、准确率、seek、4x 和低配降级门槛。

### T0136 — 实现 ModelManifest 解析与严格验证

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0013,T0135
- **修改/新增文件：** src/worker/ModelManifest.h；src/worker/ModelManifest.cpp；tests/unit/test_model_manifest.cpp
- **执行步骤：**
  1. 解析 bundle/component。
  2. 校验类型、采样率、语言、文件相对路径、hash 格式、版本范围。
  3. 禁止路径穿越。
- **验收标准：**
  - 合法 manifest 通过。
  - `../`、绝对路径、缺文件、未知类型被拒。
- **完成证据：** `[<commit>] T0136` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0137 — 实现增量模型 hash 校验

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0136
- **修改/新增文件：** src/worker/ModelVerifier.h；src/worker/ModelVerifier.cpp；tests/integration/test_model_verifier.cpp
- **执行步骤：**
  1. 首次全量 SHA-256。
  2. 后续基于 size/mtime 缓存但发布前可强制全量。
  3. 进度可取消。
- **验收标准：**
  - 篡改被发现。
  - 取消不写“已验证”。
  - 缓存命中快且模型版本变化会失效。
- **完成证据：** `[<commit>] T0137` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0138 — 实现 ModelManager 生命周期

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0136,T0137
- **修改/新增文件：** src/worker/ModelManager.h；src/worker/ModelManager.cpp；tests/integration/test_model_manager.cpp
- **执行步骤：**
  1. 加载 bundle、共享 VAD/模型资源、预热、卸载。
  2. 同一时刻只切一个 bundle。
  3. 错误回到 Ready/NoModel。
- **验收标准：**
  - 重复加载不泄漏。
  - 切 Lite/Balanced 后旧模型释放。
  - 缺内存错误可理解。
- **完成证据：** `[<commit>] T0138` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0139 — 实现模型加载进度事件

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0138
- **修改/新增文件：** src/worker/ModelManager.cpp；src/worker/WorkerSession.cpp；tests/integration/test_model_progress.cpp
- **执行步骤：**
  1. 按校验/加载/预热阶段发送 0–100。
  2. 限频。
  3. 失败带 component ID。
- **验收标准：**
  - 进度单调。
  - 失败不假到 100。
  - UI 可取消/重试。
- **完成证据：** `[<commit>] T0139` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0140 — 实现 HardwareProbe 与 auto 档位

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0138
- **修改/新增文件：** src/diagnostics/HardwareProbe.h；src/diagnostics/HardwareProbe.cpp；src/worker/ProfileSelector.cpp；tests/unit/test_profile_selector.cpp
- **执行步骤：**
  1. 获取逻辑/物理核、RAM、CPU 指令。
  2. <门槛选 Lite。
  3. 有历史 benchmark 时优先。
  4. 用户手动选择覆盖。
- **验收标准：**
  - 8GB mock 选 Lite、16GB mock 可 Balanced。
  - 结果写诊断。
  - 无硬件唯一标识上传。
- **完成证据：** `[<commit>] T0140` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0141 — 定义 IRecognizerBackend

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0135
- **修改/新增文件：** src/worker/IRecognizerBackend.h；src/worker/RecognizerTypes.h；tests/unit/test_recognizer_contract.cpp
- **执行步骤：**
  1. 定义 load/reset/accept/decode/isReady/getPartial/finalize。
  2. 输入带 generation 和媒体时间。
  3. 返回 Result。
- **验收标准：**
  - Fake/Online/SenseVoice 都可实现。
  - 接口不包含 Qt UI/DB 类型。
- **完成证据：** `[<commit>] T0141` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0142 — 实现 OnlineParaformerBackend 加载

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0138,T0141
- **修改/新增文件：** src/worker/OnlineParaformerBackend.h；src/worker/OnlineParaformerBackend.cpp；tests/integration/test_online_model_load.cpp
- **执行步骤：**
  1. 从 manifest 构造 sherpa online recognizer。
  2. 设置采样率/线程/endpoint。
  3. 预热静音和短音频。
- **验收标准：**
  - 锁定模型可加载。
  - 错模型文件返回 component 错误。
  - 重复 reset 可用。
- **完成证据：** `[<commit>] T0142` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0143 — 实现在线 PCM 输入与 decode 循环

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0142,T0129
- **修改/新增文件：** src/worker/OnlineParaformerBackend.cpp；src/worker/RecognizerPipeline.cpp；tests/integration/test_online_decode.cpp
- **执行步骤：**
  1. 按 chunk accept waveform。
  2. 调用 decode while ready。
  3. 保持 segment start sample/time。
  4. eof input finished。
- **验收标准：**
  - 完整 fixture 输出文本。
  - chunk 边界变化结果稳定。
  - generation 切换 reset。
- **完成证据：** `[<commit>] T0143` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0144 — 实现 partial 去重和 200ms 限频

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0143
- **修改/新增文件：** src/worker/RecognizerPipeline.cpp；tests/unit/test_partial_throttle.cpp
- **执行步骤：**
  1. 仅文本变化且距上次发送达到阈值才发。
  2. endpoint 前保留最后文本。
  3. Unicode trim。
- **验收标准：**
  - 高频 decode 不超过约定事件率。
  - 相同文本一次。
  - 清空文本不闪烁。
- **完成证据：** `[<commit>] T0144` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0145 — 实现 endpoint 与 Lite final

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0143
- **修改/新增文件：** src/worker/RecognizerPipeline.cpp；tests/integration/test_online_endpoint.cpp
- **执行步骤：**
  1. 识别器 endpoint 时生成 segment_id/start/end。
  2. reset stream。
  3. 空文本丢弃。
  4. 使用 sample clock 边界。
- **验收标准：**
  - 多句产生有序 final。
  - start<end。
  - 静音不产生空段。
  - 无 token 时间戳时不伪造词时间。
- **完成证据：** `[<commit>] T0145` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0146 — 实现 PunctuationBackend

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0138,T0145
- **修改/新增文件：** src/worker/PunctuationBackend.h；src/worker/PunctuationBackend.cpp；tests/integration/test_punctuation.cpp
- **执行步骤：**
  1. 加载锁定标点模型。
  2. Lite final 送标点。
  3. 错误时保留无标点文本并标 source。
- **验收标准：**
  - 中文/英文 fixture 有可读标点。
  - 模型缺失触发降级而非崩溃。
  - 空文本跳过。
- **完成证据：** `[<commit>] T0146` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0147 — 实现 VadSegmenter

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0138,T0129
- **修改/新增文件：** src/worker/VadSegmenter.h；src/worker/VadSegmenter.cpp；tests/unit/test_vad_segmenter.cpp
- **执行步骤：**
  1. 接入 Silero VAD。
  2. 按 manifest sample rate。
  3. 输出 speech start/end。
  4. 处理 discontinuity/eof。
- **验收标准：**
  - 语音/静音 fixture 边界在容许误差。
  - seek 后不跨代合并。
  - 纯噪声不爆段。
- **完成证据：** `[<commit>] T0147` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0148 — 实现 VAD padding、合并与最长段切分

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0147
- **修改/新增文件：** src/worker/VadSegmenter.cpp；src/worker/SpeechSegmentBuffer.cpp；tests/unit/test_speech_segment_buffer.cpp
- **执行步骤：**
  1. 前200/后300ms。
  2. 间隔<250ms 合并。
  3. 静音550ms断句。
  4. 最长25s低能量切。
  5. 最短400ms。
- **验收标准：**
  - 参数 golden 测试。
  - 超长连续语音被有限切分。
  - 总样本不越界。
- **完成证据：** `[<commit>] T0148` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0149 — 实现 SenseVoiceBackend

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0138,T0141,T0148
- **修改/新增文件：** src/worker/SenseVoiceBackend.h；src/worker/SenseVoiceBackend.cpp；tests/integration/test_sensevoice.cpp
- **执行步骤：**
  1. 从 manifest 加载 int8 模型。
  2. 接收完整 speech segment。
  3. 启用支持的 ITN/语言参数。
  4. 返回文本/语言/耗时。
- **验收标准：**
  - 中英样例可识别。
  - 坏 segment 不崩。
  - 输出时间沿用 VAD 而不是模型伪时间。
- **完成证据：** `[<commit>] T0149` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0150 — 实现 Balanced 双通道 pipeline

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0143,T0146-T0149
- **修改/新增文件：** src/worker/RecognizerPipeline.h；src/worker/RecognizerPipeline.cpp；tests/integration/test_balanced_pipeline.cpp
- **执行步骤：**
  1. 同一 PCM 送 online 与 VAD。
  2. online partial 绑定当前 segment。
  3. VAD 段入 final queue。
  4. 线程安全关联 ID。
- **验收标准：**
  - 事件序列可重放。
  - 每个 final 对应正确段。
  - 队列满执行背压而非丢音频。
- **完成证据：** `[<commit>] T0150` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0151 — 实现 SegmentAssembler

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0145,T0150
- **修改/新增文件：** src/worker/SegmentAssembler.h；src/worker/SegmentAssembler.cpp；tests/unit/test_segment_assembler.cpp
- **执行步骤：**
  1. 管理 segment_id/revision/start/end。
  2. 过滤空/纯标点/重复。
  3. 校正重叠。
  4. 按序提交。
- **验收标准：**
  - 乱序 final 被正确排序/等待。
  - revision 递增。
  - 100 组属性测试无非法时间。
- **完成证据：** `[<commit>] T0151` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0152 — 实现 2.5s 终稿超时与后到修订

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0146,T0149-T0151
- **修改/新增文件：** src/worker/RecognizerPipeline.cpp；tests/integration/test_final_timeout.cpp
- **执行步骤：**
  1. 句尾计时。
  2. 超时先发 punctuated online revision1。
  3. SenseVoice 后到发 revision2。
  4. 同文本不重复修订。
- **验收标准：**
  - 可注入慢 backend 复现。
  - 超时不会阻塞下一段。
  - 最新 revision 可确定。
- **完成证据：** `[<commit>] T0152` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0153 — 实现 zh/en 语言模式

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0142,T0146,T0149
- **修改/新增文件：** src/worker/LanguagePolicy.h；src/worker/LanguagePolicy.cpp；tests/unit/test_language_policy.cpp
- **执行步骤：**
  1. 把 UI 语言映射到在线/标点/final 配置。
  2. 切换语言增 generation。
  3. 无支持返回明确错误。
- **验收标准：**
  - zh/en 各跑 fixture。
  - 切换后旧结果丢弃。
  - 配置记录到 session。
- **完成证据：** `[<commit>] T0153` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0154 — 实现 auto 与日韩/粤语实验性回退

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0149,T0153
- **修改/新增文件：** src/worker/LanguagePolicy.cpp；src/worker/RecognizerPipeline.cpp；tests/integration/test_auto_language.cpp
- **执行步骤：**
  1. auto 最终语言由 SenseVoice。
  2. 在线仍中英。
  3. 无有效 partial 时发状态而非乱码。
  4. 实验语言仅 Balanced。
- **验收标准：**
  - 日/韩 fixture 至少不崩且终稿路径执行。
  - UI metadata 标实验。
  - Lite 下明确限制。
- **完成证据：** `[<commit>] T0154` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0155 — 实现 ASR 过载检测

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0130,T0150
- **修改/新增文件：** src/worker/OverloadDetector.h；src/worker/OverloadDetector.cpp；tests/unit/test_overload_detector.cpp
- **执行步骤：**
  1. 计算 RTF、final queue、recognized lag、RSS。
  2. 连续窗口触发。
  3. 有 hysteresis 防抖。
- **验收标准：**
  - 阈值边界测试。
  - 瞬时峰值不触发。
  - 持续落后触发一次事件。
- **完成证据：** `[<commit>] T0155` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0156 — 实现自动降级到 Lite

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0155,T0138
- **修改/新增文件：** src/worker/WorkerSession.cpp；src/worker/RecognizerPipeline.cpp；tests/integration/test_auto_downgrade.cpp
- **执行步骤：**
  1. 先降 partial 频率，再停 SenseVoice 新任务并转 Lite。
  2. 通知主进程。
  3. 同会话不自动反复升级。
- **验收标准：**
  - 慢 backend 场景触发。
  - 已在处理 final 有明确完成/取消策略。
  - 字幕时间不断层。
- **完成证据：** `[<commit>] T0156` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0157 — 实现 ASR benchmark 可执行程序

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0142-T0156
- **修改/新增文件：** tools/asr-benchmark/CMakeLists.txt；tools/asr-benchmark/main.cpp；tools/benchmark-asr.ps1
- **执行步骤：**
  1. 按 manifest 跑语料。
  2. 输出 JSON/CSV：CER/WER/partial/final/RTF/RSS。
  3. 固定线程和 model hash。
- **验收标准：**
  - 同机器重复结果波动在报告范围。
  - 失败条目标出原因。
  - 无 UI 依赖。
- **完成证据：** `[<commit>] T0157` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0158 — 实现 CER/WER 与对齐报告

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0157
- **修改/新增文件：** tools/asr-benchmark/TextNormalizer.*；tools/asr-benchmark/Metrics.*；tests/unit/test_asr_metrics.cpp
- **执行步骤：**
  1. 中文归一化规则版本化。
  2. 计算编辑距离。
  3. 同时保留原始/归一化文本。
  4. 不为了好看删除有意义字符。
- **验收标准：**
  - 手算样例一致。
  - 空参考/空预测处理明确。
  - 报告含规则版本。
- **完成证据：** `[<commit>] T0158` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0159 — 补齐 ASR 单元/故障测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0136-T0158
- **修改/新增文件：** tests/unit/asr/；tests/integration/asr/
- **执行步骤：**
  1. 覆盖模型损坏、内存失败、VAD 边界、partial 去重、revision、language、overload、取消。
- **验收标准：**
  - 真实模型测试可按标签运行。
  - mock 测试每 PR 运行。
  - 无随机挂起。
- **完成证据：** `[<commit>] T0159` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0160 — 运行真实语料性能与准确率门禁

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0157-T0159,T0023
- **修改/新增文件：** docs/benchmarks/asr-mvp-report.md；tests/benchmarks/results/*.json
- **执行步骤：**
  1. 在参考/最低硬件跑 Lite/Balanced/whisper 基线。
  2. 记录 commit/hash/线程。
  3. 对未达标项修复或明确阻塞。
- **验收标准：**
  - 报告能验证技术方案目标。
  - 不得只放摘要。
  - 原始 JSON 入 artifact。
- **完成证据：** `[<commit>] T0160` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0161 — 签署真实 ASR 阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0136-T0160
- **修改/新增文件：** docs/phase-gates/P5-asr-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 断网运行。
  2. 覆盖 start/seek/4x/低配/模型坏/切语言。
  3. 链接基准和缺陷。
- **验收标准：**
  - 所有发布门禁通过。
  - 未达准确率/延迟时不进入 UI 美化。
  - 状态进入主进程字幕集成。
- **完成证据：** `[<commit>] T0161` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 6：主进程字幕状态机、叠加、缓存与 SRT 导出

**阶段目标：** 把 worker 结果可靠地同步到播放时间，显示 partial/final，缓存终稿并导出确定性的 SRT。  
**阶段门：** T0162–T0194 完成；seek/倍速/切轨/崩溃场景字幕不串代，截图与 SRT 全部正确。

### T0162 — 定义 CaptionTypes

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0161
- **修改/新增文件：** src/captions/CaptionTypes.h；tests/unit/test_caption_types.cpp
- **执行步骤：**
  1. 定义 CaptionSegment/Partial/Revision/SessionKey/Coverage。
  2. 时间用 int64 ms。
  3. 文本 UTF-16/QString 边界明确。
- **验收标准：**
  - 非法时间构造失败。
  - 序列化 round-trip。
  - 不含 worker 私有类型。
- **完成证据：** `[<commit>] T0162` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0163 — 实现 CaptionStateMachine

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0162
- **修改/新增文件：** src/captions/CaptionStateMachine.h；src/captions/CaptionStateMachine.cpp；tests/unit/test_caption_state_machine.cpp
- **执行步骤：**
  1. 实现 Disabled/Starting/Running/Seeking/Suspended/Stopping/Error。
  2. 列出允许事件和 entry/exit action。
- **验收标准：**
  - 全转换表测试。
  - 非法事件返回错误不改状态。
  - 无多个布尔替代。
- **完成证据：** `[<commit>] T0163` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0164 — 实现生产 WorkerClient 消息映射

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0107,T0162
- **修改/新增文件：** src/ipc/WorkerClient.cpp；src/ipc/WorkerMessageMapper.cpp；tests/unit/test_worker_message_mapper.cpp
- **执行步骤：**
  1. 把 JSON 转强类型事件。
  2. 校验 generation/session/segment/revision/time。
  3. 未知 event 限速记录。
- **验收标准：**
  - 坏 final 不进入业务层。
  - 中文文本/大整数正确。
  - 协议错误可断开。
- **完成证据：** `[<commit>] T0164` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0165 — 实现 CaptionCoordinator 开关流程

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0163,T0164
- **修改/新增文件：** src/captions/CaptionCoordinator.h；src/captions/CaptionCoordinator.cpp；tests/integration/test_caption_start_stop.cpp
- **执行步骤：**
  1. 用户开启→检查模型→启动/复用 worker→发 open/start。
  2. 关闭→stop/清 overlay/保存 final。
  3. 所有异步可取消。
- **验收标准：**
  - 快速开关 20 次无崩溃。
  - 关闭后不显示新事件。
  - 基础播放不受影响。
- **完成证据：** `[<commit>] T0165` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0166 — 实现媒体打开与音轨参数同步

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0067,T0165
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/integration/test_caption_media_open.cpp
- **执行步骤：**
  1. file-loaded 后读取 canonical URL、duration、audio ff-index。
  2. 发送 open_media。
  3. 没有音轨时错误。
  4. 媒体切换增代。
- **验收标准：**
  - 多音轨当前 index 一致。
  - 无音轨视频提示。
  - 旧 media event 丢弃。
- **完成证据：** `[<commit>] T0166` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0167 — 实现 playhead 更新节流

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0165,T0058
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/unit/test_playhead_throttle.cpp
- **执行步骤：**
  1. 运行时定期或 time-pos 变化发送。
  2. 频率足够同步但不淹没 IPC。
  3. pause/seek 强制发送。
- **验收标准：**
  - 正常播放消息率在设定范围。
  - seek 即时。
  - 乱序 reply 不影响。
- **完成证据：** `[<commit>] T0167` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0168 — 实现 seek generation 流程

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0163,T0167
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/integration/test_caption_seek.cpp
- **执行步骤：**
  1. 检测 seeking 开始即清 partial/增代。
  2. 发 seek。
  3. 可先加载 DB 新位置。
  4. seek_ready 后 Running。
- **验收标准：**
  - seek storm 只显示最后位置字幕。
  - 旧 final 不入 DB 当前 session。
  - overlay 立即清。
- **完成证据：** `[<commit>] T0168` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0169 — 实现音轨切换字幕重建

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0067,T0168
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/integration/test_caption_track_switch.cpp
- **执行步骤：**
  1. aid 变化增代。
  2. 新 session key 包含 ff-index。
  3. 清 overlay。
  4. worker set/open track。
  5. 读取对应缓存。
- **验收标准：**
  - 两音轨字幕不混。
  - 切回可复用各自缓存。
  - 失败回到可选轨提示。
- **完成证据：** `[<commit>] T0169` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0170 — 实现 pause/speed 与 worker 同步

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0063,T0167
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/integration/test_caption_pause_speed.cpp
- **执行步骤：**
  1. 发送 paused/speed。
  2. 暂停保留当前字幕。
  3. speed 不改 segment times。
  4. 4x 触发提前量。
- **验收标准：**
  - 暂停 10 秒字幕不滚动。
  - 恢复位置正确。
  - 倍速后 SRT 时间不变。
- **完成证据：** `[<commit>] T0170` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0171 — 实现 worker 重启后的会话恢复

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0112,T0166-T0170
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/e2e/caption_worker_recovery.ps1
- **执行步骤：**
  1. ready 后重新 load bundle/open media/set playhead/start。
  2. 增 generation。
  3. 优先显示 DB final。
- **验收标准：**
  - 杀 worker 后视频继续。
  - 恢复无重复旧字幕。
  - 超过重试转 Error 可手动重试。
- **完成证据：** `[<commit>] T0171` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0172 — 实现 CaptionTimeline 查询结构

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0162
- **修改/新增文件：** src/captions/CaptionTimeline.h；src/captions/CaptionTimeline.cpp；tests/unit/test_caption_timeline.cpp
- **执行步骤：**
  1. 按 start/end 有序存 final。
  2. 单独存当前 partial。
  3. 支持 revision、区间查询、currentAt、clear generation。
- **验收标准：**
  - 1万段查询性能可接受。
  - 重叠校正规则固定。
  - 旧 revision 被覆盖。
- **完成证据：** `[<commit>] T0172` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0173 — 接入 partial/final/revision 事件

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0164,T0172
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；tests/integration/test_caption_events.cpp
- **执行步骤：**
  1. partial 只入内存。
  2. final/revision 入 timeline 并异步持久化。
  3. 相同 revision 幂等。
  4. 乱序缓冲/拒绝。
- **验收标准：**
  - 事件重放结果确定。
  - 重复消息不重复段。
  - 坏时间产生结构化错误。
- **完成证据：** `[<commit>] T0173` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0174 — 新增 caption_session/segment 数据库迁移

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0091,T0162
- **修改/新增文件：** migrations/002_caption_sessions.sql；tests/integration/test_caption_migration.cpp
- **执行步骤：**
  1. 按技术方案建表/索引/check/foreign key。
  2. 更新 schema 版本。
  3. 不改 001。
- **验收标准：**
  - 旧 DB 升级保留历史。
  - 约束拒绝非法段。
  - 降级有明确提示。
- **完成证据：** `[<commit>] T0174` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0175 — 实现 TranscriptRepository

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0092,T0174
- **修改/新增文件：** src/storage/TranscriptRepository.h；src/storage/TranscriptRepository.cpp；tests/integration/test_transcript_repository.cpp
- **执行步骤：**
  1. 创建/查 session。
  2. upsert segment revision。
  3. 更新 covered_until/status。
  4. 事务批写。
- **验收标准：**
  - revision 只增不退。
  - 删除 media 级联。
  - 并发批次无半写。
- **完成证据：** `[<commit>] T0175` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0176 — 实现媒体 identity 与 transcript cache key

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0093,T0175
- **修改/新增文件：** src/storage/MediaIdentity.h；src/storage/MediaIdentity.cpp；tests/unit/test_media_identity.cpp
- **执行步骤：**
  1. canonical URL + size + mtime。
  2. 音轨/model version/language/profile 组成 session key。
  3. 网络元数据缺失策略。
- **验收标准：**
  - 文件修改后 cache 失效。
  - 改名默认不错误复用。
  - 不同音轨/模型绝不共享。
- **完成证据：** `[<commit>] T0176` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0177 — 实现已缓存终稿加载

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0172,T0175,T0176
- **修改/新增文件：** src/captions/CaptionCoordinator.cpp；src/storage/TranscriptRepository.cpp；tests/integration/test_caption_cache.cpp
- **执行步骤：**
  1. 打开会话异步加载当前窗口及后续区间。
  2. 先显示缓存。
  3. worker 从 coverage 末端/缺口继续。
- **验收标准：**
  - 二次播放首屏无需推理。
  - DB 慢不阻塞 UI。
  - 过期 cache 不用。
- **完成证据：** `[<commit>] T0177` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0178 — 实现 coverage 进度与缺口处理

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0175,T0177
- **修改/新增文件：** src/captions/CoverageMap.h；src/captions/CoverageMap.cpp；tests/unit/test_coverage_map.cpp
- **执行步骤：**
  1. 按区间记录已完成。
  2. seek 到缺口启动识别。
  3. 有缓存区间不重复。
  4. 媒体 EOF 标 complete。
- **验收标准：**
  - 区间合并/拆分测试。
  - 乱序 final 不错误扩大 coverage。
  - complete 条件严格。
- **完成证据：** `[<commit>] T0178` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0179 — 实现 AssEscaper

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0162
- **修改/新增文件：** src/captions/AssEscaper.h；src/captions/AssEscaper.cpp；tests/unit/test_ass_escaper.cpp
- **执行步骤：**
  1. 转义反斜杠/花括号/换行。
  2. 过滤控制字符。
  3. 保留 Unicode。
  4. 防注入 ASS override。
- **验收标准：**
  - 恶意 `{\pos...}` 只显示文本。
  - emoji/中日韩保留。
  - golden 通过。
- **完成证据：** `[<commit>] T0179` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0180 — 实现字幕智能断行

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0162
- **修改/新增文件：** src/captions/CaptionLineBreaker.h；src/captions/CaptionLineBreaker.cpp；tests/unit/test_line_breaker.cpp
- **执行步骤：**
  1. 最多2行。
  2. 中文约20字符、拉丁42。
  3. 优先标点/空格。
  4. 不拆 surrogate/组合字符。
- **验收标准：**
  - 中英混合 golden。
  - 长 URL 有兜底。
  - 不会生成空首行。
- **完成证据：** `[<commit>] T0180` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0181 — 实现 MpvAssOverlayBackend

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0016,T0179,T0180
- **修改/新增文件：** src/captions/LiveCaptionOverlay.h；src/captions/LiveCaptionOverlay.cpp；tests/integration/test_ass_overlay.cpp
- **执行步骤：**
  1. 固定 overlay id。
  2. 发送 ass-events。
  3. clear/update。
  4. 用 playres/DPI 参数。
  5. 不无限创建 overlay。
- **验收标准：**
  - partial/final 可见。
  - clear 后消失。
  - 高频更新无资源增长。
- **完成证据：** `[<commit>] T0181` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0182 — 实现字幕样式与 partial/final 视觉

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0181,T0097
- **修改/新增文件：** src/captions/CaptionStyle.h；src/captions/LiveCaptionOverlay.cpp；tests/unit/test_caption_style.cpp
- **执行步骤：**
  1. partial 灰、final 白、黑描边、可选背景、字号/底边距。
  2. 范围裁剪。
  3. 字体使用系统字体。
- **验收标准：**
  - 设置即时生效。
  - 低/高 DPI 可读。
  - 不打包/泄露字体文件。
- **完成证据：** `[<commit>] T0182` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0183 — 实现按 time-pos 选择当前字幕

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0172,T0181
- **修改/新增文件：** src/captions/CaptionRenderer.h；src/captions/CaptionRenderer.cpp；tests/unit/test_caption_renderer.cpp
- **执行步骤：**
  1. 查询 `time_pos + delay`。
  2. partial 只在开放段窗口。
  3. 结束后保留<=150ms。
  4. pause 不推进。
- **验收标准：**
  - 边界前后毫秒测试。
  - seek 后不闪旧字。
  - 无段时 clear 一次而非重复命令。
- **完成证据：** `[<commit>] T0183` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0184 — 实现原字幕/实时字幕显示模式

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0068,T0183
- **修改/新增文件：** src/captions/CaptionDisplayMode.h；src/playback/MpvPlayer.cpp；src/ui/MainWindow.cpp；tests/integration/test_dual_subtitles.cpp
- **执行步骤：**
  1. 支持 original/live/both/none。
  2. both 时调整原字幕位置。
  3. 切换可逆。
- **验收标准：**
  - 四模式结果符合。
  - 媒体换轨不覆盖用户模式。
  - both 不严重重叠。
- **完成证据：** `[<commit>] T0184` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0185 — 实现用户实时字幕 delay

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0183,T0097
- **修改/新增文件：** src/captions/CaptionRenderer.cpp；src/ui/settings/CaptionSettingsPage.cpp；tests/unit/test_live_caption_delay.cpp
- **执行步骤：**
  1. 独立于 mpv sub-delay。
  2. 范围例如 ±10s。
  3. 显示/导出策略：显示应用 delay，原始 SRT 默认不应用并明确选项。
- **验收标准：**
  - 正负 delay 显示正确。
  - 原始 DB 时间不变。
  - UI 文案清晰。
- **完成证据：** `[<commit>] T0185` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0186 — 实现辅助技术可访问字幕文本

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0183
- **修改/新增文件：** src/ui/AccessibleCaptionText.h；src/ui/AccessibleCaptionText.cpp；tests/e2e/accessibility.md
- **执行步骤：**
  1. 维护不可见但 accessible 的 final 文本。
  2. partial 可配置不朗读。
  3. 设 role/name/description。
  4. 更新限频。
- **验收标准：**
  - Windows Narrator 可读取终稿。
  - 字幕关闭时清空。
  - 焦点不被抢。
- **完成证据：** `[<commit>] T0186` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0187 — 完成带实时字幕截图集成

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0077,T0181-T0183
- **修改/新增文件：** src/playback/ScreenshotService.cpp；src/captions/LiveCaptionOverlay.cpp；tests/integration/test_live_caption_screenshot.cpp
- **执行步骤：**
  1. 调用 P0 通过路径。
  2. fallback 使用同一 style/line breaker。
  3. 截图瞬间锁定当前 segment。
- **验收标准：**
  - golden 包含正确字幕和位置。
  - 快速字幕变化不截到半更新。
  - HDR 路径有记录。
- **完成证据：** `[<commit>] T0187` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0188 — 实现 SrtExporter 核心算法

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0032,T0172
- **修改/新增文件：** src/captions/SrtExporter.h；src/captions/SrtExporter.cpp；tests/unit/test_srt_exporter.cpp
- **执行步骤：**
  1. 只取最新 final。
  2. 排序、裁重叠、最短300ms、两行断行、UTF-8 BOM、>99h。
  3. 确定性。
- **验收标准：**
  - golden 字节一致。
  - 空/重复/重叠/Unicode/100h 全覆盖。
  - partial 不进入。
- **完成证据：** `[<commit>] T0188` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0189 — 实现导出路径与原子写入

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0188,T0029
- **修改/新增文件：** src/captions/SrtExporter.cpp；src/ui/ExportDialog.cpp；tests/integration/test_srt_write.cpp
- **执行步骤：**
  1. 媒体同目录可写则默认。
  2. 否则 Videos 子目录。
  3. 冲突询问。
  4. QSaveFile。
  5. 错误可复制。
- **验收标准：**
  - 只读/磁盘满/已有文件路径通过。
  - 失败不留下半文件。
  - 中文文件名可打开。
- **完成证据：** `[<commit>] T0189` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0190 — 实现“当前已完成部分”导出

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0178,T0188,T0189
- **修改/新增文件：** src/ui/ExportDialog.cpp；src/captions/SrtExporter.cpp；tests/integration/test_partial_export.cpp
- **执行步骤：**
  1. 识别未完成时明确 `_partial`。
  2. 只导 final。
  3. 对 complete 状态去后缀。
  4. 显示覆盖区间。
- **验收标准：**
  - 按钮状态正确。
  - 文件不含灰色 partial。
  - 完成后全量导出包含更多段。
- **完成证据：** `[<commit>] T0190` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0191 — 补齐 timeline/overlay 同步测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0162-T0187
- **修改/新增文件：** tests/unit/captions/；tests/integration/captions/
- **执行步骤：**
  1. 用虚拟 playhead 测 pause/seek/speed/delay/revision/旧代/重叠。
  2. 验证 overlay 命令 golden。
- **验收标准：**
  - 固定 seed 重复通过。
  - 所有非法事件有断言/错误。
  - 不依赖真实时间 sleep。
- **完成证据：** `[<commit>] T0191` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0192 — 补齐 SRT 与数据库 golden 测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0174-T0190
- **修改/新增文件：** tests/golden/srt/；tests/integration/test_transcript_roundtrip.cpp
- **执行步骤：**
  1. DB 写入→重启读取→导出。
  2. 比较字节。
  3. 覆盖 schema migration 和 revision。
- **验收标准：**
  - 同输入字节一致。
  - 事务失败回滚。
  - Windows 常见播放器可打开抽样文件。
- **完成证据：** `[<commit>] T0192` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0193 — 完成真实实时字幕端到端测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0165-T0192
- **修改/新增文件：** tests/e2e/live_caption.ps1；docs/test-plans/live-caption-e2e.md
- **执行步骤：**
  1. 打开 fixture→开字幕→观察 partial/final→pause→2x/4x→seek→切轨→杀 worker→导出→截图。
- **验收标准：**
  - 每步有机器可校验信号/产物。
  - 无旧字幕。
  - SRT/截图正确。
  - 断网完成。
- **完成证据：** `[<commit>] T0193` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0194 — 签署字幕集成阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0162-T0193
- **修改/新增文件：** docs/phase-gates/P6-caption-integration-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 执行完整同步/缓存/导出/截图矩阵。
  2. 记录 P95。
  3. 清理阻塞缺陷。
- **验收标准：**
  - 技术方案质量门槛全部满足或明确阻塞。
  - 进入完整 UI/诊断。
- **完成证据：** `[<commit>] T0194` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 7：完整 UI、设置页、可访问性与诊断

**阶段目标：** 把全部能力做成清晰、零配置、可恢复的用户体验，不暴露无实现控件。  
**阶段门：** T0195–T0218 完成；键鼠、触控板、高 DPI、屏幕阅读器和错误恢复均通过人工与自动验收。

### T0195 — 实现 MainWindow 最终布局

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0194
- **修改/新增文件：** src/ui/MainWindow.h；src/ui/MainWindow.cpp；resources/ui/
- **执行步骤：**
  1. 中央视频、底部控制、顶部/右键菜单、可收起列表、状态区。
  2. 布局自适应最小窗口。
- **验收标准：**
  - 1280x720/4K/DPI 不截断。
  - 全屏切换稳定。
  - 无媒体有清晰空状态。
- **完成证据：** `[<commit>] T0195` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0196 — 实现 PlayerControls 最终交互

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0195,T0059-T0065
- **修改/新增文件：** src/ui/PlayerControls.h；src/ui/PlayerControls.cpp；tests/e2e/player_controls.md
- **执行步骤：**
  1. 播放、时间、进度、音量、速度、字幕开关、全屏。
  2. hover tooltip 和 enabled 状态。
  3. 不重复业务逻辑。
- **验收标准：**
  - 所有按钮映射 ActionRegistry。
  - 无媒体禁用正确。
  - 键盘可聚焦。
- **完成证据：** `[<commit>] T0196` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0197 — 完善 seek slider 预览和可访问性

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0196,T0061
- **修改/新增文件：** src/ui/SeekSlider.h；src/ui/SeekSlider.cpp；tests/e2e/seek_slider.md
- **执行步骤：**
  1. 显示当前位置/hover 时间。
  2. 键盘步进。
  3. 拖动状态与真实 time-pos 分离。
  4. 时长未知处理。
- **验收标准：**
  - 长视频时间格式正确。
  - 键盘/鼠标均可。
  - seek 中不抖回。
- **完成证据：** `[<commit>] T0197` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0198 — 实现动态音轨/字幕菜单

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0066-T0069,T0195
- **修改/新增文件：** src/ui/TrackMenuBuilder.h；src/ui/TrackMenuBuilder.cpp；tests/unit/test_track_menu.cpp
- **执行步骤：**
  1. 按语言/标题/codec 构造。
  2. 当前选择打勾。
  3. 外挂可重载/移除。
  4. 实时字幕模式独立区。
- **验收标准：**
  - 多轨菜单不重复。
  - 无标题有合理 fallback。
  - 切媒体清旧 QAction。
- **完成证据：** `[<commit>] T0198` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0199 — 实现播放列表侧栏

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0088,T0195
- **修改/新增文件：** src/ui/PlaylistPanel.h；src/ui/PlaylistPanel.cpp；tests/e2e/playlist_panel.md
- **执行步骤：**
  1. 显示当前/错误/缺失状态。
  2. 拖动排序、删除、清空、打开位置。
  3. 大列表虚拟化。
- **验收标准：**
  - 1000 项滚动可用。
  - 删除当前规则正确。
  - 键盘操作可达。
- **完成证据：** `[<commit>] T0199` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0200 — 实现 CaptionStatusWidget

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0163,T0195
- **修改/新增文件：** src/ui/CaptionStatusWidget.h；src/ui/CaptionStatusWidget.cpp；tests/unit/test_caption_status_widget.cpp
- **执行步骤：**
  1. 映射模型加载、识别中、暂停、seek、overload、error、coverage。
  2. 不占字幕区域。
  3. 提供重试/详情。
- **验收标准：**
  - 每个 state 有文案/图标/动作。
  - 状态切换不闪烁。
  - overload 明确已降级。
- **完成证据：** `[<commit>] T0200` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0201 — 实现 ToastManager 与错误中心

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0031,T0195
- **修改/新增文件：** src/ui/ToastManager.h；src/ui/ToastManager.cpp；src/ui/ErrorDetailsDialog.cpp
- **执行步骤：**
  1. 瞬时消息合并。
  2. 错误可展开/复制诊断。
  3. 相同错误限频。
  4. 致命/非致命视觉区分。
- **验收标准：**
  - 错误风暴不堆满窗口。
  - 复制内容脱敏。
  - 重要错误不会自动消失过快。
- **完成证据：** `[<commit>] T0201` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0202 — 实现设置窗口导航壳

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0097,T0195
- **修改/新增文件：** src/ui/settings/SettingsDialog.h；src/ui/settings/SettingsDialog.cpp
- **执行步骤：**
  1. 分类：播放、字幕、模型、快捷键、隐私、诊断、关于。
  2. Apply/Cancel/Restore。
  3. 验证错误定位。
- **验收标准：**
  - Cancel 不保存。
  - Apply 原子。
  - 关闭重开值一致。
  - 窗口高 DPI 可滚动。
- **完成证据：** `[<commit>] T0202` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0203 — 实现播放设置页

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0071-T0074,T0202
- **修改/新增文件：** src/ui/settings/PlaybackSettingsPage.h；src/ui/settings/PlaybackSettingsPage.cpp
- **执行步骤：**
  1. 硬解、HDR、默认速度、保音调、连续播放、记忆位置。
  2. 即时/需重载标注。
- **验收标准：**
  - 范围验证。
  - 恢复默认。
  - 需重载项提示且行为正确。
- **完成证据：** `[<commit>] T0203` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0204 — 实现实时字幕设置页

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0182,T0185,T0202
- **修改/新增文件：** src/ui/settings/CaptionSettingsPage.h；src/ui/settings/CaptionSettingsPage.cpp
- **执行步骤：**
  1. 开关、语言、档位、字号、位置、背景、delay、自动导出。
  2. 隐藏说话人/云端未实现项。
- **验收标准：**
  - 所有值绑定 Settings。
  - 实验语言标记。
  - 没有假控件。
  - 即时预览可撤销。
- **完成证据：** `[<commit>] T0204` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0205 — 实现模型设置与完整性页

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0137-T0140,T0202
- **修改/新增文件：** src/ui/settings/ModelSettingsPage.h；src/ui/settings/ModelSettingsPage.cpp
- **执行步骤：**
  1. 显示安装 bundle、大小、hash 状态、推荐档位、重新校验。
  2. MVP 不提供联网下载。
- **验收标准：**
  - 损坏模型显示具体 component。
  - 校验可取消。
  - 无模型仍可播放。
- **完成证据：** `[<commit>] T0205` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0206 — 实现快捷键编辑器

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0099-T0102,T0202
- **修改/新增文件：** src/ui/settings/ShortcutSettingsPage.h；src/ui/settings/ShortcutSettingsPage.cpp
- **执行步骤：**
  1. 搜索动作。
  2. 捕获按键。
  3. 冲突提示。
  4. 恢复单项/全部。
  5. Apply 后更新 QAction。
- **验收标准：**
  - 冲突无法保存。
  - Esc 取消捕获。
  - 键盘全流程可操作。
- **完成证据：** `[<commit>] T0206` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0207 — 实现最近播放 UI

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0093-T0095,T0195
- **修改/新增文件：** src/ui/RecentMediaMenu.cpp；src/ui/HistoryDialog.cpp
- **执行步骤：**
  1. 显示文件名、位置、时间。
  2. 继续/从头、移除、清空。
  3. 缺失项标记。
- **验收标准：**
  - 最多200条。
  - 清空需确认。
  - 不在默认 UI 暴露完整敏感路径。
- **完成证据：** `[<commit>] T0207` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0208 — 完善导出 UI 与完成反馈

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0189,T0190,T0195
- **修改/新增文件：** src/ui/ExportDialog.cpp；src/ui/MainWindow.cpp
- **执行步骤：**
  1. 显示已完成区间、full/partial、目标路径、覆盖选项。
  2. 完成后可打开目录。
- **验收标准：**
  - 无 final 禁用并解释。
  - 写失败保留对话框。
  - 成功路径可复制。
- **完成证据：** `[<commit>] T0208` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0209 — 实现模型缺失/损坏恢复引导

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0205,T0201
- **修改/新增文件：** src/ui/ModelRecoveryDialog.cpp；docs/user/model-repair.md
- **执行步骤：**
  1. 说明字幕不可用但播放可用。
  2. 验证安装包/重新安装步骤。
  3. 不诱导联网来源。
- **验收标准：**
  - 缺模型启动不崩。
  - 关闭对话可继续播放。
  - 文档与实际目录一致。
- **完成证据：** `[<commit>] T0209` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0210 — 实现首次启动最小引导

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0202-T0205
- **修改/新增文件：** src/ui/FirstRunWizard.cpp；resources/defaults/settings.json
- **执行步骤：**
  1. 只做隐私离线说明、模型档位自动选择、默认应用可选引导。
  2. 可跳过。
  3. 不强制登录/联网。
- **验收标准：**
  - 首次一次出现。
  - 跳过不影响播放。
  - 选择可在设置更改。
  - 默认不开字幕可按产品决定并记录。
- **完成证据：** `[<commit>] T0210` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0211 — 实现 RcloneDiagnostics UI

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0007,T0195
- **修改/新增文件：** src/diagnostics/RcloneDiagnostics.h；src/diagnostics/RcloneDiagnostics.cpp；src/ui/RcloneDiagnosticsDialog.cpp；tests/unit/test_rclone_parser.cpp
- **执行步骤：**
  1. 用户选择 bat/命令文本后解析 cache mode/size/age/read-ahead/cache-dir。
  2. 只读。
  3. 生成 full 建议。
- **验收标准：**
  - 含 writes/off 的样例给警告。
  - full 通过。
  - 不读取/显示 token。
  - 不自动改文件。
- **完成证据：** `[<commit>] T0211` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0212 — 实现脱敏诊断报告

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0030,T0130,T0201
- **修改/新增文件：** src/diagnostics/DiagnosticReport.h；src/diagnostics/DiagnosticReport.cpp；tools/collect-diagnostics.ps1；tests/unit/test_redaction.cpp
- **执行步骤：**
  1. 汇总版本、依赖、模型 hash、设置脱敏、最近日志、硬件。
  2. 用户确认后 zip。
  3. 过滤路径/字幕/token。
- **验收标准：**
  - golden 报告无敏感样例。
  - 取消不留 zip。
  - 报告可用于定位 worker/mpv 状态。
- **完成证据：** `[<commit>] T0212` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0213 — 实现隐私设置页

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0097,T0202
- **修改/新增文件：** src/ui/settings/PrivacySettingsPage.h；src/ui/settings/PrivacySettingsPage.cpp
- **执行步骤：**
  1. 显示离线模式、无遥测、日志正文/路径开关。
  2. 危险开关有解释。
  3. 默认全关。
- **验收标准：**
  - 新安装默认值通过。
  - 开启后仅影响后续日志。
  - 关闭可清理旧日志选项。
- **完成证据：** `[<commit>] T0213` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0214 — 完成中文文案与术语统一

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0195-T0213
- **修改/新增文件：** resources/translations/app_zh_CN.ts；docs/style/terminology.md
- **执行步骤：**
  1. 统一“实时字幕/终稿/轻量模式/均衡模式/音轨/外挂字幕”。
  2. 去除技术堆栈暴露。
  3. 错误给行动。
- **验收标准：**
  - lrelease 无缺失。
  - 文案评审表完成。
  - 同一概念无多名称。
- **完成证据：** `[<commit>] T0214` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0215 — 完成高 DPI 与多显示器 UI 验收

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0195-T0214
- **修改/新增文件：** tests/e2e/ui_dpi_matrix.md；docs/phase-gates/artifacts/ui/
- **执行步骤：**
  1. 100/150/200/250%。
  2. 跨屏。
  3. 全屏。
  4. 不同缩放。
  5. 记录截图和问题。
- **验收标准：**
  - 控件/字幕不裁切。
  - 窗口位置可恢复。
  - 焦点/菜单在正确屏幕。
- **完成证据：** `[<commit>] T0215` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0216 — 完成键盘与屏幕阅读器验收

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0186,T0195-T0214
- **修改/新增文件：** tests/e2e/accessibility.md；docs/test-results/accessibility.md
- **执行步骤：**
  1. 仅键盘完成开文件、播放、开字幕、设置、导出。
  2. Narrator 读取控件和 final 字幕。
- **验收标准：**
  - 焦点顺序合理。
  - 无键盘陷阱。
  - 颜色不是唯一状态。
  - 报告列出已知限制。
- **完成证据：** `[<commit>] T0216` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0217 — 完成 UI 自动化 smoke

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0195-T0214
- **修改/新增文件：** tests/e2e/ui_smoke.ps1；tests/e2e/selectors.json
- **执行步骤：**
  1. 使用稳定 objectName/automation id。
  2. 自动开文件、播放、字幕、设置、导出。
  3. 失败截图。
- **验收标准：**
  - 干净机可运行。
  - 不依赖屏幕坐标。
  - 连续3次稳定。
- **完成证据：** `[<commit>] T0217` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0218 — 签署 UI/诊断阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0195-T0217
- **修改/新增文件：** docs/phase-gates/P7-ui-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 产品走查、可访问性、DPI、错误恢复、诊断脱敏全部附证据。
  2. 清零阻塞 UI 缺陷。
- **验收标准：**
  - 所有 MVP 控件有实现。
  - 无假功能。
  - 进入 Windows 安装集成。
- **完成证据：** `[<commit>] T0218` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 8：Windows 文件关联、默认应用与离线安装包

**阶段目标：** 生成可在干净 Windows 用户环境安装、升级、卸载、离线识别并合规分发的 MSI。  
**阶段门：** T0219–T0243 完成；Offline/Lite 两个 MSI 在干净虚拟机通过安装、关联、升级、卸载和断网 smoke。

### T0219 — 实现 IPlatformIntegration/WindowsIntegration

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0218
- **修改/新增文件：** src/platform/IPlatformIntegration.h；src/platform/windows/WindowsIntegration.h；src/platform/windows/WindowsIntegration.cpp
- **执行步骤：**
  1. 封装默认应用、关联、known folders、power、系统设置启动。
  2. 核心/UI 不直接 include Windows headers。
- **验收标准：**
  - 静态扫描核心层无 Windows API。
  - mock 平台可跑单测。
- **完成证据：** `[<commit>] T0219` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0220 — 实现 versioned ProgID 注册数据

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0219
- **修改/新增文件：** src/platform/windows/FileAssociationService.h；src/platform/windows/FileAssociationService.cpp；tests/unit/test_progid.cpp
- **执行步骤：**
  1. ProgID `RealtimeCaptionPlayer.Video.1`。
  2. 注册显示名、图标、shell open command。
  3. per-user HKCU Classes。
- **验收标准：**
  - 生成的命令正确引用 exe 和 `%1`。
  - 中文安装路径测试。
  - 不写 UserChoice。
- **完成证据：** `[<commit>] T0220` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0221 — 实现 RegisteredApplications/Capabilities

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0220
- **修改/新增文件：** src/platform/windows/FileAssociationService.cpp；packaging/wix/Associations.wxs；tests/unit/test_capabilities.cpp
- **执行步骤：**
  1. 注册应用名、Capabilities、FileAssociations、ApplicationDescription。
  2. 安装/运行时逻辑一致。
- **验收标准：**
  - Windows 默认应用列表可见本应用。
  - 注册表 golden 与预期一致。
- **完成证据：** `[<commit>] T0221` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0222 — 集中维护扩展名清单

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0079,T0221
- **修改/新增文件：** src/core/SupportedFormats.h；resources/supported-formats.json；packaging/wix/Associations.wxs；tests/unit/test_extension_consistency.cpp
- **执行步骤：**
  1. 单一源生成文件对话框、拖放、注册表和文档清单。
  2. 包含 PRD 扩展。
- **验收标准：**
  - 测试确保四处一致。
  - 大小写处理正确。
  - 新增扩展只改单一源。
- **完成证据：** `[<commit>] T0222` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0223 — 实现安全 open command/参数处理

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0041,T0220
- **修改/新增文件：** src/platform/windows/FileAssociationService.cpp；tests/unit/test_open_command_quoting.cpp
- **执行步骤：**
  1. 严格引号。
  2. 路径作为单独 argv。
  3. 不支持 shell verb 执行。
  4. 多文件仍由单实例转发。
- **验收标准：**
  - 安装路径/文件名含空格、&、括号、中文时只打开文件。
  - 无命令注入。
- **完成证据：** `[<commit>] T0223` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0224 — 实现 DefaultAppsLauncher

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0221
- **修改/新增文件：** src/platform/windows/DefaultAppsLauncher.h；src/platform/windows/DefaultAppsLauncher.cpp；tests/unit/test_default_apps_uri.cpp
- **执行步骤：**
  1. 构造本应用专属 `ms-settings:defaultapps` URI/注册参数。
  2. 失败回通用页面。
  3. 不自动点击。
- **验收标准：**
  - 按钮打开系统页。
  - URI 编码正确。
  - 测试确认代码不写 UserChoice。
- **完成证据：** `[<commit>] T0224` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0225 — 实现关联状态检测

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0220-T0224
- **修改/新增文件：** src/platform/windows/FileAssociationService.cpp；tests/integration/test_association_detection.cpp
- **执行步骤：**
  1. 查询常见扩展当前 handler。
  2. 返回 unknown/this/other。
  3. 系统限制下不夸大全部默认。
- **验收标准：**
  - 手动设置后刷新显示。
  - 查不到时显示“请在系统中确认”。
  - 不修改状态。
- **完成证据：** `[<commit>] T0225` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0226 — 实现卸载关联清理规则

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0220,T0221
- **修改/新增文件：** packaging/wix/Associations.wxs；src/platform/windows/FileAssociationService.cpp；tests/e2e/uninstall_associations.ps1
- **执行步骤：**
  1. 只删本应用 ProgID/Capabilities/RegisteredApplications。
  2. 不重写用户默认。
  3. 处理升级 refcount。
- **验收标准：**
  - 卸载后本应用不在列表。
  - 其他播放器关联不变。
  - 用户 UserChoice 未被篡改。
- **完成证据：** `[<commit>] T0226` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0227 — 完成文件关联端到端测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0220-T0226
- **修改/新增文件：** tests/e2e/file_associations.ps1；docs/test-results/file-associations.md
- **执行步骤：**
  1. 安装→右键打开方式→系统设默认→双击各扩展→升级→卸载。
  2. 记录注册表前后 diff。
- **验收标准：**
  - 支持扩展均能传到首实例。
  - 中文/空格路径。
  - 卸载无脏键（允许系统缓存说明）。
- **完成证据：** `[<commit>] T0227` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0228 — 添加 Windows 图标、版本资源与 manifest

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0028,T0219
- **修改/新增文件：** resources/icons/app.ico；resources/windows/app.rc；resources/windows/app.manifest
- **执行步骤：**
  1. 多尺寸 icon。
  2. 版本/company/product。
  3. DPI aware、longPathAware、requestedExecutionLevel asInvoker。
- **验收标准：**
  - 资源查看器值正确。
  - 任务栏/文件图标清晰。
  - 无需管理员。
  - 长路径测试通过。
- **完成证据：** `[<commit>] T0228` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0229 — 建立 Release staging 与 windeployqt

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0218,T0228
- **修改/新增文件：** tools/package.ps1；cmake/Packaging.cmake
- **执行步骤：**
  1. 安装到 staging。
  2. 运行 windeployqt。
  3. 只带实际 Qt plugins。
  4. 排除开发文件。
  5. 随后运行依赖扫描。
- **验收标准：**
  - staging 在无 Qt PATH 机器启动。
  - 无缺 DLL。
  - 没有多余 debug DLL。
- **完成证据：** `[<commit>] T0229` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0230 — 打包第三方 DLL 与加载路径加固

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0229,T0036
- **修改/新增文件：** tools/package.ps1；src/app/Application.cpp；tests/e2e/dll_loading.ps1
- **执行步骤：**
  1. 复制锁定 libmpv/FFmpeg/sherpa/onnx DLL。
  2. 设置安全 DLL 搜索目录。
  3. 验证 hash。
- **验收标准：**
  - 从含恶意同名 DLL 的当前目录启动仍加载安装目录版本。
  - DependencyProbe 一致。
- **完成证据：** `[<commit>] T0230` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0231 — 打包并校验模型目录

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0137,T0229
- **修改/新增文件：** tools/package.ps1；packaging/models/；tests/e2e/installed_models.ps1
- **执行步骤：**
  1. Offline 包含 Lite+Balanced。
  2. Lite 包只含 Lite。
  3. 安装结束运行 hash 校验。
  4. 模型路径 per-user。
- **验收标准：**
  - 断网安装后校验通过。
  - 两个包 component 列表正确。
  - 损坏安装失败/修复。
- **完成证据：** `[<commit>] T0231` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0232 — 生成 LICENSE/NOTICE/SBOM

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0037,T0229
- **修改/新增文件：** tools/generate-notices.ps1；packaging/licenses/；packaging/sbom/
- **执行步骤：**
  1. 收集项目 GPL、Qt、mpv、FFmpeg、sherpa、ONNX、模型文本。
  2. 生成 SPDX/CycloneDX 之一。
  3. 加入源码获取说明。
- **验收标准：**
  - 安装目录和安装 UI 可访问。
  - 脚本无缺项。
  - SBOM 版本/hash 与 lock 一致。
- **完成证据：** `[<commit>] T0232` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0233 — 建立 WiX/CPack MSI 骨架

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0221,T0229-T0232
- **修改/新增文件：** packaging/wix/Product.wxs；packaging/wix/Features.wxs；CPackConfig.cmake
- **执行步骤：**
  1. per-user 安装到 LocalAppData Programs。
  2. 组件 GUID 稳定。
  3. 写 Start Menu/卸载。
  4. 不请求管理员。
- **验收标准：**
  - MSI 可安装/卸载空 core。
  - 日志无 ICE 阻塞错误。
  - 安装路径符合方案。
- **完成证据：** `[<commit>] T0233` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0234 — 生成 Offline MSI

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0231-T0233
- **修改/新增文件：** packaging/wix/Features.wxs；tools/package.ps1
- **执行步骤：**
  1. 启用 core+Lite+Balanced。
  2. 命名、版本、大小、hash。
  3. 离线安装不下载。
- **验收标准：**
  - 干净断网 VM 安装后可实时字幕。
  - 包内容 manifest 匹配。
  - 启动时间可接受。
- **完成证据：** `[<commit>] T0234` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0235 — 生成 Lite MSI

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0231-T0233
- **修改/新增文件：** packaging/wix/Features.wxs；tools/package.ps1
- **执行步骤：**
  1. 启用 core+Lite，排除 SenseVoice。
  2. UI 默认 Lite。
  3. Balanced 选项不可误显示已安装。
- **验收标准：**
  - 断网可 partial/标点 final。
  - 模型页正确。
  - 包不含 Balanced hash 文件。
- **完成证据：** `[<commit>] T0235` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0236 — 固定 UpgradeCode/ProductVersion 规则

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0233
- **修改/新增文件：** packaging/wix/Product.wxs；docs/release/versioning.md；tests/e2e/msi_upgrade.ps1
- **执行步骤：**
  1. UpgradeCode 稳定。
  2. ProductCode 按 MSI 规则。
  3. 语义版本映射。
  4. 禁止同版本覆盖未知构建。
- **验收标准：**
  - v0.1→v0.2 升级保留数据。
  - 相同/更旧版本行为明确。
  - major upgrade 正确。
- **完成证据：** `[<commit>] T0236` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0237 — 实现安装后首次启动与关联注册

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0233,T0221
- **修改/新增文件：** packaging/wix/Product.wxs；src/app/Application.cpp
- **执行步骤：**
  1. 安装结束可选启动。
  2. 注册关联由 MSI 完成，应用启动只诊断/修复自有键。
  3. 不自动设默认。
- **验收标准：**
  - 取消启动仍注册能力。
  - 首次启动无管理员。
  - 修复安装恢复缺键。
- **完成证据：** `[<commit>] T0237` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0238 — 实现卸载时保留/删除用户数据选项

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0233
- **修改/新增文件：** packaging/wix/UserData.wxs；docs/user/uninstall.md；tests/e2e/uninstall_data.ps1
- **执行步骤：**
  1. 默认保留配置/DB/模型。
  2. 显式选项删除。
  3. 日志/诊断单独说明。
  4. 不删用户导出/截图。
- **验收标准：**
  - 默认卸载重装可恢复。
  - 选择删除后 app data 清理。
  - 媒体目录文件不受影响。
- **完成证据：** `[<commit>] T0238` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0239 — 接入代码签名流程

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0234,T0235
- **修改/新增文件：** tools/sign.ps1；docs/release/signing.md；CI release config
- **执行步骤：**
  1. 对 exe/dll/msi 签名。
  2. 证书只在受保护 release 环境。
  3. 签名后再算发布 hash。
- **验收标准：**
  - signtool verify 通过。
  - 无证书的内部构建明确标 unsigned，不冒充正式发布。
- **完成证据：** `[<commit>] T0239` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0240 — 完善一键 package 脚本

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0229-T0239
- **修改/新增文件：** tools/package.ps1；tools/verify-package.ps1
- **执行步骤：**
  1. clean build→test→stage→hash→licenses→MSI→sign→verify。
  2. 任一步失败停止。
  3. 输出 manifest。
- **验收标准：**
  - 一条命令生成两个包。
  - 日志指向失败步骤。
  - 重复构建在相同输入下内容可追溯。
- **完成证据：** `[<commit>] T0240` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0241 — 执行干净 VM 离线 smoke

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0234,T0235,T0240
- **修改/新增文件：** tests/e2e/installed_smoke.ps1；docs/test-results/install-smoke.md
- **执行步骤：**
  1. 无 Qt/开发工具/网络：安装、开视频、开字幕、seek、导出、截图、默认应用页、卸载。
- **验收标准：**
  - Offline/Lite 各 PASS。
  - 无缺 DLL/模型。
  - 报告含 VM 镜像版本和包 hash。
- **完成证据：** `[<commit>] T0241` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0242 — 执行升级、修复、降级矩阵

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0236,T0241
- **修改/新增文件：** tests/e2e/msi_lifecycle.ps1；docs/test-results/msi-lifecycle.md
- **执行步骤：**
  1. 旧版→新版、repair、重复安装、取消安装、进程占用、尝试降级。
  2. 检查 DB/设置/关联。
- **验收标准：**
  - 允许路径保留数据。
  - 禁止降级有清晰提示。
  - 无半安装/残留 worker。
- **完成证据：** `[<commit>] T0242` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0243 — 签署 Windows 安装阶段门

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0219-T0242
- **修改/新增文件：** docs/phase-gates/P8-windows-package-report.md；docs/implementation-status.md
- **执行步骤：**
  1. 汇总文件关联、默认应用、安装生命周期、签名、许可证、离线 smoke。
  2. 所有构件记录 SHA-256。
- **验收标准：**
  - 两个 MSI 可发布候选。
  - 无管理员可用。
  - 不绕过系统默认应用。
  - 进入最终 QA。
- **完成证据：** `[<commit>] T0243` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 阶段 9：全量 QA、性能、安全、合规与发布候选

**阶段目标：** 对所有功能和故障做系统性回归，生成可审计证据，并只在发布门禁全部通过后产出正式版本。  
**阶段门：** T0244–T0272 完成；发布门禁无未豁免 P0/P1 缺陷，正式构件、源码、SBOM、测试报告和发布说明齐全。

### T0244 — 冻结可重建媒体 fixture 清单

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0243
- **修改/新增文件：** tests/fixtures/media/manifest.json；tools/generate-media-fixtures.ps1
- **执行步骤：**
  1. 用 FFmpeg 生成容器/codec/音轨/字幕/HDR/坏文件样例。
  2. 记录命令/hash/许可。
  3. 不提交无来源素材。
- **验收标准：**
  - 删除 fixtures 后脚本可重建。
  - hash 一致或有版本化差异说明。
  - 清单覆盖测试矩阵。
- **完成证据：** `[<commit>] T0244` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0245 — 执行格式与 codec 矩阵

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0244
- **修改/新增文件：** tests/e2e/media_matrix.ps1；docs/test-results/media-matrix.md
- **执行步骤：**
  1. 逐容器/codec 打开、播放30秒等固定区间、seek、截图、音轨/字幕。
  2. 记录实际 hwdec。
- **验收标准：**
  - 所有必需格式 PASS。
  - 不支持组合有明确已知限制而非崩溃。
- **完成证据：** `[<commit>] T0245` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0246 — 执行路径与文件系统矩阵

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0244
- **修改/新增文件：** tests/e2e/path_matrix.ps1；docs/test-results/path-matrix.md
- **执行步骤：**
  1. 本地 SSD/HDD、UNC、映射盘、rclone full、中文、emoji、空格、长路径、只读目录。
- **验收标准：**
  - 打开/历史/截图/导出行为符合。
  - 完整路径不泄露日志。
  - 长路径无截断。
- **完成证据：** `[<commit>] T0246` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0247 — 执行 seek storm 压力测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0193,T0244
- **修改/新增文件：** tests/stress/seek_storm.ps1；docs/test-results/seek-storm.md
- **执行步骤：**
  1. 自动随机 seek ≥固定次数。
  2. 同时字幕开启、2x、偶尔切轨。
  3. 记录 generation/崩溃/内存。
- **验收标准：**
  - 零旧代字幕、零死锁。
  - 内存不持续增长。
  - 最终位置字幕正确。
- **完成证据：** `[<commit>] T0247` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0248 — 执行 0.5–4x 倍速压力测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0193,T0244
- **修改/新增文件：** tests/stress/speed_matrix.ps1；docs/test-results/speed-matrix.md
- **执行步骤：**
  1. 每档速度运行固定媒体区间。
  2. 测 recognized lag、音调人工抽样、降级事件。
- **验收标准：**
  - 推荐硬件符合 lag 门槛。
  - 低配按规则降级。
  - 时间戳/SRT 不变。
- **完成证据：** `[<commit>] T0248` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0249 — 执行多音轨连续切换测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0169,T0244
- **修改/新增文件：** tests/stress/audio_track_switch.ps1；docs/test-results/audio-track-switch.md
- **执行步骤：**
  1. 在多轨 MKV 中连续切换。
  2. 核对 ff-index、PCM hash、session key、字幕语言。
- **验收标准：**
  - 绝不混轨。
  - 切回缓存正确。
  - 错误轨道可恢复。
- **完成证据：** `[<commit>] T0249` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0250 — 执行 worker kill/crash/restart 压力测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0171,T0244
- **修改/新增文件：** tests/stress/worker_restart.ps1；docs/test-results/worker-restart.md
- **执行步骤：**
  1. 在加载模型、识别、seek、导出前后杀进程。
  2. 验证3次规则和孤儿进程。
- **验收标准：**
  - 视频继续。
  - 字幕状态清楚。
  - 无 DB 损坏。
  - 超过次数停止重启。
- **完成证据：** `[<commit>] T0250` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0251 — 执行模型缺失/损坏/版本不匹配测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0209,T0244
- **修改/新增文件：** tests/e2e/model_faults.ps1；docs/test-results/model-faults.md
- **执行步骤：**
  1. 删除、截断、替换模型。
  2. 改 manifest 兼容版本。
  3. 启动和运行中检测。
- **验收标准：**
  - 基础播放始终可用。
  - 字幕禁用且错误具体。
  - 修复安装恢复。
- **完成证据：** `[<commit>] T0251` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0252 — 执行磁盘满与只读测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0189,T0097,T0244
- **修改/新增文件：** tests/e2e/storage_faults.ps1；docs/test-results/storage-faults.md
- **执行步骤：**
  1. 限制日志/DB/截图/导出所在卷。
  2. 触发写失败。
  3. 恢复空间重试。
- **验收标准：**
  - 无崩溃/半文件。
  - 日志轮转不耗尽盘。
  - 用户可换路径。
- **完成证据：** `[<commit>] T0252` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0253 — 执行数据库损坏与迁移失败恢复

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0091,T0175
- **修改/新增文件：** tests/e2e/database_recovery.ps1；docs/test-results/database-recovery.md
- **执行步骤：**
  1. 截断 DB、坏 WAL、注入迁移失败。
  2. 验证备份、新库、基础播放。
- **验收标准：**
  - 原 DB 保留 recovery。
  - 不无限启动失败。
  - 字幕/历史降级说明清晰。
- **完成证据：** `[<commit>] T0253` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0254 — 执行 rclone/网络中断测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0019,T0211
- **修改/新增文件：** tests/e2e/rclone_network_faults.ps1；docs/test-results/rclone-network.md
- **执行步骤：**
  1. VFS full 播放时断网、seek、恢复。
  2. 缓存命中/未命中两种。
  3. worker 双开。
- **验收标准：**
  - 已缓存区可继续的能力按实际记录。
  - 恢复无崩溃。
  - 不声称 writes 等价。
- **完成证据：** `[<commit>] T0254` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0255 — 执行 DPI、多屏与全屏回归

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0215,T0244
- **修改/新增文件：** tests/e2e/display_matrix.md；docs/test-results/display-matrix.md
- **执行步骤：**
  1. 不同缩放、分辨率、HDR/SDR、主副屏。
  2. 拖窗口、全屏、截图、字幕位置。
- **验收标准：**
  - 无裁切/黑屏。
  - 截图和 overlay 对齐。
  - 窗口恢复合理。
- **完成证据：** `[<commit>] T0255` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0256 — 执行睡眠、锁屏、快速用户切换回归

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0083,T0244
- **修改/新增文件：** tests/e2e/power_session.md；docs/test-results/power-session.md
- **执行步骤：**
  1. 播放/识别时睡眠唤醒、锁屏、远程桌面切换。
  2. 检查 mpv/worker/IPC。
- **验收标准：**
  - 恢复可用或给重试。
  - 无孤儿/重复音频。
  - 历史位置保存。
- **完成证据：** `[<commit>] T0256` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0257 — 执行长时间稳定性与泄漏测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0244
- **修改/新增文件：** tests/stress/soak.ps1；docs/test-results/soak.md
- **执行步骤：**
  1. 循环播放、seek、字幕、切文件、worker 重启。
  2. 定期采 RSS/handles/threads/queue。
- **验收标准：**
  - 指标无持续线性增长。
  - 崩溃为0。
  - 阈值和原始 CSV 附报告。
- **完成证据：** `[<commit>] T0257` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0258 — 执行性能基准与回归比较

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0160,T0244
- **修改/新增文件：** tools/perf-benchmark.ps1；docs/benchmarks/mvp-performance.md
- **执行步骤：**
  1. 测启动、首帧、seek、partial/final、RTF、CPU/RSS、4x lag。
  2. 与 P0/上次结果比较。
- **验收标准：**
  - 所有质量目标有数值。
  - 回退超阈值 CI/门禁失败。
  - 原始数据可下载。
- **完成证据：** `[<commit>] T0258` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0259 — 加入 GUI 卡顿探针

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0195
- **修改/新增文件：** src/diagnostics/UiResponsivenessMonitor.h；src/diagnostics/UiResponsivenessMonitor.cpp；tests/stress/ui_responsiveness.ps1
- **执行步骤：**
  1. 开发/测试构建检测 GUI event loop >100ms。
  2. 记录调用上下文/阶段。
  3. 正式版仅低成本统计或关闭。
- **验收标准：**
  - 压力测试无可重复超标。
  - 发现时可定位模型/DB/扫描误入 GUI。
- **完成证据：** `[<commit>] T0259` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0260 — 执行离线网络审计

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0213,T0241
- **修改/新增文件：** tests/security/network_audit.ps1；docs/test-results/network-audit.md
- **执行步骤：**
  1. 断网与联网但阻断业务域两种。
  2. 抓取 player/worker socket/DNS。
  3. 覆盖启动、识别、导出。
- **验收标准：**
  - 无未解释外连。
  - 报告列出系统/第三方不可归因流量的排除方法。
  - 默认 allow_network=false。
- **完成证据：** `[<commit>] T0260` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0261 — 执行 DLL 劫持与加载路径测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0230
- **修改/新增文件：** tests/security/dll_hijack.ps1；docs/test-results/dll-loading.md
- **执行步骤：**
  1. 在工作目录放同名假 DLL。
  2. 从下载目录/UNC 启动。
  3. 检查实际模块路径。
- **验收标准：**
  - 只加载安装目录/系统安全目录。
  - 失败安全，不执行假 DLL。
- **完成证据：** `[<commit>] T0261` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0262 — 执行恶意 IPC 与资源耗尽测试

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0132
- **修改/新增文件：** tests/security/ipc_abuse.ps1；docs/test-results/ipc-security.md
- **执行步骤：**
  1. 坏 token、超大帧、深 JSON、消息洪水、断连、伪 generation。
  2. 监控 CPU/RSS。
- **验收标准：**
  - worker/client 不崩。
  - 连接被限速/断开。
  - 内存有界。
  - 日志不爆盘。
- **完成证据：** `[<commit>] T0262` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0263 — 审计日志和诊断脱敏

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0212
- **修改/新增文件：** tests/security/redaction_audit.ps1；docs/test-results/redaction-audit.md
- **执行步骤：**
  1. 用含姓名/路径/token/字幕敏感样例运行。
  2. 扫描日志、dump metadata、诊断 zip。
- **验收标准：**
  - 默认产物无完整路径/字幕/token。
  - 用户显式开关行为符合说明。
  - 发现项修复。
- **完成证据：** `[<commit>] T0263` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0264 — 完成许可证与依赖合规审计

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0232,T0037
- **修改/新增文件：** docs/release/license-audit.md；packaging/licenses/；dependencies.lock.json
- **执行步骤：**
  1. 逐依赖核对来源、版本、hash、build flags、license/NOTICE、模型许可。
  2. 确认 GPL 源码提供方式。
- **验收标准：**
  - 无 unknown/missing。
  - 安装包与源码包文本一致。
  - 审计签字/日期完整。
- **完成证据：** `[<commit>] T0264` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0265 — 生成可复现源码发布包

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0264,T0240
- **修改/新增文件：** tools/package-source.ps1；docs/release/source-build.md
- **执行步骤：**
  1. 包含项目源码、补丁、lock、构建脚本、必要第三方源码获取/构建信息。
  2. 不含 secrets/用户数据。
- **验收标准：**
  - 在干净环境按文档能构建。
  - 源码包 hash 记录。
  - 满足 GPL 对应源码路线。
- **完成证据：** `[<commit>] T0265` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0266 — 编写用户发布说明

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0245-T0265
- **修改/新增文件：** docs/release/release-notes.md；docs/user/quick-start.md
- **执行步骤：**
  1. 说明核心功能、硬件、离线、模型档位、默认应用步骤、rclone full、已知限制和隐私。
- **验收标准：**
  - 文案与实际 UI/包名一致。
  - 不引用未经验证 CER 宣传数字。
  - 新用户可完成首次字幕。
- **完成证据：** `[<commit>] T0266` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0267 — 编写已知限制与故障排查

- [ ] **状态：未完成**
- **优先级：** P1
- **前置任务：** T0245-T0265
- **修改/新增文件：** docs/user/troubleshooting.md；docs/release/known-issues.md
- **执行步骤：**
  1. 覆盖低配降级、重口音、网盘缓存、HDR、不可 seek、模型损坏、默认应用、日志位置。
- **验收标准：**
  - 每项有可执行步骤。
  - 不建议危险删除活动缓存。
  - 错误码可检索。
- **完成证据：** `[<commit>] T0267` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0268 — 完成 PRD→实现→测试追踪矩阵

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0245-T0267
- **修改/新增文件：** docs/release/traceability-matrix.md
- **执行步骤：**
  1. 每条 MVP 需求链接模块、TODO、自动测试、人工报告和构件。
  2. 标明后续项/不做项。
- **验收标准：**
  - 无孤立 MVP 需求。
  - 随机抽 10 条可从 PRD 追到证据。
  - 矩阵版本固定。
- **完成证据：** `[<commit>] T0268` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0269 — 完成缺陷分级与发布豁免审查

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0245-T0268
- **修改/新增文件：** docs/release/defect-review.md
- **执行步骤：**
  1. 列出所有开放缺陷、严重度、复现、影响、规避、负责人。
  2. P0/P1 默认不可豁免。
  3. 豁免需明确签字。
- **验收标准：**
  - 无“偶现/待观察”无证据描述。
  - 阻塞缺陷为0。
  - 允许项进入 known issues。
- **完成证据：** `[<commit>] T0269` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0270 — 执行 Release Candidate 干净机总验收

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0245-T0269
- **修改/新增文件：** tests/e2e/rc_acceptance.ps1；docs/test-results/rc-acceptance.md
- **执行步骤：**
  1. 使用最终签名包和 hash。
  2. 全新用户、断网、默认应用、播放、字幕、seek、导出、升级、卸载。
- **验收标准：**
  - 脚本/人工步骤全部 PASS。
  - 使用的构件与拟发布字节一致。
  - 无临时替换 DLL/模型。
- **完成证据：** `[<commit>] T0270` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0271 — 签署最终发布门禁

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0257-T0270
- **修改/新增文件：** docs/phase-gates/P9-release-gate.md；docs/implementation-status.md
- **执行步骤：**
  1. 逐条检查技术方案发布门禁、质量目标、隐私、许可、签名、测试。
  2. 记录批准人/日期/构件 hash。
- **验收标准：**
  - 所有必需项 PASS。
  - 无未豁免 P0/P1。
  - 状态标记 Release Ready。
- **完成证据：** `[<commit>] T0271` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0272 — 生成标签、release manifest 与正式构件索引

- [ ] **状态：未完成**
- **优先级：** P0
- **前置任务：** T0271
- **修改/新增文件：** release/manifest.json；docs/release/final-artifacts.md；tools/verify-release.ps1
- **执行步骤：**
  1. 记录 Git tag/commit、两个 MSI、源码包、SBOM、PDB/符号、hash、签名时间、模型 hash、报告索引。
- **验收标准：**
  - verify-release 在独立目录验证全部 hash/签名。
  - 文档链接无缺失。
  - 不得在验证后重打包。
- **完成证据：** `[<commit>] T0272` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


## 后续迭代候选（不属于 MVP 发布门禁）

**阶段目标：** 只在 T0272 完成后评估；每项都必须先更新 PRD、架构和许可/隐私评审。  
**阶段门：** 不纳入 MVP。任何任务开始前先创建独立里程碑与 ADR。

### T0500 — macOS 渲染与安装适配

- [ ] **状态：未完成**
- **优先级：** P2
- **前置任务：** T0272
- **修改/新增文件：** src/platform/macos/；packaging/macos/
- **执行步骤：**
  1. 实现 IPlatformIntegration、libmpv 渲染适配、Info.plist 文档类型、签名公证。
  2. 复用 worker/DB/ASR。
- **验收标准：**
  - macOS 支持矩阵与安装包通过。
  - 不得破坏 Windows。
- **完成证据：** `[<commit>] T0500` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0501 — 云端 ASR backend

- [ ] **状态：未完成**
- **优先级：** P2
- **前置任务：** T0272
- **修改/新增文件：** src/worker/cloud/；src/ui/settings/CloudSettingsPage.*
- **执行步骤：**
  1. 先做隐私/密钥/费用 ADR。
  2. 实现显式 opt-in backend。
  3. 本地模式默认不变。
- **验收标准：**
  - 断网和关闭云时零请求。
  - 密钥安全存储。
  - 费用/数据去向明确。
- **完成证据：** `[<commit>] T0501` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0502 — 说话人分离

- [ ] **状态：未完成**
- **优先级：** P2
- **前置任务：** T0272
- **修改/新增文件：** src/worker/diarization/；src/captions/
- **执行步骤：**
  1. 增加 IDiarizationBackend、离线模型、片段 speaker label、SRT/ASS 输出策略。
- **验收标准：**
  - 延迟/内存/准确率基准通过。
  - 无假 speaker 标签。
- **完成证据：** `[<commit>] T0502` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0503 — 可检索文稿与 Markdown/TXT/VTT 导出

- [ ] **状态：未完成**
- **优先级：** P2
- **前置任务：** T0272
- **修改/新增文件：** src/transcript/；src/captions/exporters/
- **执行步骤：**
  1. 增加全文搜索索引、时间跳转、导出接口。
  2. 数据库迁移。
- **验收标准：**
  - 大文稿搜索性能和迁移通过。
  - 原 SRT 行为不变。
- **完成证据：** `[<commit>] T0503` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0504 — 方言专项模型包

- [ ] **状态：未完成**
- **优先级：** P2
- **前置任务：** T0272
- **修改/新增文件：** resources/models/；src/worker/LanguagePolicy.*
- **执行步骤：**
  1. 建立方言语料、manifest、自动/手动语言策略和模型安装方式。
- **验收标准：**
  - 每种方言有独立基准与限制说明。
  - 不以普通话指标替代。
- **完成证据：** `[<commit>] T0504` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。

### T0505 — 受控 rclone 挂载管理与安全清缓存

- [ ] **状态：未完成**
- **优先级：** P2
- **前置任务：** T0272
- **修改/新增文件：** src/rclone/；src/ui/RcloneManager.*
- **执行步骤：**
  1. 仅管理本应用创建的 mount。
  2. 停止句柄→停止 rclone→验证目录→清理→重启。
  3. 完整回滚。
- **验收标准：**
  - 活动文件不被直接删除。
  - 错误可恢复。
  - 用户凭据不进入日志。
- **完成证据：** `[<commit>] T0505` 提交、相关测试输出，以及状态表记录；UI/性能/安装任务还要附对应截图、原始数据或构件 hash。


---

## 二、最终总检查

T0272 完成前，发布负责人必须逐项确认：

- [ ] PRD 中全部 MVP 需求都出现在追踪矩阵中；
- [ ] 技术方案中所有“必须”条款都有实现或测试证据；
- [ ] Offline/Lite 安装包、源码包、SBOM、PDB、模型 hash 和 release manifest 齐全；
- [ ] Windows 默认应用流程没有写入或破解 `UserChoice`；
- [ ] rclone 文档只把 `full` 作为视频读缓存推荐值；
- [ ] SenseVoice 没有被错误用作原生 partial 流模型；
- [ ] seek、换轨、worker 重启后没有旧 generation 字幕；
- [ ] partial 从不进入最终 SRT；
- [ ] 离线模式无未解释的业务网络请求；
- [ ] 日志、dump metadata、诊断包不泄露默认禁止的数据；
- [ ] GPL 源码获取、NOTICE、第三方许可证和构建参数完整；
- [ ] 无未豁免 P0/P1 缺陷；
- [ ] 最终验证使用的字节与发布构件完全一致。

---

*不要按“代码写完了”判断项目完成；只有任务验收、阶段门、最终发布门禁和可复现证据全部完成，项目才算交付。*
