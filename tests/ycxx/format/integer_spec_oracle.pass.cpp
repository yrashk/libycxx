// Every combination of fill/align, sign, #, 0, width and integer presentation type for the
// minimum, maximum and a few other values of every standard integer type (and of char and
// bool with an integer presentation type), compared with an oracle written from
// [format.string.std]:
//   Table 107: b/B/o/x/X/d/none are to_chars with base 2/2/8/16/16/10/10 (uppercase digits for
//     X); base prefixes 0b, 0B, 0x, 0X, and for o "0 if the corresponding argument value is
//     nonzero"; /7: # inserts the prefix "after the sign character (possibly space) if there
//     is one, or before the output of to_chars otherwise";
//   Table 105: + and space for non-negative values, - only for negative ones;
//   /8: 0 inserts zeros "following the sign or base prefix indicators (if any)", n times where
//     "n is 0 if the align option is present and is the padding width otherwise";
//   Table 104: > is the default for integers, ^ puts floor(n/2) fill characters before and
//     ceil(n/2) after; /3: the fill defaults to a space and may be any single scalar value;
//   Table 108: charT with an integer type is "converted to the unsigned version of the
//     underlying type"; Table 109: bool as static_cast<unsigned char>(value).
// Also checks formatted_size and format_to_n (truncated to half the size) for each result,
// and the wide-character forms for a subset.
#include <format>
#include <string>
#include <limits>
#include <type_traits>
#include <cstddef>
#include "check.hpp"

using U = unsigned long long;

struct Spec {
  const char* fill_align;  // "" or fill + align
  char sign;               // 0, '+', '-', ' '
  bool hash, zero;
  int width;  // 0: none
  char type;  // 0: none
};

static std::string digits(U mag, unsigned base, bool upper) {
  if (mag == 0) return "0";
  std::string r;
  const char* d = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  while (mag) {
    r.insert(r.begin(), d[mag % base]);
    mag /= base;
  }
  return r;
}

// fill is UTF-8 (one scalar value, possibly several code units), width counted in fill units
static std::string oracle(bool neg, U mag, const Spec& s) {
  unsigned base = 10;
  std::string prefix;
  switch (s.type) {
    case 'b': base = 2; prefix = "0b"; break;
    case 'B': base = 2; prefix = "0B"; break;
    case 'o': base = 8; prefix = mag ? "0" : ""; break;
    case 'x': base = 16; prefix = "0x"; break;
    case 'X': base = 16; prefix = "0X"; break;
    default: break;
  }
  if (!s.hash) prefix.clear();
  std::string sign = neg ? "-" : s.sign == '+' ? "+" : s.sign == ' ' ? " " : "";
  std::string body = digits(mag, base, s.type == 'X');
  std::string fill = " ";
  char align = 0;
  std::string fa = s.fill_align;
  if (!fa.empty()) {
    align = fa.back();
    if (fa.size() > 1) fill = fa.substr(0, fa.size() - 1);
  }
  const std::size_t len = sign.size() + prefix.size() + body.size();
  const std::size_t pad = static_cast<std::size_t>(s.width) > len ? static_cast<std::size_t>(s.width) - len : 0;
  if (s.zero && !align) return sign + prefix + std::string(pad, '0') + body;
  std::size_t before = pad, after = 0;
  if (align == '<') before = 0, after = pad;
  else if (align == '^') before = pad / 2, after = pad - pad / 2;
  std::string r;
  for (std::size_t i = 0; i < before; ++i) r += fill;
  r += sign + prefix + body;
  for (std::size_t i = 0; i < after; ++i) r += fill;
  return r;
}

static std::string spec_string(const Spec& s) {
  std::string r = "{:";
  r += s.fill_align;
  if (s.sign) r += s.sign;
  if (s.hash) r += '#';
  if (s.zero) r += '0';
  if (s.width) r += std::to_string(s.width);
  if (s.type) r += s.type;
  return r + "}";
}

static long long checked = 0;

