# libmpv 播放停滞/字幕叠加失败排查指南

## 问题描述

player_app 真机运行时：mpv 打开媒体后时间停在 0.00，`time-pos` 事件不再更新，
画面/播放停滞；同时 mpv 日志刷 `osd-overlay` 参数错误与
`after creating texture: OpenGL error INVALID_ENUM`。

## 已尝试的修复方法及失败原因

- ❌ 怀疑 FFmpeg/CPU 饿死：Get-Counter 实测系统 CPU 5.9%，worker 早已空闲，排除。
- ❌ 怀疑 ANGLE/GLES 上下文：`QT_OPENGL=desktop` 无效（实际本就是 desktop GL 4.6）。
- ❌ 怀疑 Core profile：诊断输出 profile=2 实为 **CompatibilityProfile**
  （枚举：NoProfile=0 / CoreProfile=1 / CompatibilityProfile=2），误读，排除。
- ⚠️ SurfaceFormat 设 CompatibilityProfile + 2.0：无害但非本问题根因，保留（防御性）。

## 深层问题分析（三个独立缺陷叠加）

### ✅ 缺陷 1（根因）：MPV_FORMAT_FLAG 传错指针类型

```cpp
const char* val = "no";
mpv_set_property(m_handle, "pause", MPV_FORMAT_FLAG, &val);   // 错！
```
FLAG 格式要求指向 **int（0/1）** 的指针；传 `char*` 后 mpv 把指针变量前 4 字节
当作 int 解引用——地址低位非零 ⇒ **pause=true**。`play()` 实际效果是暂停！
mpv verbose 日志的决定性证据：
`playback restart complete @ 0.000000, audio=ready, video=playing (paused)`。
修复：`int flag = 0; mpv_set_property(..., &flag);`（pause/setMuted 同型修复）。
属性事件读取端 `*static_cast<bool*>(pc->data)` 也改为读 int 再比较。

### ✅ 缺陷 2：osd-overlay 命令语法错误

mpv 手册语法：`osd-overlay <id:int> <format> <data>`——**没有 add/remove 子命令**；
同 id 重发即原位更新，移除用 `format=none`。原代码传 `"add"/"remove"` 字符串当 id，
全部报 `The osd-overlay option must be an integer`。修复：
- 显示：`{"osd-overlay", "1", "ass-events", data, nullptr}`
- 清除：`{"osd-overlay", "1", "none", "", nullptr}`

### ⚠️ 缺陷 3（非致命，已知问题）：`after creating texture: OpenGL error INVALID_ENUM`

Intel Iris Xe（GL 4.6 compat）下 mpv 建视频纹理时偶报一次，渲染器随后继续
（FBO rgba16f 测试通过），**不阻塞播放**。可与 `--gpu-dumb-mode=yes` 或
`--fbo-format=rgba16` 实验规避；播放已验证正常，列为已知小问题。

## 调试工具（本次沉淀的可复用手段）

- `RCP_TRACE=<文件>` 环境变量门控的 trace（`src/player/MpvTrace.h`，inline 单实例）：
  time-pos 事件流 / pause 状态 / end-file 原因 / mpv v 级日志 / GL 上下文信息。
- `mpv_request_log_messages(m_handle, "v")`：verbose 日志是定位
  "playing (paused)" 这类语义细节的关键。
- 注意 trace 文件必须**单实例句柄**：两个 TU 各自 static QFile 写同一文件会互相覆盖
  （行被截断、内容丢失）。inline 函数的局部 static 保证唯一实例。

## 验证结果（2026-08-28 真机）

- 26s 内 613 个 time-pos 事件推进至 21.43s，`end-file reason=0`（正常 EOF）。
- osd-overlay 报错 0 次（叠加命令全部成功）。
- a11y 树确认完整参考 UI：播放列表/转写面板（4 句字幕+时间戳）/ASR 状态卡
  （引擎/语言/模式/运行中）/控制台/隐私徽标。

## 注意事项

- libmpv 属性 API 的格式参数类型是 ABI 约定，编译器查不出来：
  FLAG=int*、DOUBLE=double*、INT64=int64_t*。字符串版
  `mpv_set_property_string` 更稳，flag 用 `"yes"/"no"` 字符串即可。
- 真机排错顺序建议：先看 mpv verbose 里 `playback restart complete` 行的
  audio/video 状态，再查 GL。

## 更新记录

- 2026-08-28 首次记录：MPV_FORMAT_FLAG 误用 + osd-overlay 语法修复，
  真机完整播放验证通过；INVALID_ENUM 记为已知非致命问题。

## ✅ 追加（黑屏最终根治，2026-08-29 深夜）

用户报"视频没有画面、没有实时字幕"。快照自验证 + PID trace 层层剥出**五个叠加缺陷**：

1. **VO 竞态（黑屏主因）**：show() 后立即 loadfile，渲染上下文未创建 →
   `mpv[fatal] No render context set / Video: no video` → 纯音频黑屏，时钟正常走。
   修复：argv/恢复会话的打开延迟 1.5s/1.6s（窗口首次绘制后）。
2. **resume 顶替**：恢复会话 loadfile(X:/网盘文件) 用 replace 顶掉显式媒体。
   修复：m_currentPath 非空时跳过恢复。
3. **构造函数同步 exists(X:/) 阻塞 20s**：恢复检查 QFile::exists 打在离线
   rclone 挂载路径上。修复：删同步检查，失败交给播放器。
