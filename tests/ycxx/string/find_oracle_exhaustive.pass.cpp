// Every search member of basic_string and basic_string_view compared against the definitions
// of [string.view.find] for all haystacks of length <= 4 and needles of length <= 3 over an
// alphabet with a null character and a character whose unsigned value is the largest, for
// every start position up to size() + 2 and npos:
//   find(str, pos): "the lowest position xpos such that ... pos <= xpos, xpos + str.size() <=
//     size(), traits::eq(data_[xpos + I], str[I]) for all elements I of str" (/3-/4);
//   rfind: the highest such xpos with xpos <= pos;
//   find_first_of / find_last_of: the lowest (highest) xpos with pos <= xpos (xpos <= pos),
//     xpos < size() and traits::eq(data_[xpos], str[I]) for some I;
//   find_first_not_of / find_last_not_of: the same with "for no element I".
// npos when there is no such position. [string.find]/1-/4: the basic_string members, for
// basic_string, const charT* with and without a count, a single charT and a string-view-like
// argument, are equivalent to those of basic_string_view. Also random haystacks up to 300
// characters long. Checked for char, char8_t, char16_t,
// char32_t and wchar_t.
#include <string>
#include <string_view>
#include <cstddef>
#include "check.hpp"

template <class Ch>
struct Oracle {
  using SV = std::basic_string_view<Ch>;
  static constexpr std::size_t npos = SV::npos;
  static bool eq(Ch a, Ch b) { return std::char_traits<Ch>::eq(a, b); }
  static bool in(Ch c, SV set) {
    for (Ch x : set)
      if (eq(c, x)) return true;
    return false;
  }
  static std::size_t find(SV h, SV s, std::size_t pos) {
    for (std::size_t x = pos; x <= h.size() && x + s.size() <= h.size(); ++x) {
      bool ok = true;
      for (std::size_t i = 0; i < s.size(); ++i) ok = ok && eq(h[x + i], s[i]);
      if (ok) return x;
    }
    return npos;
  }
  static std::size_t rfind(SV h, SV s, std::size_t pos) {
    if (s.size() > h.size()) return npos;
    std::size_t x = pos < h.size() - s.size() ? pos : h.size() - s.size();
    for (;; --x) {
      bool ok = true;
      for (std::size_t i = 0; i < s.size(); ++i) ok = ok && eq(h[x + i], s[i]);
      if (ok) return x;
      if (x == 0) return npos;
    }
  }
  static std::size_t first_of(SV h, SV s, std::size_t pos, bool want) {
    for (std::size_t x = pos; x < h.size(); ++x)
      if (in(h[x], s) == want) return x;
    return npos;
  }
  static std::size_t last_of(SV h, SV s, std::size_t pos, bool want) {
    if (h.empty()) return npos;
    for (std::size_t x = pos < h.size() ? pos : h.size() - 1;; --x) {
      if (in(h[x], s) == want) return x;
      if (x == 0) return npos;
    }
  }
};

template <class Ch>
static void all_strings(const Ch* alpha, std::size_t maxlen, std::basic_string<Ch>* out, std::size_t& n) {
  n = 0;
  out[n++] = {};
  for (std::size_t len = 1; len <= maxlen; ++len) {
    std::size_t total = 1;
    for (std::size_t i = 0; i < len; ++i) total *= 4;
    for (std::size_t code = 0; code < total; ++code) {
      std::basic_string<Ch> s;
      for (std::size_t i = 0, c = code; i < len; ++i, c /= 4) s.push_back(alpha[c % 4]);
      out[n++] = s;
    }
  }
}

