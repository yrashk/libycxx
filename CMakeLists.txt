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
option(YCXX_FREESTANDING_RUNTIME
       "Also build (and install) libycxx-freestanding.a, the runtime of freestanding programs (ycxx::freestanding)" OFF)
option(YCXX_ALLOW_UNSUPPORTED_COMPILER "Do not stop on compilers other than GCC >= 16.2 / Clang >= 23" OFF)
# The platform layer and the hosted layers (DECISIONS §18, cmake/ycxx-hosted-layers.cmake): posix
# provides every layer (src/pal/posix); none provides none, and the integrator's providers supply
# the layers named in YCXX_HOSTED_LAYERS (examples/hosted-layers).
set(YCXX_PAL "posix" CACHE STRING
    "Platform layer: posix (every hosted layer, from src/pal/posix) or none (the integrator provides the layers of YCXX_HOSTED_LAYERS)")
set_property(CACHE YCXX_PAL PROPERTY STRINGS posix none)
set(YCXX_HOSTED_LAYERS "" CACHE STRING
    "YCXX_PAL=none: the hosted layers the integrator provides (abort memory console clock threads random files environment debug clib; abort is implied)")

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
# Hidden visibility (DECISIONS §2): GCC warns about every program class with a member or base of a
# library class type ("declared with greater visibility than the type of its field"); the
# warning says nothing about the program, so users of the package do not get it.
target_compile_options(ycxx_headers INTERFACE
  $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-attributes>)

# The hosted layers (cmake/ycxx-hosted-layers.cmake): _ycxx_layers is the selected set; with
# YCXX_PAL=none, <ycxx/generated/hosted_layers.hpp> records it for ycxx/config.hpp.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ycxx-hosted-layers.cmake)
ycxx_hosted_layers_select(_ycxx_layers)
set(YCXX_GENERATED_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/generated/include)
set_property(GLOBAL PROPERTY YCXX_PAL_SELECTED ${YCXX_PAL})
set_property(GLOBAL PROPERTY YCXX_HOSTED_LAYERS_SELECTED ${_ycxx_layers})
set_property(GLOBAL PROPERTY YCXX_PAL_INCLUDE_DIR ${YCXX_INCLUDE})
if(YCXX_PAL STREQUAL "none")
  ycxx_hosted_layers_header(${YCXX_GENERATED_INCLUDE}/ycxx/generated/hosted_layers.hpp ${_ycxx_layers})
  foreach(_l IN LISTS _ycxx_layers)
    string(TOUPPER "${_l}" _u)
    # YCXX_PAL_<LAYER>_PROVIDER: attached once ycxx::ycxx exists (the end of this directory).
    if(YCXX_PAL_${_u}_PROVIDER)
      cmake_language(DEFER CALL ycxx_add_hosted_layer ${_l} PROVIDER ${YCXX_PAL_${_u}_PROVIDER})
    endif()
  endforeach()
  cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _ycxx_report_hosted_layers)
  list(JOIN _ycxx_layers " " _l)
  message(STATUS "libycxx: YCXX_PAL=none, hosted layers: ${_l}")
else()
  file(REMOVE ${YCXX_GENERATED_INCLUDE}/ycxx/generated/hosted_layers.hpp)
endif()
# Without the C library ('clib' absent), libycxx and its programs are compiled freestanding
# (DECISIONS §18): no C library headers, the compiler's own header directory for <stddef.h>
# (DECISIONS §3). C++ only: a provider written in C may use whatever C library it has.
if(NOT "clib" IN_LIST _ycxx_layers)
  set(_ycxx_clib OFF)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=include
                  OUTPUT_VARIABLE _ycxx_compiler_include OUTPUT_STRIP_TRAILING_WHITESPACE)
  target_compile_options(ycxx_headers INTERFACE
    "$<$<COMPILE_LANGUAGE:CXX>:-ffreestanding;-nostdinc;SHELL:-isystem ${_ycxx_compiler_include}>")
else()
  set(_ycxx_clib ON)
endif()

