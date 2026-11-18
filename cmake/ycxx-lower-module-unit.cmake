# Rewrites one of libycxx's own module units (src/**/*.cppm; cmake/ycxx-source-modules.cmake,
# DECISIONS §20). Run as a script:
#   cmake -DINPUT=<unit.cppm> -DOUTPUT=<file> -DMODE=plain|textual-std -DSTD_MODULE=<modules/std.cppm>
#         -DSTD_COMPAT_MODULE=<modules/std.compat.cppm> -P ycxx-lower-module-unit.cmake
# An import of std (`import std;`, `import std.compat;`) becomes the #includes of the module's
# global module fragment (every header it re-exports; std.compat re-exports std too).
#
# MODE=plain (YCXX_SOURCE_MODULES=OFF): an ordinary translation unit. `module;`,
# `export module NAME;` (or `module NAME;`) and the directive hiding the unit's module initializer
# (`asm((...__hide_symbol("_ZGIW...")));`) become empty lines, an import of std its #includes.
# Any other module syntax (another import, an export declaration) stops the build: such a unit
# needs YCXX_SOURCE_MODULES=ON.
#
# MODE=textual-std (GCC 16 with YCXX_SOURCE_MODULES=ON, for a unit whose global module fragment
# includes standard headers: GCC cannot import std after them, STATUS "known compiler gaps"): the
# unit stays a module unit; the imports of std become empty lines and their #includes go to the
# end of the global module fragment, before the module declaration.
#
# #line directives keep diagnostics on the unit's own lines.
cmake_minimum_required(VERSION 3.28)

if(NOT MODE)
  set(MODE plain)
endif()

function(_ycxx_module_includes which out)
  file(STRINGS ${STD_MODULE} _lines REGEX "^#include <")
  if(which STREQUAL "std.compat")
    file(STRINGS ${STD_COMPAT_MODULE} _compat REGEX "^#include <")
    list(APPEND _lines ${_compat})
  endif()
  list(JOIN _lines "\n" _lines)
  set(${out} "${_lines}" PARENT_SCOPE)
endfunction()

set(_module_decl "^[ \t]*(export[ \t]+)?module[ \t]+[A-Za-z_][A-Za-z0-9_.]*[ \t]*;[ \t]*(//.*)?$")
set(_import_std "^[ \t]*import[ \t]+(std|std\\.compat)[ \t]*;[ \t]*(//.*)?$")

# textual-std: which std modules the unit imports, for its global module fragment.
file(READ ${INPUT} _text)
set(_textual "")
if(MODE STREQUAL "textual-std")
  if(_text MATCHES "(^|\n)[ \t]*import[ \t]+std\\.compat[ \t]*;")
    set(_textual std.compat)
  elseif(_text MATCHES "(^|\n)[ \t]*import[ \t]+std[ \t]*;")
    set(_textual std)
  endif()
endif()

# Line by line with string(FIND): a CMake list would split at ';' and not at all inside '[...]'.
if(MODE STREQUAL "plain")
  set(_out "#line 1 \"${INPUT}\"\n")
else()
  set(_out "")  # nothing may precede `module;`
endif()
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
  math(EXPR _next "${_n} + 1")
  if(_line MATCHES "^[ \t]*module[ \t]*;[ \t]*(//.*)?$")
    if(MODE STREQUAL "plain")
      string(APPEND _out "\n")
    else()
      string(APPEND _out "${_line}\n#line ${_next} \"${INPUT}\"\n")
    endif()
  elseif(_line MATCHES "${_module_decl}")
    if(MODE STREQUAL "plain")
      string(APPEND _out "\n")
    else()
      if(_textual)
        _ycxx_module_includes(${_textual} _includes)
        string(APPEND _out "${_includes}\n#line ${_n} \"${INPUT}\"\n")
      endif()
      string(APPEND _out "${_line}\n")
    endif()
  elseif(_line MATCHES "^[ \t]*asm\\(\\(.*__hide_symbol\\(\"_ZGIW[A-Za-z0-9_]*\"\\)\\)\\);[ \t]*(//.*)?$"
         AND MODE STREQUAL "plain")
    string(APPEND _out "\n")
  elseif(_line MATCHES "${_import_std}")
    if(MODE STREQUAL "plain")
      _ycxx_module_includes(${CMAKE_MATCH_1} _includes)
      string(APPEND _out "${_includes}\n#line ${_next} \"${INPUT}\"\n")
    else()
      string(APPEND _out "\n")
    endif()
  elseif(_line MATCHES "^[ \t]*(export|import)[ \t]" AND MODE STREQUAL "plain")
    message(FATAL_ERROR "${INPUT}:${_n}: '${_line}': only `import std;` and `import std.compat;` "
                        "can be lowered to a plain translation unit; this unit needs "
                        "YCXX_SOURCE_MODULES=ON (a Ninja generator)")
  else()
    string(APPEND _out "${_line}\n")
  endif()
endwhile()
file(WRITE ${OUTPUT} "${_out}")
