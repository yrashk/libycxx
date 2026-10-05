# libycxx next to another C++ standard library in one process (tests/cmake/run.sh, DECISIONS §2):
#   other   shared library built with the toolchain's own C++ library (libstdc++ or libc++)
#   mine    shared library built with libycxx
#   prog    program built with libycxx, linked with `other`
#   host    program built with the toolchain's C++ library, linked with `other` and `mine`
# Each of `other` and `mine` throws and catches exceptions and asks std::current_exception; both
# programs print "mine 3 other 3" when each library uses its own runtime. Before libycxx hid its
# symbols, the exported definitions of one library bound the other's references on ELF (and
# Apple's libc++abi took over libycxx's weak definitions on Darwin).
cmake_minimum_required(VERSION 3.28)
project(ycxx_visibility LANGUAGES CXX)

find_package(libycxx 0.1 CONFIG REQUIRED)

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
target_link_libraries(mine PRIVATE ycxx::ycxx)

add_executable(prog main.cpp mine.cpp)
target_link_libraries(prog PRIVATE ycxx::ycxx other)

add_executable(host main.cpp)
target_compile_features(host PRIVATE cxx_std_20)
target_link_libraries(host PRIVATE other mine)
set_target_properties(host PROPERTIES BUILD_RPATH "${_rpath}")
