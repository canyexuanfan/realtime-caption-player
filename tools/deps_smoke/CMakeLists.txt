# tools/deps_smoke/CMakeLists.txt
# 仅做“依赖可解析、可链接”的编译期冒烟验证。
# 不 add_test：运行期需要 GUI/音频设备，沙箱无法执行；链接成功即视为依赖就位。
add_executable(deps_smoke main.cpp)
target_link_libraries(deps_smoke PRIVATE
    rcp::ffmpeg
    rcp::mpv
    rcp::sherpa-onnx)
set_target_properties(deps_smoke PROPERTIES
    WIN32_EXECUTABLE OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools/deps_smoke")
