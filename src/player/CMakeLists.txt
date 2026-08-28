# src/player/CMakeLists.txt
# P2: 播放内核 (rcp_player 静态库) + 渲染控件 + 基础窗口 + GUI 播放器入口。
# 依赖 rcp::mpv（由 cmake/FindRcpNativeDeps.cmake 在 BUILD_PLAYER=ON 时提供）
# 与 Qt6 Widgets/OpenGLWidgets。

find_package(Qt6 COMPONENTS OpenGLWidgets Svg REQUIRED)

add_library(rcp_player STATIC
    MpvPlayer.cpp
    MpvPlayer.h
    MpvRenderWidget.cpp
    MpvRenderWidget.h
    MainWindow.cpp
    MainWindow.h
    UiKit.h
    WorkerSupervisor.cpp
    WorkerSupervisor.h
)

target_link_libraries(rcp_player PUBLIC
    rcp_core
    rcp::mpv
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Svg
    Qt6::Network
    opengl32
)

# UI 资源（参考稿复刻）：品牌 logo + sprite 图标 SVG 集（见下方 AUTORCC + rcp.qrc）。
file(GLOB RCP_UI_ICONS "${CMAKE_SOURCE_DIR}/resources/icons/*.svg")


# 真实播放器入口（GUI 程序，无控制台窗口）。
# 资源用经典 AUTORCC + 手写 rcp.qrc（含 alias logo.png/icons/*.svg）。
# 教训：qt_add_resources 挂静态库→初始化对象被链接器丢弃；且 FILES 不支持
# alias（资源路径变 logo128.png，代码读 :/logo.png 必失败）。
add_executable(player_app main.cpp
    "${CMAKE_SOURCE_DIR}/resources/rcp.qrc"
    "${CMAKE_SOURCE_DIR}/resources/app.rc"   # exe 图标（霓虹语音 logo .ico）
)
set_property(TARGET player_app PROPERTY AUTORCC ON)
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
