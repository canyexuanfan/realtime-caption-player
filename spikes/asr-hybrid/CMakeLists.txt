# T0020/T0021/T0022 探针：sherpa-onnx ASR 混合链路
add_executable(online_probe OnlineProbe.cpp)
target_link_libraries(online_probe PRIVATE rcp::sherpa-onnx)
set_target_properties(online_probe PROPERTIES WIN32_EXECUTABLE NO CXX_STANDARD 20)

add_executable(sensevoice_probe SenseVoiceProbe.cpp)
target_link_libraries(sensevoice_probe PRIVATE rcp::sherpa-onnx)
set_target_properties(sensevoice_probe PROPERTIES WIN32_EXECUTABLE NO CXX_STANDARD 20)

add_executable(hybrid_probe HybridProbe.cpp)
target_link_libraries(hybrid_probe PRIVATE rcp::sherpa-onnx)
set_target_properties(hybrid_probe PROPERTIES WIN32_EXECUTABLE NO CXX_STANDARD 20)

add_executable(online_probe_real OnlineProbeReal.cpp)
target_link_libraries(online_probe_real PRIVATE rcp::sherpa-onnx)
set_target_properties(online_probe_real PROPERTIES WIN32_EXECUTABLE NO CXX_STANDARD 20)
