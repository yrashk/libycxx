cmake_minimum_required(VERSION 3.28)

if(NOT CC OR NOT CXX OR NOT WORK_DIR)
  message(FATAL_ERROR "Run with -DCC=<gcc> -DCXX=<g++> -DWORK_DIR=<scratch directory>")
endif()
find_program(_pkg_config pkg-config)
find_program(_ar ar REQUIRED)
cmake_path(ABSOLUTE_PATH WORK_DIR)
cmake_path(SET _lib NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../..")
string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef _nonce)
set(_run "${WORK_DIR}/${_nonce}")
set(_source "${_run}/source")
set(_build "${_run}/build")
set(_prefix "${_run}/moved")
file(MAKE_DIRECTORY "${_source}/cmake" "${_source}/tools/lib")
foreach(_file cmake/ycxx-c++.in cmake/ycxx-lower-module-unit.cmake tools/ycxx-check-binary tools/lib/ycxx_linkage.py)
  file(COPY_FILE "${_lib}/${_file}" "${_source}/${_file}")
endforeach()
file(WRITE "${_source}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.28)
project(libycxx VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
include(GNUInstallDirs)
set(YCXX_PAL posix)
set(YCXX_STATIC ON)
set(YCXX_DEFAULT_LINKAGE static)
set(YCXX_INSTALL ON)
set(YCXX_INCLUDE "${LIBYCXX_SOURCE}/include")
set(YCXX_GENERATED_INCLUDE "${CMAKE_BINARY_DIR}/generated/include")
set(YCXX_INSTALL_INCLUDEDIR include/libycxx)
include("${LIBYCXX_SOURCE}/cmake/ycxx-source-modules.cmake")
include("${LIBYCXX_SOURCE}/cmake/ycxx-consumer.cmake")

add_library(ycxx STATIC src/witness.cppm "${LIBYCXX_SOURCE}/src/runtime/linkage/static.cpp")
target_include_directories(ycxx PRIVATE "${YCXX_INCLUDE}")
target_compile_options(ycxx PRIVATE -std=c++26 -nostdinc++)
ycxx_add_module_units(ycxx STD_MODULES ycxx_modules SOURCES src/witness.cppm)
add_library(ycxx_abi STATIC src/abi.cpp src/provider_value.cpp)
set_target_properties(ycxx_abi PROPERTIES OUTPUT_NAME ycxx-abi)
target_link_libraries(ycxx INTERFACE ycxx_abi)

add_library(ycxx_modules STATIC)
set_target_properties(ycxx_modules PROPERTIES OUTPUT_NAME ycxx-modules)
target_compile_features(ycxx_modules PUBLIC cxx_std_23)
target_sources(ycxx_modules PUBLIC FILE_SET CXX_MODULES FILES modules/std.cppm)
target_link_libraries(ycxx_modules INTERFACE ycxx_abi)
cmake_language(DEFER CALL ycxx_consumer_files "")

install(TARGETS ycxx ycxx_abi ycxx_modules EXPORT libycxxTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET CXX_MODULES DESTINATION share/libycxx/modules)
install(EXPORT libycxxTargets NAMESPACE ycxx:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libycxx
        CXX_MODULES_DIRECTORY cxx-modules)
file(WRITE "${CMAKE_BINARY_DIR}/libycxxConfig.cmake" "include(\"\${CMAKE_CURRENT_LIST_DIR}/libycxxTargets.cmake\")\n")
install(FILES "${CMAKE_BINARY_DIR}/libycxxConfig.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libycxx)
]=])
file(WRITE "${_source}/modules/std.cppm" [=[
export module std;
extern "C" int initialized;
extern "C" int provider_value();
int observe_initializer() { return initialized = provider_value(); }
int initialization = observe_initializer();
export inline constexpr int imported_value = 7;
]=])
file(WRITE "${_source}/src/abi.cpp" "extern \"C\" int abi_value() { return 3; }\n")
file(WRITE "${_source}/src/provider_value.cpp" "extern \"C\" int provider_value() { return 40; }\n")
file(WRITE "${_source}/src/explicit.cpp" "extern \"C\" int explicit_value() { return 1; }\n")
file(WRITE "${_source}/main.cpp" [=[
extern "C" { int initialized = 0; }
extern "C" int witness();
#if EXPLICIT_MODULES
extern "C" int explicit_value();
#endif
int main() {
  int result = witness() + initialized;
#if EXPLICIT_MODULES
  result += explicit_value();
#endif
  return result;
}
]=])

