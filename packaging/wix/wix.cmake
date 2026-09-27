# packaging/wix/wix.cmake
#
# P8 打包（WiX v3.11.2）：把 out/bundle/runtime 自包含运行时打进 MSI 安装包。
# 工具链位于 <source>/.tools/wix（candle/light/heat.exe）。
#
# 用法（在顶层 CMakeLists 启用后）：
#   cmake -B out/build -DBUILD_PLAYER=ON ...
#   cmake --build out/build --target package_msi          # 完整包（zipformer-ctc/silero/sensevoice）
#   cmake --build out/build --target package_msi-Lite     # Lite 包（zipformer-ctc/silero；需先
#                                                         #   cmake -DRCP_BUNDLE_LITE=ON -P cmake/run_bundle.cmake）
# 产出：out/package/RealtimeCaptionPlayer-<ver>[-Lite].msi
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
  set(oneValueArgs RUNTIME DEST NAME_SUFFIX)
  cmake_parse_arguments(PKG "${options}" "${oneValueArgs}" "" ${ARGN})
  if(NOT PKG_RUNTIME)
    set(PKG_RUNTIME "${CMAKE_SOURCE_DIR}/out/bundle/runtime")
  endif()
  if(NOT PKG_DEST)
    set(PKG_DEST "${CMAKE_SOURCE_DIR}/out/package")
  endif()
  set(PKG_NAME "RealtimeCaptionPlayer-${PROJECT_VERSION}${PKG_NAME_SUFFIX}")
  file(MAKE_DIRECTORY "${PKG_DEST}")

  # 目标名：完整包 package_msi；Lite 包 package_msi-Lite。
  add_custom_target(package_msi${PKG_NAME_SUFFIX}
    COMMENT "Packaging MSI via WiX (heat -> candle -> light): ${PKG_NAME}"
    COMMAND "${WIX_HEAT}" dir "${PKG_RUNTIME}"
            -nologo -ag -sfrag -sreg -srd -scom -platform x64
            -dr INSTALLFOLDER -cg RcpRuntimeComponents -var var.RuntimeSource
            -out "${PKG_DEST}/files.wxs"
    COMMAND "${WIX_CANDLE}" -nologo -arch x64
            "-dRuntimeSource=${PKG_RUNTIME}"
            "-dSourceDirPath=${CMAKE_SOURCE_DIR}/packaging/wix"
            "${CMAKE_SOURCE_DIR}/packaging/wix/product.wxs"
            "${PKG_DEST}/files.wxs"
            -out "${PKG_DEST}/"
    # E：ICE 校验默认开启（-sval 仅限 RCP_WIX_SKIP_ICE=ON 的沙箱 workaround）。
    # 定向豁免：ICE61=AllowSameVersionUpgrades 设计如此；ICE20=定制最小 UI 的
    # ErrorDialog/FilesInUse 收尾项（合规对话框已在 product.wxs 起步，见 todolist）。
    # 其余 ICE 全部生效——已实抓并修复 ICE80 组件位数/ICE57 上下文混用。
    COMMAND "${WIX_LIGHT}" -nologo -sice:ICE61 -sice:ICE20
            $<$<BOOL:${RCP_WIX_SKIP_ICE}>:-sval>
            "${PKG_DEST}/product.wixobj" "${PKG_DEST}/files.wixobj"
            -out "${PKG_DEST}/${PKG_NAME}.msi"
    VERBATIM
  )
endfunction()

# 默认对 out/bundle/runtime 打包（需先运行 RcpBundle 生成运行时）。
option(RCP_WIX_SKIP_ICE "沙箱 workaround：跳过 WiX ICE 校验（勿用于正式发布）" OFF)
rcp_package_wix()
# E：Lite 包目标——先 `cmake -DRCP_BUNDLE_LITE=ON -P cmake/run_bundle.cmake`
# 生成 out/bundle/runtime-lite（仅 zipformer-ctc + silero）。
rcp_package_wix(
  RUNTIME "${CMAKE_SOURCE_DIR}/out/bundle/runtime-lite"
  NAME_SUFFIX "-Lite"
)
