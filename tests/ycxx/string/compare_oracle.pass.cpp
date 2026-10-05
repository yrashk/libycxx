// basic_string::compare (every overload), the comparison operators and basic_string_view::compare
// against the definition in [string.view.ops]/[string.view.comparison]: "Let rlen be the smaller of
// size() and str.size(). Determines rlen, the effective length of the strings to compare. The
// function then compares the two strings by calling traits::compare(data(), str.data(), rlen)."
// with the result decided by the sizes when that is 0 (Table 87). traits::lt is "defined
// identically to the built-in operator < for type unsigned char" for char
// ([char.traits.specializations.char]/1) and for type wchar_t for wchar_t
// ([char.traits.specializations.wchar.t]/2, so a negative wchar_t is less than L'a'). [string.compare]:
// compare(pos1, n1, ...) operates on substr(pos1, n1) and throws out_of_range when pos1 > size()
// (or pos2 > str.size()); [string.cmp]: the operators use compare, <=> yields
// weak_ordering via the traits' comparison_category (strong_ordering for the standard traits).
// All strings of length <= 3 (substrings for pairs of total length <= 4) over an alphabet with a null character and high characters, plus
// random long strings with long common prefixes.
#include <string>
#include <string_view>
#include <stdexcept>
#include <compare>
#include <cstddef>
#include "check.hpp"

template <class Ch>
static auto key(Ch c) {
  if constexpr (std::is_same_v<Ch, char>)
    return static_cast<unsigned char>(c);
  else
    return c;
}

template <class Ch>
static int oracle(std::basic_string_view<Ch> a, std::basic_string_view<Ch> b) {
  const std::size_t rlen = a.size() < b.size() ? a.size() : b.size();
  for (std::size_t i = 0; i < rlen; ++i) {
    if (key(a[i]) < key(b[i])) return -1;
    if (key(b[i]) < key(a[i])) return 1;
  }
  return a.size() < b.size() ? -1 : a.size() > b.size() ? 1 : 0;
}

static int sgn(int v) { return (v > 0) - (v < 0); }

template <class Ch>
static std::basic_string_view<Ch> sub(std::basic_string_view<Ch> v, std::size_t pos, std::size_t n) {
  return v.substr(pos, n);
}

template <class Ch>
static void check_pair(const std::basic_string<Ch>& a, const std::basic_string<Ch>& b, bool deep) {
  using S = std::basic_string<Ch>;
  using SV = std::basic_string_view<Ch>;
  const SV av(a), bv(b);
  const int o = oracle(av, bv);
  CHECK(sgn(a.compare(b)) == o && sgn(av.compare(bv)) == o && sgn(a.compare(bv)) == o);
  CHECK(sgn(a.compare(0, S::npos, b)) == o);
  CHECK(sgn(a.compare(0, a.size(), b.data(), b.size())) == o);
  CHECK((a == b) == (o == 0) && (a != b) == (o != 0) && (a < b) == (o < 0) && (a > b) == (o > 0));
  CHECK((a <= b) == (o <= 0) && (a >= b) == (o >= 0) && (a == bv) == (o == 0) && (av < b) == (o < 0));
  CHECK(((a <=> b) < 0) == (o < 0) && ((a <=> b) == 0) == (o == 0) && ((av <=> bv) > 0) == (o > 0));
  static_assert(std::is_same_v<decltype(a <=> b), std::strong_ordering>);
  if (b.find(Ch()) == S::npos) CHECK(sgn(a.compare(b.c_str())) == o && (a == b.c_str()) == (o == 0));
  if (!deep) return;
  // substrings of both sides
  const std::size_t ns[] = {0, 1, 2, S::npos};
  for (std::size_t p1 = 0; p1 <= a.size() + 1; ++p1) {
    for (std::size_t n1 : ns) {
      if (p1 > a.size()) {
        bool threw = false;
        try {
          (void)a.compare(p1, n1, b);
        } catch (const std::out_of_range&) {
          threw = true;
        }
        CHECK(threw);
        continue;
      }
      CHECK(sgn(a.compare(p1, n1, b)) == oracle(sub(av, p1, n1), bv));
      CHECK(sgn(a.compare(p1, n1, bv)) == oracle(sub(av, p1, n1), bv));
      for (std::size_t p2 = 0; p2 <= b.size() + 1; ++p2) {
        for (std::size_t n2 : ns) {
          if (p2 > b.size()) {
            bool threw = false;
            try {
              (void)a.compare(p1, n1, b, p2, n2);
            } catch (const std::out_of_range&) {
              threw = true;
            }
            CHECK(threw);
            continue;
          }
          const int e = oracle(sub(av, p1, n1), sub(bv, p2, n2));
          CHECK(sgn(a.compare(p1, n1, b, p2, n2)) == e);
          CHECK(sgn(a.compare(p1, n1, bv, p2, n2)) == e);
          CHECK(sgn(av.compare(p1, n1, bv, p2, n2)) == e);
          if (n2 != S::npos && p2 + n2 <= b.size()) CHECK(sgn(a.compare(p1, n1, b.data() + p2, n2)) == e);
        }
      }
    }
  }
}

