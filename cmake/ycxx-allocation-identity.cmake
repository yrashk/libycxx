# An ordinary default needs a private identity even when a forced archive anchor
# links its object while a strong replacement provides the ordinary public symbol.
include(CheckCXXSourceCompiles)

function(ycxx_allocation_identity_header output)
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  string(APPEND CMAKE_REQUIRED_FLAGS " ${YCXX_CXX_STANDARD_FLAG} -Werror")
  set(_YCXX_NEW_ELF_ALIAS 0)
  set(_YCXX_NEW_SIZE_CODE "")
  # Compile the actual operator aliases to select the size_t code in their mangling.
  foreach(code m j)
    string(CONFIGURE [=[
#ifndef __ELF__
#error This probe is for ELF function aliases
#endif
using size_t = decltype(sizeof(0));
namespace std { enum class align_val_t : size_t; }
[[gnu::weak]] void* operator new(size_t) { return reinterpret_cast<void*>(1); }
[[gnu::weak]] void* operator new(size_t, std::align_val_t) { return reinterpret_cast<void*>(1); }
extern "C" [[gnu::alias("_Znw@code@"), gnu::visibility("hidden"), gnu::malloc, gnu::alloc_size(1)]]
void* probe_scalar(size_t);
extern "C" [[gnu::alias("_Znw@code@St11align_val_t"), gnu::visibility("hidden"), gnu::malloc, gnu::alloc_size(1)]]
void* probe_aligned(size_t, std::align_val_t);
]=] probe @ONLY)
    check_cxx_source_compiles("${probe}" _ycxx_new_elf_alias_${code})
    if(_ycxx_new_elf_alias_${code})
      set(_YCXX_NEW_ELF_ALIAS 1)
      set(_YCXX_NEW_SIZE_CODE "${code}")
      break()
    endif()
  endforeach()
  file(CONFIGURE OUTPUT "${output}" CONTENT [=[
#pragma once
#define _YCXX_NEW_ELF_ALIAS @_YCXX_NEW_ELF_ALIAS@
#define _YCXX_NEW_SIZE_CODE "@_YCXX_NEW_SIZE_CODE@"
]=] @ONLY)
endfunction()
