# spikes/CMakeLists.txt
# 仅在 BUILD_SPIKES=ON 时被顶层 add_subdirectory 引入。
# 各探针子目录按需增量添加（随 T0015-T0023 推进）。

add_subdirectory(mpv-render)