template <class Ch>
static void run(const Ch (&alpha)[4]) {
  using S = std::basic_string<Ch>;
  S all[1 + 4 + 16 + 64];
  std::size_t n = 0;
  all[n++] = S();
  for (std::size_t len = 1; len <= 3; ++len) {
    std::size_t total = 1;
    for (std::size_t i = 0; i < len; ++i) total *= 4;
    for (std::size_t code = 0; code < total; ++code) {
      S s;
      for (std::size_t i = 0, c = code; i < len; ++i, c /= 4) s.push_back(alpha[c % 4]);
      all[n++] = s;
    }
  }
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) check_pair(all[i], all[j], all[i].size() + all[j].size() <= 4);

  // long strings with a common prefix, differing at one random position (or only in length)
  unsigned x = 31337;
  auto rnd = [&](unsigned m) {
    x = x * 1664525u + 1013904223u;
    return (x >> 9) % m;
  };
  for (int iter = 0; iter < 2000; ++iter) {
    S a;
    const std::size_t len = 1 + rnd(200);
    for (std::size_t i = 0; i < len; ++i) a.push_back(alpha[rnd(4)]);
    S b = a;
    switch (rnd(4)) {
      case 0: b[rnd(static_cast<unsigned>(len))] = alpha[rnd(4)]; break;
      case 1: b.resize(rnd(static_cast<unsigned>(len))); break;
      case 2: b.push_back(alpha[rnd(4)]); break;
      default: break;
    }
    const std::basic_string_view<Ch> av(a), bv(b);
    const int o = oracle(av, bv);
    CHECK(sgn(a.compare(b)) == o && sgn(b.compare(a)) == -o && (a < b) == (o < 0) && (a == b) == (o == 0));
    const std::size_t p = rnd(static_cast<unsigned>((a.size() < b.size() ? a.size() : b.size()) + 1));
    CHECK(sgn(a.compare(p, S::npos, b, p)) == oracle(av.substr(p), bv.substr(p)));
  }
}

int main() {
  run<char>({'a', '\0', '\x7f', '\x80'});
  run<char>({'\xff', 'b', '\x01', '\xfe'});
  run<char8_t>({u8'a', u8'\0', static_cast<char8_t>(0x80), static_cast<char8_t>(0xFF)});
  run<char16_t>({u'a', u'\0', static_cast<char16_t>(0x8000), static_cast<char16_t>(0xFFFF)});
  run<char32_t>({U'a', U'\0', static_cast<char32_t>(0x80000000), static_cast<char32_t>(0xFFFFFFFF)});
  run<wchar_t>({L'a', L'\0', static_cast<wchar_t>(-1), static_cast<wchar_t>(0x7FFFFFFF)});
  return 0;
}
