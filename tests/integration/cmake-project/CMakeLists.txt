# A project that knows nothing about libycxx, as most existing projects are (tests/integration/run.sh
# builds it with libycxx's toolchain file, with CC/CXX, and with the toolchain's own library):
# an older pinned standard, configure-time checks, a shared library with an exported package, a
# FetchContent dependency and an ExternalProject one.
cmake_minimum_required(VERSION 3.24)
project(ycxx_integration VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)            # pinned older standard: libycxx's compiler raises it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# try_compile-based checks go through the same compiler.
include(CheckCXXSourceCompiles)
include(CheckIncludeFileCXX)
check_include_file_cxx(optional HAVE_OPTIONAL)
check_cxx_source_compiles([=[
#include <string>
#include <version>
#if !defined(__cpp_lib_string_view)
#error no string_view
#endif
int main() { return std::string("x").size() == 1 ? 0 : 1; }
]=] HAVE_STRING_VIEW)
if(NOT HAVE_OPTIONAL OR NOT HAVE_STRING_VIEW)
  message(FATAL_ERROR "configure-time checks failed")
endif()

# A dependency built as part of the project.
include(FetchContent)
FetchContent_Declare(sub SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sub)
FetchContent_MakeAvailable(sub)

# A dependency built as a separate CMake project at build time: it gets the toolchain only when
# it is passed on.
include(ExternalProject)
set(_ep_args -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=Release)
if(CMAKE_TOOLCHAIN_FILE)
  list(APPEND _ep_args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()
ExternalProject_Add(ep SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sub CMAKE_ARGS ${_ep_args}
                    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sub${CMAKE_STATIC_LIBRARY_SUFFIX})
ExternalProject_Get_Property(ep INSTALL_DIR)
add_library(ep_sub STATIC IMPORTED)
set_target_properties(ep_sub PROPERTIES
  IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sub${CMAKE_STATIC_LIBRARY_SUFFIX})
add_dependencies(ep_sub ep)

# A shared library, with C and C++ sources, used by a program and installed as a package.
add_library(greet SHARED greet.cpp cpart.c)
target_include_directories(greet PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include>)
target_link_libraries(greet PRIVATE sub)
set_target_properties(greet PROPERTIES VERSION 1.0 SOVERSION 1)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE greet ep_sub)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sub)

include(GNUInstallDirs)
install(TARGETS greet app EXPORT greetTargets)
install(FILES greet.hpp DESTINATION include)
install(EXPORT greetTargets NAMESPACE greet:: DESTINATION lib/cmake/greet)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/greetConfig.cmake "include(\${CMAKE_CURRENT_LIST_DIR}/greetTargets.cmake)\n")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/greetConfig.cmake DESTINATION lib/cmake/greet)
if(APPLE)
  set_target_properties(app greet PROPERTIES INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
else()
  set_target_properties(app greet PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

enable_testing()
add_test(NAME app COMMAND app)
set_tests_properties(app PROPERTIES PASS_REGULAR_EXPRESSION "^integration: ok")
