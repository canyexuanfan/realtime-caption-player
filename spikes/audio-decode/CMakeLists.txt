# spikes/audio-decode/CMakeLists.txt
# T0017 探针：FFmpeg 指定音轨解码（链接 rcp::ffmpeg）

add_executable(audio_decode_probe main.cpp)
target_link_libraries(audio_decode_probe PRIVATE rcp::ffmpeg)
set_target_properties(audio_decode_probe PROPERTIES
    WIN32_EXECUTABLE NO
    CXX_STANDARD 20)

# T0018 探针：FFmpeg seek + 采样时钟映射
add_executable(seek_probe SeekProbe.cpp)
target_link_libraries(seek_probe PRIVATE rcp::ffmpeg)
set_target_properties(seek_probe PROPERTIES
    WIN32_EXECUTABLE NO
    CXX_STANDARD 20)
