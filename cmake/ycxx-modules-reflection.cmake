# libycxx: stop a GCC project whose target compiles with -freflection while the std module it imports
# is built without it (DECISIONS §16, docs/USAGE.md "Modules"). GCC 16 records the setting in the
# module's CMI: such a target compiles, but std::meta is empty ("'info' is not a member of
# 'std::meta'"), and nothing in the module can say why (an importer's flags are invisible to it).
# CMake 4.4 and later build such an importer its own copy of the module's interface units, with
# the importer's options, so the check is for older CMake only (the including files test that).
# A generator expression cannot see the importing target's options here (the self-reference
# evaluates to nothing), so the check runs once, at the end of a directory: a target there or
# below that links ycxx::modules or ycxx::shared_modules (directly or through the usage
# requirements of its dependencies) and whose own options or dependencies' interfaces leave
# -freflection on (the last of -freflection/-fno-reflection decides) is an error, unless the
# module's interface units are compiled with it too:
#   - add_subdirectory (the directory is the top-level one): when the modules target's own options
#     have it, which include an add_compile_options of a directory above libycxx's made before
#     add_subdirectory;
#   - installed package (the directory is find_package's): when the options exported for the
#     interface units have it (IMPORTED_CXX_MODULES_COMPILE_OPTIONS). The importing project
#     compiles the interface units with those and CMAKE_CXX_FLAGS only: not with the directory's
#     add_compile_options (CMake 3.31: std::meta is empty then).
# CMAKE_CXX_FLAGS of the top-level directory reaches the module and the program alike. Not looked
# at: per-source options, the COMPILE_FLAGS property, CMAKE_CXX_FLAGS set in a subdirectory, and
# a dependency linked through a generator expression.
# Included by CMakeLists.txt and libycxxConfig.cmake with _ycxx_reflection_scope set to that
# directory, when the modules are built without reflection, the compiler is GCC and CMake is
# older than 4.4.

function(_ycxx_reflection_targets dir out)
  get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(sub IN LISTS subdirs)
    _ycxx_reflection_targets("${sub}" more)
    list(APPEND targets ${more})
  endforeach()
  set(${out} ${targets} PARENT_SCOPE)
endfunction()

# The target an item of LINK_LIBRARIES or INTERFACE_LINK_LIBRARIES names, or "": a file, a flag,
# a generator expression, and $<LINK_ONLY:...> (no usage requirements: neither the module nor
# compile options come through it).
function(_ycxx_reflection_resolve item out)
  set(${out} "" PARENT_SCOPE)
  if(TARGET "${item}")
    get_target_property(aliased "${item}" ALIASED_TARGET)
    if(aliased)
      set(item "${aliased}")
    endif()
    set(${out} "${item}" PARENT_SCOPE)
  endif()
endfunction()

# Whether the options leave -freflection on: the last of -freflection and -fno-reflection, also
# inside generator expressions.
function(_ycxx_reflection_on options out)
  set(on FALSE)
  string(REGEX MATCHALL "-f(no-)?reflection" flags "${options}")
  foreach(f IN LISTS flags)
    if(f STREQUAL "-freflection")
      set(on TRUE)
    else()
      set(on FALSE)
    endif()
  endforeach()
  set(${out} ${on} PARENT_SCOPE)
endfunction()

function(_ycxx_check_modules_reflection scope)
  set(modules ycxx::modules ycxx::shared_modules)
  set(resolved_modules "")
  foreach(m IN LISTS modules)
    _ycxx_reflection_resolve("${m}" r)
    if(r STREQUAL "")
      set(r "${m}") # not built: a name no dependency resolves to
    endif()
    list(APPEND resolved_modules "${r}")
  endforeach()
  _ycxx_reflection_targets("${scope}" targets)
  foreach(t IN LISTS targets)
    get_target_property(type ${t} TYPE)
    if(type STREQUAL "INTERFACE_LIBRARY" OR type STREQUAL "UTILITY" OR t IN_LIST resolved_modules)
      continue()
    endif()
    # The link closure: LINK_LIBRARIES, then the dependencies' INTERFACE_LINK_LIBRARIES.
    get_target_property(queue ${t} LINK_LIBRARIES)
    if(NOT queue)
      continue()
    endif()
    set(seen "")
    set(module "")
    set(flags "")
    list(LENGTH queue n)
    while(n GREATER 0)
      list(POP_FRONT queue item)
      list(LENGTH queue n)
      _ycxx_reflection_resolve("${item}" dep)
      if(dep STREQUAL "" OR dep IN_LIST seen)
        continue()
      endif()
      list(APPEND seen "${dep}")
      if(dep IN_LIST resolved_modules)
        set(module "${dep}")
        continue()
      endif()
      get_target_property(opts "${dep}" INTERFACE_COMPILE_OPTIONS)
      if(opts)
        list(APPEND flags ${opts})
      endif()
      get_target_property(more "${dep}" INTERFACE_LINK_LIBRARIES)
      if(more)
        list(APPEND queue ${more})
        list(LENGTH queue n)
      endif()
    endwhile()
    if(module STREQUAL "")
      continue()
    endif()
    get_target_property(own ${t} COMPILE_OPTIONS)
    if(own)
      list(APPEND flags ${own})
    endif()
    _ycxx_reflection_on("${flags}" wanted)
    if(NOT wanted)
      continue()
    endif()
    get_target_property(imported "${module}" IMPORTED)
    if(imported)
      get_target_property(module_flags "${module}" IMPORTED_CXX_MODULES_COMPILE_OPTIONS)
    else()
      get_target_property(module_flags "${module}" COMPILE_OPTIONS)
    endif()
    _ycxx_reflection_on("${module_flags}" built)
    if(NOT built)
      list(FIND resolved_modules "${module}" i)
      list(GET modules ${i} module_name)
      message(FATAL_ERROR
              "libycxx: target ${t} compiles with -freflection, but the std module it imports "
              "(${module_name}) is built without it, so std::meta would be empty: configure libycxx "
              "with -DYCXX_MODULES_REFLECTION=ON (which also gives -freflection to every target that "
              "links the modules), drop -freflection from ${t}, or use CMake 4.4 or later (which "
              "builds the module for ${t} with its options; DECISIONS §16, docs/USAGE.md, \"Modules\")")
    endif()
  endforeach()
endfunction()

get_property(_ycxx_reflection_check_deferred DIRECTORY "${_ycxx_reflection_scope}"
             PROPERTY _YCXX_MODULES_REFLECTION_CHECK)
if(NOT _ycxx_reflection_check_deferred)
  set_property(DIRECTORY "${_ycxx_reflection_scope}" PROPERTY _YCXX_MODULES_REFLECTION_CHECK TRUE)
  cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY [[${_ycxx_reflection_scope}]] CALL
                 _ycxx_check_modules_reflection [[${_ycxx_reflection_scope}]])")
endif()
unset(_ycxx_reflection_check_deferred)
