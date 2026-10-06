// [locale.money.get.virtuals]/2: without showbase "the currency symbol is optional and is
// consumed only if other characters are needed to complete the format". With the pattern
// {value, symbol, none, sign} and empty sign strings nothing follows the symbol, so "10$" stops
// before the '$'; a value or a non-empty sign after the symbol makes it needed.
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

template <class charT>
struct punct : std::moneypunct<charT, false> {
  std::basic_string<charT> pos_, neg_;
  std::money_base::pattern pat_;
  punct(std::basic_string<charT> p, std::basic_string<charT> n, std::money_base::pattern pat)
      : pos_(p), neg_(n), pat_(pat) {}
  std::basic_string<charT> do_curr_symbol() const override { return std::basic_string<charT>(1, charT('$')); }
  std::basic_string<charT> do_positive_sign() const override { return pos_; }
  std::basic_string<charT> do_negative_sign() const override { return neg_; }
  std::money_base::pattern do_pos_format() const override { return pat_; }
  std::money_base::pattern do_neg_format() const override { return pat_; }
  int do_frac_digits() const override { return 0; }
};

template <class charT>
static void run(const charT* in, const charT* pos, const charT* neg, std::money_base::pattern pat, long consumed,
                std::ios_base::iostate expect, long double value) {
  using It = typename std::basic_string<charT>::const_iterator;
  std::basic_istringstream<charT> is;
  is.imbue(std::locale(std::locale::classic(), new punct<charT>(pos, neg, pat)));
  const std::basic_string<charT> s(in);
  std::ios_base::iostate err = std::ios_base::goodbit;
  long double v = -1;
  struct facet : std::money_get<charT, It> {
    facet() : std::money_get<charT, It>(1) {}
  } mg;
  It e = mg.get(s.begin(), s.end(), false, is, err, v);
  CHECK(e - s.begin() == consumed);
  CHECK(err == expect);
  CHECK(v == value);
}

int main() {
  using M = std::money_base;
  const M::pattern trailing{{M::value, M::symbol, M::none, M::sign}};
  run<char>("10$", "", "", trailing, 2, std::ios_base::goodbit, 10);
  run<wchar_t>(L"10$", L"", L"", trailing, 2, std::ios_base::goodbit, 10);
  // a sign after the symbol, with both sign strings non-empty, needs characters: '$' is consumed
  run<char>("10$+", "+", "-", trailing, 4, std::ios_base::eofbit, 10);
  // a value after the symbol needs it consumed as well
  const M::pattern leading{{M::symbol, M::value, M::none, M::sign}};
  run<char>("$10", "", "", leading, 3, std::ios_base::eofbit, 10);
}
