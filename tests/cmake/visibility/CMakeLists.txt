# libycxx next to another C++ standard library in one process (tests/cmake/run.sh, DECISIONS §2):
#   other   shared library built with the toolchain's own C++ library (libstdc++ or libc++)
#   mine    shared library built with libycxx
#   prog    program built with libycxx, linked with `other`
#   host    program built with the toolchain's C++ library, linked with `other` and `mine`
#   catcher program built with libycxx, catching the exceptions of `thrower`, a shared library
#           built with libycxx (prints "caught 15 uncaught 0 0")
# VIS_LINKAGE names the kind of libycxx the libycxx images link: ycxx (the default, ycxx::ycxx),
# static or shared (DECISIONS §20.2; tests/cmake/run.sh runs both kinds).
# With both kinds installed, the host/plugin matrix (plugin/, DECISIONS §20.6): host_<kind> dlopen()s
# plugin_<kind>, exchanges std::string and std::vector both ways and catches the plugin's
# exceptions ("plugin 15"), and reports the runtime state they share ("shared-state 15" only when
# both are shared-mode images, one runtime in libycxx.so; "shared-state 0" otherwise).
# Each of `other` and `mine` throws and catches exceptions, asks std::current_exception, and makes
# an allocation fail (its own new_handler, its own bad_alloc); both programs print "mine 7 other 7"
# when each library uses its own runtime, allocation functions included (libycxx's defaults are
# shared among libycxx images only, through the allocation table). Before libycxx hid its
# symbols, the exported definitions of one library bound the other's references on ELF (and
# Apple's libc++abi took over libycxx's weak definitions on Darwin).
cmake_minimum_required(VERSION 3.28)
project(ycxx_visibility LANGUAGES CXX)

find_package(libycxx 0.1 CONFIG REQUIRED)
set(VIS_LINKAGE ycxx CACHE STRING "The kind of libycxx: ycxx (the default), static or shared")
set(ycxx_lib ycxx::${VIS_LINKAGE})

# The toolchain's C++ library may not be the system's (GCC 16's libstdc++ under its prefix).
set(_rpath)
foreach(_lib libstdc++${CMAKE_SHARED_LIBRARY_SUFFIX} libc++${CMAKE_SHARED_LIBRARY_SUFFIX})
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${_lib} OUTPUT_VARIABLE _path
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(IS_ABSOLUTE "${_path}")
    get_filename_component(_dir "${_path}" DIRECTORY)
    list(APPEND _rpath "${_dir}")
  endif()
endforeach()

add_library(other SHARED other.cpp)
target_compile_features(other PRIVATE cxx_std_20)
set_target_properties(other PROPERTIES BUILD_RPATH "${_rpath}")

add_library(mine SHARED mine.cpp)
target_link_libraries(mine PRIVATE ${ycxx_lib})

add_executable(prog main.cpp mine.cpp)
target_link_libraries(prog PRIVATE ${ycxx_lib} other)

add_executable(host main.cpp)
target_compile_features(host PRIVATE cxx_std_20)
target_link_libraries(host PRIVATE other mine)
set_target_properties(host PROPERTIES BUILD_RPATH "${_rpath}")

# Two images built with libycxx, each with its own copy of the runtime: exceptions thrown in one
# are caught in the other.
add_library(thrower SHARED thrower.cpp)
target_link_libraries(thrower PRIVATE ${ycxx_lib})
add_executable(catcher catcher.cpp)
target_link_libraries(catcher PRIVATE ${ycxx_lib} thrower)

# The host/plugin matrix, when both kinds are installed. The plugins' entry points are extern "C";
# a static-mode plugin built with GCC needs them marked for export (their signatures name library
# types, hidden in static mode), a shared-mode one does not (PLUGIN_EXPORT_ATTRIBUTE).
if(TARGET ycxx::static AND TARGET ycxx::shared)
  foreach(kind static shared)
    add_executable(host_${kind} plugin/host.cpp)
    target_link_libraries(host_${kind} PRIVATE ycxx::${kind} ${CMAKE_DL_LIBS})
    add_library(plugin_${kind} MODULE plugin/plugin.cpp)
    target_link_libraries(plugin_${kind} PRIVATE ycxx::${kind})
  endforeach()
  target_compile_definitions(plugin_static PRIVATE PLUGIN_EXPORT_ATTRIBUTE)
endif()
