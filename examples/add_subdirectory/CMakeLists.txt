# Builds libycxx as part of this project:
#   cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++-23 [-DLIBYCXX_SOURCE_DIR=<libycxx checkout>]
# (FetchContent works the same way: FetchContent_Declare(libycxx GIT_REPOSITORY ...) and
# FetchContent_MakeAvailable(libycxx) provide the same ycxx::ycxx target.)
cmake_minimum_required(VERSION 3.28)
project(ycxx_example_add_subdirectory LANGUAGES C CXX)

set(LIBYCXX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "libycxx checkout")
add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)

add_executable(demo ../demo.cpp)
target_link_libraries(demo PRIVATE ycxx::ycxx)
