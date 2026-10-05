// [locale.moneypunct.byname], [locale.moneypunct.virtuals]: moneypunct of a named locale has that
// locale's monetary conventions; localeconv() in the same locale is the reference (the int_
// members for moneypunct<charT, true>). The patterns ([locale.moneypunct.general]/3) are derived
// from cs_precedes, sep_by_space and sign_posn: each of symbol, sign and value once, none never
// first, space neither first nor last; the symbol precedes the value iff cs_precedes; a space
// field exactly when sep_by_space is not 0. money_put / money_get round-trip through them
// ([locale.money.put.virtuals], [locale.money.get.virtuals]).
#include <limits.h>
#include <locale.h>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using M = std::money_base;

struct mconv {
  std::string symbol, intl_symbol, positive, negative;
  int frac, intl_frac;
  char cs[2][2], sep[2][2], posn[2][2]; // [intl][negative]
};

static mconv c_monetary(const char* name) {
  return in_c_locale(name, [] {
    const lconv* l = localeconv();
    return mconv{l->currency_symbol,
                 l->int_curr_symbol,
                 l->positive_sign,
                 l->negative_sign,
                 l->frac_digits,
                 l->int_frac_digits,
                 {{l->p_cs_precedes, l->n_cs_precedes}, {l->int_p_cs_precedes, l->int_n_cs_precedes}},
                 {{l->p_sep_by_space, l->n_sep_by_space}, {l->int_p_sep_by_space, l->int_n_sep_by_space}},
                 {{l->p_sign_posn, l->n_sign_posn}, {l->int_p_sign_posn, l->int_n_sign_posn}}};
  });
}

static int index_of(M::pattern p, char part) {
  for (int i = 0; i < 4; ++i)
    if (p.field[i] == part)
      return i;
  return -1;
}

static void check_pattern(M::pattern p, char cs, char sep, char posn) {
  CHECK(index_of(p, M::symbol) >= 0 && index_of(p, M::sign) >= 0 && index_of(p, M::value) >= 0);
  const int sp = index_of(p, M::space), no = index_of(p, M::none);
  CHECK((sp >= 0) != (no >= 0));
  CHECK(sp != 0 && sp != 3 && no != 0);
  if (cs == CHAR_MAX || sep == CHAR_MAX || posn == CHAR_MAX)
    return;
  CHECK((index_of(p, M::symbol) < index_of(p, M::value)) == (cs != 0));
  CHECK((sp >= 0) == (sep != 0));
  if (posn == 1 || posn == 0)
    CHECK(index_of(p, M::sign) < index_of(p, M::value) && index_of(p, M::sign) < index_of(p, M::symbol));
  if (posn == 2)
    CHECK(index_of(p, M::sign) > index_of(p, M::value) && index_of(p, M::sign) > index_of(p, M::symbol));
  if (posn == 3)
    CHECK(index_of(p, M::sign) < index_of(p, M::symbol));
  if (posn == 4)
    CHECK(index_of(p, M::sign) > index_of(p, M::symbol));
}

template <bool Intl>
static void check(const char* name) {
  const std::locale l(name);
  const mconv c = c_monetary(name);
  const auto& mp = std::use_facet<std::moneypunct<char, Intl>>(l);
  CHECK(mp.curr_symbol() == (Intl ? c.intl_symbol : c.symbol));
  CHECK(mp.positive_sign() == c.positive || (c.posn[Intl][0] == 0 && mp.positive_sign() == "()"));
  CHECK(mp.negative_sign() == c.negative || (c.posn[Intl][1] == 0 && mp.negative_sign() == "()"));
  const int frac = Intl ? c.intl_frac : c.frac;
  CHECK(mp.frac_digits() == (frac == CHAR_MAX ? 0 : frac));
  check_pattern(mp.pos_format(), c.cs[Intl][0], c.sep[Intl][0], c.posn[Intl][0]);
  check_pattern(mp.neg_format(), c.cs[Intl][1], c.sep[Intl][1], c.posn[Intl][1]);
  const auto& wmp = std::use_facet<std::moneypunct<wchar_t, Intl>>(l);
  CHECK(wmp.frac_digits() == mp.frac_digits());
  CHECK(wmp.pos_format().field[0] == mp.pos_format().field[0]);

  // put, then get what was put
  for (long double v : {0.0L, 123456789.0L, -123456789.0L, -1.0L}) {
    std::ostringstream os;
    os.imbue(l);
    os << std::showbase << std::put_money(v, Intl);
    std::istringstream is(os.str());
    is.imbue(l);
    long double back = 42;
    is >> std::noskipws >> std::showbase >> std::get_money(back, Intl); // the text may begin with the space field
    CHECK(!is.fail() && back == v);
    std::wostringstream wos;
    wos.imbue(l);
    wos << std::put_money(v, Intl);
    std::wistringstream wis(wos.str());
    wis.imbue(l);
    back = 42;
    wis >> std::noskipws >> std::get_money(back, Intl);
    CHECK(!wis.fail() && back == v);
  }
}

int main() {
  for (const char* n : {"de_DE.UTF-8", "en_US.UTF-8", "fr_FR.ISO8859-15"}) {
    require_locale(n);
    check<false>(n);
    check<true>(n);
  }
}
