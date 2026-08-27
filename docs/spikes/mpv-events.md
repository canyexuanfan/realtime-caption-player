# T0015 观察文档：libmpv 事件桥与属性观察

**探针**：`spikes/mpv-render/MpvEventProbe.cpp`（构建目标 `mpv_event_probe`，链接 `rcp::mpv` → `mpv-2.dll`）

**构建验证**：`BUILD_SPIKES=ON` 下 cmake/Ninja 真实构建通过（`BUILD_EXIT=0`），`dumpbin /dependents` 确认导入 `mpv-2.dll`。

## 验证了什么（编译/链接层）

1. **事件桥可用性**：`mpv_create` + `mpv_set_wakeup_callback(ctx, cb, d)` 能编译链接；回调签名 `(void(*)(void*))` 与头文件一致。
2. **属性观察 API**：`mpv_observe_property(ctx, userdata, name, format)` 对 `time-pos`/`duration`(DOUBLE) 与 `pause`(FLAG) 链接通过；`mpv_unobserve_property(ctx, userdata)` 配对可用。
3. **事件循环**：`mpv_wait_event(ctx, timeout)` + `mpv_event` 结构解包（`event_id`/`reply_userdata`/`data`）编译通过；对 `MPV_EVENT_SHUTDOWN`/`FILE_LOADED`/`PROPERTY_CHANGE`/`LOG_MESSAGE` 分支处理。
4. **结构体字段正确**：`mpv_event_property{name,format,data}`、`mpv_event_log_message{prefix,level,text}` 字段访问通过编译（flag→`int`、double→`double`）。

## 预期运行行为（真机，需显示器 + 视频文件）

```
mpv_event_probe.exe <video.mkv>
```

- 打印 `client API 0x...`；
- 调用 `mpv_initialize`（真机成功，沙箱无显示返回 <0，属预期）；
- 加载文件后周期性收到 `MPV_EVENT_PROPERTY_CHANGE`，`time-pos` 递增、`duration` 为总长、`pause` 反映暂停态；
- `MPV_EVENT_LOG_MESSAGE` 输出 mpv 内部日志；
- 结束后 `wakeups` 计数 >0，证明 wakeup 回调确实被触发（事件桥工作）。

## 真机回填清单（运行时验收，标 partial）

| 项 | 如何验证 | 预期 | 结果 |
|---|---|---|---|
| wakeup 回调触发 | 运行时看 `wakeups=` 计数 | >0 | ⏳ 待真机 |
| time-pos 实时更新 | 加载视频后观察 `[prop] time-pos` | 随时间递增 | ⏳ 待真机 |
| duration 正确 | 对比媒体元数据 | 与文件时长一致 | ⏳ 待真机 |
| pause 属性 | 暂停后观察 | 变成 1 | ⏳ 待真机 |
| LOG_MESSAGE | 观察 `[log:]` 行 | 有 mpv 内部日志 | ⏳ 待真机 |

> 注：本沙箱无显示器/GPU，无法真正初始化播放上下文，故运行时项均标 partial，由用户在真实 Windows 上回填。
