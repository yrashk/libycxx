cmake_minimum_required(VERSION 3.28)
project(libycxx VERSION 0.1.0
        DESCRIPTION "A clean-room C++26 standard library for GCC 16 and Clang 23"
        LANGUAGES C CXX)

# Use from another CMake project, either way:
#   find_package(libycxx CONFIG REQUIRED)        # after `cmake --install`
#   add_subdirectory(path/to/libycxx)            # or FetchContent
#   target_link_libraries(app PRIVATE ycxx::ycxx)
# ycxx::ycxx compiles against libycxx's headers instead of the toolchain's (-nostdinc++, C++26)
# and links libycxx and its ABI runtime instead of libstdc++/libc++ (-nostdlib++). Everything in
# a program must use the same standard library, so link ycxx::ycxx into every C++ target.
# See examples/ and tests/cmake.

option(YCXX_WERROR "Build libycxx itself with -Werror" ${PROJECT_IS_TOP_LEVEL})
option(YCXX_INSTALL "Generate libycxx's install and package rules" ${PROJECT_IS_TOP_LEVEL})
set(YCXX_CXX_STANDARD_FLAG "-std=c++26" CACHE STRING
    "Language-standard flag ycxx::ycxx adds to consumers (empty: none; set your own C++26 flag)")
option(YCXX_ALLOW_UNSUPPORTED_COMPILER "Do not stop on compilers other than GCC >= 16.2 / Clang >= 23" OFF)

if(NOT YCXX_ALLOW_UNSUPPORTED_COMPILER)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(_ycxx_min 16.2)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(_ycxx_min 23)
  else()
    message(FATAL_ERROR "libycxx supports GCC >= 16.2 and Clang >= 23; this is ${CMAKE_CXX_COMPILER_ID}. "
                        "tools/toolchain/provision installs both.")
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS _ycxx_min)
    message(FATAL_ERROR "libycxx needs ${CMAKE_CXX_COMPILER_ID} >= ${_ycxx_min}; found "
                        "${CMAKE_CXX_COMPILER_VERSION}. tools/toolchain/provision installs it.")
  endif()
endif()

