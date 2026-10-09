cmake_minimum_required(VERSION 3.28)

# libycxx is fetched before project(): its toolchain file chooses the compiler,
# Clang 23 by default, and downloads it into a cache when it is missing.
include(FetchContent)
FetchContent_Declare(libycxx
  GIT_REPOSITORY https://github.com/yrashk/libycxx
  GIT_TAG main          # no release yet; a release tag will go here
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR cmake)  # no CMakeLists.txt there: fetched now, added below
FetchContent_MakeAvailable(libycxx)
set(YCXX_COMPILER clang CACHE STRING "clang or gcc")
set(YCXX_PROVISION ON CACHE BOOL "Install the compiler when it is missing")
set(CMAKE_TOOLCHAIN_FILE ${libycxx_SOURCE_DIR}/cmake/ycxx-toolchain.cmake)

project(hello CXX)
add_subdirectory(${libycxx_SOURCE_DIR} ${libycxx_BINARY_DIR} EXCLUDE_FROM_ALL)

add_executable(hello hello.cpp)
# import std; from libycxx's modules (brings ycxx::ycxx)
target_link_libraries(hello PRIVATE ycxx::modules)
