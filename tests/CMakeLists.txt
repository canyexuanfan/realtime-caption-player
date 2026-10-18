# Unit tests for the pure-logic modules (no native-lib dependency).
# Each test is a standalone QtTest executable using QTEST_GUILESS_MAIN so it
# runs headless (no platform plugin required).

set(RCP_UNIT_TESTS
    test_error
    test_timecode
    test_frame_codec
    test_json_message_codec
    test_protocol_constants
    test_app_paths
    test_logging
    test_cancellation
    test_settings_validation
    test_settings_service
    test_settings_migration
    test_keymap_service
    test_keymap_conflicts
    test_bounded_queue
    test_segment_assembler
    test_caption_types
    test_media_identity
    test_srt_exporter
    test_default_settings
    test_sql_persistence
)

foreach(t ${RCP_UNIT_TESTS})
    add_executable(${t} unit/${t}.cpp)
    target_link_libraries(${t} PRIVATE rcp_core Qt6::Test Qt6::Core Qt6::Gui)
    target_include_directories(${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${t} PRIVATE RCP_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME ${t} COMMAND ${t})
endforeach()

# Qt SQL 插件运行时部署：QSQLITE 驱动以插件 DLL 形式存在，ctest 直接运行 exe 时
# 必须位于 exe 同目录的 sqldrivers/ 下，否则 test_sql_persistence 报
# "QSQLITE driver not loaded"（干净构建目录与 CI 均会命中；本地手工 windeployqt
# 不会覆盖 tests/，故由构建系统自动拷贝保证可复现）。
if(TARGET Qt6::QSQLiteDriverPlugin)
    add_custom_command(TARGET test_sql_persistence POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:test_sql_persistence>/sqldrivers"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::QSQLiteDriverPlugin>"
                "$<TARGET_FILE_DIR:test_sql_persistence>/sqldrivers/"
        COMMENT "Deploying QSQLITE plugin for test_sql_persistence")
endif()
