# Agent Handoff

- **Last updated:** 2026-08-27 15:30
- **Branch:** `main`
- **HEAD:** `69920bb`（[T0023] 合规语料+FunASR 基线，已推送 origin/main）
- **未提交：** `docs/implementation-status.md`（T0015–T0024 进度同步）、`docs/agent-handoff.md`（本更新）、`docs/phase-gates/P0-report.md`（T0024 阶段门报告草案）

## Current Phase

P0（技术验证、决策与第三方基线）—— **代码与编译/链接验证已全部完成（T0001–T0024）**，但**阶段门未签署**：运行时验证因环境约束 + 模型/ORT 版本错配未关闭（见 BLOCKER-1）。P0 报告草案已就绪，签署需真机回填证据。

## Last Completed Task

- T0010–T0013 原生依赖（FFmpeg/libmpv/sherpa-onnx + ASR 模型）落地、可链接、可自包含打包（真实 cmake/Ninja 验证）。
- T0014 `src/player/` libmpv 封装（`BUILD_PLAYER=ON` 真实构建，导入 mpv-2.dll）。
- T0015–T0019 探针（mpv 事件桥 / ASS overlay / 音轨解码 / seek / rclone 双开）—— 代码 + 编译/链接 + 部分真机式运行已验证。
- T0020–T0022 ASR 混合链路（在线 Paraformer / Silero VAD / Hybrid 融合）—— 代码 + 编译/链接已验证；运行时因 BLOCKER-1 阻塞。
- T0023 合规语料 + **FunASR** 基线（原 whisper 基线按用户规则禁用）—— 评测框架已完成。
- T0024 P0 阶段门报告草案（`docs/phase-gates/P0-report.md`）。

## Current Task

**T0024 收尾 + 关闭 P0 阶段门**：P0 所有任务的代码与编译/链接验证已完成；当前阻塞与待回填项：
1. **BLOCKER-1**（模型/ORT opset 错配，阻断 T0020–T0022 运行时）：升级 sherpa-onnx+ORT 或换配套模型。
2. **真机回填**（T0016 渲染出图 / T0019 rclone 断网 / T0020–22 真实语音 ASR / T0023 CER-WER）：沙箱无显示器/GPU/语料/rclone，需用户真机运行并回传证据。
3. 用户在本报告签字后关闭 P0，进入 P1 产品实装。

## 用户关切的答案（"依赖能打包进程序吗？用户还要不要下载？"）

**能，且最终用户零运行时下载。**

链路已端到端证明：
1. 开发者侧（一次性，非用户侧）：下载脚本把 FFmpeg / libmpv / sherpa-onnx 解压进工作空间 `<source>/.tools/`（已下载就绪，~674MB）。
2. 构建侧：`cmake/RcpBundle.cmake` 把 `.tools/` 里的 `*.dll` 运行时库 + `models/` 拷进 `out/bundle/runtime`（仅拷 `.dll`，跳过 `.lib/.def/.a` 与 CLI exe，避免包体膨胀；跳过 `.part/.bad` 断点残留）。
3. 发布侧：WiX（或 CPack）把 `out/bundle/runtime` 整体打进 MSI 安装包。
4. 用户侧：拿到的是**完整安装包，安装即用，无需任何外部下载**。

> 注：`.tools/` 与 `out/` 已写入 `.gitignore`，不进 Git（避免 674MB 二进制入库），通过下载脚本/构建系统本地化还原。

## What Was Just Done（本会话）

1. **补全 libmpv 开发库**（官方 mpv msvc zip 只含 `mpv.exe` CLI，无 libmpv 开发文件）：
   - 从 `zhongfly/mpv-winbuild` 取 `mpv-dev-x86_64-*.7z`，抽出 `mpv/client.h` 等头文件与 `libmpv-2.dll`。
   - `libmpv-2.dll` 重命名为 `mpv-2.dll`（与导入库命名一致）。
   - 复制 `vulkan-1.dll` 到 `bin/`（libmpv 运行期依赖）。
2. **现场生成 MSVC 导入库 `mpv.lib`**（MinGW 的 `libmpv.dll.a` 对 `link.exe` 无效）：
   - `dumpbin /exports mpv-2.dll` → 解析导出表为 `mpv.def` → `lib.exe /machine:X64 /def:mpv.def /out:mpv.lib`。
   - 解析器关键点：导出表表头后有空行，**遇空行 continue、遇 `Summary` 才 break**，否则首符号丢失导致 0 符号。
