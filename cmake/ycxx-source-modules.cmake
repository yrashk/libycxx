# libycxx's own sources as C++ module units (DECISIONS §21). A source of the library's archives
# may be a module interface unit, src/**/*.cppm, instead of a .cpp; nothing requires one. With
# source modules enabled, CMake compiles such a unit as a module unit (a private CXX_MODULES file
# set: its interface is the library's own and is not installed). Otherwise,
# cmake/ycxx-lower-module-unit.cmake lowers the unit to a plain translation unit so the library
# builds with any generator. Lowering handles the module lines a unit may have
# (`module;`, `export module NAME;`, `import std;`, `import std.compat;`); a unit with other module
# syntax needs YCXX_SOURCE_MODULES=ON. Where a unit cannot import std (ycxx_add_module_units), its
# imports are replaced by std's headers.
#
# Whatever a unit defines in namespace std, __ycxx or __cxxabiv1 must stay attached to the global
# module: it is in the global module fragment or in an `extern "C++"` block of the purview
# ([module.unit]/7). An entity attached to the named module is mangled with it (_ZW...), which
# would change libycxx's ABI; tools/check_source_modules.py checks the archive for such symbols.

set(YCXX_SOURCE_MODULES AUTO CACHE STRING
    "Implementation module mode: AUTO follows YCXX_MODULES on Ninja, ON builds modules on Ninja, OFF lowers them")
set_property(CACHE YCXX_SOURCE_MODULES PROPERTY TYPE STRING)
set_property(CACHE YCXX_SOURCE_MODULES PROPERTY STRINGS AUTO ON OFF)

set(_ycxx_native_source_modules OFF)
string(TOUPPER "${YCXX_SOURCE_MODULES}" _ycxx_source_modules_mode)
if(_ycxx_source_modules_mode STREQUAL "AUTO")
  if(YCXX_MODULES AND CMAKE_GENERATOR MATCHES "Ninja")
    set(_ycxx_native_source_modules ON)
  endif()
elseif(YCXX_SOURCE_MODULES) # Existing cache and parent overrides accept CMake Boolean values.
  set(_ycxx_native_source_modules ON)
endif()
if(_ycxx_native_source_modules AND NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(FATAL_ERROR "libycxx: YCXX_SOURCE_MODULES=ON needs a Ninja generator (CMake builds C++ "
                      "modules only with Ninja); configure with -G Ninja or -DYCXX_SOURCE_MODULES=OFF")
endif()

# ycxx_add_module_units(<target> [LOWERED] [NO_STD] [STD_MODULES <std-modules-target>]
#                       SOURCES <source>...):
# the sources of <target> that are module units (*.cppm; the others are ignored). Call it after
# add_library(<target> <all sources>): the units are taken out of the target's ordinary sources
# now and added back, as module units or lowered, at the end of the directory (once the std
# modules' targets exist). LOWERED: always lowered, whatever YCXX_SOURCE_MODULES says (libycxx.so:
# a lambda in a module unit's function has a weak symbol where a plain translation unit's has none,
# which the shared library would export from its default-visibility namespaces). A unit that imports
# std:
# - NO_STD: stops the configuration. For the targets below the hosted library: the ABI runtime,
#   each image's part (ycxx_nonshared) and the freestanding runtime. A unit under src/abi never
#   imports std, whatever its target.
# - STD_MODULES <t>: is compiled against the std modules of <t> when <t> exists (YCXX_MODULES),
#   which the target then links: for libycxx.a, with the static kind's std modules.
# - otherwise, or when <t> does not exist, or with GCC when the unit's global module fragment
#   includes headers: has its imports of std replaced by std's headers (the textual-std rewrite),
#   still a module unit.
function(ycxx_add_module_units target)
  cmake_parse_arguments(PARSE_ARGV 1 _a "LOWERED;NO_STD" "STD_MODULES" "SOURCES")
  set(_units "")
  foreach(_s IN LISTS _a_SOURCES)
    if(_s MATCHES "\\.cppm$")
      cmake_path(ABSOLUTE_PATH _s BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} NORMALIZE)
      list(APPEND _units ${_s})
    endif()
  endforeach()
  if(NOT _units)
    return()
  endif()
  # Adding or removing a standard import can change the chosen source and its link dependencies.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_units})
  get_target_property(_sources ${target} SOURCES)
  list(FILTER _sources EXCLUDE REGEX "\\.cppm$")
  set_property(TARGET ${target} PROPERTY SOURCES ${_sources})
  foreach(_u IN LISTS _units)
    cmake_path(RELATIVE_PATH _u BASE_DIRECTORY ${PROJECT_SOURCE_DIR}/src OUTPUT_VARIABLE _rel)
    _ycxx_unit_imports_std(${_u} _imports _textual)
    _ycxx_check_initializer_hidden(${_u} ${_rel})
    if(_imports AND (_a_NO_STD OR _rel MATCHES "^abi/"))
      message(FATAL_ERROR "libycxx: src/${_rel} imports std, which the units of ${target} and of "
                          "src/abi cannot (DECISIONS §21)")
    endif()
  endforeach()
  # A deferred call's arguments are evaluated when it runs; EVAL fixes them now.
  list(JOIN _units "]==] [==[" _list)
  if(_ycxx_native_source_modules AND NOT _a_LOWERED)
    set(_modules ON)
  else()
    set(_modules OFF)
  endif()
  cmake_language(EVAL CODE "cmake_language(DEFER CALL _ycxx_add_module_units_now [==[${target}]==] \
${_modules} [==[${_a_STD_MODULES}]==] [==[${_list}]==])")
endfunction()

