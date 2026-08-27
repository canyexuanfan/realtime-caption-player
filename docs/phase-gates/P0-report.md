# P0 阶段门报告 — Realtime Caption Player

> 阶段目标（来自 TODO）：消除最可能导致全盘返工的许可、渲染、字幕截图、音轨映射、网盘双开和 ASR 流式路线风险。
> 本报告汇总 T0003–T0023 证据。**签署（关闭 P0）需用户在真机回填运行时验证证据**（见 §4）。

## 1. 环境约束与验证口径

| 验证层 | 本环境是否可做 | 说明 |
|---|---|---|
| 编译（cmake/Ninja, MSVC） | ✅ | 已用真实构建系统验证 |
| 链接（libmpv / FFmpeg / sherpa-onnx 三套原生库） | ✅ | `dumpbin /dependents` 确认导入 `mpv-2.dll`/`avformat-63.dll`/`sherpa-onnx-c-api.dll` |
| 纯逻辑单元测试（rcp_core） | ✅ | 21/21 ctest PASS（T0001–T0009） |
| 无显示器的纯解码/推理 smoke | ✅（部分） | T0017/T0018 真机式运行通过；T0021（VAD+SenseVoice 离线）真机 exit=0 通过；T0020/T0022 因在线 Paraformer×ORT 1.27.1 崩溃无法加载（见 §3 BLOCKER-2） |
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
| T0020 Online Paraformer partial | BLOCKED | `online_probe` 编译+链接 OK；真机在 `SherpaOnnxCreateOnlineRecognizer` 硬崩溃（零 stderr，exit 127），**官方 `sherpa-onnx-vad-with-online-asr.exe` 同模型+正确参数复现**（见 §3 BLOCKER-2） |
| T0021 Silero VAD 分句 + SenseVoice 终稿 | DONE（运行时验证通过） | `sensevoice_probe` 真机 **exit=0**：Silero VAD detector 创建成功、SenseVoice 离线 recognizer 创建成功、离线 decode 链路跑通（合成非语音音频，仅验证管线，非识别准确率）；证明 ORT 1.27.1 在离线路径可用 |
| T0022 Hybrid 融合（VAD×Paraformer→SRT） | BLOCKED | `hybrid_probe` 编译+链接 OK；真机在 `SherpaOnnxCreateOnlineRecognizer`（在线 Paraformer）崩溃，同 BLOCKER-2 |

### 基准与门报告（T0023–T0024）
| 任务 | 状态 | 验证 |
|---|---|---|
| T0023 合规语料 + **FunASR** 基线（原案 whisper 已禁用） | DONE（framework） | `benchmark-asr.py` py_compile OK、CER/WER 单测正确、FunASR 缺失时优雅退出不伪造；**真机评测需 FunASR+授权语料** → PARTIAL |
| T0024 P0 阶段门报告 | DONE（草案+证据表） | 本文档；**签署需真机运行时证据回填** → PARTIAL |

## 3. 关键阻塞项（必须解决才能关闭 P0）

### ✅ BLOCKER-1（已修复，原诊断误诊）
- **原误记**：旧报告称"Bundled ORT 不支持 opset 27，需升级 sherpa-onnx+ORT"。
- **实测更正**：`sherpa-onnx-version.exe` 输出 `sherpa-onnx 1.13.6` / `onnxruntime 1.27.1`（**1.27.1 支持 opset 27**），sherpa-onnx 已是最新版，模型文件完整。所谓"BLOCKER-1"并不存在。
- **真正根因**：`C:\Windows\System32`（版本 `1.10.220126`）与 `C:\Windows\SysWOW64` 存在**过时的 `onnxruntime.dll`（ORT 1.10）**。spike 探针 exe 目录未携带 bundled dll，按 DLL 搜索顺序（exe 目录 → 系统目录 → PATH）加载了系统的 ORT 1.10，该旧版本只支持 opset 1–10，故报 `The given version [27] is not supported ...` 并崩溃（exit 139/127）。此即 sherpa-onnx 官方 FAQ 所述"机器上存在第二个过时 onnxruntime.dll 被优先加载"的情形。
- **修复**：将 bundled 的 4 个 dll（`onnxruntime.dll` / `onnxruntime_providers_shared.dll` / `sherpa-onnx-c-api.dll` / `sherpa-onnx-cxx-api.dll`）拷到探针 exe 旁，利用 exe 目录优先于系统目录的规则 → 探针加载正确的 ORT 1.27.1。修复后 `sensevoice_probe` 真机 **exit=0**，证明 ORT 1.27.1 完全可用、离线路径（VAD + SenseVoice）通畅。
- **对产品的影响**：真实产品的 `RcpBundle`/`WiX`（见 `dependencies.lock.json` 的 `bundled_runtime.dlls`）已把上述 4 个 dll 打进运行时目录（`out/bundle/runtime` + MSI），安装后 exe 目录自带正确 ORT，**产品本身不受 System32 冲突影响**；仅 spike 验证构建需此手动修复。

