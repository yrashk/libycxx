cmake_minimum_required(VERSION 3.28)
project(libycxx VERSION 0.1.0
        DESCRIPTION "An independent C++26 standard library, implemented from the working draft, for GCC 16 and Clang 23"
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
# The kinds of library built (DECISIONS §20.2): the static archives (libycxx.a, libycxx-abi.a), the
# shared library (libycxx.so.0.<minor> / libycxx.0.<minor>.dylib with libycxx_nonshared.a), or both.
# With both, the unqualified consumers (ycxx::ycxx, ycxx-c++, libycxx.pc) link statically.
option(YCXX_STATIC "Build the static archives libycxx.a and libycxx-abi.a (ycxx::static)" ON)
option(YCXX_SHARED "Build the shared library libycxx.so.0.<minor> / libycxx.0.<minor>.dylib (ycxx::shared)" OFF)
if(NOT YCXX_STATIC AND NOT YCXX_SHARED)
  message(FATAL_ERROR "libycxx: YCXX_STATIC and YCXX_SHARED are both OFF; build at least one kind")
endif()
if(YCXX_STATIC)
  set(YCXX_DEFAULT_LINKAGE static)
else()
  set(YCXX_DEFAULT_LINKAGE shared)
endif()

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
# The ABI version in the sources (ycxx/config.hpp, __abi_version: the allocation table's version
# check, DECISIONS §20.6) must be the soname's, the project's <major>.<minor>.
file(STRINGS ${YCXX_INCLUDE}/ycxx/config.hpp _ycxx_abi_line ENCODING UTF-8 REGEX "__abi_version\\[\\] = ")
if(NOT _ycxx_abi_line MATCHES "\"${PROJECT_VERSION_MAJOR}\\.${PROJECT_VERSION_MINOR}\"")
  message(FATAL_ERROR "libycxx: ycxx/config.hpp's __abi_version (${_ycxx_abi_line}) is not "
                      "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}, the project's version")
endif()
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
  # the demangler, for <stacktrace> and <cxxabi.h>'s abi::__cxa_demangle (malloc'd results)
  src/hosted/demangle.cpp
  src/hosted/cxa_demangle.cpp
  src/hosted/cxa_demangle_entry.cpp
  # the string inserters, extractors and getline for char and wchar_t ([string.io])
  src/hosted/string_io.cpp
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
  src/hosted/locale_named.cpp
  src/hosted/num.cpp
  src/hosted/time.cpp
  src/hosted/chrono.cpp
  # formatting
  src/hosted/format.cpp
  src/hosted/print.cpp)