4. **INVALID_ENUM 纹理创建失败（Intel 驱动兼容）**：高级渲染管线建视频纹理
   报 INVALID_ENUM → 帧永不显示。修复：`gpu-dumb-mode=yes`（固定 rgba8 管线），
   实测该错误归零。
5. **同步 loadfile 阻塞 GUI 19.5s**：mpv_command(loadfile) 打开网盘源时阻塞
   主线程。修复：`mpv_command_async`。

**附带发现并修复**：UI 重写时 startCaptioningFor 调用丢失 → worker 从未启动
（字幕"未运行"直接原因）。

**验证工具升级**：rcpTrace 改为每次调用独立开关文件（Append+PID）——inline
静态 QFile 在 MSVC 多编译单元下行为不确定，trace 行随机丢失（三轮误诊根源）；
新增 rcpMark（C 标准库直写标记）定位构造路径卡点。

**遗留**：RCP_SNAPSHOT 模式 16s 的 .b 快照偶发缺失（grab/save 偶发失败），
自检手段问题非产品问题，待查。

验证：无 trace 纯净运行 479 个 time-pos 事件持续流动、INVALID_ENUM=0、
worker 运行中、用户界面交互（暂停/恢复）正常响应。

## ✅ 追加 2（画面+字幕端到端根治，2026-08-29 深夜 II）

上轮五缺陷修复后 mpv 内部已正常播放（time-pos 流动）但**画面仍黑**。
对照官方 mpv-examples/libmpv/qt_opengl 逐行比对，剥出三个新根因：

### 根因 6（黑屏真正主因）：paintGL 把帧画进 FBO 0

QOpenGLWidget 的绘制目标是 **Qt 内部 FBO**（Qt 之后把该纹理合成到屏幕），
`mpv_opengl_fbo.fbo = 0` 会画进 0 号默认帧缓冲——该内容被 Qt 合成流程
完全丢弃 → mpv 明明画了、屏幕永远黑。官方示例：

```cpp
mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()), ...};
```

修复时一并传**物理像素**尺寸（`width()*devicePixelRatioF()`，FBO 含 DPR 缩放）。
教训：`GL_FRAMEBUFFER_BINDING=0` 的像素诊断结果把排查引向"绑定正常"歧路——
QOpenGLWidget 下 0 号绑定本身就是错误信号。

### 根因 7（重绘链脆弱）：invokeMethod(w,"update") → 官方 maybeUpdate 模式

改用官方 `on_update → invokeMethod("maybeUpdate")` + maybeUpdate 槽：
窗口最小化时 QWidget::update() 被 Qt 跳过，mpv render API 会因渲染超时
产生间歇卡顿（官方注释原话），此时需手动 makeCurrent+paintGL+swapBuffers 兜底。

### 根因 8（重载风暴）：argv 与 resume 两个打开定时器抢跑双 loadfile

ctor 注册的恢复定时器（1600ms）与 main 注册的 argv 定时器（1500ms）几乎同时
到期；先触发者打开文件，后触发者无条件再 openFile → loadfile replace →
`end-file reason=2`（STOP）→ 停止/重启风暴，mpv 刷
`mpv_render_context_render() not being called or stuck`。
原"m_currentPath 非空跳过"守卫只防得住 argv 先行，防不住 resume 先行。
修复：`setStartupMedia()` 成员——argv 媒体存在时恢复会话**直接让位**
（不是比谁快，而是根本不并发）。

### 工具坑（差点误诊为崩溃）：RCP_SNAPSHOT 残留 2.5s quit 定时器

快照模式里还留着二分时代定时器：2.5s grab 保存后**直接 QApplication::quit()**。
启动慢时它迟到触发 → "trace 突然中断 + 进程消失 + 16s .b 快照缺失"，
Windows 事件日志无崩溃记录（Id=1000 为空）证明不是 crash。已删除该定时器，
快照模式现在干净地 5s 首验 + 16s 复验后退出。

### ✅ 可复用调试手段

- **GUI 心跳**：QTimer(500ms) → trace `hb N`——心跳断流的时间窗即 GUI 线程
  被阻塞的窗口（本轮证明 GUI 没堵，堵的是渲染消费）。
- **调用点标签**：openFile/loadFile 入口 + `open-timer[argv]/[resume]` 标签，
  一次运行即可定位"谁开的文件"。
- **排除崩溃**：`Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000}`
  为空 = 非 WER 崩溃，优先怀疑提前退出/挂起。
- **概念模型**：vo=libmpv 是**拉模型**——客户端不持续调用
  mpv_render_context_render，VO 队列满后整个核心冻结在第一帧（音频、时钟全停）；
  "渲染问题"和"播放停滞"在这种情况下是同一个 bug。

### 最终验证（2026-08-29，真机双快照）

- `vidU.png`(5s)：testsrc2 彩条帧 + 时间码 00:00:01.480 + 字幕叠加
  「欢迎使用实时字幕播放器。」+ 波形 + 进度 00:01/00:21。
- `vidU.b.png`(16s)：时间码 00:00:12.380（持续播放）+ 叠加 partial 长句 +
  final 白字「人工智能正在改变世界，字幕让视频更容易理解。」+
  转写面板 2 条 final + 字幕行数 2 + 实时延迟 0.0s + ASR 运行中。
- trace：**单次** loadFile、无 end-file reason=2、stuck 仅切换瞬间 1 条、
  time-pos 实时推进（12.3s/12.7s 墙钟）、hb 心跳 30+ 连续无断流。
- 全量 ctest 21/21 通过。
