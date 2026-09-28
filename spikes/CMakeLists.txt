# spikes/CMakeLists.txt
# 仅在 BUILD_SPIKES=ON 时被顶层 add_subdirectory 引入。
# 各探针子目录按需增量添加（随 T0015-T0023 推进）。

add_subdirectory(mpv-render)
add_subdirectory(audio-decode)
add_subdirectory(rclone-double-open)
add_subdirectory(asr-hybrid)
add_subdirectory(seek-e2e)
add_subdirectory(pipeline-probe)
add_subdirectory(visual-probe)
