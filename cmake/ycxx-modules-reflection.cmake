# libycxx: stop a GCC project whose target compiles with -freflection while the std module it imports
# was built without it (DECISIONS §16, docs/USAGE.md "Modules"). GCC 16 records the setting in the
# module's CMI: such a target compiles, but std::meta is empty ("'info' is not a member of
# 'std::meta'"), and nothing in the module can say why (an importer's flags are invisible to it).
# A generator expression cannot see the importing target's options here (the self-reference
# evaluates to nothing), so the check runs once, at the end of the top-level directory: a target
# that links ycxx::modules or ycxx::shared_modules (directly or through its link dependencies)
# and gets -freflection from its own options or from a dependency's interface is an error, unless
# the module is compiled with it too:
#   - add_subdirectory: when the modules target's own options (which include the directory's
#     add_compile_options from before add_subdirectory) have it;
#   - installed package: when the target's directory has it (add_compile_options): the importing
#     project compiles the module's interface units with that directory's options.
# CMAKE_CXX_FLAGS reaches the module and the program alike. Per-source options are not looked at.
# Included by CMakeLists.txt and libycxxConfig.cmake when the modules are built without
# reflection (YCXX_MODULES_REFLECTION=OFF) and the compiler is GCC.

function(_ycxx_reflection_targets dir out)
  get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(sub IN LISTS subdirs)
    _ycxx_reflection_targets("${sub}" more)
    list(APPEND targets ${more})
  endforeach()
  set(${out} ${targets} PARENT_SCOPE)
endfunction()

# The target an item of LINK_LIBRARIES names, or nothing (a file, a flag, a generator expression
# other than $<LINK_ONLY:...>).
function(_ycxx_reflection_resolve item out)
  if(item MATCHES "^\\$<LINK_ONLY:([^$<>]+)>$")
    set(item "${CMAKE_MATCH_1}")
  endif()
  set(${out} "" PARENT_SCOPE)
  if(TARGET "${item}")
    get_target_property(aliased "${item}" ALIASED_TARGET)
    if(aliased)
      set(item "${aliased}")
    endif()
    set(${out} "${item}" PARENT_SCOPE)
  endif()
endfunction()

function(_ycxx_check_modules_reflection)
  set(modules ycxx::modules ycxx::shared_modules)
  set(resolved_modules "")
  foreach(m IN LISTS modules)
    _ycxx_reflection_resolve("${m}" r)
    if(NOT r)
      set(r "${m}") # not built: a name no dependency resolves to
    endif()
    list(APPEND resolved_modules ${r})
  endforeach()
  _ycxx_reflection_targets("${CMAKE_SOURCE_DIR}" targets)
  foreach(t IN LISTS targets)
    get_target_property(type ${t} TYPE)
    if(type STREQUAL "INTERFACE_LIBRARY" OR type STREQUAL "UTILITY" OR t IN_LIST resolved_modules)
      continue()
    endif()
    # The link closure: LINK_LIBRARIES, then the dependencies' INTERFACE_LINK_LIBRARIES.
    get_target_property(queue ${t} LINK_LIBRARIES)
    set(seen "")
    set(module "")
    set(flags "")
    while(queue)
      list(POP_FRONT queue item)
      _ycxx_reflection_resolve("${item}" dep)
      if(NOT dep OR dep IN_LIST seen)
        continue()
      endif()
      list(APPEND seen ${dep})
      if(dep IN_LIST resolved_modules)
        set(module ${dep})
        continue()
      endif()
      get_target_property(opts ${dep} INTERFACE_COMPILE_OPTIONS)
      if(opts)
        list(APPEND flags ${opts})
      endif()
      get_target_property(more ${dep} INTERFACE_LINK_LIBRARIES)
      if(more)
        list(APPEND queue ${more})
      endif()
    endwhile()
    if(NOT module)
      continue()
    endif()
    list(FIND resolved_modules ${module} i)
    list(GET modules ${i} module_name)
    get_target_property(own ${t} COMPILE_OPTIONS)
    get_target_property(imported ${module} IMPORTED)
    if(imported)
      # The directory's options reach the module's interface units too.
      get_target_property(dir ${t} SOURCE_DIR)
      get_property(dir_opts DIRECTORY "${dir}" PROPERTY COMPILE_OPTIONS)
      if(own)
        foreach(o IN LISTS dir_opts)
          list(REMOVE_ITEM own "${o}")
        endforeach()
      endif()
      set(module_flags "")
    else()
      get_target_property(module_flags ${module} COMPILE_OPTIONS)
    endif()
    if(own)
      list(APPEND flags ${own})
    endif()
    if("${flags}" MATCHES "-freflection" AND NOT "${module_flags}" MATCHES "-freflection")
      message(FATAL_ERROR
              "libycxx: target ${t} compiles with -freflection, but the std module it imports "
              "(${module_name}) was built without it, so std::meta would be empty: configure libycxx "
              "with -DYCXX_MODULES_REFLECTION=ON (which also gives -freflection to every target that "
              "links the modules), or drop -freflection from ${t} (DECISIONS §16, docs/USAGE.md, "
              "\"Modules\")")
    endif()
  endforeach()
endfunction()

get_property(_ycxx_reflection_check_deferred GLOBAL PROPERTY _YCXX_MODULES_REFLECTION_CHECK)
if(NOT _ycxx_reflection_check_deferred)
  set_property(GLOBAL PROPERTY _YCXX_MODULES_REFLECTION_CHECK TRUE)
  cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL _ycxx_check_modules_reflection)
endif()
unset(_ycxx_reflection_check_deferred)
