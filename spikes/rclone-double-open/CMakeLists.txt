# T0019 探针：本地与 rclone full 双开读取
add_executable(double_open_probe DoubleOpenProbe.cpp)
target_link_libraries(double_open_probe PRIVATE rcp::mpv rcp::ffmpeg)
set_target_properties(double_open_probe PROPERTIES
    WIN32_EXECUTABLE NO
    CXX_STANDARD 20)
