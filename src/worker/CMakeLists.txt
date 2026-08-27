# src/worker/CMakeLists.txt
# P4: caption-worker 可执行进程（文件模式；后续接 IPC server）。
add_executable(caption_worker main.cpp)

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

# 沙箱链接限制：禁用 manifest 嵌入(/MANIFEST:NO)，避免 cvtres 在受限 TEMP 下失败。
target_link_options(caption_worker PRIVATE "/MANIFEST:NO")
