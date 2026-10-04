// [ext.manip]/4: in >> get_money(mon) calls mg.get(Iter(str.rdbuf()), Iter(), intl, str, err,
// mon) and then setstate(err). [locale.money.get.virtuals]/1: "If a valid sequence is
// recognized, does not change err; otherwise, sets err to (err | str.failbit), or (err |
// str.failbit | str.eofbit) if no more characters are available, and does not change units or
// digits." A failed extraction leaves both the long double and the string untouched.
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  {
    long double v = 999.0L;
    std::istringstream is("abc");
    is >> std::get_money(v);
    CHECK(is.fail());
    CHECK(v == 999.0L);
  }
  {
    std::string s = "keep";
    std::istringstream is("xyz");
    is >> std::get_money(s);
    CHECK(is.fail());
    CHECK(s == "keep");
  }
  {
    // the facet directly
    using Iter = std::istreambuf_iterator<char>;
    std::istringstream is("");
    const auto& mg = std::use_facet<std::money_get<char>>(is.getloc());
    std::ios_base::iostate err = std::ios_base::goodbit;
    long double v = 42.0L;
    mg.get(Iter(is.rdbuf()), Iter(), false, is, err, v);
    CHECK((err & std::ios_base::failbit) != 0 && (err & std::ios_base::eofbit) != 0);
    CHECK(v == 42.0L);
  }
  return 0;
}