# The 'debug' layer (with the C library).
set(YCXX_DEBUG_SOURCES src/hosted/stacktrace.cpp)
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
# ---- The ABI runtime's sources (src/abi): the Itanium C++ ABI runtime (exception handling, RTTI,
# guards), built with RTTI and exceptions ----
# src/abi/entry: the entry points under the Itanium C++ ABI's names, forwarding to the runtime's own
# (src/abi/entry.hpp, DECISIONS §20.6).
file(GLOB YCXX_ABI_SOURCES CONFIGURE_DEPENDS src/abi/*.cpp src/abi/entry/*.cpp)
set_property(SOURCE ${YCXX_ABI_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -fexceptions -frtti
  # The hierarchy walks keep fixed tables of visited virtual bases on the stack, written before
  # they are read; GCC 16 would zero-fill them on every dynamic_cast and handler search (C++26
  # erroneous values).
  $<$<CXX_COMPILER_ID:GNU>:-ftrivial-auto-var-init=uninitialized>)
# The fundamental type_info objects the compiler emits with __fundamental_type_info's key function
# (src/abi/rtti_classes.cpp), as the target's assembler names. GCC gives them default visibility
# whatever -fvisibility says, and the set depends on the target (AArch64 adds __bf16, __mfp8 and the
# SVE types), so the list rtti_classes.cpp hides is taken from the compiler itself: a probe
# defining the key function is compiled with the runtime's flags, and its type_info symbols listed
# with nm.
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
// function, as assembler names; rtti_classes.cpp hides them. Ends with nullptr.
#pragma once

namespace __ycxx::__abi {
inline constexpr const char* fundamental_type_info_symbols[] = {
    @_ycxx_ti_text@nullptr};
} // namespace __ycxx::__abi
]=] @ONLY)
message(STATUS "libycxx: ${_ycxx_ti_count} fundamental type_info symbols to hide (src/abi/rtti_classes.cpp)")
# src/abi/rtti_float16.cpp defines the _Float16 type_info objects for a compiler whose
# rtti_classes.cpp does not emit them (Clang). Where the probe shows that the compiler emits them
# itself (GCC), it is left out: GCC on Darwin emits them non-weak, so both definitions in one image
# (a -force_load or --whole-archive link of libycxx-abi.a) are duplicate symbols.
if(_ycxx_ti_list MATCHES "(^|;)_?_ZTIDF16_(;|$)")
  set_source_files_properties(src/abi/rtti_float16.cpp PROPERTIES HEADER_FILE_ONLY ON)
  message(STATUS "libycxx: the compiler emits the _Float16 type_info objects; src/abi/rtti_float16.cpp not built")
endif()

set_source_files_properties(src/abi/exception_classes.cpp src/hosted/stdexcept.cpp
                            PROPERTIES COMPILE_DEFINITIONS _YCXX_EXCEPTION_KEY_FUNCTIONS)
# The standard stream objects are initialized with a priority reserved for the implementation,
# ahead of every program's static objects (src/hosted/iostream.cpp); so is the classic locale
# (src/hosted/locale.cpp). ELF only: Mach-O has no initialization priorities, and <iostream>
# defines an ios_base::Init per translation unit there (_YCXX_HAS_INIT_PRIORITY, config.hpp).
set_source_files_properties(src/hosted/iostream.cpp src/hosted/locale.cpp PROPERTIES COMPILE_OPTIONS
  $<IF:$<CXX_COMPILER_ID:GNU>,-Wno-prio-ctor-dtor,-Wno-init-priority-reserved>)
# Likewise the main thread's thread-end sentinel (src/pal/posix/pal.c, DECISIONS §3), registered
# before the program can construct a thread_local.
set_source_files_properties(src/pal/posix/pal.c PROPERTIES COMPILE_OPTIONS
  $<$<C_COMPILER_ID:GNU>:-Wno-prio-ctor-dtor>)
# The classic locale holds the Annex D codecvt facets ([depr.locale.category]), which are
# declared [[deprecated]].
set_property(SOURCE src/hosted/locale.cpp APPEND PROPERTY COMPILE_OPTIONS -Wno-deprecated-declarations)

# Every source by its absolute path (the lists mix globs and names).
foreach(_v YCXX_LIBRARY_SOURCES YCXX_ABI_SOURCES)
  list(TRANSFORM ${_v} PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/ REGEX "^[^/]")
endforeach()
# What each image that links libycxx has its own hidden copy of (DECISIONS §20.6): the Itanium
# entry points (forwarders to the runtime), the __cxxabiv1 vtables and the fundamental type_info
# objects, std::nothrow, the default allocation functions and the allocation table. In static mode
# they are members of the archives like the rest; in shared mode they are libycxx_nonshared.a, which
# every image linking libycxx.so links, libycxx.so included.
file(GLOB YCXX_PER_IMAGE_SOURCES CONFIGURE_DEPENDS src/abi/entry/*.cpp)
list(APPEND YCXX_PER_IMAGE_SOURCES ${YCXX_NEW_SOURCES})
foreach(_s src/abi/rtti_classes.cpp src/abi/rtti_float16.cpp src/abi/nothrow.cpp src/hosted/cxa_demangle_entry.cpp)
  list(APPEND YCXX_PER_IMAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${_s})
endforeach()

# ---- Static mode (YCXX_STATIC): ycxx::core (libycxx.a, the library's out-of-line parts and the
# platform layer) and ycxx::abi (libycxx-abi.a, the ABI runtime) ----
if(YCXX_STATIC)
  add_library(ycxx STATIC ${YCXX_LIBRARY_SOURCES} src/runtime/linkage/static.cpp)
  set_target_properties(ycxx PROPERTIES EXPORT_NAME core)
  target_link_libraries(ycxx PRIVATE ycxx_headers)
  target_compile_options(ycxx PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++26 ${_ycxx_warnings}>
    $<$<COMPILE_LANGUAGE:C>:${_ycxx_warnings}>)
  add_library(ycxx_abi STATIC ${YCXX_ABI_SOURCES})
  set_target_properties(ycxx_abi PROPERTIES OUTPUT_NAME ycxx-abi EXPORT_NAME abi)
  target_link_libraries(ycxx_abi PRIVATE ycxx_headers)
  target_compile_options(ycxx_abi PRIVATE -std=c++26 ${_ycxx_warnings})
  target_include_directories(ycxx_abi PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
endif()

# ---- Sanitizer builds (YCXX_SANITIZE): libycxx itself instrumented ----
# A comma-separated list of the compilers' -fsanitize= names: address, undefined, thread (e.g.
# "address,undefined"). The archives are compiled with it, so that the sanitizer sees what the
# library does: ThreadSanitizer the library's own atomics and the synchronization built on them
# (futex mutexes, the thread pool's queue, reference counts), which it cannot see in uninstrumented
# code; AddressSanitizer and UBSan the library's memory accesses and operations. Programs linking
# such a build must be linked with the same -fsanitize= (ycxx::ycxx adds it to their link).
# tools/run-conformance builds one per compiler and sanitizer set when SANITIZER is set
# (build/<cc>-<sanitizers>); the default build is unchanged. DECISIONS §6.8.
# What the sanitizer runtimes supply instead of libycxx is linked from them: the global allocation
# functions (each in its own archive member, never pulled in when the runtime defines them) and,
# with ThreadSanitizer, the static-local guards (src/abi/guard.cpp).
set(YCXX_SANITIZE "" CACHE STRING
    "Instrument libycxx with -fsanitize=<list> (address, undefined, thread; comma-separated); empty: not instrumented")
if(YCXX_SANITIZE)
  string(REPLACE "," ";" _ycxx_sanitizers "${YCXX_SANITIZE}")
  foreach(_s IN LISTS _ycxx_sanitizers)
    if(NOT _s MATCHES "^(address|undefined|thread)$")
      message(FATAL_ERROR "libycxx: YCXX_SANITIZE: unknown sanitizer '${_s}' (address, undefined, thread)")
    endif()
  endforeach()
  set(_ycxx_sanitize_flags -fsanitize=${YCXX_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
  # GCC warns (-Wtsan) that ThreadSanitizer does not model atomic_thread_fence. The library's
  # seq_cst fences (the atomic wait/notify protocol, hazard pointers, rcu) order a store before a
  # later load (Dekker); no plain data relies on them for happens-before, which comes from the
  # atomic operations themselves, so the sanitizer misses nothing it would report.
  if(YCXX_SANITIZE MATCHES "thread" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND _ycxx_sanitize_flags -Wno-tsan)
  endif()
  if(YCXX_SHARED)
    message(FATAL_ERROR "libycxx: YCXX_SANITIZE needs a static-only build (YCXX_SHARED=OFF): instrumented "
                        "shared libraries are not supported yet (DECISIONS §20.8)")
  endif()
  target_compile_options(ycxx PRIVATE ${_ycxx_sanitize_flags})
  target_compile_options(ycxx_abi PRIVATE ${_ycxx_sanitize_flags})
  message(STATUS "libycxx: instrumented with -fsanitize=${YCXX_SANITIZE}")
endif()

if(YCXX_STATIC)
  # The two archives refer to each other (the runtime uses the platform layer and operator
  # delete; the library throws through the runtime): a cyclic static dependency, which CMake
  # links by repeating the pair.
  target_link_libraries(ycxx INTERFACE ycxx_abi)
  target_link_libraries(ycxx_abi INTERFACE ycxx)
  set_property(TARGET ycxx ycxx_abi PROPERTY LINK_INTERFACE_MULTIPLICITY 3)
  # Usable in shared objects and position-independent executables.
  set_target_properties(ycxx ycxx_abi PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
# Nothing the archives define is exported from the program or shared object that links them,
# except the allocation table: another C++ library in the process (Apple's libc++/libc++abi, which
# libSystem loads into every process; libstdc++) can neither take over libycxx's definitions nor
# be taken over by them (DECISIONS §2). The sources say so themselves, with visibility attributes
# on their namespaces and declarations, not with -fvisibility=hidden.

# The allocation table stays in every program and is visible to the shared libraries it loads
# (cmake/ycxx-link.cmake; DECISIONS §2).
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ycxx-link.cmake)
ycxx_link_options(_ycxx_table_link_options "${YCXX_NEW_SOURCES}")
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

# ---- Shared mode (YCXX_SHARED, DECISIONS §20): libycxx.so.0.<minor> (Mach-O:
# libycxx.0.<minor>.dylib), the library and the ABI runtime, exporting std::__y1, __ycxx, the
# runtime's entry points under its own names (__ycxx_abi_*) and the allocation table; and
# libycxx_nonshared.a, each image's hidden part ----
if(YCXX_SHARED)
  if(NOT YCXX_PAL STREQUAL "posix")
    message(FATAL_ERROR "libycxx: YCXX_SHARED needs YCXX_PAL=posix: a shared library cannot use hosted-layer "
                        "providers that live in the program (DECISIONS §20.2)")
  endif()
  set(_ycxx_abi_version ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
  # libycxx_nonshared.a: the per-image sources, and the platform layer's forwarders to the shared
  # library (src/nonshared/pal.c, tools/gen_pal_forwarders.py).
  add_library(ycxx_nonshared STATIC ${YCXX_PER_IMAGE_SOURCES} src/nonshared/pal.c)
  set_target_properties(ycxx_nonshared PROPERTIES OUTPUT_NAME ycxx_nonshared EXPORT_NAME nonshared
                        POSITION_INDEPENDENT_CODE ON)
  # The library's sources less the per-image ones, the shared marker and the exported platform layer.
  set(_ycxx_so_sources ${YCXX_LIBRARY_SOURCES} ${YCXX_ABI_SOURCES})
  list(REMOVE_ITEM _ycxx_so_sources ${YCXX_PER_IMAGE_SOURCES})
  add_library(ycxx_so SHARED ${_ycxx_so_sources} src/runtime/linkage/shared.cpp src/pal/export.c)
  # libycxx.so.0.1.0, soname libycxx.so.0.1 (ELF); libycxx.0.1.0.dylib, install name
  # @rpath/libycxx.0.1.dylib, compatibility version 0.1 (Mach-O). DECISIONS §20.7.
  set_target_properties(ycxx_so PROPERTIES OUTPUT_NAME ycxx EXPORT_NAME shared_library
                        VERSION ${PROJECT_VERSION} SOVERSION ${_ycxx_abi_version}
                        MACOSX_RPATH ON INSTALL_NAME_DIR "@rpath")
  foreach(_t ycxx_so ycxx_nonshared)
    target_link_libraries(${_t} PRIVATE ycxx_headers)
    target_compile_definitions(${_t} PRIVATE YCXX_SHARED)
    target_compile_options(${_t} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-std=c++26 ${_ycxx_warnings}>
      $<$<COMPILE_LANGUAGE:C>:${_ycxx_warnings}>)
    target_include_directories(${_t} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
  endforeach()
  # The export list (DECISIONS §20.3), from src/export.txt: a version script on ELF (every export
  # gets the version YCXX_<major>.<minor>), an exported-symbols list on Mach-O. The visibility
  # attributes decide what is exported; the list is the safety net that keeps every other name
  # local (GCC's predeclared __cxa_* functions; on Darwin, libgcc's emulated-TLS entry points,
  # which -static-libgcc puts in the dylib as weak definitions, DECISIONS §20.12).
  file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/src/export.txt _ycxx_export_lines ENCODING UTF-8 REGEX "^[^#]")
  set(_ycxx_exports "")
  foreach(_l IN LISTS _ycxx_export_lines)
    string(STRIP "${_l}" _l)
    if(_l)
      list(APPEND _ycxx_exports "${_l}")
    endif()
  endforeach()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/export.txt)
  if(APPLE)
    set(_ycxx_list ${CMAKE_CURRENT_BINARY_DIR}/libycxx.exports)
    set(_text "")
    foreach(_p IN LISTS _ycxx_exports)
      if(_p MATCHES "^[*]")
        string(APPEND _text "${_p}\n")
      else()
        string(APPEND _text "_${_p}\n")
      endif()
    endforeach()
    file(WRITE ${_ycxx_list} "${_text}")
    target_link_options(ycxx_so PRIVATE LINKER:-exported_symbols_list,${_ycxx_list})
  else()
    set(_ycxx_list ${CMAKE_CURRENT_BINARY_DIR}/libycxx.map)
    list(JOIN _ycxx_exports ";\n    " _text)
    file(WRITE ${_ycxx_list} "YCXX_${_ycxx_abi_version} {\n  global:\n    ${_text};\n  local: *;\n};\n")
    # --no-undefined: everything is resolved now, not at load time. Not -Bsymbolic-functions: the
    # library's calls to std::is_debugger_present must reach a program's replacement
    # ([debugging.utility]), as every other exported function may be interposed (the allocation
    # functions are hidden, per image).
    target_link_options(ycxx_so PRIVATE LINKER:--version-script=${_ycxx_list} LINKER:--no-undefined)
  endif()
  set_property(TARGET ycxx_so APPEND PROPERTY LINK_DEPENDS ${_ycxx_list})
  # The library is itself an image that links libycxx: its per-image part, and the allocation
  # table's anchor (the table is exported, and the program's, when it has one, wins).
  target_link_libraries(ycxx_so PRIVATE ycxx_nonshared)
  # Only the table's anchor: in a shared library GNU ld's --export-dynamic-symbol acts as a dynamic
  # list, which binds every other symbol locally (the program's copy of std::cout would then not
  # be the library's); the export list exports the table.
  set(_ycxx_so_table_options ${_ycxx_table_link_options})
  list(FILTER _ycxx_so_table_options EXCLUDE REGEX "export-dynamic-symbol")
  target_link_options(ycxx_so PRIVATE $<$<LINK_LANGUAGE:CXX>:-nostdlib++> ${_ycxx_so_table_options})
  if(APPLE)
    # GCC on Darwin: libgcc linked statically, as into programs (tools/ycxx-cxx).
    target_link_options(ycxx_so PRIVATE $<$<CXX_COMPILER_ID:GNU>:-static-libgcc>)
  else()
    target_link_libraries(ycxx_so PRIVATE m)
    target_link_options(ycxx_so PRIVATE -shared-libgcc)
    if(YCXX_GCC_INSTALL_DIR)
      target_link_options(ycxx_so PRIVATE $<$<CXX_COMPILER_ID:Clang>:--gcc-install-dir=${YCXX_GCC_INSTALL_DIR}>)
    endif()
  endif()
  message(STATUS "libycxx: shared library libycxx ${PROJECT_VERSION} (ABI ${_ycxx_abi_version}), export list ${_ycxx_list}")
endif()

# <ycxx/generated/linkage.hpp>: in a shared-only build, translation units are compiled in shared mode
# unless they say otherwise (YCXX_SHARED, ycxx/config.hpp).
if(YCXX_STATIC)
  file(REMOVE ${YCXX_GENERATED_INCLUDE}/ycxx/generated/linkage.hpp)
else()
  file(WRITE ${YCXX_GENERATED_INCLUDE}/ycxx/generated/linkage.hpp
       "// Generated by CMakeLists.txt: this libycxx has its shared library only (DECISIONS §20.3).\n#pragma once\n#define _YCXX_DEFAULT_SHARED 1\n")
endif()

# ---- ycxx::static, ycxx::shared, ycxx::ycxx: what applications link ----
# ycxx::static links the archives, ycxx::shared the shared library and libycxx_nonshared.a (and
# compiles in shared mode, YCXX_SHARED); ycxx::ycxx is the default kind's. Each records its kind in
# the interface property YCXX_LINKAGE, which must agree across an image's dependencies: CMake stops
# a target that links both (COMPATIBLE_INTERFACE_STRING).
function(_ycxx_consumer_target target linkage)
  add_library(${target} INTERFACE)
  set_target_properties(${target} PROPERTIES EXPORT_NAME ${linkage} INTERFACE_YCXX_LINKAGE ${linkage}
                        COMPATIBLE_INTERFACE_STRING YCXX_LINKAGE)
  if(linkage STREQUAL "static")
    target_link_libraries(${target} INTERFACE ycxx_headers ycxx)
  else()
    target_link_libraries(${target} INTERFACE ycxx_headers ycxx_so ycxx_nonshared)
    target_compile_definitions(${target} INTERFACE YCXX_SHARED)
  endif()
  # <cmath> calls the C library's math functions at run time; libm is separate from libc on
  # glibc and musl (on Darwin it is part of libSystem).
  if(NOT APPLE AND _ycxx_clib)
    target_link_libraries(${target} INTERFACE m)
  endif()
  # -shared-libgcc: one unwinder per process. glibc's pthread_exit/pthread_cancel unwind through
  # libgcc_s.so; a second, static copy (libgcc_eh) used by the personality routine would abort.
  target_link_options(${target} INTERFACE $<$<LINK_LANGUAGE:CXX>:-nostdlib++>
                      $<$<AND:$<LINK_LANGUAGE:CXX>,$<PLATFORM_ID:Linux>>:-shared-libgcc>)
  # An instrumented libycxx needs its sanitizers' runtimes in the program.
  if(YCXX_SANITIZE)
    target_link_options(${target} INTERFACE -fsanitize=${YCXX_SANITIZE})
    # Clang links ThreadSanitizer's C++ part whole: strong global allocation functions that would
    # bypass libycxx's (and clash with a program's replacement); malloc stays intercepted
    # (tools/ycxx-cxx does the same). GCC's libtsan.so defines them too; the link options of
    # cmake/ycxx-link.cmake link libycxx's ahead of it.
    if(YCXX_SANITIZE MATCHES "thread")
      target_link_options(${target} INTERFACE $<$<CXX_COMPILER_ID:Clang>:-fno-sanitize-link-c++-runtime>)
    endif()
  endif()
  target_link_options(${target} INTERFACE ${_ycxx_table_link_options})
  if(YCXX_GCC_INSTALL_DIR)
    # The build tree's targets (add_subdirectory, FetchContent); libycxxConfig.cmake adds the
    # same to the installed ones, where the directory can be overridden by the consumer.
    target_compile_options(${target} INTERFACE
      $<BUILD_INTERFACE:$<$<COMPILE_LANG_AND_ID:CXX,Clang>:--gcc-install-dir=${YCXX_GCC_INSTALL_DIR}>>)
    target_link_options(${target} INTERFACE
      $<BUILD_INTERFACE:$<$<AND:$<LINK_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:--gcc-install-dir=${YCXX_GCC_INSTALL_DIR}>>)
  endif()
endfunction()
set(_ycxx_consumer_targets "")
if(YCXX_STATIC)
  _ycxx_consumer_target(ycxx_static static)
  list(APPEND _ycxx_consumer_targets ycxx_static)
  add_library(ycxx::static ALIAS ycxx_static)
endif()
if(YCXX_SHARED)
  _ycxx_consumer_target(ycxx_shared shared)
  list(APPEND _ycxx_consumer_targets ycxx_shared)
  add_library(ycxx::shared ALIAS ycxx_shared)
endif()
add_library(ycxx_ycxx INTERFACE)
set_target_properties(ycxx_ycxx PROPERTIES EXPORT_NAME ycxx)
target_link_libraries(ycxx_ycxx INTERFACE ycxx_${YCXX_DEFAULT_LINKAGE})
message(STATUS "libycxx: kinds built: ${_ycxx_consumer_targets}; ycxx::ycxx is ycxx::${YCXX_DEFAULT_LINKAGE}")

# ---- For projects that know nothing about libycxx (docs/BUILDING_PROJECTS.md) ----
# bin/ycxx-c++ and bin/ycxx-cc (the compilers), toolchain.cmake, libycxx.pc (pkg-config) and
# meson-native.ini, in the build tree and installed (cmake/ycxx-consumer.cmake).
include(GNUInstallDirs)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ycxx-consumer.cmake)
if(YCXX_PAL STREQUAL "posix")
  ycxx_consumer_files("${_ycxx_table_link_options}")
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
    ${YCXX_FREESTANDING_SOURCES} src/runtime/linkage/static.cpp ${YCXX_CHARCONV_SOURCES} ${YCXX_ATOMIC_SOURCES}
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
  if(CMAKE_CXX_COMPILER_TARGET)
    set(_ycxx_freestanding_target "${CMAKE_CXX_COMPILER_TARGET}")
  else()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
                    OUTPUT_VARIABLE _ycxx_freestanding_target OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  if(_ycxx_freestanding_target MATCHES "^(aarch64|arm64)")
      # Outline atomics probe libc for LSE support; neither the bare-metal runtime nor
      # its consumers have libc. Use the base ISA's inline atomics in both.
      target_compile_options(ycxx_freestanding PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-mno-outline-atomics>)
  endif()
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
# Built per kind (DECISIONS §20.4): the BMIs and the initializers carry the mode's visibility.
# ycxx::modules is the default kind's; with both kinds built, ycxx::shared_modules is the shared one's.
function(_ycxx_modules_target target output export linkage)
  add_library(${target} STATIC)
  target_sources(${target} PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules
                 FILES modules/std.cppm modules/std.compat.cppm)
  set_target_properties(${target} PROPERTIES OUTPUT_NAME ${output} EXPORT_NAME ${export}
                        CXX_SCAN_FOR_MODULES ON POSITION_INDEPENDENT_CODE ON)
  # CMake scans and builds modules only for targets that require C++20 or later; the consumers of
  # the module need it too (cxx_std_23: CMake 3.28 does not know C++26 for these compilers);
  # ycxx::headers' own -std=c++26 comes later on the command line and wins.
  target_compile_features(${target} PUBLIC cxx_std_23)
  target_link_libraries(${target} PUBLIC ycxx_${linkage})
  # The installed package's consumers compile the interface units with the flags CMake exports
  # from this target's own properties (IMPORTED_CXX_MODULES_*); CMake 3.28 does not add those of
  # its link dependencies (ycxx::headers) there, so the headers' flags are repeated here. The
  # warning flags are libycxx's own build's only. `std` is a module name reserved to the
  # implementation ([module.unit]/1), which libycxx is.
  # -isystem as an option: CMake 3.28 would export include directories without SYSTEM. The
  # installed package's libycxxConfig.cmake adds the installed directory the same way.
  target_compile_options(${target} PRIVATE "$<BUILD_INTERFACE:SHELL:-isystem ${YCXX_INCLUDE}>"
                         -nostdinc++ ${YCXX_CXX_STANDARD_FLAG}
                         $<$<CXX_COMPILER_ID:GNU>:-Wno-attributes>
                         $<$<CXX_COMPILER_ID:Clang>:-Wno-reserved-module-identifier>
                         $<BUILD_INTERFACE:${_ycxx_warnings}>)
  if(linkage STREQUAL "shared")
    target_compile_definitions(${target} PRIVATE YCXX_SHARED)
  endif()
endfunction()
set(_ycxx_modules_targets "")
if(YCXX_MODULES)
  _ycxx_modules_target(ycxx_modules ycxx-modules modules ${YCXX_DEFAULT_LINKAGE})
  list(APPEND _ycxx_modules_targets ycxx_modules)
  add_library(ycxx::modules ALIAS ycxx_modules)
  if(YCXX_STATIC AND YCXX_SHARED)
    _ycxx_modules_target(ycxx_shared_modules ycxx-modules-shared shared_modules shared)
    list(APPEND _ycxx_modules_targets ycxx_shared_modules)
    add_library(ycxx::shared_modules ALIAS ycxx_shared_modules)
  endif()
endif()

add_library(ycxx::headers ALIAS ycxx_headers)
if(YCXX_STATIC)
  add_library(ycxx::core ALIAS ycxx)
  add_library(ycxx::abi ALIAS ycxx_abi)
endif()
if(YCXX_SHARED)
  add_library(ycxx::shared_library ALIAS ycxx_so)
  add_library(ycxx::nonshared ALIAS ycxx_nonshared)
endif()
add_library(ycxx::ycxx ALIAS ycxx_ycxx)

# ---- install and package ----
if(YCXX_INSTALL)
  include(CMakePackageConfigHelpers)
  set(YCXX_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/libycxx)
  install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/libycxx)
  install(DIRECTORY include/ ${YCXX_GENERATED_INCLUDE}/ DESTINATION ${YCXX_INSTALL_INCLUDEDIR})
  set(_ycxx_installed ycxx_headers ycxx_ycxx ${_ycxx_consumer_targets})
  if(YCXX_STATIC)
    list(APPEND _ycxx_installed ycxx ycxx_abi)
  endif()
  if(YCXX_SHARED)
    list(APPEND _ycxx_installed ycxx_nonshared)
  endif()
  if(YCXX_FREESTANDING_RUNTIME)
    list(APPEND _ycxx_installed ycxx_freestanding)
  endif()
  install(TARGETS ${_ycxx_installed} EXPORT libycxxTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  if(YCXX_SHARED)
    # ELF: libycxx.so is a linker script naming the shared library and the per-image archive, so
    # that -lycxx links both (as glibc's libc.so does); Mach-O has no linker scripts, and its
    # consumers name the pair (DECISIONS §20.2).
    if(APPLE)
      install(TARGETS ycxx_so EXPORT libycxxTargets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    else()
      install(TARGETS ycxx_so EXPORT libycxxTargets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP)
      set(_ycxx_script ${CMAKE_CURRENT_BINARY_DIR}/consumer/libycxx.so)
      file(WRITE ${_ycxx_script} "/* libycxx (DECISIONS §20.2): the shared library and each image's part of it. */\n"
           "GROUP ( libycxx.so.${_ycxx_abi_version} libycxx_nonshared.a )\n")
      install(FILES ${_ycxx_script} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
  endif()
  if(YCXX_MODULES)
    # The interface units are installed as sources: an importing project builds its own BMIs.
    install(TARGETS ${_ycxx_modules_targets} EXPORT libycxxTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_DATADIR}/libycxx/modules)
  endif()
  install(EXPORT libycxxTargets NAMESPACE ycxx:: DESTINATION ${YCXX_CMAKE_DIR}
          CXX_MODULES_DIRECTORY cxx-modules)
  list(JOIN _ycxx_layers ";" YCXX_SELECTED_HOSTED_LAYERS)
  set(YCXX_BUILT_STATIC ${YCXX_STATIC})
  set(YCXX_BUILT_SHARED ${YCXX_SHARED})
  configure_package_config_file(cmake/libycxxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfig.cmake INSTALL_DESTINATION ${YCXX_CMAKE_DIR})
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/libycxxConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/libycxxConfigVersion.cmake
                cmake/ycxx-hosted-layers.cmake
          DESTINATION ${YCXX_CMAKE_DIR})
endif()
