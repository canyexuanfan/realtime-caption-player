# packaging/wix/wix.cmake
#
# P8 打包（WiX v3.11.2）：把 out/bundle/runtime 自包含运行时打进 MSI 安装包。
# 工具链位于 <source>/.tools/wix（candle/light/heat.exe）。
#
# 用法（在顶层 CMakeLists 启用后）：
#   cmake -B out/build -DBUILD_PLAYER=ON ...
#   cmake --build out/build --target package_msi
# 产出：out/package/RealtimeCaptionPlayer-<ver>.msi
#
# 流程：
#   1) heat dir <runtime> -> files.wxs  (组件组 RcpRuntimeComponents, 安装到 INSTALLFOLDER)
#   2) candle product.wxs + files.wxs   -> .wixobj
#   3) light .wixobj*                   -> .msi

find_program(WIX_HEAT   NAMES heat   PATHS ${CMAKE_SOURCE_DIR}/.tools/wix NO_DEFAULT_PATH REQUIRED)
find_program(WIX_CANDLE NAMES candle PATHS ${CMAKE_SOURCE_DIR}/.tools/wix NO_DEFAULT_PATH REQUIRED)
find_program(WIX_LIGHT  NAMES light  PATHS ${CMAKE_SOURCE_DIR}/.tools/wix NO_DEFAULT_PATH REQUIRED)

function(rcp_package_wix)
  set(options "")
  set(oneValueArgs RUNTIME DEST)
  cmake_parse_arguments(PKG "${options}" "${oneValueArgs}" "" ${ARGN})
  if(NOT PKG_RUNTIME)
    set(PKG_RUNTIME "${CMAKE_SOURCE_DIR}/out/bundle/runtime")
  endif()
  if(NOT PKG_DEST)
    set(PKG_DEST "${CMAKE_SOURCE_DIR}/out/package")
  endif()
  file(MAKE_DIRECTORY "${PKG_DEST}")

  add_custom_target(package_msi
    COMMENT "Packaging MSI via WiX (heat -> candle -> light)"
    COMMAND "${WIX_HEAT}" dir "${PKG_RUNTIME}"
            -nologo -ag -sfrag -sreg -srd -scom
            -dr INSTALLFOLDER -cg RcpRuntimeComponents -var var.RuntimeSource
            -out "${PKG_DEST}/files.wxs"
    COMMAND "${WIX_CANDLE}" -nologo
            "-dRuntimeSource=${PKG_RUNTIME}"
            "-dSourceDirPath=${CMAKE_SOURCE_DIR}/packaging/wix"
            "${CMAKE_SOURCE_DIR}/packaging/wix/product.wxs"
            "${PKG_DEST}/files.wxs"
            -out "${PKG_DEST}/"
    COMMAND "${WIX_LIGHT}" -nologo -sval
            "${PKG_DEST}/product.wixobj" "${PKG_DEST}/files.wixobj"
            -out "${PKG_DEST}/RealtimeCaptionPlayer-${PROJECT_VERSION}.msi"
    VERBATIM
  )
endfunction()

# 默认对 out/bundle/runtime 打包（需先运行 RcpBundle 生成运行时）。
rcp_package_wix()
