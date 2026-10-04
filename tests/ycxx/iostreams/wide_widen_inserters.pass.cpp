// Narrow characters reach a wide stream through the locale's ctype<wchar_t>::widen:
// [basic.ios.members]: widen(c) "Returns: use_facet<ctype<char_type>>(getloc()).widen(c)".
// [ostream.inserters.character]: operator<<(basic_ostream<charT, traits>&, char c) inserts
// out.widen(c); operator<<(basic_ostream<charT, traits>&, const char* s): "Creates a character
// sequence seq of n characters starting at s, each widened using out.widen()".
// [facet.num.put.virtuals] Stage 2: each char other than '.' is converted "via
// use_facet<ctype<charT>>(loc).widen(c)"; [facet.num.get.virtuals] Stage 2: the atoms are
// "use_facet<ctype<charT>>(loc).widen(src, src + sizeof(src), atoms)". The truename/falsename
// strings come from numpunct<charT> and are not widened ([facet.num.put.virtuals]/6).
// [facet.ctype.virtuals]: do_widen is virtual. Here a ctype<wchar_t> widens 'a'..'z' to
// upper case.
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

struct Upper : std::ctype<wchar_t> {
  static wchar_t up(char c) {
    if (c >= 'a' && c <= 'z') return static_cast<wchar_t>(L'A' + (c - 'a'));
    return static_cast<wchar_t>(static_cast<unsigned char>(c));
  }
  wchar_t do_widen(char c) const override { return up(c); }
  const char* do_widen(const char* lo, const char* hi, wchar_t* to) const override {
    for (; lo != hi; ++lo, ++to) *to = up(*lo);
    return hi;
  }
};

int main() {
  const std::locale l(std::locale::classic(), new Upper);
  {
    std::wostringstream os;
    os.imbue(l);
    CHECK(os.widen('q') == L'Q');
    os << "abc" << ' ' << 'x' << L'y' << L"z";
    CHECK(os.str() == L"ABC Xyz");
  }
  {
    std::wostringstream os;
    os.imbue(l);
    os << std::hex << 255 << ' ' << std::dec << std::scientific << 1.5e10 << ' ' << std::boolalpha << true;
    CHECK(os.str() == L"FF 1.500000E+10 true");
  }
  {
    std::wostringstream os;
    os.imbue(l);
    os.width(6);
    os.fill(L'.');
    os << "ab";
    CHECK(os.str() == L"....AB");
  }
  {
    std::wistringstream is(L"FF ff");
    is.imbue(l);
    int a = 0, b = 7;
    is >> std::hex >> a;
    CHECK(a == 255 && !is.fail());
    is >> b;  // 'f' is not an atom any more ('a'..'f' widen to 'A'..'F')
    CHECK(b == 0 && is.fail());
  }
  {
    std::wistringstream is(L"1.5E3 2.5e3");
    is.imbue(l);
    double d = 0;
    is >> d;
    CHECK(d == 1500.0 && !is.fail());
    is >> d;  // "2.5" then 'e' is not an atom
    CHECK(d == 2.5 && !is.fail());
  }
  return 0;
}
