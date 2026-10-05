// libycxx core: an assembler directive that hides one symbol (DECISIONS §2), for definitions the
// compilers give default visibility whatever the source says: the initializers of the modules
// std and std.compat (modules/std.cppm: `asm((ycxx::detail::hide_symbol("_ZGIW3std")));`; the
// compilers emit them with default visibility, -fvisibility=hidden included). `.hidden` on ELF,
// `.private_extern` on Mach-O, whose symbols carry the C prefix '_'.
#pragma once

#include <ycxx/config.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct asm_text {
  char text[128]{};
  decltype(sizeof 0) length = 0;
  constexpr const char* data() const noexcept { return text; }
  constexpr decltype(sizeof 0) size() const noexcept { return length; }
};

consteval asm_text hide_symbol(const char* mangled) {
  asm_text d;
  for (const char* p = cfg::darwin ? ".private_extern _" : ".hidden "; *p; ++p)
    d.text[d.length++] = *p;
  for (; *mangled; ++mangled)
    d.text[d.length++] = *mangled;
  d.text[d.length++] = '\n';
  return d;
}

}} // namespace ycxx::detail
