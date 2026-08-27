# RealtimeCaptionPlayer — shared static library of pure-logic + platform modules.
# Native-library-dependent modules (libmpv/FFmpeg/sherpa-onnx) are added in
# later phases and gated behind the corresponding dependency targets.

set(RCP_CORE_SOURCES
    core/Error.cpp
    core/Timecode.cpp
    core/Logging.cpp
    core/AppPaths.cpp
)

set(RCP_IPC_SOURCES
    ipc/FrameCodec.cpp
    ipc/JsonMessageCodec.cpp
    ipc/Protocol.cpp
)

set(RCP_CAPTION_SOURCES
    captions/AssEscaper.cpp
    captions/CaptionStateMachine.cpp
    captions/SrtExporter.cpp
)

set(RCP_SETTINGS_SOURCES
    settings/SettingsValidation.cpp
    settings/SettingsService.cpp
    settings/SettingsMigration.cpp
    settings/KeymapService.cpp
)

set(RCP_STORAGE_SOURCES
    storage/MediaIdentity.cpp
)

set(RCP_WORKER_SOURCES
    worker/SegmentAssembler.cpp
)

# Header-only modules (no .cpp): core/Result.h, core/Error.h, core/Cancellation.h,
# core/Generation.h, worker/BoundedQueue.h, ipc/Protocol.h, captions/CaptionTypes.h,
# settings/Settings.h — included via add_library with the headers listed so IDEs
# and automoc see them.

add_library(rcp_core STATIC
    ${RCP_CORE_SOURCES}
    ${RCP_IPC_SOURCES}
    ${RCP_CAPTION_SOURCES}
    ${RCP_SETTINGS_SOURCES}
    ${RCP_STORAGE_SOURCES}
    ${RCP_WORKER_SOURCES}
    core/Result.h
    core/Error.h
    core/Cancellation.h
    core/Generation.h
    ipc/Protocol.h
    captions/CaptionTypes.h
    settings/Settings.h
    worker/BoundedQueue.h
)

target_include_directories(rcp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rcp_core PUBLIC
    Qt6::Core
    Qt6::Gui
)

# QtTest is only needed by the test target, not the library itself.
