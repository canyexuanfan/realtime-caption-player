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
    test_caption_state_machine
    test_media_identity
    test_ass_escaper
    test_srt_exporter
    test_default_settings
)

foreach(t ${RCP_UNIT_TESTS})
    add_executable(${t} unit/${t}.cpp)
    target_link_libraries(${t} PRIVATE rcp_core Qt6::Test Qt6::Core Qt6::Gui)
    target_include_directories(${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${t} PRIVATE RCP_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME ${t} COMMAND ${t})
endforeach()
