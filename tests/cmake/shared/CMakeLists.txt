# libycxx's shared library from a CMake project (tests/cmake/run.sh, 14.; DECISIONS §20.2), with an
# installation of both kinds (-DYCXX_SHARED=ON):
#   demo_shared   examples/demo.cpp linked with ycxx::shared: needs libycxx.so.0.<minor>
#   demo_static   the same with ycxx::static: needs no libycxx
#   demo_default  the same with ycxx::ycxx: the default kind, static
#   modules_shared  examples/modules.cpp (import std.compat;) with ycxx::shared_modules
# With -DBOTH=ON a target links both kinds (through a dependency), which CMake must refuse (the
# YCXX_LINKAGE interface property).
cmake_minimum_required(VERSION 3.28)
project(ycxx_shared_test LANGUAGES CXX)

find_package(libycxx 0.1 CONFIG REQUIRED)
if(NOT libycxx_BUILT_SHARED OR NOT libycxx_BUILT_STATIC OR NOT TARGET ycxx::shared OR NOT TARGET ycxx::static)
  message(FATAL_ERROR "libycxx was not built with both kinds (-DYCXX_SHARED=ON)")
endif()

set(examples ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples)
add_executable(demo_shared ${examples}/demo.cpp)
target_link_libraries(demo_shared PRIVATE ycxx::shared)
add_executable(demo_static ${examples}/demo.cpp)
target_link_libraries(demo_static PRIVATE ycxx::static)
add_executable(demo_default ${examples}/demo.cpp)
target_link_libraries(demo_default PRIVATE ycxx::ycxx)
if(TARGET ycxx::shared_modules)
  add_executable(modules_shared ${examples}/modules.cpp)
  target_link_libraries(modules_shared PRIVATE ycxx::shared_modules)
endif()

if(BOTH)
  add_library(uses_static INTERFACE)
  target_link_libraries(uses_static INTERFACE ycxx::static)
  add_executable(demo_both ${examples}/demo.cpp)
  target_link_libraries(demo_both PRIVATE uses_static ycxx::shared)
endif()
