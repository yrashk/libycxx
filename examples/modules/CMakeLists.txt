# `import std;` and `import std.compat;` with libycxx's modules (ycxx::modules). Needs CMake >= 3.28
# and a Ninja generator (C++20 module scanning):
#   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++-23 -DCMAKE_PREFIX_PATH=<libycxx install prefix>
# or, with libycxx built as part of the project:
#   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-16 -DLIBYCXX_SOURCE_DIR=<libycxx checkout>
cmake_minimum_required(VERSION 3.28)
project(ycxx_example_modules LANGUAGES C CXX)

if(LIBYCXX_SOURCE_DIR)
  add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)
else()
  find_package(libycxx 0.1 CONFIG REQUIRED)
endif()

add_executable(demo_modules ../modules.cpp)
# ycxx::modules brings ycxx::ycxx; CMake builds the modules' interfaces for this project.
target_link_libraries(demo_modules PRIVATE ycxx::modules)
