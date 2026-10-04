// [ext.manip]/4: in >> get_money(mon, intl) calls money_get::get of the stream's locale and
// sets the error state; /6: out << put_money(mon, intl) "behaves as a formatted output
// function" calling money_put::put with str.fill(). /8: get_time(tmb, fmt) calls
// time_get::get(..., tmb, fmt, fmt + length) and sets err; /10: put_time(tmb, fmt) calls
// time_put::put. [locale.money.put.virtuals] / [locale.money.get.virtuals]: units are in the
// smallest currency unit, the decimal point is inserted frac_digits() before the end; the
// currency symbol is written only with showbase; the sign is negative_sign(). A moneypunct
// with explicit values is used, so that nothing depends on the "C" locale's monetary values.
// [locale.time.put.virtuals]: conversion specifiers as for strftime in the "C" locale;
// [locale.time.get.virtuals]: %Y %m %d %H %M %S are read as by strptime; a mismatch sets
// failbit.
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>
#include <ctime>
#include "check.hpp"

struct Money : std::moneypunct<char, false> {
  char do_decimal_point() const override { return '.'; }
  char do_thousands_sep() const override { return ','; }
  std::string do_grouping() const override { return ""; }
  std::string do_curr_symbol() const override { return "$"; }
  std::string do_positive_sign() const override { return ""; }
  std::string do_negative_sign() const override { return "-"; }
  int do_frac_digits() const override { return 2; }
  pattern do_pos_format() const override { return {{symbol, sign, none, value}}; }
  pattern do_neg_format() const override { return {{symbol, sign, none, value}}; }
};

int main() {
  const std::locale loc(std::locale::classic(), new Money);
  {
    std::ostringstream os;
    os.imbue(loc);
    os << std::put_money(123456.0L);
    CHECK(os.str() == "1234.56");
    os.str("");
    os << std::put_money(-500.0L);
    CHECK(os.str() == "-5.00");
    os.str("");
    os << std::showbase << std::put_money(std::string("789"));
    CHECK(os.str() == "$7.89");
    os.str("");
    os << std::noshowbase << std::put_money(std::string("-100"));
    CHECK(os.str() == "-1.00");
  }
  {
    std::istringstream is("1234.56 -1.07 $9.99");
    is.imbue(loc);
    long double v = 0;
    is >> std::get_money(v);
    CHECK(is && v == 123456.0L);
    std::string s;
    is >> std::get_money(s);
    CHECK(is && s == "-107");
    is >> std::showbase >> std::get_money(v);  // the symbol is required with showbase
    CHECK(is && v == 999.0L);
    std::istringstream bad("abc");
    bad.imbue(loc);
    bad >> std::get_money(v);
    CHECK(bad.fail());
  }
  {
    std::tm t{};
    t.tm_year = 124;  // 2024
    t.tm_mon = 1;
    t.tm_mday = 29;
    t.tm_hour = 13;
    t.tm_min = 5;
    t.tm_sec = 9;
    t.tm_wday = 4;
    t.tm_yday = 59;
    std::ostringstream os;
    os << std::put_time(&t, "%Y-%m-%d %H:%M:%S %%");
    CHECK(os.str() == "2024-02-29 13:05:09 %");
    os.str("");
    os << std::put_time(&t, "[%a %b %e]");
    CHECK(os.str() == "[Thu Feb 29]");
    std::wostringstream w;
    w << std::put_time(&t, L"%H:%M");
    CHECK(w.str() == L"13:05");
  }
  {
    std::tm t{};
    std::istringstream is("2023-11-05 07:08:09");
    is >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
    CHECK(!is.fail());
    CHECK(t.tm_year == 123 && t.tm_mon == 10 && t.tm_mday == 5);
    CHECK(t.tm_hour == 7 && t.tm_min == 8 && t.tm_sec == 9);
    std::tm u{};
    std::istringstream bad("2023/11/05");
    bad >> std::get_time(&u, "%Y-%m-%d");
    CHECK(bad.fail());
  }
  return 0;
}
