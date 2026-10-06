# libycxx: the link options that keep the allocation table in a program (DECISIONS §2, "The
# allocation table"; src/runtime/new/allocation_table.hpp).
#
# Every image that links libycxx holds `__ycxx_allocation_functions`, and the dynamic linker makes
# all of them use the first image's, the program's. A program gets the table from the archive only
# when something it links references a default allocation function; one that replaces every form
# it uses would not, and then a shared library's table would be the process's, bypassing the
# program's replacements. So the program is linked with the table's archive member named as
# undefined (-u __ycxx_allocation_table_anchor, a hidden symbol of that member: the table itself would
# not do, since a libycxx shared library on the link line exports it and satisfies the reference
# first), and, where programs export only what the shared libraries named at link time import
# (ELF), with the table exported (--export-dynamic-symbol), so that a library loaded later binds to
# it.
#
# Both are found by asking the toolchain: the assembler name's prefix from the compiler
# (__USER_LABEL_PREFIX__, read from a compiled object, so cross compilation works), and whether the
# linker accepts --export-dynamic-symbol (check_linker_flag). The result, a list for
# target_link_options, is also written to <build>/ycxx-link-options for tools/ycxx-cxx.

include(CheckLinkerFlag)

function(ycxx_link_options out_var)
  set(dir ${CMAKE_CURRENT_BINARY_DIR}/link_probe)
  file(WRITE ${dir}/prefix.c [=[
#define YCXX_STR2(x) #x
#define YCXX_STR(x) YCXX_STR2(x)
const char ycxx_label_prefix[] = "YCXX_LABEL_PREFIX[" YCXX_STR(__USER_LABEL_PREFIX__) "]";
]=])
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  try_compile(ok SOURCES ${dir}/prefix.c COPY_FILE ${dir}/prefix.a OUTPUT_VARIABLE log)
  unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
  if(NOT ok)
    message(FATAL_ERROR "libycxx: the symbol-prefix probe did not compile:\n${log}")
  endif()
  file(STRINGS ${dir}/prefix.a found REGEX "YCXX_LABEL_PREFIX\\[[^]]*\\]")
  if(NOT found MATCHES "YCXX_LABEL_PREFIX\\[([^]]*)\\]")
    message(FATAL_ERROR "libycxx: the symbol-prefix probe's object holds no prefix string")
  endif()
  set(prefix "${CMAKE_MATCH_1}")

  set(options "LINKER:-u,${prefix}__ycxx_allocation_table_anchor")
  check_linker_flag(CXX "LINKER:--export-dynamic-symbol=__ycxx_allocation_functions" has_export_dynamic_symbol)
  if(has_export_dynamic_symbol)
    list(APPEND options "LINKER:--export-dynamic-symbol=__ycxx_allocation_functions")
  endif()
  set(${out_var} "${options}" PARENT_SCOPE)

  # For tools/ycxx-cxx: the same options as driver arguments, one per line.
  string(REPLACE "LINKER:" "-Wl," lines "${options}")
  list(JOIN lines "\n" lines)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ycxx-link-options "${lines}\n")
  message(STATUS "libycxx: link options for the allocation table: ${options}")
endfunction()
