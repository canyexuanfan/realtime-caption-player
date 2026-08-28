# sherpa-onnx VAD 崩溃（真实语音首句即崩）排查指南

## 问题描述

`caption_worker.exe --file <媒体>` 文件模式在处理**真实语音**时必然段错误（RC=139），
且崩溃点在第一个分句尚未完成时（约 0.5s 处）。喂全零静音、纯音调则全程正常。
此前 T0020–T0022 探针（online_probe/hybrid_probe/sensevoice_probe）真机 rc=0，
但从未暴露此问题——因为探针只喂过合成音/纯音调，VAD 从未真正进入"语音中"状态。

## 已尝试的修复方法及失败原因

- ❌ 怀疑 DLL 版本错配：sha256 对比 bundle/runtime、.tools、探针目录的
  `sherpa-onnx-c-api.dll` / `onnxruntime.dll` 完全一致，排除。
- ❌ 怀疑 FFmpeg 抽音轨破坏堆：加 `RCP_DIAG_RAWWAV=1` 绕过 FFmpeg 直接读 wav，
  仍崩，排除。
- ❌ 怀疑内容依赖的 sherpa 内部缺陷：用同 DLL/同模型/同配置写独立最小探针
  （`spikes/asr-hybrid/OnlineProbeReal.cpp`）喂同一真实 wav，rc=0 且识别正确，
  排除。
- ❌ `SetUnhandledExceptionFilter` 抓不到崩溃（被后续注册覆盖/时序问题），
  改用 `AddVectoredExceptionHandler(1, ...)`（首机会、线程无关）才拿到故障模块。

## 深层问题分析

`src/asr/AsrEngine.cpp` 的 `drainVad()` 误用了 VAD C API 语义：

```cpp
// 错误写法：
while (SherpaOnnxVoiceActivityDetectorDetected(d_->vad)) {
    const SherpaOnnxSpeechSegment* seg = SherpaOnnxVoiceActivityDetectorFront(d_->vad);
    // 直接使用 seg->start —— seg 可能为 NULL！
```

c-api.h 明确注释：
- `Detected()` = "**当前处于语音中**"（speech in progress），**不保证**有已完成分句入队；
- `Front()` = 取队首**已完成**分句，**无则返回 NULL**；
- 正确的排空判据是 `Empty()`（=1 表示没有已完成分句）。

真实语音一进来 → VAD 进入语音状态 → `Detected()==1` → 分句尚未完成（还需
min_silence_duration 静音收尾）→ `Front()` 返回 NULL → `seg->start` 空指针解引用 → 崩溃。
静音/音调永不触发 `Detected()`，所以"测试全过、真话就崩"。

VEH 拿到的决定性证据：
```
[CRASH] code=0xC0000005 ... off=+0x731F READ target=0x0 module=caption_worker.exe
```
READ target=0x0 = EXE 代码内空指针读，与"Front() 返回 NULL 后立刻取字段"吻合。

## ✅ 成功修复方法

```cpp
// 正确写法：
while (!SherpaOnnxVoiceActivityDetectorEmpty(d_->vad)) {
    const SherpaOnnxSpeechSegment* seg = SherpaOnnxVoiceActivityDetectorFront(d_->vad);
    if (!seg) break;  // 防御性兜底
    ...
}
```

同一轮排查还连带修复：
1. **时间戳错位**：时间戳必须用 `seg->start`（VAD 给的是自流开始的**绝对采样索引**），
   `seg->start - consumed_` 只是缓冲区局部索引，用于取样本指针，不能当时间戳。
2. **use-after-free**：`localEnd = localStart + seg->n` 必须在
   `SherpaOnnxDestroySpeechSegment(seg)` 之前计算。
3. **缓冲区收缩失效**：erase 数量要用缓冲区局部索引（localEnd），不能混用绝对索引。

## 验证结果（2026-08-28 真机）

- TTS 合成 4 句中文（Microsoft Huihui）→ FFmpeg 合成 21.5s mp4 →
  `caption_worker --file` 全链路 RC=0。
- 65 个实时 `[partial]`、4 个 `[final]` **文字全部正确**，时间戳顺序正确不重叠，
  SRT 导出正确。

## 下一步排查策略 / 调试工具

- 遇 ASR/DLL 段错误先做**内容判别**：静音/音调/真实语音三分对照 + 独立最小探针。
- 在 main 入口注册 `AddVectoredExceptionHandler(1, handler)` 打印
  `code/addr/READ-WRITE/target/module+offset`，一条日志即可定位元凶模块。
- 阶段标记 stderr（无缓冲）+ `setvbuf(stdout, NULL, _IONBF, 0)` 保证崩溃前日志落地。
- 注意本环境 PATH export 不生效：ninja/cl 一律用绝对路径调用。

## 注意事项

- 新 DLL 头文件接入时，逐个核对**语义注释**而不是只看函数名
  （Detected/Empty/Front 这类命名直觉极易误用）。
- "探针 rc=0" ≠ "链路真实验证"：探针喂什么数据决定了它覆盖了哪些路径。
  合成音频永远走不到"真实语音分句"路径。

## 更新记录

- 2026-08-28 首次记录：问题定位（VEH+内容判别+最小探针）、根因、修复、真机验证通过。
