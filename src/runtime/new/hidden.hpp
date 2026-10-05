// libycxx runtime: hiding the default replaceable allocation functions (DECISIONS §2).
//
// The archives are built with -fvisibility=hidden, but the compilers declare the replaceable
// allocation functions themselves with default visibility, and a visibility attribute on a
// definition conflicts with that (GCC ignores it with a warning, Clang rejects it). Each file
// defining a default therefore hides its symbol with an assembler directive. A program's own
// replacement is unaffected: its definition is linked instead of the archive member holding the
// directive ([replacement.functions]).
#pragma once

#include <cstddef>
#include <type_traits>
#include <ycxx/config.hpp>

namespace ycxx::detail {

struct asm_directive {
  char text[96]{};
  std::size_t length = 0;
  constexpr const char* data() const noexcept { return text; }
  constexpr std::size_t size() const noexcept { return length; }
};

// `mangled`: the function's Itanium name with '#' for std::size_t's code ('m' or 'j').
consteval asm_directive hide_allocation_function(const char* mangled) {
  asm_directive d;
  // Mach-O symbols carry the C prefix '_'.
  for (const char* p = cfg::darwin ? ".private_extern _" : ".hidden "; *p; ++p)
    d.text[d.length++] = *p;
  for (; *mangled; ++mangled)
    d.text[d.length++] = *mangled != '#' ? *mangled : std::is_same_v<std::size_t, unsigned long> ? 'm' : 'j';
  d.text[d.length++] = '\n';
  return d;
}

} // namespace ycxx::detail
