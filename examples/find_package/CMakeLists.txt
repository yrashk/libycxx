# Uses an installed libycxx:
#   cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-16 -DCMAKE_PREFIX_PATH=<libycxx install prefix>
cmake_minimum_required(VERSION 3.28)
project(ycxx_example_find_package LANGUAGES CXX)

find_package(libycxx 0.1 CONFIG REQUIRED)

add_executable(demo ../demo.cpp)
target_link_libraries(demo PRIVATE ycxx::ycxx)
