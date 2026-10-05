// [locale.money.get.virtuals]/1 and [locale.money.put.virtuals]/1: the facet used is
// "a moneypunct<charT, Intl> facet reference mp" with Intl the intl argument, so a locale with
// different moneypunct<C, false> and moneypunct<C, true> facets formats and parses differently
// for intl false and true; the same holds for wchar_t, where digits are classified with
// ctype<wchar_t> ("In digits, only the optional leading minus sign and the immediately
// subsequent digit characters (as classified according to ct) are used").
// [locale.moneypunct.virtuals]/7: the four required specializations return
// { symbol, sign, none, value } from do_pos_format and do_neg_format.
// [locale.moneypunct.virtuals]/3: grouping is "defined identically as" numpunct's;
// [facet.numpunct.virtuals]/3: vec[i] is the size of group i from the right, "If vec.size() <= i,
// the number is the same as group (i - 1); if (i < 0 || vec[i] <= 0 || vec[i] == CHAR_MAX), the
// size of the digit group is unlimited." money_get checks the separators' placement after all
// format components have been read ([locale.money.get.virtuals]/1) and fails otherwise, without
// changing the result.
#include <locale>
#include <sstream>
#include <iterator>
#include <string>
#include <climits>
#include "check.hpp"

using B = std::ios_base;
using M = std::money_base;

static M::pattern pat(M::part a, M::part b, M::part c, M::part d) {
  M::pattern p;
  p.field[0] = static_cast<char>(a);
  p.field[1] = static_cast<char>(b);
  p.field[2] = static_cast<char>(c);
  p.field[3] = static_cast<char>(d);
  return p;
}

template <class C, bool Intl>
struct Punct : std::moneypunct<C, Intl> {
  using S = std::basic_string<C>;
  S sym;
  int frac;
  std::string grp;
  Punct(S s, int f, std::string g) : sym(s), frac(f), grp(g) {}
  C do_decimal_point() const override { return C('.'); }
  C do_thousands_sep() const override { return C(','); }
  std::string do_grouping() const override { return grp; }
  S do_curr_symbol() const override { return sym; }
  S do_positive_sign() const override { return S(); }
  S do_negative_sign() const override { return S(1, C('-')); }
  int do_frac_digits() const override { return frac; }
  M::pattern do_pos_format() const override { return pat(M::symbol, M::sign, M::none, M::value); }
  M::pattern do_neg_format() const override { return pat(M::sign, M::symbol, M::none, M::value); }
};

template <class C, bool Intl>
struct Default : std::moneypunct<C, Intl> {};

template <class C, bool Intl>
void check_default_patterns() {
  Default<C, Intl> d;
  for (const M::pattern& p : {d.pos_format(), d.neg_format()}) {
    CHECK(p.field[0] == M::symbol && p.field[1] == M::sign && p.field[2] == M::none && p.field[3] == M::value);
  }
}

template <class C>
std::basic_string<C> put(const std::locale& l, bool intl, const std::basic_string<C>& digits, B::fmtflags f = B::showbase) {
  std::basic_ostringstream<C> os;
  os.imbue(l);
  os.flags(f);
  std::use_facet<std::money_put<C>>(l).put(std::ostreambuf_iterator<C>(os), intl, os, C('*'), digits);
  return os.str();
}

template <class C>
std::basic_string<C> get(const std::locale& l, bool intl, const std::basic_string<C>& in, B::iostate& err,
                         B::fmtflags f = B::showbase) {
  std::basic_istringstream<C> is(in);
  is.imbue(l);
  is.flags(f);
  using It = std::istreambuf_iterator<C>;
  std::basic_string<C> digits(1, C('?'));
  err = B::goodbit;
  std::use_facet<std::money_get<C>>(l).get(It(is), It(), intl, is, err, digits);
  return digits;
}

