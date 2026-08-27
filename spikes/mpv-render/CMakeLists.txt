# spikes/mpv-render/CMakeLists.txt
# T0015 探针：libmpv 事件桥 / 属性观察（链接 rcp::mpv）

add_executable(mpv_event_probe MpvEventProbe.cpp)
target_link_libraries(mpv_event_probe PRIVATE rcp::mpv)
set_target_properties(mpv_event_probe PROPERTIES
    WIN32_EXECUTABLE NO
    CXX_STANDARD 20)

# T0016 探针：ASS 字幕 overlay + 带字幕截图
add_executable(overlay_probe OverlayProbe.cpp)
target_link_libraries(overlay_probe PRIVATE rcp::mpv)
set_target_properties(overlay_probe PROPERTIES
    WIN32_EXECUTABLE NO
    CXX_STANDARD 20)
