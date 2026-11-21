cmake_minimum_required(VERSION 3.28)

if(NOT CXX OR NOT WORK_DIR)
  message(FATAL_ERROR "Run with -DCXX=<compiler> -DWORK_DIR=<scratch directory>")
endif()
cmake_path(ABSOLUTE_PATH WORK_DIR)
set(_source "${WORK_DIR}/source")
set(_build "${WORK_DIR}/build")
set(_quote "${WORK_DIR}/quote")
set(_include "${WORK_DIR}/include")
file(WRITE "${_source}/unit.cppm" [=[module;
# include "local.hpp" /* Source-local wins. */ // The build tree and -iquote also have this name.
#include "quote-only.hpp"
#include <external.hpp>
/*
#include "commented.hpp"
*/
// This comment continues on the next line. \
#include "continued.hpp"
#if NEGATIVE
#error original line after comments
#include "missing.hpp"
#endif
export module ycxx.check;
import std;
static_assert(local == 1 && quote_only == 2 && nested == 3 && standard == 4);
]=])
file(WRITE "${_source}/local.hpp" "inline constexpr int local = 1;\n")
file(WRITE "${_source}/nested.hpp" "#error an external header must not search the unit's directory\n")
file(WRITE "${_build}/local.hpp" "#error the generated source is not the original include context\n")
file(WRITE "${_quote}/local.hpp" "#error caller quote paths follow the source's directory\n")
file(WRITE "${_quote}/quote-only.hpp" "inline constexpr int quote_only = 2;\n")
file(WRITE "${_include}/external.hpp" "#include \"nested.hpp\"\n")
file(WRITE "${_include}/fallback/nested.hpp" "inline constexpr int nested = 3;\n")
file(WRITE "${_include}/standard.hpp" "inline constexpr int standard = 4;\n")
file(WRITE "${WORK_DIR}/std.cppm" "#include <standard.hpp>\n")

foreach(_mode plain textual-std)
  set(_output "${_build}/${_mode}.cpp")
  set(_flags "")
  if(_mode STREQUAL "textual-std")
    set(_output "${_build}/${_mode}.cppm")
    set(_flags -fmodules)
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" "-DINPUT=${_source}/unit.cppm" "-DOUTPUT=${_output}"
                  "-DMODE=${_mode}" "-DSTD_MODULE=${WORK_DIR}/std.cppm"
                  -P "${CMAKE_CURRENT_LIST_DIR}/../../cmake/ycxx-lower-module-unit.cmake"
                  COMMAND_ERROR_IS_FATAL ANY)
  set(_compile "${CXX}" -std=c++26 ${_flags} -fsyntax-only -iquote "${_quote}"
               "-I${_include}" "-I${_include}/fallback" "${_output}")
  execute_process(COMMAND ${_compile} WORKING_DIRECTORY "${_build}" COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND ${_compile} -DNEGATIVE=1 WORKING_DIRECTORY "${_build}"
                  RESULT_VARIABLE _status ERROR_VARIABLE _diagnostic)
  string(FIND "${_diagnostic}" "${_source}/unit.cppm:11:" _after_comments)
  string(FIND "${_diagnostic}" "${_source}/unit.cppm:12:" _missing_header)
  if(_status EQUAL 0 OR _after_comments EQUAL -1 OR _missing_header EQUAL -1)
    message(FATAL_ERROR "${_mode} did not diagnose errors at the original lines:\n${_diagnostic}")
  endif()
  message(STATUS "${_mode} preserves quoted include lookup and diagnostic location")
endforeach()