int main() {
  check_default_patterns<char, false>();
  check_default_patterns<char, true>();
  check_default_patterns<wchar_t, false>();
  check_default_patterns<wchar_t, true>();

  B::iostate err;
  {
    // char: different local and international facets.
    std::locale l(std::locale::classic(), new Punct<char, false>("$", 2, "\3"));
    l = std::locale(l, new Punct<char, true>("USD ", 3, ""));
    CHECK(put<char>(l, false, "1234567") == "$12,345.67");
    CHECK(put<char>(l, true, "1234567") == "USD 1234.567");
    CHECK(put<char>(l, false, "-1234567") == "-$12,345.67");
    CHECK(put<char>(l, true, "-1234567") == "-USD 1234.567");
    CHECK(get<char>(l, false, "$12,345.67", err) == "1234567" && !(err & B::failbit));
    CHECK(get<char>(l, true, "USD 1234.567", err) == "1234567" && !(err & B::failbit));
    CHECK(get<char>(l, true, "-USD 1234.567", err) == "-1234567" && !(err & B::failbit));
    // The other facet's symbol or number of fractional digits does not match.
    CHECK(get<char>(l, true, "$12,345.67", err) == "?" && (err & B::failbit));
    CHECK(get<char>(l, false, "USD 1234.567", err) == "?" && (err & B::failbit));
  }
  {
    // wchar_t.
    std::locale l(std::locale::classic(), new Punct<wchar_t, false>(L"\u20ac", 2, "\3"));
    l = std::locale(l, new Punct<wchar_t, true>(L"EUR ", 0, "\3"));
    CHECK(put<wchar_t>(l, false, L"1234567") == L"\u20ac12,345.67");
    CHECK(put<wchar_t>(l, true, L"1234567") == L"EUR 1,234,567");
    CHECK(put<wchar_t>(l, false, L"-123x4") == L"-\u20ac1.23");  // only the leading digits
    // U+0663 ARABIC-INDIC DIGIT THREE is not a digit for ctype<wchar_t> (iswdigit: 0-9 only).
    CHECK(put<wchar_t>(l, true, L"12\u0663") == L"EUR 12");
    CHECK(get<wchar_t>(l, false, L"\u20ac12,345.67", err) == L"1234567" && !(err & B::failbit));
    CHECK(get<wchar_t>(l, true, L"-EUR 1,234,567", err) == L"-1234567" && !(err & B::failbit));
    CHECK(get<wchar_t>(l, true, L"\u20ac1,234", err) == L"?" && (err & B::failbit));
    CHECK(get<wchar_t>(l, false, L"EUR 1,234.56", err) == L"?" && (err & B::failbit));
  }
  {
    // Grouping vectors, frac_digits 0.
    auto with = [](std::string g) {
      return std::locale(std::locale::classic(), new Punct<char, false>("", 0, g));
    };
    const B::fmtflags none{};
    CHECK(put<char>(with("\3\2"), false, "123456789", none) == "12,34,56,789");
    CHECK(put<char>(with("\1"), false, "1234", none) == "1,2,3,4");
    CHECK(put<char>(with(std::string{3, CHAR_MAX}), false, "123456789", none) == "123456,789");
    CHECK(put<char>(with(std::string{2, 0}), false, "123456789", none) == "1234567,89");
    CHECK(put<char>(with("\2\3"), false, "-1234567", none) == "-12,345,67");
    CHECK(put<char>(with("\3"), false, "123", none) == "123");
    CHECK(get<char>(with("\3\2"), false, "12,34,56,789", err, none) == "123456789" && !(err & B::failbit));
    CHECK(get<char>(with("\3\2"), false, "123456789", err, none) == "123456789" && !(err & B::failbit));
    CHECK(get<char>(with("\3\2"), false, "1,234,56,789", err, none) == "?" && (err & B::failbit));
    CHECK(get<char>(with("\3\2"), false, "123,456,789", err, none) == "?" && (err & B::failbit));
    CHECK(get<char>(with(std::string{3, CHAR_MAX}), false, "123456,789", err, none) == "123456789" &&
          !(err & B::failbit));
    CHECK(get<char>(with(std::string{3, CHAR_MAX}), false, "123,456,789", err, none) == "?" && (err & B::failbit));
  }
  return 0;
}