# What the C library provides (cmake/ycxx-c-library.cmake): <ycxx/generated/c_library.hpp> in the build
# tree, read by ycxx/config.hpp; installed with the headers.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ycxx-c-library.cmake)
if(_ycxx_clib)
  ycxx_c_library_probe(${YCXX_GENERATED_INCLUDE}/ycxx/generated/c_library.hpp)
else()
  file(REMOVE ${YCXX_GENERATED_INCLUDE}/ycxx/generated/c_library.hpp)
endif()
target_include_directories(ycxx_headers SYSTEM INTERFACE $<BUILD_INTERFACE:${YCXX_GENERATED_INCLUDE}>)

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
# Their work buffers (up to tens of kilobytes) are written before they are read; GCC 16 would
# zero-fill every automatic variable in C++26 mode (erroneous values) on each call.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set_source_files_properties(${YCXX_CHARCONV_SOURCES} PROPERTIES COMPILE_OPTIONS -ftrivial-auto-var-init=uninitialized)
endif()
# The lock and wait tables of <atomic> (also in the freestanding runtime archive).
file(GLOB YCXX_ATOMIC_SOURCES CONFIGURE_DEPENDS src/runtime/atomic/*.cpp)
# <debugging> (also in the freestanding runtime archive); is_debugger_present is replaceable.
file(GLOB YCXX_DEBUGGING_SOURCES CONFIGURE_DEPENDS src/runtime/debugging/*.cpp)
# <contracts>: the replaceable default ::handle_contract_violation (also freestanding).
file(GLOB YCXX_CONTRACTS_SOURCES CONFIGURE_DEPENDS src/runtime/contracts/*.cpp)
# The hosted sources by what they need (DECISIONS §18). YCXX_PAL=posix builds all of them with
# src/pal/posix; YCXX_PAL=none builds those of the selected layers, and fallbacks for three absent
# ones.
# Need neither the C library nor a particular layer (a use of a feature whose layer is absent
# fails to link, naming the missing primitive).
set(YCXX_RUNTIME_SOURCES
  ${YCXX_NEW_SOURCES}
  ${YCXX_CHARCONV_SOURCES}
  ${YCXX_ATOMIC_SOURCES}
  ${YCXX_DEBUGGING_SOURCES}
  ${YCXX_CONTRACTS_SOURCES}
  src/hosted/contracts.cpp
  src/hosted/stdexcept.cpp
  src/hosted/memory_resource.cpp
  src/hosted/system_error.cpp
  src/hosted/random.cpp)
# The 'threads' layer (with the C library: thread.cpp uses <cfenv>).
set(YCXX_THREADS_SOURCES
  src/hosted/thread.cpp
  src/hosted/parallel_scheduler.cpp
  src/hosted/parallel_scheduler_query.cpp
  src/hosted/future.cpp
  src/hosted/rcu.cpp
  src/hosted/hazard_pointer.cpp)
# The C library ('clib').
set(YCXX_CLIB_SOURCES
  src/hosted/string.cpp
  src/hosted/ctime.cpp
  src/hosted/strfrom.cpp
  src/hosted/uchar.cpp
  src/hosted/cmath.cpp
  src/hosted/cmath_check.cpp
  src/hosted/syncstream.cpp
  src/hosted/text_encoding.cpp
  src/hosted/regex.cpp
  # iostreams and localization
  src/hosted/ios.cpp
  src/hosted/iostream.cpp
  src/hosted/locale.cpp
  src/hosted/num.cpp
  src/hosted/time.cpp
  src/hosted/chrono.cpp
  # formatting
  src/hosted/format.cpp
  src/hosted/print.cpp)
# The 'debug' layer (with the C library).
set(YCXX_DEBUG_SOURCES src/hosted/stacktrace.cpp src/hosted/demangle.cpp)
# Files: C stdio's (src/hosted/fstream.cpp) or the 'files' layer's (fstream_pal.cpp).
set(YCXX_STDIO_FILE_SOURCES src/hosted/fstream.cpp)
# POSIX only: they call POSIX directly (DECISIONS §8).
set(YCXX_POSIX_SOURCES src/hosted/tzdb.cpp src/hosted/filesystem.cpp)
if(YCXX_PAL STREQUAL "posix")
  set(YCXX_LIBRARY_SOURCES ${YCXX_RUNTIME_SOURCES} ${YCXX_THREADS_SOURCES} ${YCXX_CLIB_SOURCES}
      ${YCXX_DEBUG_SOURCES} ${YCXX_STDIO_FILE_SOURCES} ${YCXX_POSIX_SOURCES}
      src/pal/posix/pal.c
      src/pal/posix/pal_debug.c
      src/pal/posix/pal_encoding.c)
else()
  set(YCXX_LIBRARY_SOURCES ${YCXX_RUNTIME_SOURCES})
  if(_ycxx_clib)
    list(APPEND YCXX_LIBRARY_SOURCES ${YCXX_CLIB_SOURCES})
    if("files" IN_LIST _ycxx_layers)
      list(APPEND YCXX_LIBRARY_SOURCES src/hosted/fstream_pal.cpp)
    else()
      list(APPEND YCXX_LIBRARY_SOURCES ${YCXX_STDIO_FILE_SOURCES})
    endif()
  else()
    # Without the C library: <format>'s L option with the classic locale's values; standard
    # output through the console layer.
    list(APPEND YCXX_LIBRARY_SOURCES src/hosted/format_classic.cpp)
    if("console" IN_LIST _ycxx_layers)
      list(APPEND YCXX_LIBRARY_SOURCES src/hosted/print_console.cpp)
    endif()
  endif()
  if("threads" IN_LIST _ycxx_layers)
    list(APPEND YCXX_LIBRARY_SOURCES ${YCXX_THREADS_SOURCES})
  else()
    # One thread of execution (ycxx/pal.h: the fallbacks).
    list(APPEND YCXX_LIBRARY_SOURCES
      src/freestanding/pal/wait.cpp src/freestanding/pal/wake_all.cpp src/freestanding/pal/wake_one.cpp
      src/freestanding/pal/thread_self.cpp src/freestanding/pal/thread_yield.cpp
      src/freestanding/pal/single_threaded.cpp src/pal/fallback/thread_atexit.cpp)
  endif()
  if("debug" IN_LIST _ycxx_layers)
    list(APPEND YCXX_LIBRARY_SOURCES ${YCXX_DEBUG_SOURCES})
  else()
    list(APPEND YCXX_LIBRARY_SOURCES src/freestanding/pal/debugger_present.cpp)
  endif()
  if(NOT "environment" IN_LIST _ycxx_layers)
    list(APPEND YCXX_LIBRARY_SOURCES src/pal/fallback/error_message.cpp src/pal/fallback/environment_encoding.cpp)
  endif()
endif()
add_library(ycxx STATIC ${YCXX_LIBRARY_SOURCES})
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
target_compile_options(ycxx_abi PRIVATE -std=c++26 ${_ycxx_warnings} -fexceptions -frtti
  # The hierarchy walks keep fixed tables of visited virtual bases on the stack, written before
  # they are read; GCC 16 would zero-fill them on every dynamic_cast and handler search (C++26
  # erroneous values).
  $<$<CXX_COMPILER_ID:GNU>:-ftrivial-auto-var-init=uninitialized>)
# The fundamental type_info objects the compiler emits with __fundamental_type_info's key function
# (src/abi/rtti.cpp), as the target's assembler names. GCC gives them default visibility whatever
# -fvisibility says, and the set depends on the target (AArch64 adds __bf16, __mfp8 and the SVE
# types), so the list rtti.cpp hides is taken from the compiler itself: a probe defining the key
# function is compiled with the runtime's flags, and its type_info symbols listed with nm.
set(_ycxx_probe ${CMAKE_CURRENT_BINARY_DIR}/fundamental_type_info_probe)
file(WRITE ${_ycxx_probe}/probe.cpp [=[
namespace std { class type_info { public: virtual ~type_info(); const char* name_; }; }
namespace __cxxabiv1 {
class __fundamental_type_info : public std::type_info { public: ~__fundamental_type_info() override; };
__fundamental_type_info::~__fundamental_type_info() {}
}
]=])
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
try_compile(_ycxx_probe_ok SOURCES ${_ycxx_probe}/probe.cpp
  COMPILE_DEFINITIONS -std=c++26 -nostdinc++ -frtti -fPIC
  COPY_FILE ${_ycxx_probe}/probe.a OUTPUT_VARIABLE _ycxx_probe_log)
unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
if(NOT _ycxx_probe_ok)
  message(FATAL_ERROR "libycxx: the fundamental type_info probe did not compile:\n${_ycxx_probe_log}")
endif()
execute_process(COMMAND ${CMAKE_NM} ${_ycxx_probe}/probe.a OUTPUT_VARIABLE _ycxx_nm RESULT_VARIABLE _ycxx_nm_rc)
if(NOT _ycxx_nm_rc EQUAL 0)
  message(FATAL_ERROR "libycxx: ${CMAKE_NM} failed on the fundamental type_info probe")
endif()
# Defined symbols only (a type letter other than U), named _ZTI/_ZTS (Mach-O: with its '_' prefix),
# except the type_info classes' own.
string(REGEX MATCHALL "[ \t][A-TV-Za-tv-z][ \t]+_?_ZT[IS][A-Za-z0-9_]+" _ycxx_tis "${_ycxx_nm}")
set(_ycxx_ti_list "")
foreach(_s IN LISTS _ycxx_tis)
  string(REGEX REPLACE ".*[ \t]" "" _s "${_s}")
  if(NOT _s MATCHES "cxxabiv|St9type_info")
    list(APPEND _ycxx_ti_list "${_s}")
  endif()
endforeach()
list(REMOVE_DUPLICATES _ycxx_ti_list)
list(SORT _ycxx_ti_list)
list(LENGTH _ycxx_ti_list _ycxx_ti_count)
list(JOIN _ycxx_ti_list "\",\n    \"" _ycxx_ti_text)
if(_ycxx_ti_count GREATER 0)
  set(_ycxx_ti_text "\"${_ycxx_ti_text}\", ")
endif()
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/abi/fundamental_type_infos.hpp CONTENT [=[
// Generated by CMakeLists.txt (the fundamental type_info probe) for @CMAKE_CXX_COMPILER_ID@ @CMAKE_CXX_COMPILER_VERSION@.
// The fundamental type_info objects this compiler emits with __fundamental_type_info's key
// function, as assembler names; rtti.cpp hides them. Ends with nullptr.
#pragma once

namespace ycxx::abi {
inline constexpr const char* fundamental_type_info_symbols[] = {
    @_ycxx_ti_text@nullptr};
} // namespace ycxx::abi
]=] @ONLY)
target_include_directories(ycxx_abi PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
message(STATUS "libycxx: ${_ycxx_ti_count} fundamental type_info symbols to hide (src/abi/rtti.cpp)")

set_source_files_properties(src/abi/exception_classes.cpp src/hosted/stdexcept.cpp
                            PROPERTIES COMPILE_DEFINITIONS YCXX_EXCEPTION_KEY_FUNCTIONS)
# The standard stream objects are initialized with a priority reserved for the implementation,
# ahead of every program's static objects (src/hosted/iostream.cpp); so is the classic locale
# (src/hosted/locale.cpp). ELF only: Mach-O has no initialization priorities, and <iostream>
# defines an ios_base::Init per translation unit there (YCXX_HAS_INIT_PRIORITY, config.hpp).
set_source_files_properties(src/hosted/iostream.cpp src/hosted/locale.cpp PROPERTIES COMPILE_OPTIONS
  $<IF:$<CXX_COMPILER_ID:GNU>,-Wno-prio-ctor-dtor,-Wno-init-priority-reserved>)
# The classic locale holds the Annex D codecvt facets ([depr.locale.category]), which are
# declared [[deprecated]].
set_property(SOURCE src/hosted/locale.cpp APPEND PROPERTY COMPILE_OPTIONS -Wno-deprecated-declarations)

# The two archives refer to each other (the runtime uses the platform layer and operator
# delete; the library throws through the runtime): a cyclic static dependency, which CMake
# links by repeating the pair.
target_link_libraries(ycxx INTERFACE ycxx_abi)
target_link_libraries(ycxx_abi INTERFACE ycxx)
set_property(TARGET ycxx ycxx_abi PROPERTY LINK_INTERFACE_MULTIPLICITY 3)
# Usable in shared objects and position-independent executables.
set_target_properties(ycxx ycxx_abi PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Nothing the archives define is exported from the program or shared object that links them,
# except the allocation table: another C++ library in the process (Apple's libc++/libc++abi, which
# libSystem loads into every process; libstdc++) can neither take over libycxx's definitions nor
# be taken over by them (DECISIONS §2). The sources say so themselves, with visibility attributes
# on their namespaces and declarations, not with -fvisibility=hidden.

# ---- ycxx::ycxx: what applications link ----
add_library(ycxx_ycxx INTERFACE)
set_target_properties(ycxx_ycxx PROPERTIES EXPORT_NAME ycxx)
target_link_libraries(ycxx_ycxx INTERFACE ycxx_headers ycxx)
# <cmath> calls the C library's math functions at run time; libm is separate from libc on
# glibc and musl (on Darwin it is part of libSystem).
if(NOT APPLE AND _ycxx_clib)
  target_link_libraries(ycxx_ycxx INTERFACE m)
endif()
# -shared-libgcc: one unwinder per process. glibc's pthread_exit/pthread_cancel unwind through
# libgcc_s.so; a second, static copy (libgcc_eh) used by the personality routine would abort.
target_link_options(ycxx_ycxx INTERFACE $<$<LINK_LANGUAGE:CXX>:-nostdlib++>
                    $<$<AND:$<LINK_LANGUAGE:CXX>,$<PLATFORM_ID:Linux>>:-shared-libgcc>)
# The allocation table stays in every program and is visible to the shared libraries it loads
# (cmake/ycxx-link.cmake; DECISIONS §2).
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ycxx-link.cmake)
ycxx_link_options(_ycxx_table_link_options)
target_link_options(ycxx_ycxx INTERFACE ${_ycxx_table_link_options})
# Linux: Clang takes the C runtime startup files and libgcc (the unwinder libycxx's ABI runtime
# uses) from a GCC installation, by default the newest one in the system's standard places, which
# can be older than GCC 16. Clang consumers get --gcc-install-dir=<GCC 16's>, as with
# tools/ycxx-cxx and cmake/ycxx-toolchain.cmake. YCXX_GCC_INSTALL_DIR names it (empty: let Clang
# choose); by default: $ENV{YCXX_GCC_INSTALL_DIR} (tools/toolchain/activate.sh sets it), else the
# --gcc-install-dir already in CMAKE_CXX_FLAGS (the toolchain file's), else the directory of
# libgcc of the GCC building libycxx, of $ENV{YCXX_GXX}, or of g++-16 on PATH. The installed
# package records it (libycxxConfig.cmake).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT DEFINED YCXX_GCC_INSTALL_DIR)
  set(_ycxx_gcc_dir "$ENV{YCXX_GCC_INSTALL_DIR}")
  if(NOT _ycxx_gcc_dir AND CMAKE_CXX_FLAGS MATCHES "--gcc-install-dir=([^ ]+)")
    set(_ycxx_gcc_dir "${CMAKE_MATCH_1}")
  elseif(NOT _ycxx_gcc_dir)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(_ycxx_gxx "${CMAKE_CXX_COMPILER}")
    elseif(DEFINED ENV{YCXX_GXX})
      set(_ycxx_gxx "$ENV{YCXX_GXX}")
    else()
      find_program(_ycxx_gxx NAMES g++-16 NO_CACHE)
    endif()
    if(_ycxx_gxx)
      execute_process(COMMAND "${_ycxx_gxx}" -print-libgcc-file-name OUTPUT_VARIABLE _ycxx_libgcc
                      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE _ycxx_rc)
      if(_ycxx_rc EQUAL 0 AND IS_ABSOLUTE "${_ycxx_libgcc}")
        get_filename_component(_ycxx_gcc_dir "${_ycxx_libgcc}" DIRECTORY)
      endif()
    endif()
  endif()
  set(YCXX_GCC_INSTALL_DIR "${_ycxx_gcc_dir}" CACHE PATH
      "GCC installation whose crt files and libgcc Clang consumers of ycxx::ycxx use (--gcc-install-dir; empty: Clang's choice)")
endif()
if(YCXX_GCC_INSTALL_DIR)
  # The build tree's ycxx::ycxx (add_subdirectory, FetchContent); libycxxConfig.cmake adds the
  # same to the installed one, where the directory can be overridden by the consumer.
  target_compile_options(ycxx_ycxx INTERFACE
    $<BUILD_INTERFACE:$<$<COMPILE_LANG_AND_ID:CXX,Clang>:--gcc-install-dir=${YCXX_GCC_INSTALL_DIR}>>)
  target_link_options(ycxx_ycxx INTERFACE
    $<BUILD_INTERFACE:$<$<AND:$<LINK_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:--gcc-install-dir=${YCXX_GCC_INSTALL_DIR}>>)
endif()

# ---- ycxx::freestanding (libycxx-freestanding.a, YCXX_FREESTANDING_RUNTIME): what a freestanding
# program links instead of ycxx::ycxx (DECISIONS §3): the default replaceable allocation functions
# (no heap: the plain forms fail through the error handler, the nothrow forms return null; a
# program with a heap replaces them),
# std::nothrow, floating-point <charconv>, the <atomic> lock and wait tables, <debugging>, the
# default contract-violation handler and the platform layer's wait and debugger hooks. Built for
# the compiler's target with -ffreestanding -nostdinc (no C library headers; the compiler's own
# directory is added back for <stddef.h>), without exceptions
# or RTTI, hidden. Consumers get libycxx's headers and -ffreestanding; the C library functions the
# compilers may call (memcpy, memmove, memset, memcmp) and the entry point are the program's
# (tests/freestanding/rt.c). tools/check_freestanding.sh builds the same sources for bare-metal
# targets.
if(YCXX_FREESTANDING_RUNTIME)
  file(GLOB YCXX_FREESTANDING_SOURCES CONFIGURE_DEPENDS
       src/runtime/new/*.cpp src/freestanding/new/*.cpp src/freestanding/contracts/*.cpp src/freestanding/pal/*.cpp)
  add_library(ycxx_freestanding STATIC
    ${YCXX_FREESTANDING_SOURCES} ${YCXX_CHARCONV_SOURCES} ${YCXX_ATOMIC_SOURCES}
    ${YCXX_DEBUGGING_SOURCES} ${YCXX_CONTRACTS_SOURCES})
  set_target_properties(ycxx_freestanding PROPERTIES OUTPUT_NAME ycxx-freestanding EXPORT_NAME freestanding
                        POSITION_INDEPENDENT_CODE ON)
  target_link_libraries(ycxx_freestanding PUBLIC ycxx_headers)
  # -nostdinc drops the compiler's own headers too; core needs its <stddef.h> (::max_align_t).
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=include
                  OUTPUT_VARIABLE _ycxx_compiler_include OUTPUT_STRIP_TRAILING_WHITESPACE)
  target_compile_options(ycxx_freestanding PRIVATE -std=c++26 -nostdinc "SHELL:-isystem ${_ycxx_compiler_include}"
    -fno-exceptions -fno-rtti ${_ycxx_warnings}
    # One allocation function per file; no zero fill of the charconv work buffers (see above).
    $<$<CXX_COMPILER_ID:GNU>:-Wno-sized-deallocation -ftrivial-auto-var-init=uninitialized>
    PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding>)
  add_library(ycxx::freestanding ALIAS ycxx_freestanding)
endif()

# ---- ycxx::modules (libycxx-modules.a, YCXX_MODULES): the standard library modules std and
# std.compat ([std.modules]; modules/*.cppm, generated by tools/gen_std_module.py). A target that
# links ycxx::modules (it brings ycxx::ycxx) can `import std;` / `import std.compat;`. The
# interface units are a CXX_MODULES file set: CMake compiles their BMIs with each consumer's build
# (installed package: per importing configuration), and libycxx-modules.a holds their object
# code (the module initializers). C++20 module support in CMake needs CMake >= 3.28 and a Ninja
# generator (Clang: clang-scan-deps next to the compiler; GCC >= 14). DECISIONS §16.
if(CMAKE_GENERATOR MATCHES "Ninja" AND YCXX_PAL STREQUAL "posix")
  set(_ycxx_modules_default ON)
else()
  set(_ycxx_modules_default OFF)
endif()
option(YCXX_MODULES "Build the standard library modules std and std.compat (ycxx::modules; Ninja generators)"
       ${_ycxx_modules_default})
if(YCXX_MODULES)
  add_library(ycxx_modules STATIC)
  target_sources(ycxx_modules PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules
                 FILES modules/std.cppm modules/std.compat.cppm)
  set_target_properties(ycxx_modules PROPERTIES OUTPUT_NAME ycxx-modules EXPORT_NAME modules
                        CXX_SCAN_FOR_MODULES ON POSITION_INDEPENDENT_CODE ON)
  # CMake scans and builds modules only for targets that require C++20 or later; the consumers of
  # the module need it too (cxx_std_23: CMake 3.28 does not know C++26 for these compilers);
  # ycxx::headers' own -std=c++26 comes later on the command line and wins.
  target_compile_features(ycxx_modules PUBLIC cxx_std_23)
  target_link_libraries(ycxx_modules PUBLIC ycxx_ycxx)
  # The installed package's consumers compile the interface units with the flags CMake exports
  # from this target's own properties (IMPORTED_CXX_MODULES_*); CMake 3.28 does not add those of
  # its link dependencies (ycxx::headers) there, so the headers' flags are repeated here. The
  # warning flags are libycxx's own build's only. `std` is a module name reserved to the
  # implementation ([module.unit]/1), which libycxx is.
  # -isystem as an option: CMake 3.28 would export include directories without SYSTEM. The
  # installed package's libycxxConfig.cmake adds the installed directory the same way.
  target_compile_options(ycxx_modules PRIVATE "$<BUILD_INTERFACE:SHELL:-isystem ${YCXX_INCLUDE}>"
                         -nostdinc++ ${YCXX_CXX_STANDARD_FLAG}
                         $<$<CXX_COMPILER_ID:GNU>:-Wno-attributes>
                         $<$<CXX_COMPILER_ID:Clang>:-Wno-reserved-module-identifier>
                         $<BUILD_INTERFACE:${_ycxx_warnings}>)
  add_library(ycxx::modules ALIAS ycxx_modules)
endif()

add_library(ycxx::headers ALIAS ycxx_headers)
add_library(ycxx::core ALIAS ycxx)
add_library(ycxx::abi ALIAS ycxx_abi)
add_library(ycxx::ycxx ALIAS ycxx_ycxx)

# ---- install and package ----
if(YCXX_INSTALL)
  include(CMakePackageConfigHelpers)
  set(YCXX_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/libycxx)
  install(DIRECTORY include/ ${YCXX_GENERATED_INCLUDE}/ DESTINATION ${YCXX_INSTALL_INCLUDEDIR})
  set(_ycxx_installed ycxx_headers ycxx ycxx_abi ycxx_ycxx)
  if(YCXX_FREESTANDING_RUNTIME)
    list(APPEND _ycxx_installed ycxx_freestanding)
  endif()
  install(TARGETS ${_ycxx_installed} EXPORT libycxxTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  if(YCXX_MODULES)
    # The interface units are installed as sources: an importing project builds its own BMIs.
    install(TARGETS ycxx_modules EXPORT libycxxTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_DATADIR}/libycxx/modules)
  endif()
  install(EXPORT libycxxTargets NAMESPACE ycxx:: DESTINATION ${YCXX_CMAKE_DIR}
          CXX_MODULES_DIRECTORY cxx-modules)
  list(JOIN _ycxx_layers ";" YCXX_SELECTED_HOSTED_LAYERS)
  configure_package_config_file(cmake/libycxxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfig.cmake INSTALL_DESTINATION ${YCXX_CMAKE_DIR})
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/libycxxConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfigVersion.cmake
                cmake/ycxx-hosted-layers.cmake
          DESTINATION ${YCXX_CMAKE_DIR})
endif()
