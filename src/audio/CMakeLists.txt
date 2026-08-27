# src/audio/CMakeLists.txt
# P4: FFmpeg 音频抽取库（抽音轨 -> 16k mono float 分块）。
add_library(rcp_audio STATIC
    AudioExtractor.cpp
    AudioExtractor.h
)

target_include_directories(rcp_audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(rcp_audio PUBLIC
    rcp::ffmpeg
    Qt6::Core
)
