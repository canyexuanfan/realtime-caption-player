# Agent Handoff

- **Last updated:** 2026-08-27
- **Branch:** `main`
- **HEAD:** `69736c5`（assets）/ `d8a34a0`（[T0009] 真实构建+21 测试 PASS）

## Current Phase

P0（技术验证、决策与第三方基线）

## Last Completed Task

T0009 — 纯逻辑核心模块 + 构建系统 + 21 项单元测试全绿（commit `d8a34a0`）

## Current Task

T0010（自建并锁定 FFmpeg，**BLOCKED** 于原生依赖）/ T0011–T0024 同理 BLOCKED。

## What Was Just Done

1. 首次**真实构建**验证纯逻辑模块（此前仅写未提交、未编译）：
   - 用 venv `cmake 4.4.2` + `ninja 1.13.0` + 仓库内 `.qt6/6.8.1/msvc2022_64` 构建。
   - 修复 15 处编译错误（缺失 include / `using namespace` / 前向声明遮蔽 `QFile` / `QJsonObject` 构造 / 常量未 include 等）。
   - 修复 2 处真实逻辑缺陷：
     * `Timecode::formatClock` 无小时分支分钟误零填充（`"00:00"`→`"0:00"`，符合 tech plan "MM:SS"）。
     * `BoundedQueue::push/pop` 在 cancellation 时因 `CancellationSource::cancel()` 不通知条件变量而永久死锁
       → 改为 `wait_for(kCancelPollInterval, pred)` 轮询取消标记（T0009 验证 21/21 PASS）。
2. 提交此前未提交的实现与配置：
   - `73a7e81` [T0008] 依赖锁 schema 与构建预设（dependencies.lock.json, CMakePresets.json）
   - `d8a34a0` [T0009] 纯逻辑核心模块 + 构建系统 + 21 项单元测试全绿（src/, tests/, CMakeLists.txt, resources/, migrations/, tools/, docs/development/）
   - `69736c5` [assets] 原始设计参考与 UI 原型资产（04/05/06）
3. 校正 `docs/implementation-status.md` 与本文，反映真实状态（T0008/T0009 DONE；T0010–T0024 BLOCKED）。

## Files Changed（本会话）

- 修改：`src/core/Timecode.cpp`（formatClock 分钟零填充）、`src/worker/BoundedQueue.h`（wait_for 取消轮询）
- 修改（编译修复，本批次随 d8a34a0 提交）：`src/core/Logging.h`、`src/settings/SettingsService.h`、
  `src/captions/CaptionTypes.h`、`src/ipc/FrameCodec.cpp`、`src/ipc/JsonMessageCodec.cpp`、
  `src/core/Timecode.cpp`、`src/settings/SettingsMigration.cpp`、`tests/unit/test_error.cpp`、
  `tests/unit/test_json_message_codec.cpp`、`tests/unit/test_timecode.cpp`、
  `tests/unit/test_keymap_service.cpp`、`tests/unit/test_keymap_conflicts.cpp`、
  `tests/unit/test_settings_validation.cpp`
- 新增（提交）：src/、tests/、CMakeLists.txt、CMakePresets.json、dependencies.lock.json、resources/、migrations/、tools/、docs/development/、04/05/06 资产

## Verification Performed

- `cmake -G Ninja ... -DBUILD_TESTS=ON -DBUILD_PLAYER=OFF -DBUILD_WORKER=OFF -DCMAKE_BUILD_TYPE=Release` → 配置成功
- `cmake --build <build> --config Release` → 61/61 目标成功（rcp_core 静态库 + 21 测试 EXE）
- `ctest --test-dir <build> -C Release --timeout 90 --output-on-failure` → **21/21 Passed（9.20s）**
- `git log` 确认 73a7e81 / d8a34a0 / 69736c5 已落地

## Current Known Problems

- libmpv / FFmpeg / sherpa-onnx / 模型权重 / WiX 缺失 → P0 demo 任务（T0010–T0024）BLOCKED。
- 沙箱构建必须使用 `Ninja` generator（CMakePresets 默认 VS2022 generator 崩溃）；Release + 复制 Qt DLL 到 ASCII tests 目录。
- 真实模型基准（T0023）需合法语料与模型下载，暂不可行。

## Next Exact Action

1. 阻塞未解除前，**不要**伪造任何构建/测试通过。继续维护 `implementation-status` 与本文。
2. 当具备原生库源码与构建环境时，从 T0010 起推进：自建并锁定 FFmpeg（写 `dependencies.lock.json` + `third_party/build-metadata/ffmpeg.txt`），
   随后 T0011 libmpv、T0012 sherpa-onnx、T0013 模型 manifest。
3. 每完成一个任务：更新 TODO、`implementation-status`、`agent-handoff`；`git commit -m "[Txxxx] ..."`；`git push`。
4. 安装 WiX 以解除 P8 MSI 打包 BLOCKED。

## Important Decisions

- 工作环境为 Windows + MSVC；Qt 版本采用实际可获取的 **6.8.1**（技术实现方案建议 6.11.2 为基线建议，真实可用版本在 lock 中记录并说明）。
- 纯逻辑模块用 Qt 类型（QString 等）实现以便真实编译与 QtTest；依赖 libmpv/FFmpeg/sherpa 的模块在库就绪前仅保留接口/头文件，不实装，不伪造。
- 所有模型/媒体大文件不进 Git；经 `.gitignore` 排除。

## Commands To Resume

```bash
# 沙箱构建（单条内联全部 MSVC 环境；Bash 状态不跨调用持久）
export MSYS_NO_PATHCONV=1 && export MSYS2_ARG_CONV_EXCL='*' && \
export PATH="<VS>/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64;<WinSDK>/10/bin/10.0.26100.0/x64;$PATH" && \
export INCLUDE="<VS>/.../include;<WinSDK>/.../ucrt;<WinSDK>/.../um;..." && \
export LIB="<VS>/.../lib/x64;<WinSDK>/.../um/x64;<WinSDK>/.../ucrt/x64" && \
export CMAKE_PREFIX_PATH="F:/workbuddy/视频播放器/.qt6/6.8.1/msvc2022_64" && \
export QT_QPA_PLATFORM=offscreen && \
cmake -G Ninja -B F:/workbuddy/rcp-build-rel -S F:/workbuddy/视频播放器 \
  -DBUILD_TESTS=ON -DBUILD_PLAYER=OFF -DBUILD_WORKER=OFF -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="<VS>/.../cl.exe" -DCMAKE_C_COMPILER="<VS>/.../cl.exe" \
  -DCMAKE_RC_COMPILER="<WinSDK>/.../rc.exe" -DCMAKE_MT="<WinSDK>/.../mt.exe" \
  -DCMAKE_EXE_LINKER_FLAGS="/MANIFEST:NO" -DCMAKE_SHARED_LINKER_FLAGS="/MANIFEST:NO" && \
cmake --build F:/workbuddy/rcp-build-rel --config Release && \
cp -f .qt6/6.8.1/msvc2022_64/bin/{Qt6Core,Qt6Test,Qt6Gui}.dll F:/workbuddy/rcp-build-rel/tests/ && \
ctest --test-dir F:/workbuddy/rcp-build-rel -C Release --timeout 90 --output-on-failure
```