function(_witness import value)
  file(WRITE "${_source}/src/witness.cppm" "module;\n#include <ycxx/core/hidden_symbol.hpp>\n"
       "export module ycxx.witness;\n${import}\n"
       "asm((::__ycxx::__detail::__hide_symbol(\"_ZGIW4ycxxW7witness\")));\n"
       "extern \"C\" int abi_value();\nextern \"C\" int witness() { return ${value} + abi_value(); }\n")
endfunction()

function(_run_command expected description)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE _status OUTPUT_VARIABLE _log ERROR_VARIABLE _log)
  if(NOT _status STREQUAL "${expected}")
    message(FATAL_ERROR "${description} returned ${_status}, expected ${expected}\n${_log}")
  endif()
endfunction()

function(_link name expected)
  set(_exe "${_run}/${name}")
  execute_process(COMMAND ${ARGN} -o "${_exe}" RESULT_VARIABLE _status OUTPUT_VARIABLE _log ERROR_VARIABLE _log)
  if(EXPECT_MISSING_PROVIDER)
    if(_status EQUAL 0 OR NOT _log MATCHES "_ZGIW3std|initializer for module std")
      message(FATAL_ERROR "${name} did not report the missing standard module initializer\n${_log}")
    endif()
    message(STATUS "${name} failed with the missing standard module initializer")
  else()
    if(NOT _status EQUAL 0)
      message(FATAL_ERROR "${name} failed to link\n${_log}")
    endif()
    _run_command("${expected}" "${name} executable" "${_exe}")
  endif()
endfunction()

function(_pkg_flags dir name out)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E env "PKG_CONFIG_PATH=" "PKG_CONFIG_LIBDIR=${dir}"
                  "${_pkg_config}" --cflags --libs "${name}" OUTPUT_VARIABLE _flags COMMAND_ERROR_IS_FATAL ANY)
  separate_arguments(_flags UNIX_COMMAND "${_flags}")
  set(${out} "${_flags}" PARENT_SCOPE)
endfunction()

_witness("import std;" imported_value)
_run_command(0 "fixture configure" "${CMAKE_COMMAND}" -G Ninja -S "${_source}" -B "${_build}"
             "-DCMAKE_C_COMPILER=${CC}" "-DCMAKE_CXX_COMPILER=${CXX}" "-DLIBYCXX_SOURCE=${_lib}"
             -DCMAKE_INSTALL_LIBDIR=lib-probe -DYCXX_SOURCE_MODULES=ON)
_run_command(0 "fixture build" "${CMAKE_COMMAND}" --build "${_build}" --parallel 1)
_run_command(0 "fixture install" "${CMAKE_COMMAND}" --install "${_build}" --prefix "${_run}/installed")
file(RENAME "${_run}/installed" "${_prefix}")

set(_module_archive libycxx-modules.a)
set(_static "${CMAKE_COMMAND}" -E env YCXX_LINKAGE=static)
set(_dev ${_static} "YCXX_GXX=${CXX}" "${_lib}/tools/ycxx-cxx" gcc "--libdir=${_build}")
set(_main "${_source}/main.cpp")
_link(development 50 ${_dev} "${_main}")
_link(build-wrapper 50 ${_static} "${_build}/bin/ycxx-c++" "${_main}")
_link(installed-wrapper 50 ${_static} "${_prefix}/bin/ycxx-c++" "${_main}")
if(_pkg_config)
  _pkg_flags("${_build}/pkgconfig" libycxx _pc_build)
  _link(build-pkg-config 50 "${CXX}" "${_main}" ${_pc_build})
  _pkg_flags("${_prefix}/lib-probe/pkgconfig" libycxx-static _pc_install)
  _link(installed-pkg-config 50 "${CXX}" "${_main}" ${_pc_install})