3. **真实 cmake/Ninja 构建验证三库可链接**（`BUILD_DEPS_SMOKE=ON`）：
   - 配置：`-- Found RcpNativeDeps`，解析到 `mpv.lib` / `sherpa-onnx-c-api.lib` / ffmpeg include。
   - 构建：`cmake --build out/build/smoke --config Release --target deps_smoke` → **BUILD_EXIT=0，4/4 步**。
   - 依赖链（`dumpbin /dependents`）：`avformat-63.dll` + `mpv-2.dll` + `sherpa-onnx-c-api.dll` 三者齐备，外加 MSVC 运行期（VCRUNTIME140 / api-ms-win-crt-*）。
4. **修正 `tools/deps_smoke/main.cpp`**：原为 C 库头文件加 `extern "C"` 包裹（否则 LNK2019 名字修饰）；并调用真实 `SherpaOnnxGetVersionStr()`（仅声明 struct 不会触发 DLL 导入），确保 sherpa 真正进入依赖链。
5. **重写 `cmake/RcpBundle.cmake`**：原仅拷 `bin/`（漏掉 `lib/` 下的 `sherpa-onnx-c-api.dll`）且误拷 CLI `.exe`；改为同时收集 `bin/` 与 `lib/`，且只拷 `*.dll`。
6. **更新锁文件与状态文档**：`dependencies.lock.json`（mpv/ffmpeg/sherpa/模型全部 `installed`，新增 `bundled_runtime` 段：13 DLL + 3 模型 ≈674MB，已验证链接）、`docs/implementation-status.md`（T0010–T0013 DONE；T0014–T0024 TODO/UNBLOCKED）。

### 本会话第二段（T0014 播放内核）

7. **新增 `src/player/` —— libmpv 播放封装（T0014）**：
   - `MpvPlayer.h/.cpp`：类封装 `mpv_handle` 生命周期 + 客户端/渲染 API 调用——`clientApiVersion()`、`create()`（mpv_create）、`setOption()`（mpv_set_option_string）、`initialize()`（mpv_initialize）、`loadFile()`（mpv_command loadfile）、`createRenderContext()`（mpv_render_context_create + `mpv_opengl_init_params`）。析构调用 `mpv_destroy()`。
   - **关键修正**：本 libmpv 构建（zhongfly mpv-dev）**未导出 `mpv_detach_destroy`**，仅导出 `mpv_destroy` / `mpv_terminate_destroy`；故析构用 `mpv_destroy()`，勿调用 `mpv_detach_destroy`（否则 LNK2019）。
   - **关键修正**：`render_gl.h` 将 `mpv_render_context_create` 宏重定向为运行期函数指针（需 `mpv_render_context_create_fn` 取址）；本工程静态链接 `mpv.lib`，需在 include 后 `#undef mpv_render_context_create` 以保留 `render.h` 的真实导出符号。
   - `player_app.cpp`：链接验证入口——调 `clientApiVersion()` / `create()` / `setOption()` / `initialize()`，并引用 `createRenderContext` 符号确保渲染 API 进入依赖链（沙箱无显示设备，不实际创建 GL 上下文）。
   - `src/player/CMakeLists.txt`：门控 `BUILD_PLAYER`，建 `rcp_player` 静态库 + `player_app` 可执行文件，链接 `rcp::mpv`（即 `mpv.lib`）。
8. **根 `CMakeLists.txt` 增强**：
   - `BUILD_PLAYER` 开关 + `add_subdirectory(src/player)`。
   - **Qt6 自动发现**：向 `CMAKE_PREFIX_PATH` 追加工作空间内 `.qt6/` 目录（`<src>/.qt6/<ver>/msvc2022_64`），避免每次构建手填 `CMAKE_PREFIX_PATH` 才能找到 Qt6。
9. **根 `CMakeLists.txt` `HEAD`/文档同步**：`implementation-status.md` T0014 → DONE，当前任务推进到 T0015。

## Files Changed（本会话，待提交）

