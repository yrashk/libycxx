# A consumer of the package tests/integration/cmake-project installs (install(EXPORT)).
cmake_minimum_required(VERSION 3.24)
project(ycxx_integration_consumer LANGUAGES CXX)
find_package(greet CONFIG REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE greet::greet)