function(_ycxx_add_module_units_now target modules std_modules)
  set(_units ${ARGN})
  set(_link_std OFF)
  set(_module_files "")
  foreach(_u IN LISTS _units)
    cmake_path(RELATIVE_PATH _u BASE_DIRECTORY ${PROJECT_SOURCE_DIR}/src OUTPUT_VARIABLE _rel)
    _ycxx_unit_imports_std(${_u} _imports _textual)
    if(NOT modules)
      # Lowered: <build>/lowered/<target>/<path under src>.cpp.
      string(REGEX REPLACE "\\.cppm$" ".cpp" _out "${PROJECT_BINARY_DIR}/lowered/${target}/${_rel}")
      _ycxx_rewrite_unit(${_u} ${_out} plain "${_imports}")
      target_sources(${target} PRIVATE ${_out})
      continue()
    endif()
    if(_imports AND (NOT std_modules OR NOT TARGET "${std_modules}"
                     OR (_textual AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")))
      # The unit includes std's headers instead of importing it; still a module unit. GCC 16
      # cannot import std after an #include of a standard header (STATUS, known compiler gaps).
      set(_out ${PROJECT_BINARY_DIR}/textual-std/${target}/${_rel})
      _ycxx_rewrite_unit(${_u} ${_out} textual-std "${_imports}")
      list(APPEND _module_files ${_out})
    else()
      if(_imports)
        set(_link_std ON)
      endif()
      list(APPEND _module_files ${_u})
    endif()
  endforeach()
  if(NOT modules)
    return()
  endif()
  target_sources(${target} PRIVATE FILE_SET ycxx_module_units TYPE CXX_MODULES
                 BASE_DIRS ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/textual-std/${target}
                 FILES ${_module_files})
  # CMake scans the file set's units (only for a target that requires C++20 or later; the
  # target's own -std=c++26 wins); the target's other sources stay unscanned
  # (CMAKE_CXX_SCAN_FOR_MODULES is OFF), so they cannot import anything.
  target_compile_features(${target} PRIVATE cxx_std_23)
  if(_link_std)
    # A static library's private link dependency is its consumers' link dependency: a program
    # that links the target links libycxx-modules.a, whose module initializers the importing
    # units call (GCC).
    target_link_libraries(${target} PRIVATE ${std_modules})
  endif()
endfunction()

# _ycxx_unit_imports_std(<unit> <imports-var> <textual-var>): the widest standard import of the
# unit (std.compat, std or empty), and whether the unit includes a header other than the two
# libycxx headers known to include no standard header (GCC can import std only after those).
function(_ycxx_unit_imports_std unit imports_var textual_var)
  file(STRINGS ${unit} _imports REGEX "^[ \t]*import[ \t]+std(\\.compat)?[ \t]*;")
  file(STRINGS ${unit} _includes REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]")
  list(FILTER _includes EXCLUDE REGEX "<ycxx/config\\.hpp>|<ycxx/core/hidden_symbol\\.hpp>")
  set(_kind "")
  if(_imports)
    set(_kind std)
    list(FILTER _imports INCLUDE REGEX "^[ \t]*import[ \t]+std\\.compat[ \t]*;")
    if(_imports)
      set(_kind std.compat)
    endif()
  endif()
  set(${imports_var} "${_kind}" PARENT_SCOPE)
  if(_includes)
    set(${textual_var} ON PARENT_SCOPE)
  else()
    set(${textual_var} OFF PARENT_SCOPE)
  endif()
endfunction()

# _ycxx_check_initializer_hidden(<unit> <path under src>): the unit hides its module initializer
# (DECISIONS §21), which the compilers give default visibility whatever the source says. Its
# Itanium name is _ZGIW then each component of the module name as <length><name>, joined by W:
# ycxx.hosted.print -> _ZGIW4ycxxW6hostedW5print.
function(_ycxx_check_initializer_hidden unit rel)
  file(STRINGS ${unit} _decl REGEX "^[ \t]*export[ \t]+module[ \t]+[A-Za-z_][A-Za-z0-9_.]*[ \t]*;")
  if(NOT _decl)
    return()  # not an interface unit: no initializer of its own
  endif()
  string(REGEX REPLACE "^[ \t]*export[ \t]+module[ \t]+([A-Za-z0-9_.]*).*" "\\1" _name "${_decl}")
  string(REPLACE "." ";" _parts "${_name}")
  set(_mangled "")
  foreach(_p IN LISTS _parts)
    string(LENGTH "${_p}" _len)
    if(_mangled)
      string(APPEND _mangled "W")
    endif()
    string(APPEND _mangled "${_len}${_p}")
  endforeach()
  set(_mangled "_ZGIW${_mangled}")
  file(STRINGS ${unit} _hide REGEX "__hide_symbol\\(\"${_mangled}\"\\)")
  if(NOT _hide)
    message(FATAL_ERROR "libycxx: src/${rel} must hide its module initializer (DECISIONS §21): "
                        "#include <ycxx/core/hidden_symbol.hpp> in its global module fragment and, "
                        "after `export module ${_name};` and its imports,\n"
                        "  asm((::__ycxx::__detail::__hide_symbol(\"${_mangled}\")));")
  endif()
endfunction()

# Generate <out> from <unit> and the header lists it imports (std.compat also includes std).
# Copy the source file properties after every set_source_files_properties in the directory.
function(_ycxx_rewrite_unit unit out mode imports)
  cmake_path(RELATIVE_PATH unit BASE_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE _rel)
  set(_lower ${PROJECT_SOURCE_DIR}/cmake/ycxx-lower-module-unit.cmake)
  set(_std ${PROJECT_SOURCE_DIR}/modules/std.cppm)
  set(_std_compat ${PROJECT_SOURCE_DIR}/modules/std.compat.cppm)
  set(_depends ${unit} ${_lower})
  if(imports)
    list(APPEND _depends ${_std})
    if(imports STREQUAL "std.compat")
      list(APPEND _depends ${_std_compat})
    endif()
  endif()
  add_custom_command(OUTPUT ${out}
    COMMAND ${CMAKE_COMMAND} -DINPUT=${unit} -DOUTPUT=${out} -DMODE=${mode}
            -DSTD_MODULE=${_std} -DSTD_COMPAT_MODULE=${_std_compat} -P ${_lower}
    DEPENDS ${_depends}
    COMMENT "Rewriting module unit ${_rel} (${mode})"
    VERBATIM)
  foreach(_p COMPILE_OPTIONS COMPILE_DEFINITIONS INCLUDE_DIRECTORIES)
    get_source_file_property(_v ${unit} ${_p})
    if(_v)
      set_property(SOURCE ${out} APPEND PROPERTY ${_p} ${_v})
    endif()
  endforeach()
endfunction()