template <class Ch>
static void run(const Ch (&alpha)[4]) {
  using S = std::basic_string<Ch>;
  using SV = std::basic_string_view<Ch>;
  using O = Oracle<Ch>;
  static S hay[1 + 4 + 16 + 64 + 256], ndl[1 + 4 + 16 + 64];
  std::size_t nh, nn;
  all_strings(alpha, 4, hay, nh);
  all_strings(alpha, 3, ndl, nn);
  for (std::size_t hi = 0; hi < nh; ++hi) {
    const S& h = hay[hi];
    const SV hv(h);
    for (std::size_t ni = 0; ni < nn; ++ni) {
      const S& s = ndl[ni];
      const SV sv(s);
      for (std::size_t pos = 0; pos <= h.size() + 3; ++pos) {
        const std::size_t p = pos == h.size() + 3 ? S::npos : pos;
        const std::size_t f = O::find(hv, sv, p), rf = O::rfind(hv, sv, p);
        const std::size_t ffo = O::first_of(hv, sv, p, true), flo = O::last_of(hv, sv, p, true);
        const std::size_t ffn = O::first_of(hv, sv, p, false), fln = O::last_of(hv, sv, p, false);
        CHECK(hv.find(sv, p) == f && h.find(s, p) == f && h.find(s.data(), p, s.size()) == f);
        CHECK(h.find(sv, p) == f);
        CHECK(hv.rfind(sv, p) == rf && h.rfind(s, p) == rf && h.rfind(s.data(), p, s.size()) == rf);
        CHECK(h.rfind(sv, p) == rf);
        CHECK(hv.find_first_of(sv, p) == ffo && h.find_first_of(s, p) == ffo &&
              h.find_first_of(s.data(), p, s.size()) == ffo);
        CHECK(hv.find_last_of(sv, p) == flo && h.find_last_of(s, p) == flo &&
              h.find_last_of(s.data(), p, s.size()) == flo);
        CHECK(hv.find_first_not_of(sv, p) == ffn && h.find_first_not_of(s, p) == ffn &&
              h.find_first_not_of(s.data(), p, s.size()) == ffn);
        CHECK(hv.find_last_not_of(sv, p) == fln && h.find_last_not_of(s, p) == fln &&
              h.find_last_not_of(s.data(), p, s.size()) == fln);
        if (s.find(Ch()) == S::npos) {  // the null-terminated overloads see the whole needle
          CHECK(h.find(s.c_str(), p) == f && h.rfind(s.c_str(), p) == rf);
          CHECK(h.find_first_of(s.c_str(), p) == ffo && h.find_last_of(s.c_str(), p) == flo);
          CHECK(h.find_first_not_of(s.c_str(), p) == ffn && h.find_last_not_of(s.c_str(), p) == fln);
        }
        if (s.size() == 1) {
          const Ch c = s[0];
          CHECK(h.find(c, p) == f && hv.find(c, p) == f && h.rfind(c, p) == rf && hv.rfind(c, p) == rf);
          CHECK(h.find_first_of(c, p) == ffo && h.find_last_of(c, p) == flo);
          CHECK(h.find_first_not_of(c, p) == ffn && h.find_last_not_of(c, p) == fln);
        }
      }
    }
  }
  // long haystacks (a vectorised search only starts beyond some length): random strings over
  // the same alphabet (skewed so that long runs and near-matches occur), needles that are
  // substrings of the haystack, possibly with one character changed
  unsigned x = 2024;
  auto rnd = [&](unsigned m) {
    x = x * 1103515245u + 12345u;
    return (x >> 8) % m;
  };
  for (int iter = 0; iter < 3000; ++iter) {
    S h;
    const std::size_t len = rnd(300);
    for (std::size_t i = 0; i < len; ++i) h.push_back(alpha[rnd(10) < 7 ? 0 : 1 + rnd(3)]);
    S s;
    const std::size_t nl = rnd(24);
    if (len > nl && rnd(3) != 0) {
      s = h.substr(rnd(static_cast<unsigned>(len - nl)), nl);
      if (nl && rnd(2)) s[rnd(static_cast<unsigned>(nl))] = alpha[rnd(4)];
    } else {
      for (std::size_t i = 0; i < nl % 5; ++i) s.push_back(alpha[rnd(4)]);
    }
    const SV hv(h), sv(s);
    for (int k = 0; k < 4; ++k) {
      const std::size_t p = k == 3 ? S::npos : rnd(static_cast<unsigned>(len + 5));
      CHECK(h.find(s, p) == O::find(hv, sv, p));
      CHECK(h.rfind(s, p) == O::rfind(hv, sv, p));
      CHECK(h.find_first_of(s, p) == O::first_of(hv, sv, p, true));
      CHECK(h.find_last_of(s, p) == O::last_of(hv, sv, p, true));
      CHECK(h.find_first_not_of(s, p) == O::first_of(hv, sv, p, false));
      CHECK(h.find_last_not_of(s, p) == O::last_of(hv, sv, p, false));
      const Ch c = alpha[rnd(4)];
      const SV cv(&c, 1);
      CHECK(h.find(c, p) == O::find(hv, cv, p) && h.rfind(c, p) == O::rfind(hv, cv, p));
      CHECK(h.find_first_not_of(c, p) == O::first_of(hv, cv, p, false));
      CHECK(h.find_last_not_of(c, p) == O::last_of(hv, cv, p, false));
    }
  }

  // default positions
  const S h = S(1, alpha[0]) + alpha[1] + alpha[0];
  CHECK(h.find(S(1, alpha[0])) == 0 && h.rfind(S(1, alpha[0])) == 2 && h.rfind(S()) == 3 && h.find(S()) == 0);
  CHECK(h.find_last_of(alpha[1]) == 1 && h.find_last_not_of(alpha[0]) == 1 && h.find_first_not_of(alpha[0]) == 1);
}

int main() {
  run<char>({'a', 'b', '\0', '\xff'});
  run<char8_t>({u8'a', u8'b', u8'\0', static_cast<char8_t>(0xFF)});
  run<char16_t>({u'a', u'\xff', u'\0', static_cast<char16_t>(0xFFFF)});
  run<char32_t>({U'a', U'\x10FFFF', U'\0', static_cast<char32_t>(0xFFFFFFFF)});
  run<wchar_t>({L'a', static_cast<wchar_t>(-1), L'\0', static_cast<wchar_t>(0x100)});
  return 0;
}
