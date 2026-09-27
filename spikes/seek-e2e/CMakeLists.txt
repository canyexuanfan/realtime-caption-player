# spikes/seek-e2e/CMakeLists.txt - caption pipeline seek acceptance probe (phase B)
add_executable(seek_e2e_probe SeekE2EProbe.cpp)
target_link_libraries(seek_e2e_probe PRIVATE
    rcp_audio
    rcp_asr
    rcp_core
    rcp::sherpa-onnx
)
set_target_properties(seek_e2e_probe PROPERTIES WIN32_EXECUTABLE NO)