- 新增：`cmake/FindRcpNativeDeps.cmake`（依赖查找与导入目标 `rcp::ffmpeg/mpv/sherpa-onnx`）、`cmake/RcpBundle.cmake`、`cmake/RcpBundleRuntime.cmake`、`cmake/run_bundle.cmake`
- 新增：`tools/deps_smoke/CMakeLists.txt`、`tools/deps_smoke/main.cpp`（链接冒烟测试）
- 新增（T0014）：`src/player/MpvPlayer.h`、`src/player/MpvPlayer.cpp`、`src/player/player_app.cpp`、`src/player/CMakeLists.txt`
- 修改：`CMakeLists.txt`（新增 `BUILD_DEPS_SMOKE` 选项 + `BUILD_PLAYER` 选项 + Qt6 自动发现 + 含 finder/子目录）、`dependencies.lock.json`、`docs/implementation-status.md`
- 修改（本文）：T0010–T0024 解除 BLOCKED；T0014 标记 DONE

## Verification Performed

- `cmake -G Ninja -B out/build/smoke -S F:/workbuddy/视频播放器 -DBUILD_DEPS_SMOKE=ON ...` → 配置成功，`Found RcpNativeDeps`。
- `cmake --build out/build/smoke --config Release --target deps_smoke` → **BUILD_EXIT=0**。
- `dumpbin /dependents out/build/smoke/tools/deps_smoke/deps_smoke.exe` → 含 `avformat-63.dll` / `mpv-2.dll` / `sherpa-onnx-c-api.dll`。
- 运行时 DLL 物理存在：`.tools/ffmpeg/bin/avformat-63.dll`、`mpv-2.dll`、`sherpa-onnx/lib/sherpa-onnx-c-api.dll`。

### T0014 播放内核验证

- `cmake -G Ninja -B out/build/player -S F:/workbuddy/视频播放器 -DBUILD_PLAYER=ON -DCMAKE_PREFIX_PATH="F:/workbuddy/视频播放器/.qt6/6.8.1/msvc2022_64" ...` → 配置成功（Qt6 现已自动发现，无需手填 prefix）。
- `cmake --build out/build/player --config Release --target player_app` → **BUILD_EXIT=0**（`rcp_core` → `rcp_player` → `player_app.exe`）。
- `dumpbin /dependents out/build/player/bin/player_app.exe` → 含 `mpv-2.dll` + `Qt6Core.dll`（libmpv 真实嵌入并通过链接）。
- 唯一告警：`MediaIdentity.cpp` 中 `QCryptographicHash::addData` 弃用提示（既有代码，与本改动无关）。

## Current Known Problems / 坑位

- **libmpv 官方 msvc 包无开发库**：必须从 `zhongfly/mpv-winbuild` 的 `mpv-dev-*` 取头文件 + `libmpv-2.dll`，并现场生成 `mpv.lib`（见上）。
- **C 库头必须 `extern "C"`**：`mpv/client.h`、`libavformat/*.h`、`sherpa-onnx/c-api/c-api.h` 在 C++ TU 中不加 `extern "C"` 会 LNK2019。
- **本 libmpv 构建未导出 `mpv_detach_destroy`**：仅 `mpv_destroy` / `mpv_terminate_destroy`。析构用 `mpv_destroy()`，引用 `mpv_detach_destroy` 会 LNK2019。
- **`render_gl.h` 宏重定向 `mpv_render_context_create`**：该头把符号重定义为运行期函数指针（需 `mpv_render_context_create_fn` 取址）。静态链接 `mpv.lib` 时要 `#undef mpv_render_context_create` 以保留 `render.h` 的真正导出符号，否则链接报未解析。
- **沙箱构建必须用 `Ninja` generator**（CMakePresets 默认 VS2022 generator 在沙箱崩溃）；Release + 显式 MSVC 环境，且需 `MSYS_NO_PATHCONV=1` + `MSYS2_ARG_CONV_EXCL='*'` 绕过 MSYS 路径改写与 `rc.exe` 找不到的问题。
- **`dumpbin` 在 Git-Bash 下 PATH 需用 `:` 分隔的 Unix 风格路径**（用 `;` 的 Windows 风格 PATH 在 bash 中不生效）。
- **T0023 基线工具用 FunASR（非 whisper）**：用户长期规则禁用 Whisper；`benchmark-asr.py` 以 FunASR 为对照，`manifest.json` 中 `whisper_baseline=DISABLED_BY_POLICY`。
- **BLOCKER-1（模型/ORT opset 错配）**：`.tools/models/` 的 Paraformer/SenseVoice/Silero 用 opset 27，bundled sherpa-onnx 1.13.6 的 ORT 不支持，加载报 `version [27] not supported` 崩溃；非代码缺陷，需升级 sherpa-onnx+ORT 或换配套模型。

