# P0 阶段门报告 — Realtime Caption Player

> 阶段目标（来自 TODO）：消除最可能导致全盘返工的许可、渲染、字幕截图、音轨映射、网盘双开和 ASR 流式路线风险。
> 本报告汇总 T0003–T0023 证据。**签署（关闭 P0）需用户在真机回填运行时验证证据**（见 §4）。

## 1. 环境约束与验证口径

| 验证层 | 本环境是否可做 | 说明 |
|---|---|---|
| 编译（cmake/Ninja, MSVC） | ✅ | 已用真实构建系统验证 |
| 链接（libmpv / FFmpeg / sherpa-onnx 三套原生库） | ✅ | `dumpbin /dependents` 确认导入 `mpv-2.dll`/`avformat-63.dll`/`sherpa-onnx-c-api.dll` |
| 纯逻辑单元测试（rcp_core） | ✅ | 21/21 ctest PASS（T0001–T0009） |
| 无显示器的纯解码/推理 smoke | ✅（部分） | T0017/T0018 真机式运行通过；T0020–22 因模型/ORT 版本错配无法加载 |
| GUI 渲染 / 真机视频播放 / 字幕截图 / 真实语音 ASR | ❌ | 沙箱无显示器、GPU、授权语料、rclone 挂载 |

**铁律**：任何数据不支持时不写"通过"。运行时项一律标 `PARTIAL`（待真机回填），未伪造完成。

## 2. 任务证据表

### 地基（T0001–T0013）
| 任务 | 状态 | 验证 |
|---|---|---|
| T0001–T0009 rcp_core 纯逻辑库 + 21 单元测试 | DONE | ctest 21/21 PASS |
| T0010–T0013 原生依赖（FFmpeg/libmpv/sherpa-onnx）+ ASR 模型权重锁定进 `.tools/` | DONE | deps_smoke 真实构建；依赖链含三库 DLL；模型已下载 |
| 自包含打包链路（RcpBundle → WiX MSI，用户侧零下载） | DONE | bundle 脚本验证（仅拷 `.dll`+模型，跳过 `.lib/.def/.part/.bad`） |

### 播放内核与渲染（T0014–T0016）
| 任务 | 状态 | 验证 |
|---|---|---|
| T0014 libmpv 封装 `src/player/`（`BUILD_PLAYER`） | DONE | 真实构建 `player_app.exe`，导入 `mpv-2.dll`+`Qt6Core.dll` |
| T0015 libmpv 事件桥/属性观察 | DONE | `mpv_event_probe` 构建+链接 `mpv-2.dll` |
| T0016 ASS overlay + 带字幕截图 | DONE（code） | `overlay_probe` 构建+链接 OK；headless 运行 RUN_EXIT=0（跳过无 GL 上下文）；**渲染出图需真机** → PARTIAL |

### 音频映射与双开（T0017–T0019）
| 任务 | 状态 | 验证 |
|---|---|---|
| T0017 指定音轨解码 | DONE | `audio_decode_probe` 真机式运行：两音轨解码出不同 checksum（3395679301 vs 3349846617） |
| T0018 seek + 采样时钟 | DONE | `seek_probe` 真机式运行：target 2.000s → 首帧 pts 2.005s（Δ5ms），sample_offset 自洽 |
| T0019 本地 + rclone 双开 | DONE（code） | `double_open_probe` 真机式运行：FFmpeg+mpv 同开同一文件双开=YES；**rclone full 缓存/断网行为需真机挂载** → PARTIAL |

### ASR 流式链路（T0020–T0022）
| 任务 | 状态 | 验证 |
|---|---|---|
| T0020 Online Paraformer partial | DONE（code） | `online_probe` 编译+链接 OK；**运行时因模型/ORT 版本错配阻塞**（见 §3） → PARTIAL |
| T0021 Silero VAD 分句 + SenseVoice 终稿 | DONE（code） | `sensevoice_probe` 编译+链接 OK；API 调用正确（修正 VAD detector / 离线 Accept / SenseVoice 离线模型）；**运行时同 §3 阻塞** → PARTIAL |
| T0022 Hybrid 融合（VAD×Paraformer→SRT） | DONE（code） | `hybrid_probe` 编译+链接 OK；**运行时同 §3 阻塞** → PARTIAL |

### 基准与门报告（T0023–T0024）
| 任务 | 状态 | 验证 |
|---|---|---|
| T0023 合规语料 + **FunASR** 基线（原案 whisper 已禁用） | DONE（framework） | `benchmark-asr.py` py_compile OK、CER/WER 单测正确、FunASR 缺失时优雅退出不伪造；**真机评测需 FunASR+授权语料** → PARTIAL |
| T0024 P0 阶段门报告 | DONE（草案+证据表） | 本文档；**签署需真机运行时证据回填** → PARTIAL |

## 3. 关键阻塞项（必须解决才能关闭 P0）

### 🔴 BLOCKER-1：模型与 onnxruntime 版本错配（影响 T0020–T0022 运行时）
- 现象：加载 `.tools/models/` 中 Paraformer/SenseVoice/Silero 时，
  `The given version [27] is not supported, only version 1 to 10 is supported in this build.` 并崩溃（exit 139）。
- 根因：bundled **sherpa-onnx 1.13.6** 的 onnxruntime 不支持模型所用的 opset 27；模型与 ORT 版本不匹配。
- 非代码缺陷（探针已正确调用 API）。
- **修复方向（二选一，需用户决策）**：
  1. 升级 sherpa-onnx + onnxruntime 至支持 opset 17+ 的版本（推荐，与模型库配套）；**或**
  2. 重新下载与 sherpa-onnx 1.13.6 配套的 Paraformer/SenseVoice/Silero 模型（opset ≤ 该 ORT 支持）。

### 🟡 PARTIAL-2：真机运行时验证清单（须用户回填）
- [ ] T0016：真机带 GL 加载器渲染视频并叠加 ASS 字幕、出截图
- [ ] T0019：rclone mount + 断网下 VFS full 随机 seek 恢复
- [ ] T0020–22：版本对齐后用真实语音跑 partial / VAD 分句 / Hybrid SRT
- [ ] T0023：真机 FunASR 安装 + 授权语料跑出 CER/WER/RTF 数字

## 4. 签署条件（Sign-off）
满足以下全部方关闭 P0：
1. BLOCKER-1 修复（模型/ORT 版本对齐）；
2. §3 PARTIAL-2 全部项在真机回填证据（截图 / 日志 / CER-WER 数字 / RTF）；
3. 用户在本报告签字确认。

**当前状态：P0 代码与编译/链接验证全部完成；运行时验证因环境约束 + 模型版本错配未关闭，待用户真机回填。**
