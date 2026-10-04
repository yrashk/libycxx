// [char.traits.specializations.char]/2: "The two-argument member assign is defined
// identically to the built-in operator =. The two-argument members eq and lt are defined
// identically to the built-in operators == and < for type unsigned char."
// char8_t, char16_t, char32_t, wchar_t: "The two-argument members assign, eq, and lt are
// defined identically to the built-in operators =, ==, and <, respectively."
// [tab:char.traits.req]: compare(p,q,n) is negative if, for the first j where !eq,
// lt(p[j],q[j]); so char compares as unsigned char.
#include <string_view>
#include "check.hpp"

using CT = std::char_traits<char>;
static_assert(CT::lt('\x01', '\x80'));   // 1 < 128 as unsigned char
static_assert(!CT::lt('\x80', '\x01'));
static_assert(CT::lt('\x7F', '\xFF') && !CT::lt('\xFF', '\x7F'));
static_assert(!CT::lt('\x00', '\x00') && CT::lt('\x00', '\xFF'));
static_assert(CT::eq('\xFF', static_cast<char>(-1)) && !CT::eq('a', 'A'));
static_assert(CT::compare("\x80", "\x01", 1) > 0);
static_assert(CT::compare("a\xFF", "a\x01", 2) > 0);
static_assert(CT::compare("\x01", "\xFE", 1) < 0);

// wchar_t: built-in < on wchar_t itself
using WT = std::char_traits<wchar_t>;
static_assert(WT::lt(static_cast<wchar_t>(-1), static_cast<wchar_t>(0)) ==
              (static_cast<wchar_t>(-1) < static_cast<wchar_t>(0)));
static_assert(WT::lt(L'a', L'b') && !WT::lt(L'b', L'a') && WT::eq(L'z', L'z'));

template <class C>
constexpr bool builtin(C a, C b) {
  using T = std::char_traits<C>;
  if (T::eq(a, b) != (a == b) || T::lt(a, b) != (a < b) || T::lt(b, a) != (b < a)) return false;
  C r = a;
  T::assign(r, b);
  return r == b;
}
static_assert(builtin<char8_t>(u8'a', static_cast<char8_t>(0xF0)));
static_assert(builtin<char16_t>(u'\x7F', static_cast<char16_t>(0xFFFF)));
static_assert(builtin<char32_t>(U'\U0010FFFF', U'\x80'));
static_assert(builtin<wchar_t>(L'\x80', L'\x7F'));

constexpr bool assign_char() {
  char c = 'x';
  CT::assign(c, 'y');
  return c == 'y';
}
static_assert(assign_char());

int main() {
  CHECK(assign_char());
  // runtime evaluation agrees with constant evaluation
  volatile char vhi = '\x90', vlo = '\x10';
  const char hi = vhi, lo = vlo;  // values unknown to the optimiser
  CHECK(CT::lt(lo, hi) && !CT::lt(hi, lo));
  char a[] = {'\xC0', 0}, b[] = {'\x40', 0};
  CHECK(CT::compare(a, b, 1) > 0 && CT::compare(b, a, 1) < 0);
  return 0;
}
