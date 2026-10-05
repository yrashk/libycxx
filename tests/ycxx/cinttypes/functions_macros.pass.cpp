// [cinttypes.syn]: <cinttypes> includes <cstdint>; declares imaxdiv_t, the constexpr functions
// imaxabs and imaxdiv, strtoimax, strtoumax, wcstoimax, wcstoumax; and the PRI/SCN macros of
// ISO C 7.8.1, including the C23 binary conversions PRIbN, PRIBN, SCNbN (and their LEAST,
// FAST, MAX and PTR forms). /1: "The contents and meaning of the header <cinttypes> are the
// same as the C standard library header <inttypes.h>" (constexpr: cinttypes/constexpr_functions).
// ISO C 7.8.2.1: imaxabs computes the absolute value; 7.8.2.2: imaxdiv computes quot and rem
// as for the / and % operators; 7.8.2.3-4: strtoimax etc. are strtoll / wcstoll for intmax_t.
// [cinttypes.syn]/2: the PRIbN and SCNbN macros are defined with their typedefs, the PRIBN ones
// only if fprintf supports the B conversion. What the C23 conversions then do (printf/scanf %b,
// the 0b prefix of strtoimax) is the C library's: one that predates C23 (Darwin's) gets a note
// and those run-time checks are skipped; the macros themselves are checked everywhere.
#include <cinttypes>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <cwchar>
#include <type_traits>
#include "check.hpp"

// <cstdint> comes with <cinttypes>
static_assert(sizeof(std::int64_t) == 8 && sizeof(std::uint32_t) == 4);
static_assert(std::is_same_v<decltype(std::imaxabs(std::intmax_t{})), std::intmax_t>);
static_assert(std::is_same_v<decltype(std::imaxdiv(std::intmax_t{}, std::intmax_t{})), std::imaxdiv_t>);
static_assert(std::is_same_v<decltype(std::imaxdiv_t{}.quot), std::intmax_t>);
static_assert(std::is_same_v<decltype(std::imaxdiv_t{}.rem), std::intmax_t>);
static_assert(std::is_same_v<decltype(std::strtoimax("", nullptr, 10)), std::intmax_t>);
static_assert(std::is_same_v<decltype(std::strtoumax("", nullptr, 10)), std::uintmax_t>);
static_assert(std::is_same_v<decltype(std::wcstoimax(L"", nullptr, 10)), std::intmax_t>);
static_assert(std::is_same_v<decltype(std::wcstoumax(L"", nullptr, 10)), std::uintmax_t>);

template <class... A>
static std::string fmt(const char* f, A... a) {
  char buf[200];
  std::snprintf(buf, sizeof buf, f, a...);
  return buf;
}

static bool ends_with_b(const char* m) {
  return std::strlen(m) >= 1 && m[std::strlen(m) - 1] == 'b';
}

