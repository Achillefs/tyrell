cmake_minimum_required(VERSION 3.22)
project(VP330 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat warnings as errors only in CI; locally just warn.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

option(VP330_ENABLE_TESTS  "Build vp330_tests and golden test suite" ON)
option(VP330_ENABLE_RENDER "Build vp330_render CLI"                  ON)

if(VP330_ENABLE_TESTS OR VP330_ENABLE_RENDER)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
endif()

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.14.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG b2d9ed2dddefc4b84318d664b4f221eb792d89c7
)
# Note: RC_ENABLE_CATCH causes rapidcheck to build its bundled Catch2 (in ext/)
# which conflicts with our external Catch2. Instead, we build rapidcheck without
# it and manually create the rapidcheck_catch target (matching extras/catch).
FetchContent_MakeAvailable(rapidcheck)

FetchContent_Declare(
    clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        e8de9e8571626633b8541a54c2406fccc4272767
)
FetchContent_MakeAvailable(clap_juce_extensions)

# Manually create rapidcheck_catch INTERFACE library.
# Mirrors extras/catch/CMakeLists.txt (integration layer, not bundled Catch2).
add_library(rapidcheck_catch INTERFACE)
target_link_libraries(rapidcheck_catch INTERFACE rapidcheck)
target_include_directories(rapidcheck_catch INTERFACE
    $<BUILD_INTERFACE:${rapidcheck_SOURCE_DIR}/extras/catch/include>
)

enable_testing()
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_subdirectory(domain)
add_subdirectory(infrastructure)
if(VP330_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