## Next Exact Action

1. **提交 T0015–T0024 成果**：`git add docs/implementation-status.md docs/agent-handoff.md docs/phase-gates/P0-report.md` → `[T0024] P0 阶段门报告 + 进度同步` → `git push`。
2. **修复 BLOCKER-1**：升级 `sherpa-onnx`+`onnxruntime` 至支持 opset 17+ 的版本（或换与 1.13.6 配套的模型）；之后重跑 `online_probe`/`sensevoice_probe`/`hybrid_probe` 回填运行时证据。
3. **真机回填**：用户在带显示器/授权语料/rclone 挂载的 Windows 上运行各 spike，回传渲染截图、双开断网、ASR partial/分句/SRT、FunASR CER-WER 证据；据此关闭 P0 阶段门。
4. 每完成一步：更新 `implementation-status`、`agent-handoff`、`P0-report.md` 签署区；`git commit`；`git push`。

## Important Decisions

- 工作环境为 Windows + MSVC；Qt 实际可用版本 **6.8.1**（技术实现方案建议 6.11.2 为基线，lock 中记录真实版本并说明）。
- 原生依赖采用**预编译库 + 工作空间 `.tools/` 本地化**策略（非 vcpkg 在线拉取），保证可复现且用户侧零下载。
- 所有模型/媒体/二进制大文件不进 Git，经 `.gitignore` 排除；通过下载脚本与 `RcpBundle.cmake` 在构建期还原进发布包。
- 绝不伪造构建/测试通过：依赖就位以"真实 cmake 构建 + `dumpbin /dependents` 含三库 DLL"为判据。

## Commands To Resume

```bash
# 沙箱原生依赖链接冒烟（单条内联全部 MSVC 环境；Bash 状态不跨调用持久）
export MSYS_NO_PATHCONV=1
export MSYS2_ARG_CONV_EXCL='*'
export PATH="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64:C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64:$PATH"
export INCLUDE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/include;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared"
export LIB="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
cmake -G Ninja -B out/build/smoke -S F:/workbuddy/视频播放器 \
  -DBUILD_TESTS=ON -DBUILD_PLAYER=OFF -DBUILD_WORKER=OFF -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_DEPS_SMOKE=ON \
  -DCMAKE_C_COMPILER="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe" \
  -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe" \
  -DCMAKE_RC_COMPILER="C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/rc.exe" \
  -DCMAKE_MT="C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/mt.exe" \
  -DCMAKE_EXE_LINKER_FLAGS="/MANIFEST:NO" -DCMAKE_SHARED_LINKER_FLAGS="/MANIFEST:NO" && \
cmake --build out/build/smoke --config Release --target deps_smoke && \
dumpbin /dependents out/build/smoke/tools/deps_smoke/deps_smoke.exe | grep -i dll
```

```bash
# T0014 播放内核构建（BUILD_PLAYER=ON；Qt6 现已自动发现，可省略 CMAKE_PREFIX_PATH）
export MSYS_NO_PATHCONV=1
export MSYS2_ARG_CONV_EXCL='*'
export PATH="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64:C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64:$PATH"
export INCLUDE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/include;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared"
export LIB="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
cmake -G Ninja -B out/build/player -S F:/workbuddy/视频播放器 \
  -DBUILD_TESTS=OFF -DBUILD_DEPS_SMOKE=OFF -DBUILD_PLAYER=ON -DBUILD_WORKER=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="F:/workbuddy/视频播放器/.qt6/6.8.1/msvc2022_64" \
  -DCMAKE_C_COMPILER="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe" \
  -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe" \
  -DCMAKE_RC_COMPILER="C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/rc.exe" \
  -DCMAKE_MT="C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/mt.exe" \
  -DCMAKE_EXE_LINKER_FLAGS="/MANIFEST:NO" -DCMAKE_SHARED_LINKER_FLAGS="/MANIFEST:NO" && \
cmake --build out/build/player --config Release --target player_app && \
dumpbin /dependents out/build/player/bin/player_app.exe | grep -i dll
```
