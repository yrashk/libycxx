// libycxx core: an assembler directive that hides one symbol (DECISIONS §2), for definitions the
// compilers give default visibility whatever the source says: the initializers of the modules
// std and std.compat (modules/std.cppm: `asm((__ycxx::__detail::__hide_symbol("_ZGIW3std")));`; the
// compilers emit them with default visibility, -fvisibility=hidden included). `.hidden` on ELF,
// `.private_extern` on Mach-O, whose symbols carry the C prefix '_'.
#pragma once

#include <ycxx/config.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __asm_text {
  char __text[128]{};
  decltype(sizeof 0) length = 0;
  constexpr const char* data() const noexcept { return __text; }
  constexpr decltype(sizeof 0) size() const noexcept { return length; }
};

consteval __asm_text __hide_symbol(const char* __mangled) {
  __asm_text d;
  for (const char* p = __cfg::__darwin ? ".private_extern _" : ".hidden "; *p; ++p)
    d.__text[d.length++] = *p;
  for (; *__mangled; ++__mangled)
    d.__text[d.length++] = *__mangled;
  d.__text[d.length++] = '\n';
  return d;
}

}} // namespace __ycxx::__detail
