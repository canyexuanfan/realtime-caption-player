# src/player/CMakeLists.txt
# P2: 播放内核 (rcp_player 静态库) + 渲染控件 + 基础窗口 + GUI 播放器入口。
# 依赖 rcp::mpv（由 cmake/FindRcpNativeDeps.cmake 在 BUILD_PLAYER=ON 时提供）
# 与 Qt6 Widgets/OpenGLWidgets。

find_package(Qt6 COMPONENTS OpenGLWidgets REQUIRED)

add_library(rcp_player STATIC
    MpvPlayer.cpp
    MpvPlayer.h
    MpvRenderWidget.cpp
    MpvRenderWidget.h
    MainWindow.cpp
    MainWindow.h
    WorkerSupervisor.cpp
    WorkerSupervisor.h
)

target_include_directories(rcp_player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(rcp_player PUBLIC
    rcp_core
    rcp::mpv
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Network
    opengl32
)

# 真实播放器入口（GUI 程序，无控制台窗口）。
add_executable(player_app main.cpp)
target_link_libraries(player_app PRIVATE
    rcp_player
)
set_target_properties(player_app PROPERTIES
    WIN32_EXECUTABLE ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# 沙箱链接限制：禁用 manifest 嵌入(/MANIFEST:NO)，避免 cvtres 在受限 TEMP 下
# 写临时文件报 CVT1108/LNK1123。验证壳无需 manifest；正式发布可恢复嵌入。
target_link_options(player_app PRIVATE "/MANIFEST:NO")
