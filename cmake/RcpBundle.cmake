# cmake/RcpBundle.cmake
#
# 把原生依赖(FFmpeg / libmpv / sherpa-onnx 的运行时 DLL)与 ASR 模型
# 打包进一个"自包含运行时目录"，使发布产物无需任何外部下载即可离线运行。
#
# 设计目标(直接回答"能否打包进程序 / 用户还要不要下载"):
#   - 开发者侧：本脚本/下载流程一次性把依赖放进 <source>/.tools/ (已下载，非用户侧)。
#   - 构建/打包侧：本模块把 .tools 里的 DLL 与 models/ 拷进运行时目录。
#   - 发布侧：WiX(或 CPack) 把该运行时目录整体打进安装包。
#   => 最终用户拿到的是一个完整安装包，安装即用，零运行时下载。
#
# 支持两种用法：
#   A) 脚本模式(cmake -P)：直接调用 rcp_copy_native_runtime(DEST <dir>)
#   B) 工程模式：include 后调用 rcp_bundle_target(<target>) 给目标加 post-build 拷贝
#
# 注意：
#   - 只拷贝 *.dll 运行时库，跳过 *.lib/*.def/*.a 等导入库与 CLI exe，避免包体膨胀。
#   - 跳过 *.part 临时文件(断点续传残留)与 *.bad 隔离坏文件，避免把未完成内容打进包。

if(NOT DEFINED RCP_DEPS_ROOT)
  set(RCP_DEPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.tools")
endif()
message(STATUS "RcpBundle: RCP_DEPS_ROOT = ${RCP_DEPS_ROOT}")

# 收集需要拷贝的运行时目录(bin 与 lib 都算；存在才加入)
function(rcp_collect_dll_dirs out_var)
  set(_dirs "")
  foreach(_dep ffmpeg mpv sherpa-onnx)
    foreach(_sub bin lib)
      set(_b "${RCP_DEPS_ROOT}/${_dep}/${_sub}")
      if(EXISTS "${_b}")
        list(APPEND _dirs "${_b}")
      endif()
    endforeach()
  endforeach()
  set(${out_var} "${_dirs}" PARENT_SCOPE)
endfunction()

# 拷贝单个目录下的 *.dll 到 DEST(平铺)，跳过 .part/.bad
function(_rcp_copy_dlls src dst)
  if(NOT EXISTS "${src}")
    return()
  endif()
  file(GLOB_RECURSE _files "${src}/*.dll")
  foreach(_f ${_files})
    if(_f MATCHES "\\.(part|bad)$")
      continue()
    endif()
    if(EXISTS "${_f}.part")
      message(STATUS "RcpBundle: skip incomplete (has .part) ${_f}")
      continue()
    endif()
    get_filename_component(_name "${_f}" NAME)
    set(_d "${dst}/${_name}")
    file(MAKE_DIRECTORY "${dst}")
    file(COPY "${_f}" DESTINATION "${dst}")
  endforeach()
endfunction()

# 拷贝模型目录树(保留目录结构)，跳过 .part/.bad
function(_rcp_copy_tree src dst)
  if(NOT EXISTS "${src}")
    return()
  endif()
  file(GLOB_RECURSE _files "${src}/*")
  foreach(_f ${_files})
    if(_f MATCHES "\\.(part|bad)$")
      continue()
    endif()
    if(IS_DIRECTORY "${_f}")
      continue()
    endif()
    if(EXISTS "${_f}.part")
      message(STATUS "RcpBundle: skip incomplete (has .part) ${_f}")
      continue()
    endif()
    file(RELATIVE_PATH _rel "${src}" "${_f}")
    set(_d "${dst}/${_rel}")
    get_filename_component(_dd "${_d}" DIRECTORY)
    file(MAKE_DIRECTORY "${_dd}")
    file(COPY "${_f}" DESTINATION "${_dd}")
  endforeach()
endfunction()

# 主入口：把原生依赖与模型拷进 DEST
function(rcp_copy_native_runtime)
  set(options "")
  set(oneValueArgs DEST)
  set(multiValueArgs "")
  cmake_parse_arguments(RCP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT RCP_DEST)
    message(FATAL_ERROR "rcp_copy_native_runtime requires DEST <dir>")
  endif()
  file(MAKE_DIRECTORY "${RCP_DEST}")

  # 1) 各依赖 bin/lib 目录里的运行时 DLL
  rcp_collect_dll_dirs(_dirs)
  foreach(_d ${_dirs})
    message(STATUS "RcpBundle: copy runtime DLLs from ${_d}")
    _rcp_copy_dlls("${_d}" "${RCP_DEST}")
  endforeach()

  # 2) ASR 模型目录(models/)
  set(_models "${RCP_DEPS_ROOT}/models")
  if(EXISTS "${_models}")
    message(STATUS "RcpBundle: copy models from ${_models}")
    _rcp_copy_tree("${_models}" "${RCP_DEST}/models")
  else()
    message(WARNING "RcpBundle: models dir not found at ${_models}")
  endif()

  message(STATUS "RcpBundle: self-contained runtime ready at ${RCP_DEST}")
endfunction()

# 工程模式：给已有 target 加 post-build 拷贝到其输出目录
function(rcp_bundle_target target)
  rcp_collect_dll_dirs(_dirs)
  set(_models "${RCP_DEPS_ROOT}/models")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMENT "Bundling native deps + models next to ${target}"
    COMMAND "${CMAKE_COMMAND}"
            "-DRCP_DEPS_ROOT=${RCP_DEPS_ROOT}"
            "-DRCP_BUNDLE_DEST=$<TARGET_FILE_DIR:${target}>"
            -P "${CMAKE_CURRENT_LIST_DIR}/RcpBundleRuntime.cmake")
endfunction()
