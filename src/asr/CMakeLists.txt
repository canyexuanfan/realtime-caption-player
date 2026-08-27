# src/asr/CMakeLists.txt
# P4: 双引擎 ASR 库（Zipformer2-CTC 实时 partial + SenseVoice 精准终稿）。
add_library(rcp_asr STATIC
    AsrEngine.cpp
    AsrEngine.h
)

target_include_directories(rcp_asr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(rcp_asr PUBLIC
    rcp_core
    rcp::sherpa-onnx
    Qt6::Core
)
