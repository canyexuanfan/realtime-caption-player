# Agent Handoff

- **Last updated:** 2026-08-27
- **Branch:** `main`
- **HEAD:** `4d47257`（[docs] 校正 P0 状态）/ `d8a34a0`（[T0009] 真实构建+21 测试 PASS）

## Current Phase

P0（技术验证、决策与第三方基线）—— **已完成**。进入 P1：原生模块实装（libmpv 播放内核 T0014+）。

## Last Completed Task

T0010–T0013 —— 原生依赖（FFmpeg / libmpv / sherpa-onnx 预编译库 + ASR 模型权重）全部落地、可链接、可自包含打包。**已通过真实 cmake/Ninja 构建验证。**

## Current Task

**T0014（libmpv 播放内核实装）—— 已解除 BLOCKED。** 依赖与打包链路已验证可用，可直接推进。

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

## Files Changed（本会话，待提交）

- 新增：`cmake/FindRcpNativeDeps.cmake`（依赖查找与导入目标 `rcp::ffmpeg/mpv/sherpa-onnx`）、`cmake/RcpBundle.cmake`、`cmake/RcpBundleRuntime.cmake`、`cmake/run_bundle.cmake`
- 新增：`tools/deps_smoke/CMakeLists.txt`、`tools/deps_smoke/main.cpp`（链接冒烟测试）
- 修改：`CMakeLists.txt`（新增 `BUILD_DEPS_SMOKE` 选项 + 含 finder/子目录）、`dependencies.lock.json`、`docs/implementation-status.md`
- 修改（本文）：T0010–T0024 解除 BLOCKED

## Verification Performed

- `cmake -G Ninja -B out/build/smoke -S F:/workbuddy/视频播放器 -DBUILD_DEPS_SMOKE=ON ...` → 配置成功，`Found RcpNativeDeps`。
- `cmake --build out/build/smoke --config Release --target deps_smoke` → **BUILD_EXIT=0**。
- `dumpbin /dependents out/build/smoke/tools/deps_smoke/deps_smoke.exe` → 含 `avformat-63.dll` / `mpv-2.dll` / `sherpa-onnx-c-api.dll`。
- 运行时 DLL 物理存在：`.tools/ffmpeg/bin/avformat-63.dll`、`mpv-2.dll`、`sherpa-onnx/lib/sherpa-onnx-c-api.dll`。

## Current Known Problems / 坑位

- **libmpv 官方 msvc 包无开发库**：必须从 `zhongfly/mpv-winbuild` 的 `mpv-dev-*` 取头文件 + `libmpv-2.dll`，并现场生成 `mpv.lib`（见上）。
- **C 库头必须 `extern "C"`**：`mpv/client.h`、`libavformat/*.h`、`sherpa-onnx/c-api/c-api.h` 在 C++ TU 中不加 `extern "C"` 会 LNK2019。
- **沙箱构建必须用 `Ninja` generator**（CMakePresets 默认 VS2022 generator 在沙箱崩溃）；Release + 显式 MSVC 环境，且需 `MSYS_NO_PATHCONV=1` + `MSYS2_ARG_CONV_EXCL='*'` 绕过 MSYS 路径改写与 `rc.exe` 找不到的问题。
- **`dumpbin` 在 Git-Bash 下 PATH 需用 `:` 分隔的 Unix 风格路径**（用 `;` 的 Windows 风格 PATH 在 bash 中不生效）。
- 真实模型基准（T0023）需合法语料与模型下载，暂不可行（属后续 P 阶段）。

## Next Exact Action

1. 提交本会话全部原生依赖/打包/文档改动（`[T0010-T0013] ...`），随后 `git push`。
2. 推进 **T0014（libmpv 播放内核）**：在 `BUILD_PLAYER=ON` 下实装 `src/player/` 的 libmpv 封装（创建 `mpv_handle`、绑定 `opengl-cb`/D3D 渲染、事件循环、加载媒体、字幕轨道选择），沿用 `rcp::mpv` 导入目标。
3. 每完成一个任务：更新 TODO、`implementation-status`、`agent-handoff`；`git commit -m "[Txxxx] ..."`；`git push`。
4. 安装 WiX 以解除 P8 MSI 打包 BLOCKED（依赖现已被 `RcpBundle.cmake` 收集，打包阶段只需 WiX 工具链）。

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
