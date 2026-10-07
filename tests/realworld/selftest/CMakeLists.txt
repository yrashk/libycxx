# The self-test of tools/realworld's linkage proof (not a project of its own): a program and a
# shared library that use the standard library's strings, streams and exceptions, built once
# against libycxx (the proof must accept it) and once with the toolchain's own C++ library
# (libstdc++, or libc++ on macOS: the proof must reject it on every count).
cmake_minimum_required(VERSION 3.20)
project(realworld_selftest CXX)
set(CMAKE_CXX_STANDARD 20)
add_library(selftest_greeting SHARED greeting.cpp)
add_executable(selftest main.cpp)
target_link_libraries(selftest PRIVATE selftest_greeting)
enable_testing()
add_test(NAME greeting COMMAND selftest)
