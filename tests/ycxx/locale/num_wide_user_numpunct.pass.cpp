// num_get / num_put for wchar_t with a user numpunct<wchar_t> whose decimal point and thousands
// separator are not ASCII (U+066B ARABIC DECIMAL SEPARATOR, U+066C ARABIC THOUSANDS SEPARATOR),
// and for char with the decimal point ',' and the separator '.'.
// [facet.num.put.virtuals] Stage 2: characters other than '.' are widened with ctype; "punct.
// thousands_sep() characters are inserted into the sequence as determined by the value returned
// by punct.do_grouping()" (for every arithmetic type, so also into the integer part of a
// floating-point value); "Decimal point characters(.) are replaced by punct.decimal_point()".
// /6: boolalpha writes truename() / falsename().
// [facet.num.get.virtuals] Stage 2: c = src[find(atoms, ..., ct) - atoms] with atoms the
// widened "0123456789abcdefpxABCDEFPX+-"; "if (ct == ...decimal_point()) c = '.'"; ct equal to
// thousands_sep() with a non-empty grouping is remembered and ignored before the decimal point,
// and "if '.' has already been accumulated, the character is discarded and Stage 2 terminates".
// Any other character (a '.' that is not decimal_point(), a digit of another script) ends the
// field. /6 (bool, boolalpha): the field is matched against truename() and falsename(); "If
// there is no match, false is stored" and failbit is set; a prefix of a name alone does not
// match; "The input iterator in is compared to end only when necessary to obtain a character."
#include <locale>
#include <sstream>
#include <iterator>
#include <string>
#include "check.hpp"

using B = std::ios_base;

struct WPunct : std::numpunct<wchar_t> {
  wchar_t do_decimal_point() const override { return L'\u066b'; }
  wchar_t do_thousands_sep() const override { return L'\u066c'; }
  std::string do_grouping() const override { return "\3"; }
  std::wstring do_truename() const override { return L"\u0635\u062d"; }
  std::wstring do_falsename() const override { return L"\u062e"; }
};

struct CPunct : std::numpunct<char> {
  char do_decimal_point() const override { return ','; }
  char do_thousands_sep() const override { return '.'; }
  std::string do_grouping() const override { return "\3"; }
};

template <class C, class V>
std::basic_string<C> put(const std::locale& l, V v, B::fmtflags f = {}, int prec = 6) {
  std::basic_ostringstream<C> os;
  os.imbue(l);
  os.flags(f);
  os.precision(prec);
  os << v;
  return os.str();
}

template <class C, class V>
V get(const std::locale& l, const std::basic_string<C>& in, B::iostate& st, std::basic_string<C>& rest,
      B::fmtflags f = B::dec | B::skipws) {
  std::basic_istringstream<C> is(in);
  is.imbue(l);
  is.flags(f);
  V v{};
  if constexpr (std::is_same_v<V, bool>) v = true;
  else v = V(99);
  is >> v;
  st = is.rdstate();
  is.clear();
  rest.assign(std::istreambuf_iterator<C>(is), std::istreambuf_iterator<C>());
  return v;
}

int main() {
  const std::locale w(std::locale::classic(), new WPunct);
  B::iostate st;
  std::wstring wr;

  // Output.
  CHECK(put<wchar_t>(w, 1234567L) == L"1\u066c234\u066c567");
  CHECK(put<wchar_t>(w, -1234) == L"-1\u066c234");
  CHECK(put<wchar_t>(w, 1234.5, B::fixed, 1) == L"1\u066c234\u066b5");
  CHECK(put<wchar_t>(w, 0.25, B::scientific, 2) == L"2\u066b50e-01");
  CHECK(put<wchar_t>(w, 1234567u, B::hex) == L"12d\u066c687");  // 0x12d687
  CHECK(put<wchar_t>(w, true, B::boolalpha) == L"\u0635\u062d");
  CHECK(put<wchar_t>(w, false, B::boolalpha) == L"\u062e");

  // Input.
  CHECK(get<wchar_t, long>(w, L"1\u066c234\u066c567", st, wr) == 1234567 && st == B::eofbit);
  CHECK(get<wchar_t, double>(w, L"1\u066c234\u066b5", st, wr) == 1234.5 && st == B::eofbit);
  CHECK(get<wchar_t, double>(w, L"-0\u066b25e1x", st, wr) == -2.5 && st == B::goodbit && wr == L"x");
  CHECK(get<wchar_t, long>(w, L"12\u066b5", st, wr) == 12 && st == B::goodbit && wr == L"\u066b5");
  CHECK(get<wchar_t, double>(w, L"1.5", st, wr) == 1.0 && st == B::goodbit && wr == L".5");
  CHECK(get<wchar_t, double>(w, L"1,5", st, wr) == 1.0 && st == B::goodbit && wr == L",5");
  // A separator after the decimal point ends the field.
  CHECK(get<wchar_t, double>(w, L"1\u066b5\u066c6", st, wr) == 1.5 && !(st & B::failbit));
  // FULLWIDTH DIGIT ONE is not an atom: nothing is accumulated.
  CHECK(get<wchar_t, long>(w, L"\uff11", st, wr) == 0 && (st & B::failbit));
  // Names.
  // "The input iterator in is compared to end only when necessary to obtain a character": the
  // one-character falsename is matched uniquely without looking further, so no eofbit.
  CHECK(get<wchar_t, bool>(w, L"\u062e", st, wr, B::boolalpha | B::skipws) == false && st == B::goodbit);
  CHECK(get<wchar_t, bool>(w, L"\u0635\u062d!", st, wr, B::boolalpha | B::skipws) == true &&
        st == B::goodbit && wr == L"!");
  CHECK(get<wchar_t, bool>(w, L"\u0635", st, wr, B::boolalpha | B::skipws) == false && (st & B::failbit));
  CHECK(get<wchar_t, bool>(w, L"true", st, wr, B::boolalpha | B::skipws) == false && (st & B::failbit));

  // char with ',' as the decimal point and '.' as the separator.
  const std::locale c(std::locale::classic(), new CPunct);
  std::string cr;
  CHECK(put<char>(c, 1234.5, B::fixed, 1) == "1.234,5");
  CHECK(put<char>(c, 1e6, B::fixed, 0) == "1.000.000");
  CHECK(put<char>(c, 1e6, B::fixed | B::showpoint, 0) == "1.000.000,");
  CHECK(get<char, double>(c, "1.234,5", st, cr) == 1234.5 && st == B::eofbit);
  CHECK(get<char, double>(c, "1234,5", st, cr) == 1234.5 && st == B::eofbit);
  CHECK(get<char, long>(c, "1.234", st, cr) == 1234 && st == B::eofbit);
  CHECK(get<char, long>(c, "12,5", st, cr) == 12 && st == B::goodbit && cr == ",5");
  CHECK(get<char, double>(c, "0,5e1", st, cr) == 5.0 && st == B::eofbit);
  return 0;
}
