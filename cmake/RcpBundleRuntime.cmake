# cmake/RcpBundleRuntime.cmake
# 由 rcp_bundle_target() 的 POST_BUILD 调用(脚本模式 -P)。
# 把原生依赖与模型拷进 RCP_BUNDLE_DEST(通常为 $<TARGET_FILE_DIR:...>)。
if(NOT DEFINED RCP_BUNDLE_DEST)
  message(FATAL_ERROR "RCP_BUNDLE_DEST not set")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/RcpBundle.cmake")
rcp_copy_native_runtime(DEST "${RCP_BUNDLE_DEST}")
