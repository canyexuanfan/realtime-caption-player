# Windows 开发环境搭建（固定 Qt 与构建工具）

本文档描述在干净 Windows x64 开发机上复现构建环境的最小步骤。所有版本与
`dependencies.lock.json` 保持一致；重复运行 `tools/bootstrap.ps1` 结果一致，
缺件时明确失败且不静默修改系统配置。

## 1. 系统前置

| 组件 | 要求 | 验证 |
|---|---|---|
| OS | Windows 10/11 x64 | — |
| MSVC | VS2022 BuildTools + MSVC v143 + Windows 10/11 SDK | `cl` 可用 |
| git | 任意较新版本 | `git --version` |
| GitHub CLI | 用于创建私有仓库与推送 | `gh auth status` |
| Python | 3.10+（用于 aqt / pip 安装 cmake、ninja） | `python --version` |
| 网络 | 可访问 Qt 镜像（本机经 Clash Verge TUN 代理） | 可下载 |

> 代理说明：本环境使用 Clash Verge TUN 模式，系统层已路由；如需显式代理可设
> `HTTPS_PROXY=http://127.0.0.1:7897`。aqt 在**非中文工作目录**运行，避免 GBK
> 字节导致 `UnicodeDecodeError` 崩溃。

## 2. 构建工具（cmake / ninja）

本环境使用隔离 venv（不污染系统）：

```
C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/cmake.exe   # 4.4.2
C:/Users/<你的用户名>/.workbuddy/binaries/python/envs/default/Scripts/ninja.exe   # 1.13.0
```

干净机器可用 `python -m pip install cmake ninja` 安装到 venv 或用户目录，
并将其加入 PATH。

## 3. Qt 6.8.1（仅 qtbase）

**正确命令**（qtbase 是默认包，禁止用 `-m qtbase`；架构写全 `win64_msvc2022_64`）：

```bat
python -m aqt install-qt windows desktop 6.8.1 win64_msvc2022_64 ^
  --outputdir C:/path/to/Qt6 --timeout 300 ^
  --accept-licenses --accept-obligations
```

安装完成后将 Qt 根目录设为 `CMAKE_PREFIX_PATH`：

```bat
set CMAKE_PREFIX_PATH=C:/path/to/Qt6/6.8.1/msvc2022_64
```

> 坑位记录：① 中文工作目录会因控制台 GBK 字节使 aqt 崩溃 → 在 ASCII 目录运行；
> ② `--timeout` 默认仅 5 秒，大归档会 read timeout → 用 `--timeout 300` 并加重试；
> ③ 纯逻辑构建只需 qtbase（Core/Gui/Widgets/Sql/Network/Test），无需 qtdeclarative。

## 4. 配置 / 构建 / 测试

```bat
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug --parallel
ctest --preset windows-msvc-debug --output-on-failure
```

纯逻辑模块（`rcp_core` 静态库 + 21 个 QtTest）不依赖 libmpv/FFmpeg/sherpa，
在 Qt 安装完成后即可真实编译与运行。

## 5. 一键引导与依赖校验

```bat
pwsh ./tools/bootstrap.ps1                 # 检测/安装 cmake+ninja+Qt，失败明确报错
pwsh ./tools/verify-dependencies.ps1       # 校验 dependencies.lock.json 字段完整
```

## 6. 已知阻塞（BLOCKED，不伪造完成）

- libmpv / FFmpeg / sherpa-onnx：未构建、未锁定（T0010–T0012）。
- 模型权重：未下载（T0013，需合法来源与体积）。
- WiX Toolset：未安装，MSI 打包（P8）阻塞。
