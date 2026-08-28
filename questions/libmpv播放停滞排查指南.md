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
