# cmake/run_bundle.cmake — 独立验证驱动(脚本模式 cmake -P)
# 用法:
#   cmake -P cmake/run_bundle.cmake                        # 完整运行时 → out/bundle/runtime
#   RCP_BUNDLE_LITE=1 cmake -P cmake/run_bundle.cmake      # Lite 运行时 → out/bundle/runtime-lite
# 作用: 把 .tools 下的原生依赖 DLL 与白名单 ASR 模型拷进运行时目录，
#       验证"打包进程序"机制可用(无需 Qt / 编译器)。
# Lite 变体(E 阶段): 仅 zipformer-ctc + silero 两套模型（不含 SenseVoice 大模型），
#   安装包体积显著缩小；worker 端 AsrEngine 检测到 sensevoice 缺失时跳过其创建，
#   终稿自动回退为分句结束时的 partial 快照（Lite 档行为）。
# 路径全部由本文件位置推导（仓库相对），禁止硬编码本机绝对路径（公开仓库）。
get_filename_component(RCP_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(RCP_DEPS_ROOT "${RCP_SOURCE_ROOT}/.tools")
include("${CMAKE_CURRENT_LIST_DIR}/RcpBundle.cmake")

# 开关双通道：cmake -D 变量或同名环境变量（script 模式下 -D 不总可见，实测踩坑）。
if(NOT RCP_BUNDLE_LITE)
  set(RCP_BUNDLE_LITE "$ENV{RCP_BUNDLE_LITE}")
endif()

# RcpBundle 内部以 CACHE 变量读白名单（函数作用域），此处须 FORCE 覆盖。
if(RCP_BUNDLE_LITE)
  set(RCP_MODEL_BUNDLES zipformer-ctc silero CACHE STRING "" FORCE)
  set(_RCP_DEST "${RCP_SOURCE_ROOT}/out/bundle/runtime-lite")
else()
  set(RCP_MODEL_BUNDLES zipformer-ctc silero sensevoice CACHE STRING "" FORCE)
  set(_RCP_DEST "${RCP_SOURCE_ROOT}/out/bundle/runtime")
endif()

set(_QT_BIN "${RCP_SOURCE_ROOT}/.qt6/6.8.1/msvc2022_64")

rcp_copy_native_runtime(DEST "${_RCP_DEST}")

# Qt 运行时部署：E 阶段起新建 runtime(-lite) 不再依赖历史 windeployqt 残留，
# 统一从仓库内 .qt6 拷齐 GUI 运行所需的 DLL 与平台插件。
set(_QT_DLLS Qt6Core Qt6Gui Qt6OpenGL Qt6Widgets Qt6OpenGLWidgets Qt6Network Qt6Svg Qt6Sql D3Dcompiler_47)
foreach(_dll ${_QT_DLLS})
  if(EXISTS "${_QT_BIN}/bin/${_dll}.dll")
    file(COPY "${_QT_BIN}/bin/${_dll}.dll" DESTINATION "${_RCP_DEST}")
  endif()
endforeach()
if(EXISTS "${_QT_BIN}/plugins/platforms/qwindows.dll")
  file(MAKE_DIRECTORY "${_RCP_DEST}/platforms")
  file(COPY "${_QT_BIN}/plugins/platforms/qwindows.dll"
       DESTINATION "${_RCP_DEST}/platforms")
endif()
if(EXISTS "${_QT_BIN}/plugins/styles/qwindowsvistastyle.dll")
  file(MAKE_DIRECTORY "${_RCP_DEST}/styles")
  file(COPY "${_QT_BIN}/plugins/styles/qwindowsvistastyle.dll"
       DESTINATION "${_RCP_DEST}/styles")
endif()
if(EXISTS "${_QT_BIN}/plugins/sqldrivers/qsqlite.dll")
  file(MAKE_DIRECTORY "${_RCP_DEST}/sqldrivers")
  file(COPY "${_QT_BIN}/plugins/sqldrivers/qsqlite.dll"
       DESTINATION "${_RCP_DEST}/sqldrivers")
endif()
