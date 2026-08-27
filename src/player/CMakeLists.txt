# src/player/CMakeLists.txt
# T0014: libmpv 播放封装（rcp_player 静态库）+ 链接验证可执行文件 player_app。
# 依赖 rcp::mpv（由 cmake/FindRcpNativeDeps.cmake 在 BUILD_PLAYER=ON 时提供）。

add_library(rcp_player STATIC
    MpvPlayer.cpp
    MpvPlayer.h
)

target_include_directories(rcp_player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rcp_player PUBLIC
    rcp_core
    rcp::mpv
    Qt6::Core
    Qt6::Gui
)

# 链接验证：在沙箱中以"编译 + 链接"确认 libmpv 可嵌入。
# 真实渲染需 GUI/GL 上下文，运行期在真实 Windows 会话验证（不在此执行）。
add_executable(player_app player_app.cpp)
target_link_libraries(player_app PRIVATE
    rcp_player
)
set_target_properties(player_app PROPERTIES
    WIN32_EXECUTABLE OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# 沙箱链接限制：禁用 manifest 嵌入(/MANIFEST:NO)，避免 cvtres 在受限 TEMP 下
# 写临时文件报 CVT1108/LNK1123。验证壳无需 manifest；正式发布可恢复嵌入。
target_link_options(player_app PRIVATE "/MANIFEST:NO")
