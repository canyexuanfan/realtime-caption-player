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
option(BUILD_SPIKES "Build spike probes (T0015-T0023)" OFF)

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
# 工作空间自带 .qt6/<ver>/msvc2022_64；若它存在则自动加入 CMAKE_PREFIX_PATH，
# 使 find_package(Qt6) 在干净环境/沙箱中也能解析（无需全局 Qt 安装或手动传 -DCMAKE_PREFIX_PATH）。
if(EXISTS "${CMAKE_SOURCE_DIR}/.qt6/6.8.1/msvc2022_64")
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/.qt6/6.8.1/msvc2022_64")
endif()
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
# rcp::ffmpeg / rcp::mpv / rcp::sherpa-onnx 由 cmake/FindRcpNativeDeps.cmake 定义；
# 在 BUILD_DEPS_SMOKE 或 BUILD_PLAYER 时都需要。
if(BUILD_DEPS_SMOKE OR BUILD_PLAYER OR BUILD_SPIKES OR BUILD_WORKER)
  include(cmake/FindRcpNativeDeps.cmake)
endif()

if(BUILD_DEPS_SMOKE)
  add_subdirectory(tools/deps_smoke)
endif()

if(BUILD_PLAYER)
  add_subdirectory(src/player)
endif()

if(BUILD_WORKER)
  add_subdirectory(src/audio)
  add_subdirectory(src/asr)
  add_subdirectory(src/worker)
endif()

if(BUILD_SPIKES)
  add_subdirectory(spikes)
endif()

# ---- Packaging (P8, requires WiX) ----
# CPack / WiX configuration is added when the installer phase begins.
# P8 启用：把 out/bundle/runtime 自包含运行时打进 MSI（需先运行 RcpBundle 生成运行时）。
include(packaging/wix/wix.cmake)

message(STATUS "RealtimeCaptionPlayer ${PROJECT_VERSION}")
message(STATUS "BUILD_TESTS=${BUILD_TESTS} BUILD_PLAYER=${BUILD_PLAYER} BUILD_WORKER=${BUILD_WORKER} BUILD_SPIKES=${BUILD_SPIKES}")
