// [cstdio.syn]: <cstdio> defines __STDC_VERSION_STDIO_H__ (202311L) and _PRINTF_NAN_LEN_MAX, and
// /1: "The contents and meaning of the header <cstdio> are the same as the C standard library
// header <stdio.h>" (ISO/IEC 9899:2024). C23 additions exercised here:
//   7.23.6.1: the b conversion (unsigned int as binary; the # flag prefixes "0b" to a nonzero
//     value), B where supported (as b with "0B"; optional, so not checked), the wN and wfN
//     length modifiers (an argument of type intN_t / int_fastN_t, for the N the
//     implementation provides; N = 32 is required by <stdint.h> on every target here as
//     int32_t exists, and int_fast32_t always exists), and the H/D/DD modifiers are
//     not checked (decimal floating types);
//   7.23.6.2: scanf's b conversion ("matches an optionally signed binary integer");
//   7.23.1: _PRINTF_NAN_LEN_MAX "expands to an integer constant expression that is the maximum
//     number of characters that can be output by the printf family for a NaN" with the
//     conversions f, F, e, E, g, G, a, A (no n-char-sequence in this check: none is printed for
//     the plain quiet NaN).
// XFAIL: any-darwin  Darwin's printf has no C23 b conversion (snprintf("%b", 5u) writes "b"); <cstdio> is the C library's (STATUS, macOS)
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include "check.hpp"

static_assert(__STDC_VERSION_STDIO_H__ >= 202311L);
static_assert(_PRINTF_NAN_LEN_MAX >= 3);

std::string fmt(const char* f, auto... args) {
  char buf[128];
  int n = std::snprintf(buf, sizeof buf, f, args...);
  CHECK(n >= 0 && n < static_cast<int>(sizeof buf));
  return std::string(buf, static_cast<std::size_t>(n));
}

int main() {
  CHECK(fmt("%b", 5u) == "101");
  CHECK(fmt("%b", 0u) == "0");
  CHECK(fmt("%#b", 5u) == "0b101");
  CHECK(fmt("%#b", 0u) == "0");
  CHECK(fmt("%08b", 5u) == "00000101");
  CHECK(fmt("%.6b", 5u) == "000101");
  CHECK(fmt("%lb", 6ul) == "110");
  CHECK(fmt("%llb", 1ull << 40) == "1" + std::string(40, '0'));
  CHECK(fmt("%hhb", 0x1ffu) == "11111111");

  std::int32_t i32 = -42;
  std::int_fast32_t f32 = 7;
  CHECK(fmt("%w32d", i32) == "-42");
  CHECK(fmt("%w32x", static_cast<std::uint32_t>(255)) == "ff");
  CHECK(fmt("%wf32d", f32) == "7");
  CHECK(fmt("%w64d", static_cast<std::int64_t>(-1) << 40) == "-1099511627776");

  unsigned u = 0;
  CHECK(std::sscanf("1101", "%b", &u) == 1 && u == 13u);
  CHECK(std::sscanf("0b11", "%b", &u) == 1 && u == 3u);
  std::int32_t r = 0;
  CHECK(std::sscanf("-17", "%w32d", &r) == 1 && r == -17);

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const char* convs[] = {"%f", "%F", "%e", "%E", "%g", "%G", "%a", "%A"};
  for (const char* c : convs) {
    CHECK(static_cast<int>(fmt(c, nan).size()) <= _PRINTF_NAN_LEN_MAX);
    CHECK(static_cast<int>(fmt(c, -nan).size()) <= _PRINTF_NAN_LEN_MAX);
  }
  return 0;
}