int main() {
  // the binary conversion macros exist (they are used unconditionally) and name 'b'
  for (const char* m : {PRIb8, PRIb16, PRIb32, PRIb64, PRIbLEAST8, PRIbLEAST16, PRIbLEAST32, PRIbLEAST64, PRIbFAST8,
                        PRIbFAST16, PRIbFAST32, PRIbFAST64, PRIbMAX, PRIbPTR, SCNb8, SCNb16, SCNb32, SCNb64, SCNbLEAST8,
                        SCNbLEAST16, SCNbLEAST32, SCNbLEAST64, SCNbFAST8, SCNbFAST16, SCNbFAST32, SCNbFAST64, SCNbMAX,
                        SCNbPTR})
    CHECK(ends_with_b(m));
  const bool c23_printf = fmt("%b", 5u) == "101";
  unsigned probe = 0;
  const bool c23_scanf = std::sscanf("101", "%b", &probe) == 1 && probe == 5;
  char* probe_end = nullptr;
  const bool c23_strto = std::strtoimax("0b11", &probe_end, 0) == 3;
  if (!c23_printf || !c23_scanf || !c23_strto)
    dprintf(2, "note: the C library's printf %%b: %d, scanf %%b: %d, strtoimax 0b: %d (C23); checks skipped\n",
            c23_printf, c23_scanf, c23_strto);
  // PRI macros: d i o u x X and b B
  CHECK(fmt("%" PRId8 " %" PRIi16 " %" PRId32 " %" PRId64, std::int8_t{-8}, std::int16_t{-16}, std::int32_t{-32},
            std::int64_t{-64}) == "-8 -16 -32 -64");
  CHECK(fmt("%" PRIu64 " %" PRIx64 " %" PRIX64 " %" PRIo64, std::uint64_t{18446744073709551615u}, std::uint64_t{255},
            std::uint64_t{255}, std::uint64_t{8}) == "18446744073709551615 ff FF 10");
  CHECK(fmt("%" PRIdMAX " %" PRIuMAX " %" PRIxPTR, std::intmax_t{-1}, std::uintmax_t{1}, std::uintptr_t{0xab}) == "-1 1 ab");
  CHECK(fmt("%" PRIdLEAST32 " %" PRIuFAST16 " %" PRIxLEAST8, std::int_least32_t{-5}, std::uint_fast16_t{6},
            std::uint_least8_t{15}) == "-5 6 f");
  if (c23_printf) {
    CHECK(fmt("%" PRIb8 " %" PRIb16 " %" PRIb32 " %" PRIb64, std::uint8_t{5}, std::uint16_t{6}, std::uint32_t{0},
              std::uint64_t{9}) == "101 110 0 1001");
    CHECK(fmt("%#" PRIb32, std::uint32_t{5}) == "0b101");
    CHECK(fmt("%" PRIbMAX " %" PRIbPTR " %" PRIbLEAST16 " %" PRIbFAST64, std::uintmax_t{2}, std::uintptr_t{3},
              std::uint_least16_t{4}, std::uint_fast64_t{7}) == "10 11 100 111");
  }
#if defined(PRIB16) && defined(PRIB32)
  CHECK(fmt("%" PRIB16 " %#" PRIB32, std::uint16_t{6}, std::uint32_t{5}) == "110 0B101");
#endif
  // SCN macros
  std::int32_t a = 0;
  std::uint64_t b = 0;
  std::intmax_t c = 0;
  std::uint16_t d = 0;
  CHECK(std::sscanf("-123 456 789 ff", "%" SCNd32 " %" SCNu64 " %" SCNdMAX " %" SCNx16, &a, &b, &c, &d) == 4);
  CHECK(a == -123 && b == 456 && c == 789 && d == 255);
  std::uint32_t e = 0;
  std::int8_t f = 0;
  if (c23_scanf) {
    CHECK(std::sscanf("1011 -7", "%" SCNb32 " %" SCNi8, &e, &f) == 2);
    CHECK(e == 11 && f == -7);
  }
  // string conversions
  char* end = nullptr;
  CHECK(std::strtoimax("  -9223372036854775807xyz", &end, 10) == -INTMAX_MAX && std::strcmp(end, "xyz") == 0);
  CHECK(std::strtoumax("0x1F", &end, 16) == 31 && *end == 0);
  if (c23_strto) {
    CHECK(std::strtoimax("0b101", &end, 0) == 5 && *end == 0);  // C23: 0b prefix with base 0 and 2
    CHECK(std::strtoimax("0B11", &end, 2) == 3 && *end == 0);
  }
  errno = 0;
  CHECK(std::strtoimax("99999999999999999999999", &end, 10) == INTMAX_MAX && errno == ERANGE);
  errno = 0;
  CHECK(std::strtoumax("-1", &end, 10) == UINTMAX_MAX && errno == 0);  // negated in the return type
  wchar_t* wend = nullptr;
  CHECK(std::wcstoimax(L"-77 rest", &wend, 10) == -77 && std::wcscmp(wend, L" rest") == 0);
  CHECK(std::wcstoumax(L"777", &wend, 8) == 511 && *wend == 0);
  if (c23_strto) CHECK(std::wcstoimax(L"0b1111", &wend, 0) == 15);
  // the runtime functions agree with the constexpr evaluation
  volatile std::intmax_t n = -17, m = 5;
  std::imaxdiv_t r = std::imaxdiv(n, m);
  CHECK(r.quot == -3 && r.rem == -2 && std::imaxabs(n) == 17);
}
