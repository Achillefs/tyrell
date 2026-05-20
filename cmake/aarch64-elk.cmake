# Cross-compilation toolchain for Raspberry Pi 4 / Elk Audio OS (aarch64-linux-gnu).
#
# Usage (local):
#   cmake -B build-arm -S . \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-elk.cmake \
#     -DELK_SYSROOT=/path/to/elk-sdk/sysroots/aarch64-elk-linux \
#     -DVP330_ENABLE_TESTS=OFF \
#     -DVP330_ENABLE_RENDER=OFF
#
# In CI the ELK_SYSROOT variable is passed via -D from the workflow.

cmake_minimum_required(VERSION 3.22)

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_AR           aarch64-linux-gnu-ar    CACHE FILEPATH "")
set(CMAKE_RANLIB       aarch64-linux-gnu-ranlib CACHE FILEPATH "")
set(CMAKE_STRIP        aarch64-linux-gnu-strip  CACHE FILEPATH "")

# ELK_SYSROOT can be passed as a CMake variable or environment variable.
if(DEFINED ELK_SYSROOT)
    set(CMAKE_SYSROOT        "${ELK_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH "${ELK_SYSROOT}")
elseif(DEFINED ENV{ELK_SYSROOT})
    set(CMAKE_SYSROOT        "$ENV{ELK_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH "$ENV{ELK_SYSROOT}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
