# cmake/FindRcpNativeDeps.cmake
#
# 在工作空间 `.tools/` 下查找预编译的原生依赖（FFmpeg / libmpv / sherpa-onnx），
# 并创建导入接口库：
#   rcp::ffmpeg        -> avformat avcodec avutil swresample avfilter
#   rcp::mpv           -> mpv.lib (libmpv client API)
#   rcp::sherpa-onnx   -> sherpa-onnx-c-api.lib
#
# 依赖由 tools 下的下载脚本解压到：
#   <source>/.tools/ffmpeg/{bin,include,lib}
#   <source>/.tools/mpv/{bin,include,lib}
#   <source>/.tools/sherpa-onnx/{bin,include,lib}
#
# 设置 RCP_DEPS_ROOT 可覆盖默认查找根目录。
# 由顶层 CMakeLists 在 BUILD_DEPS_SMOKE=ON 时 include；缺失即 FATAL_ERROR。

if(NOT DEFINED RCP_DEPS_ROOT)
  set(RCP_DEPS_ROOT "${CMAKE_SOURCE_DIR}/.tools")
endif()
message(STATUS "RCP_DEPS_ROOT = ${RCP_DEPS_ROOT}")

# ---------------- FFmpeg (lgpl-shared) ----------------
find_path(FFMPEG_INCLUDE_DIR
  NAMES libavformat/avformat.h
  HINTS "${RCP_DEPS_ROOT}/ffmpeg/include"
  NO_DEFAULT_PATH NO_CMAKE_SYSTEM_PATH)

set(_ffmpeg_components avformat avcodec avutil swresample avfilter)
set(FFMPEG_LIBRARIES "")
set(_ffmpeg_missing "")
foreach(_c ${_ffmpeg_components})
  find_library(FFMPEG_${_c}_LIBRARY
    NAMES ${_c}
    HINTS "${RCP_DEPS_ROOT}/ffmpeg/lib"
    NO_DEFAULT_PATH NO_CMAKE_SYSTEM_PATH)
  if(NOT FFMPEG_${_c}_LIBRARY)
    list(APPEND _ffmpeg_missing ${_c})
  else()
    list(APPEND FFMPEG_LIBRARIES "${FFMPEG_${_c}_LIBRARY}")
  endif()
endforeach()

# ---------------- libmpv ----------------
find_path(MPV_INCLUDE_DIR
  NAMES mpv/client.h
  HINTS "${RCP_DEPS_ROOT}/mpv/include"
  NO_DEFAULT_PATH NO_CMAKE_SYSTEM_PATH)
find_library(MPV_LIBRARY
  NAMES mpv
  HINTS "${RCP_DEPS_ROOT}/mpv/lib"
  NO_DEFAULT_PATH NO_CMAKE_SYSTEM_PATH)

# ---------------- sherpa-onnx ----------------
find_path(SHERPA_ONNX_INCLUDE_DIR
  NAMES sherpa-onnx/c-api/c-api.h
  HINTS "${RCP_DEPS_ROOT}/sherpa-onnx/include"
  NO_DEFAULT_PATH NO_CMAKE_SYSTEM_PATH)
find_library(SHERPA_ONNX_C_API_LIBRARY
  NAMES sherpa-onnx-c-api
  HINTS "${RCP_DEPS_ROOT}/sherpa-onnx/lib"
  NO_DEFAULT_PATH NO_CMAKE_SYSTEM_PATH)

# ---------------- 断言 + 建接口库 ----------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RcpNativeDeps
  REQUIRED_VARS
    FFMPEG_INCLUDE_DIR FFMPEG_LIBRARIES
    MPV_INCLUDE_DIR MPV_LIBRARY
    SHERPA_ONNX_INCLUDE_DIR SHERPA_ONNX_C_API_LIBRARY)

if(RcpNativeDeps_FOUND)
  add_library(rcp::ffmpeg INTERFACE IMPORTED)
  set_target_properties(rcp::ffmpeg PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}")

  add_library(rcp::mpv INTERFACE IMPORTED)
  set_target_properties(rcp::mpv PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${MPV_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${MPV_LIBRARY}")

  add_library(rcp::sherpa-onnx INTERFACE IMPORTED)
  set_target_properties(rcp::sherpa-onnx PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SHERPA_ONNX_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${SHERPA_ONNX_C_API_LIBRARY}")

  message(STATUS "RcpNativeDeps: ffmpeg libs = ${FFMPEG_LIBRARIES}")
  message(STATUS "RcpNativeDeps: mpv lib      = ${MPV_LIBRARY}")
  message(STATUS "RcpNativeDeps: sherpa lib   = ${SHERPA_ONNX_C_API_LIBRARY}")
endif()