template <class T>
static void check_value(T v) {
  bool neg = false;
  U mag;
  if constexpr (std::is_same_v<T, bool>) {
    mag = static_cast<unsigned char>(v);
  } else if constexpr (std::is_same_v<T, char>) {
    mag = static_cast<unsigned char>(v);
  } else if constexpr (std::is_signed_v<T>) {
    neg = v < 0;
    mag = neg ? U(0) - static_cast<U>(v) : static_cast<U>(v);
  } else {
    mag = static_cast<U>(v);
  }
  constexpr bool needs_type = std::is_same_v<T, bool> || std::is_same_v<T, char>;
  const char* aligns[] = {"", "<", ">", "^", "*<", "*>", "*^", "\xc3\xa9^", "0>"};
  const char signs[] = {0, '+', '-', ' '};
  const char types[] = {0, 'b', 'B', 'd', 'o', 'x', 'X'};
  const int widths[] = {0, 1, 7, 70};
  for (const char* fa : aligns)
    for (char sg : signs)
      for (int h = 0; h < 2; ++h)
        for (int z = 0; z < 2; ++z)
          for (int w : widths)
            for (char ty : types) {
              if (needs_type && ty == 0) continue;
              const Spec s{fa, sg, h != 0, z != 0, w, ty};
              const std::string fmt = spec_string(s);
              const std::string exp = oracle(neg, mag, s);
              const std::string got = std::vformat(fmt, std::make_format_args(v));
              if (got != exp) dprintf(2, "format(\"%s\") gave \"%s\", expected \"%s\"\n", fmt.c_str(), got.c_str(), exp.c_str());
              CHECK(got == exp);
              CHECK(std::formatted_size(std::runtime_format(fmt), v) == exp.size());
              char buf[160];
              const auto half = static_cast<std::ptrdiff_t>(exp.size() / 2);
              auto r = std::format_to_n(buf, half, std::runtime_format(fmt), v);
              CHECK(r.size == static_cast<std::ptrdiff_t>(exp.size()) && r.out == buf + half);
              CHECK(std::string(buf, r.out) == exp.substr(0, exp.size() / 2));
              ++checked;
            }
}

template <class T>
static void check_type() {
  using L = std::numeric_limits<T>;
  check_value<T>(L::min());
  check_value<T>(L::max());
  check_value<T>(T(0));
  check_value<T>(T(1));
  if constexpr (!std::is_same_v<T, bool>) {
    check_value<T>(static_cast<T>(L::min() + 1));
    check_value<T>(static_cast<T>(L::max() - 1));
    check_value<T>(static_cast<T>(10));
    if constexpr (std::is_signed_v<T>) check_value<T>(T(-1));
  }
}

// wide: same oracle, ASCII-only fills, widened
template <class T>
static void check_wide(T v) {
  const bool neg = v < 0;
  const U mag = neg ? U(0) - static_cast<U>(v) : static_cast<U>(v);
  const char* aligns[] = {"", "<", "^", "*>"};
  const char types[] = {0, 'b', 'o', 'X'};
  for (const char* fa : aligns)
    for (char ty : types)
      for (int z = 0; z < 2; ++z) {
        const Spec s{fa, '+', true, z != 0, 30, ty};
        const std::string fmt = spec_string(s), exp = oracle(neg, mag, s);
        const std::wstring wfmt(fmt.begin(), fmt.end()), wexp(exp.begin(), exp.end());
        CHECK(std::vformat(wfmt, std::make_wformat_args(v)) == wexp);
      }
}

int main() {
  check_type<signed char>();
  check_type<unsigned char>();
  check_type<short>();
  check_type<unsigned short>();
  check_type<int>();
  check_type<unsigned>();
  check_type<long>();
  check_type<unsigned long>();
  check_type<long long>();
  check_type<unsigned long long>();
  check_type<char>();
  check_type<bool>();
  check_wide(std::numeric_limits<long long>::min());
  check_wide(std::numeric_limits<unsigned long long>::max());
  check_wide(0);
  check_wide(-42);
  // wchar_t with an integer presentation: converted to the unsigned underlying type
  CHECK(std::format(L"{:d}", static_cast<wchar_t>(-1)) ==
        std::to_wstring(static_cast<std::make_unsigned_t<wchar_t>>(static_cast<wchar_t>(-1))));
  CHECK(std::format(L"{:#x}", L'A') == L"0x41");
  CHECK(checked > 100000);
  return 0;
}
