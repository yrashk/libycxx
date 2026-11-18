# Lowers one of libycxx's own module units (src/**/*.cppm) to an ordinary translation unit, for
# builds without C++ modules (YCXX_SOURCE_MODULES=OFF; cmake/ycxx-source-modules.cmake, DECISIONS
# §20). Run as a script:
#   cmake -DINPUT=<unit.cppm> -DOUTPUT=<unit.cpp> -DSTD_MODULE=<modules/std.cppm>
#         -DSTD_COMPAT_MODULE=<modules/std.compat.cppm> -P ycxx-lower-module-unit.cmake
# A unit is lowerable when its module lines are the only module syntax in it: `module;`,
# `export module NAME;` (or `module NAME;`), the directive hiding the unit's module initializer
# (`asm((...__hide_symbol("_ZGIW...")));`, DECISIONS §20) and `import std;` / `import std.compat;`.
# The first three become empty lines; an import of std becomes the #includes of that module's
# global module fragment (every header it re-exports). Anything else (another import, an export
# declaration) stops the build: such a unit needs YCXX_SOURCE_MODULES=ON. #line directives keep
# diagnostics on the unit's own lines.
cmake_minimum_required(VERSION 3.28)

function(_ycxx_module_includes module_file out)
  file(STRINGS ${module_file} _lines REGEX "^#include <")
  set(${out} "${_lines}" PARENT_SCOPE)
endfunction()

# Line by line with string(FIND): a CMake list would split at ';' and not at all inside '[...]'.
file(READ ${INPUT} _text)
set(_out "#line 1 \"${INPUT}\"\n")
set(_n 0)
while(NOT _text STREQUAL "")
  string(FIND "${_text}" "\n" _eol)
  if(_eol EQUAL -1)
    set(_line "${_text}")
    set(_text "")
  else()
    string(SUBSTRING "${_text}" 0 ${_eol} _line)
    math(EXPR _eol "${_eol} + 1")
    string(SUBSTRING "${_text}" ${_eol} -1 _text)
  endif()
  math(EXPR _n "${_n} + 1")
  if(_line MATCHES "^[ \t]*module[ \t]*;[ \t]*(//.*)?$"
     OR _line MATCHES "^[ \t]*(export[ \t]+)?module[ \t]+[A-Za-z_][A-Za-z0-9_.]*[ \t]*;[ \t]*(//.*)?$"
     OR _line MATCHES "^[ \t]*asm\\(\\(.*__hide_symbol\\(\"_ZGIW[A-Za-z0-9_]*\"\\)\\)\\);[ \t]*(//.*)?$")
    string(APPEND _out "\n")
  elseif(_line MATCHES "^[ \t]*import[ \t]+(std|std\\.compat)[ \t]*;[ \t]*(//.*)?$")
    if(CMAKE_MATCH_1 STREQUAL "std")
      _ycxx_module_includes(${STD_MODULE} _includes)
    else()
      _ycxx_module_includes(${STD_COMPAT_MODULE} _includes)
    endif()
    list(JOIN _includes "\n" _includes)
    math(EXPR _next "${_n} + 1")
    string(APPEND _out "${_includes}\n#line ${_next} \"${INPUT}\"\n")
  elseif(_line MATCHES "^[ \t]*(export|import)[ \t]")
    message(FATAL_ERROR "${INPUT}:${_n}: '${_line}': only `import std;` and `import std.compat;` "
                        "can be lowered to a plain translation unit; this unit needs "
                        "YCXX_SOURCE_MODULES=ON (a Ninja generator)")
  else()
    string(APPEND _out "${_line}\n")
  endif()
endwhile()
file(WRITE ${OUTPUT} "${_out}")
