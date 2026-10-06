// libycxx runtime: hiding the default replaceable allocation functions (DECISIONS §2).
//
// The runtime's definitions are hidden (DECISIONS §2), but the compilers declare the replaceable
// allocation functions themselves with default visibility, and a visibility attribute on a
// definition conflicts with that (GCC ignores it with a warning, Clang rejects it). Each file
// defining a default therefore hides its symbol with an assembler directive. A program's own
// replacement is unaffected: its definition is linked instead of the archive member holding the
// directive ([replacement.functions]).
#pragma once

#include <cstddef>
#include <type_traits>
#include <ycxx/config.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __asm_directive {
  char __text[96]{};
  std::size_t length = 0;
  constexpr const char* data() const noexcept { return __text; }
  constexpr std::size_t size() const noexcept { return length; }
};

// `__mangled`: the function's Itanium name with '#' for std::size_t's code ('m' or 'j').
consteval __asm_directive __hide_allocation_function(const char* __mangled) {
  __asm_directive d;
  // Mach-O symbols carry the C prefix '_'.
  for (const char* p = __cfg::__darwin ? ".private_extern _" : ".hidden "; *p; ++p)
    d.__text[d.length++] = *p;
  for (; *__mangled; ++__mangled)
    d.__text[d.length++] = *__mangled != '#' ? *__mangled : std::is_same_v<std::size_t, unsigned long> ? 'm' : 'j';
  d.__text[d.length++] = '\n';
  return d;
}

}} // namespace __ycxx::__detail
