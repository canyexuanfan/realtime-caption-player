# spikes/pipeline-probe/CMakeLists.txt - caption pipeline crash-location probe (B2)
add_executable(pipeline_probe PipelineProbe.cpp)
target_link_libraries(pipeline_probe PRIVATE
    rcp_worker_core
    rcp_audio
    rcp_asr
    rcp_core
    rcp::sherpa-onnx
)
set_target_properties(pipeline_probe PROPERTIES WIN32_EXECUTABLE NO)
