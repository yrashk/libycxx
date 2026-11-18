# `import std;` and std::meta with libycxx built with YCXX_MODULES_REFLECTION=ON (GCC): the project
# sets no -freflection of its own; ycxx::modules brings it, for the module's CMI and the program
# alike (tests/cmake/run.sh, 10), and none to the program's C source (side.c, built with -Werror).
#   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-16 -DCMAKE_PREFIX_PATH=<libycxx install prefix>
# or, with libycxx built as part of the project:
#   cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-16 -DLIBYCXX_SOURCE_DIR=<libycxx checkout> -DYCXX_MODULES_REFLECTION=ON
# With libycxx built without the option, YCXX_TEST_OWN_REFLECTION gives the program -freflection
# itself: "target" (its own options) and "dependency" (a linked interface library's) stop the
# configuration before CMake 4.4 (cmake/ycxx-modules-reflection.cmake) and build with CMake 4.4
# and later; so does "directory" (add_compile_options ahead of libycxx) with find_package, while
# with add_subdirectory the module gets it too and it builds with any CMake.
cmake_minimum_required(VERSION 3.28)
project(ycxx_test_modules_reflection LANGUAGES C CXX)

if(YCXX_TEST_OWN_REFLECTION STREQUAL "directory")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-freflection>)
endif()
if(LIBYCXX_SOURCE_DIR)
  add_subdirectory(${LIBYCXX_SOURCE_DIR} libycxx EXCLUDE_FROM_ALL)
else()
  find_package(libycxx 0.1 CONFIG REQUIRED)
endif()

add_executable(demo_reflection main.cpp side.c)
set_source_files_properties(side.c PROPERTIES COMPILE_OPTIONS -Werror)
target_link_libraries(demo_reflection PRIVATE ycxx::modules)
if(YCXX_TEST_OWN_REFLECTION STREQUAL "target")
  target_compile_options(demo_reflection PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-freflection>)
elseif(YCXX_TEST_OWN_REFLECTION STREQUAL "dependency")
  add_library(demo_reflection_flags INTERFACE)
  target_compile_options(demo_reflection_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-freflection>)
  target_link_libraries(demo_reflection PRIVATE demo_reflection_flags)
endif()
