# libycxx's own sources as C++ module units (DECISIONS §20). A source of the library's archives
# may be a module interface unit, src/**/*.cppm, instead of a .cpp; nothing requires one. With
# YCXX_SOURCE_MODULES (the default with a Ninja generator) CMake compiles such a unit as a module
# unit (a private CXX_MODULES file set: its interface is the library's own and is not installed);
# without it, cmake/ycxx-lower-module-unit.cmake lowers the unit to a plain translation unit,
# so the library builds with any generator. Lowering handles the module lines a unit may have
# (`module;`, `export module NAME;`, `import std;`, `import std.compat;`); a unit with other module
# syntax needs YCXX_SOURCE_MODULES=ON.
#
# Whatever a unit defines in namespace std, __ycxx or __cxxabiv1 must stay attached to the global
# module: it is in the global module fragment or in an `extern "C++"` block of the purview
# ([module.unit]/7). An entity attached to the named module is mangled with it (_ZW...), which
# would change libycxx's ABI; tools/check_source_modules.py checks the archive for such symbols.

if(CMAKE_GENERATOR MATCHES "Ninja")
  set(_ycxx_source_modules_default ON)
else()
  set(_ycxx_source_modules_default OFF)
endif()
option(YCXX_SOURCE_MODULES
       "Compile libycxx's own module units (src/**/*.cppm) as C++ modules (Ninja generators); OFF lowers them to plain translation units"
       ${_ycxx_source_modules_default})
if(YCXX_SOURCE_MODULES AND NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(FATAL_ERROR "libycxx: YCXX_SOURCE_MODULES=ON needs a Ninja generator (CMake builds C++ "
                      "modules only with Ninja); configure with -G Ninja or -DYCXX_SOURCE_MODULES=OFF")
endif()

# ycxx_add_module_units(<target> <source>...): the sources of <target> that are module units
# (*.cppm; the others are ignored). Call it after add_library(<target> <all sources>): the units are
# taken out of the target's ordinary sources and added back as module units or lowered.
function(ycxx_add_module_units target)
  set(_units "")
  foreach(_s IN LISTS ARGN)
    if(_s MATCHES "\\.cppm$")
      cmake_path(ABSOLUTE_PATH _s BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} NORMALIZE)
      list(APPEND _units ${_s})
    endif()
  endforeach()
  if(NOT _units)
    return()
  endif()
  get_target_property(_sources ${target} SOURCES)
  set(_kept "")
  foreach(_s IN LISTS _sources)
    if(NOT _s MATCHES "\\.cppm$")
      list(APPEND _kept ${_s})
    endif()
  endforeach()
  set_property(TARGET ${target} PROPERTY SOURCES ${_kept})

  if(YCXX_SOURCE_MODULES)
    target_sources(${target} PRIVATE FILE_SET ycxx_module_units TYPE CXX_MODULES
                   BASE_DIRS ${PROJECT_SOURCE_DIR}/src FILES ${_units})
    # CMake scans the file set's units (only for a target that requires C++20 or later; the
    # target's own -std=c++26 wins); the target's other sources stay unscanned
    # (CMAKE_CXX_SCAN_FOR_MODULES is OFF), so they cannot import anything.
    target_compile_features(${target} PRIVATE cxx_std_23)
    foreach(_u IN LISTS _units)
      file(STRINGS ${_u} _imports REGEX "^[ \t]*import[ \t]+std(\\.compat)?[ \t]*;")
      if(_imports)
        set(_imports_std ON)
      endif()
    endforeach()
    if(_imports_std)
      # `import std;` in the library itself: the std modules' interfaces (defined later in
      # CMakeLists.txt), built from the same headers.
      target_link_libraries(${target} PRIVATE ycxx_modules)
      cmake_language(EVAL CODE "cmake_language(DEFER CALL _ycxx_check_std_modules [==[${target}]==])")
    endif()
    return()
  endif()

  # Lowered: <build>/lowered/<path under src>.cpp, regenerated when the unit changes. Source file
  # properties set on the unit (COMPILE_OPTIONS, COMPILE_DEFINITIONS) are copied to the lowered
  # file at the end of the directory, so they may be set after this call.
  foreach(_u IN LISTS _units)
    cmake_path(RELATIVE_PATH _u BASE_DIRECTORY ${PROJECT_SOURCE_DIR}/src OUTPUT_VARIABLE _rel)
    set(_out ${PROJECT_BINARY_DIR}/lowered/${_rel})
    string(REGEX REPLACE "\\.cppm$" ".cpp" _out "${_out}")
    add_custom_command(OUTPUT ${_out}
      COMMAND ${CMAKE_COMMAND} -DINPUT=${_u} -DOUTPUT=${_out}
              -DSTD_MODULE=${PROJECT_SOURCE_DIR}/modules/std.cppm
              -DSTD_COMPAT_MODULE=${PROJECT_SOURCE_DIR}/modules/std.compat.cppm
              -P ${PROJECT_SOURCE_DIR}/cmake/ycxx-lower-module-unit.cmake
      DEPENDS ${_u} ${PROJECT_SOURCE_DIR}/cmake/ycxx-lower-module-unit.cmake
      COMMENT "Lowering module unit src/${_rel}"
      VERBATIM)
    target_sources(${target} PRIVATE ${_out})
    # A deferred call's arguments are evaluated when it runs; EVAL fixes them now.
    cmake_language(EVAL CODE
      "cmake_language(DEFER CALL _ycxx_copy_unit_properties [==[${_u}]==] [==[${_out}]==])")
  endforeach()
endfunction()

function(_ycxx_copy_unit_properties unit lowered)
  foreach(_p COMPILE_OPTIONS COMPILE_DEFINITIONS INCLUDE_DIRECTORIES)
    get_source_file_property(_v ${unit} ${_p})
    if(_v)
      set_property(SOURCE ${lowered} APPEND PROPERTY ${_p} ${_v})
    endif()
  endforeach()
endfunction()

function(_ycxx_check_std_modules target)
  if(NOT TARGET ycxx_modules)
    message(FATAL_ERROR "libycxx: a module unit of ${target} imports std, which needs YCXX_MODULES=ON")
  endif()
endfunction()
