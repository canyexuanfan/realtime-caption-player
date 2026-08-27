cmake_minimum_required(VERSION 3.20)

project(RealtimeCaptionPlayer
    VERSION 0.1.0
    DESCRIPTION "Realtime Caption Player - local-first offline realtime captioning player (Windows x64)"
    LANGUAGES CXX)

# ---- Global policy / standards ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(BUILD_TESTS "Build unit/integration tests" ON)
option(BUILD_PLAYER "Build the player application" OFF)
option(BUILD_WORKER "Build the caption-worker application" OFF)
option(BUILD_DEPS_SMOKE "Build native-dependency link smoke test (FFmpeg/mpv/sherpa-onnx)" OFF)

# ---- Compiler strictness (MSVC) ----
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)
    # C4163 etc. are noisy; keep warnings but do not treat as errors in early phases.
    add_compile_definitions(UNICODE _UNICODE NOMINMAX)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ---- Qt ----
# Qt 6 is required. Version is recorded in dependencies.lock.json.
# Install via: pwsh ./tools/bootstrap.ps1  (or aqtinstall 6.8.1 win64 qtbase)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Sql Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---- Source layout ----
# Only the modules that currently have real, buildable sources are compiled.
# Modules depending on libmpv / FFmpeg / sherpa-onnx are added in later phases
# (P2/P4/P5) once those native dependencies are built and locked (T0010-T0013).
add_subdirectory(src)
add_subdirectory(resources)
add_subdirectory(migrations)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---- Native dependency integration (T0010-T0012) ----
# Prebuilt libs live in <source>/.tools/{ffmpeg,mpv,sherpa-onnx}.
# Enable BUILD_DEPS_SMOKE to compile a link smoke test proving they resolve.
if(BUILD_DEPS_SMOKE)
  include(cmake/FindRcpNativeDeps.cmake)
  add_subdirectory(tools/deps_smoke)
endif()

# ---- Packaging (P8, requires WiX) ----
# CPack / WiX configuration is added when the installer phase begins.
# include(packaging/wix/wix.cmake)  # enabled in P8

message(STATUS "RealtimeCaptionPlayer ${PROJECT_VERSION}")
message(STATUS "BUILD_TESTS=${BUILD_TESTS} BUILD_PLAYER=${BUILD_PLAYER} BUILD_WORKER=${BUILD_WORKER}")
