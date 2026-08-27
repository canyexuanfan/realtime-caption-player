# cmake/run_bundle.cmake — 独立验证驱动(脚本模式 cmake -P)
# 用法: cmake -P cmake/run_bundle.cmake
# 作用: 把 .tools 下的原生依赖 DLL 与 ASR 模型拷进 out/bundle/runtime，
#       验证"打包进程序"机制可用(无需 Qt / 编译器)。
set(RCP_DEPS_ROOT "F:/workbuddy/视频播放器/.tools")
include("${CMAKE_CURRENT_LIST_DIR}/RcpBundle.cmake")
rcp_copy_native_runtime(DEST "F:/workbuddy/视频播放器/out/bundle/runtime")