include(GNUInstallDirs)
set(YCXX_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Installed under their own directory: the headers have the standard names (<vector>, ...).
set(YCXX_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/libycxx)

# libycxx builds its own runtime with itself: no system C++ headers or library.
# The C++ standard flag is given explicitly: CMake does not know every compiler's C++26 flag.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_C_STANDARD 23)

# ---- ycxx::headers: libycxx's headers in place of the toolchain's ----
add_library(ycxx_headers INTERFACE)
set_target_properties(ycxx_headers PROPERTIES EXPORT_NAME headers)
target_include_directories(ycxx_headers SYSTEM INTERFACE
  $<BUILD_INTERFACE:${YCXX_INCLUDE}> $<INSTALL_INTERFACE:${YCXX_INSTALL_INCLUDEDIR}>)
target_compile_options(ycxx_headers INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++ ${YCXX_CXX_STANDARD_FLAG}>)

set(_ycxx_warnings -Wall -Wextra $<$<BOOL:${YCXX_WERROR}>:-Werror>)

# ---- ycxx::core (libycxx.a): the library's out-of-line parts and the platform layer ----
# One replaceable allocation function per source file (see src/runtime/new).
file(GLOB YCXX_NEW_SOURCES CONFIGURE_DEPENDS src/runtime/new/*.cpp src/hosted/new/*.cpp)
# Defining one deallocation function per file is the point; GCC warns about each unpaired one.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set_source_files_properties(${YCXX_NEW_SOURCES} PROPERTIES COMPILE_OPTIONS -Wno-sized-deallocation)
endif()
# Floating-point <charconv> (also in the freestanding runtime archive).
file(GLOB YCXX_CHARCONV_SOURCES CONFIGURE_DEPENDS src/runtime/charconv/*.cpp)
add_library(ycxx STATIC
  ${YCXX_NEW_SOURCES}
  ${YCXX_CHARCONV_SOURCES}
  src/hosted/stdexcept.cpp
  src/hosted/memory_resource.cpp
  src/hosted/string.cpp
  src/hosted/cmath_check.cpp
  src/hosted/syncstream.cpp
  src/hosted/system_error.cpp
  # iostreams and localization
  src/hosted/fstream.cpp
  src/hosted/ios.cpp
  src/hosted/iostream.cpp
  src/hosted/locale.cpp
  src/hosted/num.cpp
  src/hosted/time.cpp
  src/pal/posix/pal.c
)
set_target_properties(ycxx PROPERTIES EXPORT_NAME core)
target_link_libraries(ycxx PRIVATE ycxx_headers)
target_compile_options(ycxx PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++26 ${_ycxx_warnings}>
  $<$<COMPILE_LANGUAGE:C>:${_ycxx_warnings}>)

# ---- ycxx::abi (libycxx-abi.a): the Itanium C++ ABI runtime (exception handling, RTTI,
# guards), built with RTTI and exceptions ----
file(GLOB YCXX_ABI_SOURCES CONFIGURE_DEPENDS src/abi/*.cpp)
add_library(ycxx_abi STATIC ${YCXX_ABI_SOURCES})
set_target_properties(ycxx_abi PROPERTIES OUTPUT_NAME ycxx-abi EXPORT_NAME abi)
target_link_libraries(ycxx_abi PRIVATE ycxx_headers)
target_compile_options(ycxx_abi PRIVATE -std=c++26 ${_ycxx_warnings} -fexceptions -frtti)
set_source_files_properties(src/abi/exception_classes.cpp src/hosted/stdexcept.cpp
                            PROPERTIES COMPILE_DEFINITIONS YCXX_EXCEPTION_KEY_FUNCTIONS)
# The standard stream objects are initialized with a priority reserved for the implementation,
# ahead of every program's static objects (src/hosted/iostream.cpp).
set_source_files_properties(src/hosted/iostream.cpp PROPERTIES COMPILE_OPTIONS
  $<IF:$<CXX_COMPILER_ID:GNU>,-Wno-prio-ctor-dtor,-Wno-init-priority-reserved>)

# The two archives refer to each other (the runtime uses the platform layer and operator
# delete; the library throws through the runtime): a cyclic static dependency, which CMake
# links by repeating the pair.
target_link_libraries(ycxx INTERFACE ycxx_abi)
target_link_libraries(ycxx_abi INTERFACE ycxx)
set_property(TARGET ycxx ycxx_abi PROPERTY LINK_INTERFACE_MULTIPLICITY 3)
# Usable in shared objects and position-independent executables.
set_target_properties(ycxx ycxx_abi PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- ycxx::ycxx: what applications link ----
add_library(ycxx_ycxx INTERFACE)
set_target_properties(ycxx_ycxx PROPERTIES EXPORT_NAME ycxx)
target_link_libraries(ycxx_ycxx INTERFACE ycxx_headers ycxx)
# <cmath> calls the C library's math functions at run time; libm is separate from libc on
# glibc and musl (on Darwin it is part of libSystem).
if(NOT APPLE)
  target_link_libraries(ycxx_ycxx INTERFACE m)
endif()
# -shared-libgcc: one unwinder per process. glibc's pthread_exit/pthread_cancel unwind through
# libgcc_s.so; a second, static copy (libgcc_eh) used by the personality routine would abort.
target_link_options(ycxx_ycxx INTERFACE $<$<LINK_LANGUAGE:CXX>:-nostdlib++>
                    $<$<AND:$<LINK_LANGUAGE:CXX>,$<PLATFORM_ID:Linux>>:-shared-libgcc>)

add_library(ycxx::headers ALIAS ycxx_headers)
add_library(ycxx::core ALIAS ycxx)
add_library(ycxx::abi ALIAS ycxx_abi)
add_library(ycxx::ycxx ALIAS ycxx_ycxx)

# ---- install and package ----
if(YCXX_INSTALL)
  include(CMakePackageConfigHelpers)
  set(YCXX_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/libycxx)
  install(DIRECTORY include/ DESTINATION ${YCXX_INSTALL_INCLUDEDIR})
  install(TARGETS ycxx_headers ycxx ycxx_abi ycxx_ycxx EXPORT libycxxTargets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(EXPORT libycxxTargets NAMESPACE ycxx:: DESTINATION ${YCXX_CMAKE_DIR})
  configure_package_config_file(cmake/libycxxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfig.cmake INSTALL_DESTINATION ${YCXX_CMAKE_DIR})
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/libycxxConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfigVersion.cmake
          DESTINATION ${YCXX_CMAKE_DIR})
endif()
