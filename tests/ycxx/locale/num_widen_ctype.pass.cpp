// num_put / num_get go through the stream locale's ctype facet for every character:
// [facet.num.put.virtuals] Stage 2: "Any character c other than a decimal point(.) is converted
// to a charT via use_facet<ctype<charT>>(loc).widen(c)". [facet.num.get.virtuals] Stage 2:
// "char c = src[find(atoms, atoms + sizeof(src) - 1, ct) - atoms]" with atoms obtained by
// "use_facet<ctype<charT>>(loc).widen(src, src + sizeof(src), atoms)". [facet.ctype.char.
// members]: ctype<char>::widen(c) returns do_widen(c), widen(low, high, to) returns
// do_widen(low, high, to); [facet.ctype.char.virtuals]: both are virtual and may be
// overridden by a derived facet. Here the digits are widened to '!'..'*' and 'a'..'f' to
// 'g'..'l', so that a result that ignores ctype is detected (char, not only wchar_t).
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

struct Odd : std::ctype<char> {
  static char map(char c) {
    if (c >= '0' && c <= '9') return static_cast<char>('!' + (c - '0'));
    if (c >= 'a' && c <= 'f') return static_cast<char>('g' + (c - 'a'));
    return c;
  }
  char do_widen(char c) const override { return map(c); }
  const char* do_widen(const char* lo, const char* hi, char* to) const override {
    for (; lo != hi; ++lo, ++to) *to = map(*lo);
    return hi;
  }
};

template <class T>
static std::string put(const std::locale& l, T v, std::ios_base::fmtflags f = std::ios_base::dec) {
  std::ostringstream os;
  os.imbue(l);
  os.flags(f);
  os << v;
  return os.str();
}

template <class T>
static T get(const std::locale& l, const std::string& s, std::ios_base::fmtflags f = std::ios_base::dec,
             bool* failed = nullptr) {
  std::istringstream is(s);
  is.imbue(l);
  is.flags(f | std::ios_base::skipws);
  T v{};
  is >> v;
  if (failed) *failed = is.fail();
  return v;
}

int main() {
  const std::locale l(std::locale::classic(), new Odd);
  using B = std::ios_base;
  CHECK(put(l, -42) == "-%#");
  CHECK(put(l, 1234567890u) == "\"#$%&'()*!");
  CHECK(put(l, 255, B::hex) == "ll");
  CHECK(put(l, 255, B::hex | B::uppercase | B::showbase) == "!XFF");
  CHECK(put(l, 1.5) == "\".&");
  CHECK(put(l, 1e10, B::scientific) == "\".!!!!!!k+\"!");  // "1.000000e+10", 'e' -> 'k'
  CHECK(put(l, true) == "\"");

  bool f = true;
  CHECK(get<int>(l, "-%#", B::dec, &f) == -42 && !f);
  CHECK(get<unsigned>(l, "\"#$%&'()*!", B::dec, &f) == 1234567890u && !f);
  CHECK(get<int>(l, "ll", B::hex, &f) == 255 && !f);
  CHECK(get<int>(l, "!XFF", B::hex, &f) == 255 && !f);
  CHECK(get<double>(l, "\".&", B::dec, &f) == 1.5 && !f);
  CHECK(get<double>(l, "\".&k#", B::dec, &f) == 150.0 && !f);  // "1.5e2"
  CHECK(get<bool>(l, "\"", B::dec, &f) == true && !f);
  // The plain digits are not digits in this locale.
  CHECK(get<int>(l, "42", B::dec, &f) == 0 && f);
  return 0;
}
