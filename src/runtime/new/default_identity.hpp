// The private identity of an ordinary allocation default (DECISIONS §2).
#pragma once

#if __has_include(<abi/allocation_identity.hpp>)
#  include <abi/allocation_identity.hpp>
#else
#  define _YCXX_NEW_ELF_ALIAS 0
#  define _YCXX_NEW_MACHO_SECTION 0
#endif

#if !_YCXX_HAS_EXCEPTIONS && _YCXX_NEW_MACHO_SECTION
#  define _YCXX_NEW_DEFAULT_SECTION [[__gnu__::__section__("__TEXT,__ycxx_new,regular,pure_instructions")]]
#else
#  define _YCXX_NEW_DEFAULT_SECTION
#endif
