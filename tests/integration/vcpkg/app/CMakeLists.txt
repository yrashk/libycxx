# Uses the overlay port ycxx-sample through vcpkg (tests/integration/vcpkg).
cmake_minimum_required(VERSION 3.24)
project(ycxx_vcpkg_app LANGUAGES CXX)
find_path(SUB_INCLUDE sub.hpp REQUIRED)
find_library(SUB_LIB sub REQUIRED)
add_executable(app main.cpp)
target_include_directories(app PRIVATE ${SUB_INCLUDE})
target_link_libraries(app PRIVATE ${SUB_LIB})