### 🔴 BLOCKER-2（新，阻断 T0020/T0022）：在线 Paraformer 模型 × ORT 1.27.1(CPU EP) 致命崩溃
- **现象**：`online_probe` / `hybrid_probe` 在 `SherpaOnnxCreateOnlineRecognizer`（加载流式 Paraformer）处硬崩溃，无 stderr，exit 127。
- **官方二进制复现（决定性证据）**：`sherpa-onnx-vad-with-online-asr.exe` 用相同模型 + 正确参数
  `--paraformer-encoder=.tools/models/paraformer/encoder.int8.onnx --paraformer-decoder=.tools/models/paraformer/decoder.onnx --tokens=... --silero-vad-model=... <wav>` 运行，打印 `Creating recognizer ...` 后同样崩溃。说明**不是探针代码缺陷**，而是该模型图 × 当前 ORT 的兼容性问题。
- **排除损坏**：模型尺寸完整（`encoder.int8.onnx` 165,462,184 B / `decoder.onnx` 25,071,627 B / `tokens.txt` 75,756 B）。
- **定位**：SenseVoice（离线）+ Silero VAD 在 ORT 1.27.1 下运行正常 → 缺陷**特定于流式 Paraformer** 模型图；`encoder.int8.onnx` 为 int8 量化模型，最可能触发 ORT 1.27.1 CPU EP 的某个量化算子缺陷（硬访问违规，而非干净的 op 不支持报错）。
- **影响**：真实产品的**实时流式 partial** 字幕能力（T0020/T0022 的核心）在现依赖下不可用；但离线终稿路径（T0021：VAD 分句 + SenseVoice 终稿）正常，产品仍可做非流式字幕。
- **修复方向（按优先级）**：
  1. 用 fp32 `encoder.onnx`（非 int8）替换后复测 `online_probe`/`hybrid_probe`，排除 int8 量化算子问题（最可能直接解决）；
  2. 若仍崩，将 onnxruntime 钉到 sherpa-onnx 1.13.6 官方验证过的 ORT 版本（而非 1.27.1），复测；
  3. 复测通过后回填 T0020/T0022 真机证据，再关闭 P0 流式子特性。

### 🟡 PARTIAL-2：真机运行时验证清单（须用户回填，不伪造）
- [ ] T0016：真机带 GL 加载器渲染视频并叠加 ASS 字幕、出截图
- [ ] T0019：rclone mount + 断网下 VFS full 随机 seek 恢复
- [ ] T0020/T0022：BLOCKER-2 修复后用真实语音跑 partial / Hybrid SRT（T0021 已通）
- [ ] T0023：真机 FunASR 安装 + 授权语料跑出 CER/WER/RTF 数字

## 4. 签署条件（Sign-off）
满足以下全部方关闭 P0 流式子特性：
1. BLOCKER-2 修复（在线 Paraformer 在 ORT 1.27.1 下成功创建识别器并跑通 partial）；
2. §3 PARTIAL-2 全部项在真机回填证据（截图 / 日志 / CER-WER 数字 / RTF）；
3. 用户在本报告签字确认。

**当前状态（诚实记录）**：
- ✅ 地基/编译/链接/打包/离线 ASR（VAD+SenseVoice）全部验证通过；BLOCKER-1 误诊已更正并实质修复（dll 冲突）。
- 🔴 实时流式 partial（T0020/T0022）被 **BLOCKER-2（在线 Paraformer × ORT 1.27.1 崩溃，官方二进制复现）** 阻断，**未签署通过、未伪造**。
- 🟡 T0016/T0019/T0023 因沙箱无显示器/GPU/语料/rclone 挂载，维持 PARTIAL。
- **结论**：P0 阶段门**条件性通过**——离线字幕子特性（VAD 分句 + SenseVoice 终稿）已具备运行时验证；实时流式 partial 子特性需先解 BLOCKER-2 方可关闭。建议先按 BLOCKER-2 修复方向①（fp32 encoder 复测）推进。
