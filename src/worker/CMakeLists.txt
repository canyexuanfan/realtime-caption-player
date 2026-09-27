# src/worker/CMakeLists.txt
# P4: caption-worker 可执行进程（文件模式；后续接 IPC server）。
add_executable(caption_worker
    main.cpp
    IpcServer.cpp
    IpcServer.h
)

target_include_directories(caption_worker PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(caption_worker PRIVATE
    rcp_audio
    rcp_asr
    rcp_core
    rcp::ffmpeg
    rcp::sherpa-onnx
    Qt6::Core
    Qt6::Network
)

set_target_properties(caption_worker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# manifest 嵌入默认开启；沙箱受限 TEMP 下 cvtres 失败时加 -DRCP_SANDBOX_NO_MANIFEST=ON。
option(RCP_SANDBOX_NO_MANIFEST "沙箱链接 workaround：禁用 manifest 嵌入" OFF)
if(RCP_SANDBOX_NO_MANIFEST)
    target_link_options(caption_worker PRIVATE "/MANIFEST:NO")
endif()
