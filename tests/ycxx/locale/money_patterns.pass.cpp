// [locale.money.get.virtuals]/1-4 and [locale.money.put.virtuals]/1-3 with explicit moneypunct
// facets ([locale.moneypunct.general]/1-4: a pattern has symbol, sign, value and space or
// none, each exactly once).
// money_get: "Uses the pattern returned by mp.neg_format() to parse all values"; thousands
// separators "are optional; if present, they are checked for correct placement only after all
// format components have been read"; "The number of digits required after the decimal point
// (if any) is exactly the value returned by frac_digits()"; /2: space as a non-last element
// requires at least one whitespace, none allows any, and as the last element no whitespace is
// consumed; "If (str.flags() & str.showbase) is false, the currency symbol is optional and is
// consumed only if other characters are needed to complete the format; otherwise, the
// currency symbol is required." /3: the first character of the sign is matched at the sign
// position, the rest "are required after all the other format components"; Example 2: with
// showbase off and neg "()" and symbol "L", in "(100 L)" the "L" is consumed, with neg "-" the
// "L" in "-100 L" is not; an empty sign string makes the sign optional (with the sign of the
// source of that empty string). /4: digits are "placed in digits ... in the order in which
// they appear, preceded by a minus sign if and only if the result is negative". On failure
// err gets failbit (and eofbit if no more characters are available) and the result is
// unchanged. /5: returns an iterator "immediately beyond the last character recognized".
// money_put: units are converted as by sprintf("%.0Lf", units); a leading '-' selects
// neg_format(); "In digits, only the optional leading minus sign and the immediately
// subsequent digit characters ... are used"; the symbol is generated iff showbase; padding:
// internal -> where none or space appears, left -> after, otherwise before; width(0).
#include <locale>
#include <sstream>
#include <iterator>
#include <string>
#include "check.hpp"

using B = std::ios_base;
using P = std::money_base::pattern;
using M = std::money_base;

struct Punct : std::moneypunct<char, false> {
  std::string sym, pos, neg, grp;
  int frac;
  P pf, nf;
  Punct(std::string s, std::string p, std::string n, int f, P pfmt, P nfmt, std::string g = "\3")
      : sym(s), pos(p), neg(n), grp(g), frac(f), pf(pfmt), nf(nfmt) {}
  char do_decimal_point() const override { return '.'; }
  char do_thousands_sep() const override { return ','; }
  std::string do_grouping() const override { return grp; }
  std::string do_curr_symbol() const override { return sym; }
  std::string do_positive_sign() const override { return pos; }
  std::string do_negative_sign() const override { return neg; }
  int do_frac_digits() const override { return frac; }
  P do_pos_format() const override { return pf; }
  P do_neg_format() const override { return nf; }
};

static P pat(M::part a, M::part b, M::part c, M::part d) {
  P p;
  p.field[0] = static_cast<char>(a);
  p.field[1] = static_cast<char>(b);
  p.field[2] = static_cast<char>(c);
  p.field[3] = static_cast<char>(d);
  return p;
}

static std::string get(const std::locale& l, const std::string& in, bool showbase, B::iostate& err,
                       std::string* rest = nullptr) {
  std::istringstream is(in);
  is.imbue(l);
  if (showbase) is.setf(B::showbase);
  using It = std::istreambuf_iterator<char>;
  std::string digits = "unchanged";
  err = B::goodbit;
  It it = std::use_facet<std::money_get<char>>(l).get(It(is), It(), false, is, err, digits);
  if (rest) {
    rest->clear();
    for (; it != It(); ++it) rest->push_back(*it);
  }
  return digits;
}

template <class V>
static std::string put(const std::locale& l, V v, B::fmtflags f, int width = 0, char fill = '*') {
  std::ostringstream os;
  os.imbue(l);
  os.flags(f);
  os.width(width);
  using It = std::ostreambuf_iterator<char>;
  std::use_facet<std::money_put<char>>(l).put(It(os), false, os, fill, v);
  CHECK(os.width() == 0);
  return os.str();
}

