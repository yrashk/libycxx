# `import std;` and std::meta with libycxx built with YCXX_MODULES_REFLECTION=ON (GCC): the project
# sets no -freflection of its own; ycxx::modules brings it, for the module's CMI and the program
# alike (tests/cmake/run.sh, 10).
#   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-16 -DCMAKE_PREFIX_PATH=<libycxx install prefix>
# or, with libycxx built as part of the project:
#   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-16 -DLIBYCXX_SOURCE_DIR=<libycxx checkout> -DYCXX_MODULES_REFLECTION=ON
cmake_minimum_required(VERSION 3.28)
project(ycxx_test_modules_reflection LANGUAGES C CXX)

if(LIBYCXX_SOURCE_DIR)
  add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)
else()
  find_package(libycxx 0.1 CONFIG REQUIRED)
endif()

add_executable(demo_reflection main.cpp)
target_link_libraries(demo_reflection PRIVATE ycxx::modules)
