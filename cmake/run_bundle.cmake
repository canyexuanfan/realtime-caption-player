# cmake/run_bundle.cmake — 独立验证驱动(脚本模式 cmake -P)
# 用法: cmake -P cmake/run_bundle.cmake
# 作用: 把 .tools 下的原生依赖 DLL 与白名单 ASR 模型拷进 out/bundle/runtime，
#       验证"打包进程序"机制可用(无需 Qt / 编译器)。
# 路径全部由本文件位置推导（仓库相对），禁止硬编码本机绝对路径（公开仓库）。
get_filename_component(RCP_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(RCP_DEPS_ROOT "${RCP_SOURCE_ROOT}/.tools")
include("${CMAKE_CURRENT_LIST_DIR}/RcpBundle.cmake")
rcp_copy_native_runtime(DEST "${RCP_SOURCE_ROOT}/out/bundle/runtime")

# D1：Qt SQL 运行时（Qt6Sql.dll + sqldrivers/qsqlite.dll，Qt 会在 exe 旁
# 的 sqldrivers/ 子目录查找驱动插件）。
set(_QT_BIN "${RCP_SOURCE_ROOT}/.qt6/6.8.1/msvc2022_64")
foreach(_dll Qt6Sql)
  if(EXISTS "${_QT_BIN}/bin/${_dll}.dll")
    file(COPY "${_QT_BIN}/bin/${_dll}.dll" DESTINATION "${RCP_SOURCE_ROOT}/out/bundle/runtime")
  endif()
endforeach()
if(EXISTS "${_QT_BIN}/plugins/sqldrivers/qsqlite.dll")
  file(MAKE_DIRECTORY "${RCP_SOURCE_ROOT}/out/bundle/runtime/sqldrivers")
  file(COPY "${_QT_BIN}/plugins/sqldrivers/qsqlite.dll"
       DESTINATION "${RCP_SOURCE_ROOT}/out/bundle/runtime/sqldrivers")
endif()
