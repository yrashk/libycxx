# A dependency: built by FetchContent within the project, and by ExternalProject on its own.
cmake_minimum_required(VERSION 3.28)   # 3.28: C++20 sources are scanned for modules
project(sub LANGUAGES CXX)
add_library(sub STATIC sub.cpp)
# C++20: CMake scans it for module dependencies (with Clang, clang-scan-deps; the toolchain file names it).
set_target_properties(sub PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_STANDARD 20)
target_include_directories(sub PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS sub ARCHIVE DESTINATION lib)