else()
  message(STATUS "pkg-config unavailable, skipping its link commands")
endif()

file(WRITE "${_source}/consumer/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.28)
project(consumer LANGUAGES CXX)
find_package(libycxx CONFIG REQUIRED)
add_executable(verify ../main.cpp)
target_link_libraries(verify PRIVATE ycxx::ycxx)
]=])
_run_command(0 "installed consumer configure" "${CMAKE_COMMAND}" -G Ninja -S "${_source}/consumer" -B "${_run}/consumer"
             "-DCMAKE_CXX_COMPILER=${CXX}" "-Dlibycxx_DIR=${_prefix}/lib-probe/cmake/libycxx")
_run_command(0 "installed consumer build" "${CMAKE_COMMAND}" --build "${_run}/consumer" --parallel 1)
_run_command(50 "installed CMake consumer" "${_run}/consumer/verify")

if(EXPECT_MISSING_PROVIDER AND _pkg_config)
  list(FIND _pc_build -Wl,--end-group _group_end)
  if(_group_end EQUAL -1)
    list(APPEND _pc_build "${_build}/${_module_archive}")
  else()
    list(INSERT _pc_build ${_group_end} "${_build}/${_module_archive}")
  endif()
  _run_command(0 "manual provider link" "${CXX}" "${_main}" ${_pc_build} -o "${_run}/manual")
  _run_command(50 "manual provider control" "${_run}/manual")
  message(STATUS "Installed CMake and explicit provider controls ran the module initializer")
elseif(NOT EXPECT_MISSING_PROVIDER)
  file(MAKE_DIRECTORY "${_run}/explicit")
  file(WRITE "${_run}/explicit/flags" "-DEXPLICIT_MODULES=1\n")
  _run_command(0 "explicit archive object" "${CXX}" -c "${_source}/src/explicit.cpp" -o "${_run}/explicit.o")
  _run_command(0 "explicit archive" "${_ar}" rcs "${_run}/explicit/${_module_archive}" "${_run}/explicit.o")
  _link(explicit-archive 51 ${_dev} "--std-modules=${_run}/explicit" "${_main}")

  file(RENAME "${_build}/${_module_archive}" "${_run}/provider.a")
  execute_process(COMMAND ${_dev} "${_main}" -o "${_run}/missing"
                  RESULT_VARIABLE _status OUTPUT_VARIABLE _log ERROR_VARIABLE _log)
  if(_status EQUAL 0 OR NOT _log MATCHES "libycxx-modules\\.a")
    message(FATAL_ERROR "Link did not diagnose the removed provider archive\n${_log}")
  endif()
  _run_command(0 "compile-only wrapper" ${_static} "${_build}/bin/ycxx-c++" -fsyntax-only "${_main}")
  file(RENAME "${_run}/provider.a" "${_build}/${_module_archive}")

  _witness("" 7)
  _run_command(0 "rebuild after removing import" "${CMAKE_COMMAND}" --build "${_build}" --parallel 1)
  if(NOT EXISTS "${_build}/${_module_archive}")
    message(FATAL_ERROR "Expected the old provider archive to remain in the build tree")
  endif()
  file(READ "${_build}/pkgconfig/libycxx.pc" _pc_plain)
  file(READ "${_build}/ycxx-static-module-archive" _record)
  file(READ "${_build}/bin/ycxx-c++" _wrapper)
  if(_pc_plain MATCHES "libycxx-modules\\.a" OR _wrapper MATCHES "libycxx-modules\\.a" OR _record MATCHES "[^\r\n]")
    message(FATAL_ERROR "Removing the import left a standard module archive in the static link metadata")
  endif()
  _link(plain 10 ${_dev} "${_main}")
  file(REMOVE "${_build}/ycxx-static-module-archive")
  _link(old-build 10 ${_dev} "${_main}")
  message(STATUS "Static consumers ran the standard module initializer and removing the import cleared metadata")
endif()

file(REMOVE_RECURSE "${_run}")
