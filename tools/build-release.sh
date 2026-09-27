#!/usr/bin/env bash
# tools/build-release.sh — 一键 Release 构建（固化原「git-bash 内联 MSVC 环境 + Ninja」手工命令）
#
# 用法（git-bash / MSYS）:
#   bash tools/build-release.sh [额外 cmake -D 参数...]
# 示例:
#   bash tools/build-release.sh                                # configure + 构建 player_app caption_worker
#   bash tools/build-release.sh -DRCP_SANDBOX_NO_MANIFEST=ON   # 沙箱受限 TEMP 下 cvtres 失败时的 workaround
#
# 环境变量可覆盖工具位置（脚本不硬编码任何用户目录）:
#   RCP_VS_ROOT   默认 VS2022 BuildTools 标准安装路径
#   RCP_SDK_ROOT  默认 Windows Kits 10 标准安装路径
#   RCP_NINJA     ninja 可执行文件（默认从 PATH 查找）
#   RCP_CMAKE     cmake 可执行文件（默认从 PATH 查找，其次取 ninja 同目录）
set -euo pipefail
cd "$(dirname "$0")/.."

VS="${RCP_VS_ROOT:-C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools}"
SDK="${RCP_SDK_ROOT:-C:/Program Files (x86)/Windows Kits/10}"

MSVC_ROOT=$(ls -d "$VS"/VC/Tools/MSVC/14.* 2>/dev/null | sort -V | tail -1)
[ -n "${MSVC_ROOT:-}" ] || { echo "错误: 未找到 MSVC（$VS/VC/Tools/MSVC/14.*）；用 RCP_VS_ROOT 指定 VS 安装根"; exit 1; }
SDK_VER=$(ls "$SDK/Include" 2>/dev/null | sort -V | tail -1)
[ -n "${SDK_VER:-}" ] || { echo "错误: 未找到 Windows SDK（$SDK/Include）；用 RCP_SDK_ROOT 指定"; exit 1; }

# MSVC 环境变量（INCLUDE/LIB 用 Windows 原生反斜杠路径；MSYS 路径转换必须关闭）。
# 注意：不把 MSVC/SDK 塞进 PATH——MSYS bash 与 Windows 版 cmake 对 PATH 风格要求相反，
# 显式把编译器绝对路径传给 CMake（会烧进 build.ninja），链接器/mt/cvtres 由 cl 自身目录解析。
export INCLUDE="$MSVC_ROOT\\include;$SDK\\Include\\$SDK_VER\\ucrt;$SDK\\Include\\$SDK_VER\\um;$SDK\\Include\\$SDK_VER\\shared"
export LIB="$MSVC_ROOT\\lib\\x64;$SDK\\Lib\\$SDK_VER\\ucrt\\x64;$SDK\\Lib\\$SDK_VER\\um\\x64"
export MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*'

CL_BIN="$MSVC_ROOT/bin/Hostx64/x64/cl.exe"
RC_BIN="$SDK/bin/$SDK_VER/x64/rc.exe"
[ -f "$CL_BIN" ] || { echo "错误: 未找到 cl.exe: $CL_BIN"; exit 1; }

NINJA="${RCP_NINJA:-$(command -v ninja || true)}"
[ -n "$NINJA" ] || { echo "错误: 未找到 ninja（PATH 或 RCP_NINJA）"; exit 1; }
CMAKE="${RCP_CMAKE:-$(command -v cmake || true)}"
[ -n "$CMAKE" ] || CMAKE="$(dirname "$NINJA")/cmake.exe"
[ -x "$CMAKE" ] || { echo "错误: 未找到 cmake（PATH、RCP_CMAKE 或 ninja 同目录）"; exit 1; }

CMAKE_ARGS=(-DCMAKE_MAKE_PROGRAM="$NINJA"
            "-DCMAKE_C_COMPILER=$(cygpath -m "$CL_BIN")"
            "-DCMAKE_CXX_COMPILER=$(cygpath -m "$CL_BIN")")
if [ -f "$RC_BIN" ]; then
  CMAKE_ARGS+=("-DCMAKE_RC_COMPILER=$(cygpath -m "$RC_BIN")")
fi
# mt.exe：manifest 恢复嵌入后链接期必需，必须显式指定（否则 CMAKE_MT-NOTFOUND）。
# 注意 mt.exe 由 Windows SDK 提供（BuildTools 的 MSVC bin 目录里没有）。
MT_BIN="$SDK/bin/$SDK_VER/x64/mt.exe"
if [ -f "$MT_BIN" ]; then
  CMAKE_ARGS+=("-DCMAKE_MT=$(cygpath -m "$MT_BIN")")
else
  echo "警告: 未找到 mt.exe（$MT_BIN）；如链接期报 CMAKE_MT-NOTFOUND 请检查 SDK 安装"
fi

echo "== preset: windows-ninja-release =="
"$CMAKE" --preset windows-ninja-release "${CMAKE_ARGS[@]}" "$@"

echo "== build: all targets（player_app / caption_worker / tests）=="
# 构建全部目标：INCLUDE/LIB 环境只在脚本进程内有效，任何 ninja 调用都必须走本脚本
"$NINJA" -C out/build/windows-ninja-release

echo "== 部署 Qt DLL 到测试目录 =="
# 中文仓库路径下 MSYS→Windows 的 PATH 转换会乱码（Known Issue），
# 测试进程找不到 Qt DLL（0xc0000135），故直接把 Qt6*.dll 拷到各测试 exe 旁。
QT_BIN="$(pwd)/.qt6/6.8.1/msvc2022_64/bin"
if [ -d "$QT_BIN" ]; then
  find out/build/windows-ninja-release -name "test_*.exe" | while read -r t; do
    d=$(dirname "$t")
    cp -f "$QT_BIN"/Qt6*.dll "$d/"
  done
  echo "Qt DLL 已部署到测试目录"
fi

echo "完成: out/build/windows-ninja-release/bin/{player_app.exe,caption_worker.exe}"
echo "测试: $(dirname "$NINJA")/ctest.exe --preset windows-ninja-release（Qt DLL 已自动部署）"
