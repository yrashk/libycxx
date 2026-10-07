# A dependency: built by FetchContent within the project, and by ExternalProject on its own.
cmake_minimum_required(VERSION 3.24)
project(sub LANGUAGES CXX)
add_library(sub STATIC sub.cpp)
set_target_properties(sub PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(sub PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
install(TARGETS sub ARCHIVE DESTINATION lib)