int main() {
  const std::locale c = std::locale::classic();
  const P sign_sym_val_none = pat(M::sign, M::symbol, M::value, M::none);
  const P sym_sign_none_val = pat(M::symbol, M::sign, M::none, M::value);
  const std::locale us(c, new Punct("$", "", "()", 2, sym_sign_none_val, sign_sym_val_none));
  B::iostate err;
  std::string rest;

  CHECK(get(us, "(1,056.23)", false, err) == "-105623" && err == B::eofbit);
  CHECK(get(us, "($1,056.23)", false, err) == "-105623" && err == B::eofbit);
  CHECK(get(us, "1,056.23", false, err) == "105623" && err == B::eofbit);
  CHECK(get(us, "$1,056.23", false, err) == "105623" && err == B::eofbit);
  CHECK(get(us, "1056.23 x", false, err, &rest) == "105623" && err == B::goodbit && rest == " x");
  CHECK(get(us, "(1056.23) ", false, err, &rest) == "-105623" && rest == " ");
  CHECK(get(us, ".23", false, err) == "23" && err == B::eofbit);
  CHECK(get(us, "1056", false, err) == "1056" && err == B::eofbit);  // no fractional part: digits as they appear
  CHECK(get(us, "1.", false, err) == "unchanged" && (err & B::failbit));
  CHECK(get(us, "1.2 ", false, err) == "unchanged" && (err & B::failbit));
  CHECK(get(us, "(1,056.23", false, err) == "unchanged" && err == (B::failbit | B::eofbit));
  CHECK(get(us, "(1,056.2)", false, err) == "unchanged" && (err & B::failbit));
  CHECK(get(us, " 1.00", false, err) == "unchanged" && (err & B::failbit));
  CHECK(get(us, "", false, err) == "unchanged" && err == (B::failbit | B::eofbit));
  // showbase: the symbol is required.
  CHECK(get(us, "1.00", true, err) == "unchanged" && (err & B::failbit));
  CHECK(get(us, "$1.00", true, err) == "100" && err == B::eofbit);
  CHECK(get(us, "($1.00)", true, err) == "-100" && err == B::eofbit);
  CHECK(get(us, "(1.00)", true, err) == "unchanged" && (err & B::failbit));
  {
    // units
    std::istringstream is("(1,056.23)");
    is.imbue(us);
    using It = std::istreambuf_iterator<char>;
    long double u = 7;
    err = B::goodbit;
    std::use_facet<std::money_get<char>>(us).get(It(is), It(), false, is, err, u);
    CHECK(u == -105623.0L && err == B::eofbit);
  }

  // Example 2 (frac_digits 0, showbase off).
  const P sign_val_space_sym = pat(M::sign, M::value, M::space, M::symbol);
  const std::locale paren(c, new Punct("L", "", "()", 0, sign_val_space_sym, sign_val_space_sym));
  const std::locale dash(c, new Punct("L", "", "-", 0, sign_val_space_sym, sign_val_space_sym));
  CHECK(get(paren, "(100 L)", false, err) == "-100" && err == B::eofbit);
  CHECK(get(dash, "-100 L", false, err, &rest) == "-100" && !(err & B::failbit) && rest == "L");
  CHECK(get(dash, "-100L", false, err) == "unchanged" && (err & B::failbit));  // space needs whitespace
  CHECK(get(dash, "-100 \t L", true, err) == "-100" && err == B::eofbit);
  CHECK(get(dash, "100 L", true, err) == "100");

  // Same first character of pos and neg: positive.
  const std::locale same(c, new Punct("$", "-a", "-b", 0, sign_val_space_sym, sign_val_space_sym, ""));
  CHECK(get(same, "-5 $a", true, err) == "5" && err == B::eofbit);

  // money_put
  CHECK(put(us, std::string("105623"), B::showbase) == "$1,056.23");
  CHECK(put(us, std::string("105623"), B::fmtflags{}) == "1,056.23");
  CHECK(put(us, std::string("-105623"), B::showbase) == "($1,056.23)");
  CHECK(put(us, std::string("-105623"), B::fmtflags{}) == "(1,056.23)");
  CHECK(put(us, std::string("123abc456"), B::fmtflags{}) == "1.23");
  CHECK(put(us, std::string("1234567"), B::fmtflags{}) == "12,345.67");
  CHECK(put(us, 105623.0L, B::showbase) == "$1,056.23");
  CHECK(put(us, -100.0L, B::fmtflags{}) == "(1.00)");
  CHECK(put(us, 250.5L, B::fmtflags{}) == "2.50");   // %.0Lf: 250.5 -> "250" (to even)
  CHECK(put(us, 251.5L, B::fmtflags{}) == "2.52");
  CHECK(put(us, std::string("100"), B::showbase | B::internal, 10) == "$*****1.00");
  CHECK(put(us, std::string("100"), B::showbase | B::left, 10) == "$1.00*****");
  CHECK(put(us, std::string("100"), B::showbase | B::right, 10) == "*****$1.00");
  CHECK(put(us, std::string("100"), B::showbase, 10) == "*****$1.00");
  CHECK(put(us, std::string("100"), B::showbase, 3) == "$1.00");
  // "%.0Lf" of -0.4 is "-0": neg_format; the space element is ct.widen(' ')
  // ([locale.moneypunct.general]/3: "The space character used is the value ct.widen(' ')"),
  // not the fill character, when no padding is needed.
  CHECK(put(paren, -0.4L, B::fmtflags{}) == "(0 )");
  CHECK(put(dash, std::string("-100"), B::showbase) == "-100 L");
  {
    const std::string r = put(dash, std::string("-100"), B::showbase | B::internal, 8, '_');
    CHECK(r.size() == 8 && r.substr(0, 4) == "-100" && r[7] == 'L');  // the fill goes where space is
    CHECK((r.substr(4, 3) == " __" || r.substr(4, 3) == "__ "));
  }
  // Thousands separators in the wrong places: failure, and "does not change units or digits".
  CHECK(get(us, "(1,05,6.23)", false, err) == "unchanged" && (err & B::failbit));
  CHECK(get(us, "12,34.00", false, err) == "unchanged" && (err & B::failbit));
  return 0;
}
