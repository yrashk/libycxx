// [locale.time.get.virtuals]/4: do_get_date reads "those tm members and remaining format
// characters used by time_put<>::put to produce one of the following formats ... The format
// depends on the value returned by date_order() as shown in Table 102": no_order "%m%d%y",
// dmy "%d%m%y", mdy "%m%d%y", ymd "%y%m%d", ydm "%y%d%m". date_order() calls the virtual
// do_date_order() ([locale.time.get.members]/1), so a derived facet that overrides it selects
// the format. /5: "An implementation may also accept additional implementation-defined
// formats", so only the table's own formats (no separators) are checked. POSIX strptime: %d and
// %m take at most two digits, %y 00-68 is 2000-2068 and 69-99 is 1969-1999.
#include <locale>
#include <sstream>
#include <iterator>
#include <ctime>
#include <string>
#include "check.hpp"

using B = std::ios_base;
using It = std::istreambuf_iterator<char>;
using TG = std::time_get<char>;

static std::tm sentinel() {
  std::tm t{};
  t.tm_year = t.tm_mon = t.tm_mday = t.tm_hour = t.tm_min = t.tm_sec = t.tm_yday = -77;
  return t;
}

struct Ordered : TG {
  std::time_base::dateorder o;
  explicit Ordered(std::time_base::dateorder d) : TG(1), o(d) {}
  dateorder do_date_order() const override { return o; }
};

static std::tm date(const TG& tg, const char* in, B::iostate& err, std::string& rest) {
  std::istringstream is(in);
  std::tm t = sentinel();
  err = B::goodbit;
  It it = tg.get_date(It(is), It(), is, err, &t);
  rest.assign(it, It());
  return t;
}

int main() {
  B::iostate err;
  std::string rest;
  std::tm t;

  // Table 102 with a user date_order().
  const Ordered dmy(std::time_base::dmy), mdy(std::time_base::mdy), ymd(std::time_base::ymd),
      ydm(std::time_base::ydm), none(std::time_base::no_order);
  CHECK(dmy.date_order() == std::time_base::dmy);
  t = date(dmy, "050724 x", err, rest);
  CHECK(!(err & B::failbit) && t.tm_mday == 5 && t.tm_mon == 6 && t.tm_year == 124 && rest == " x");
  t = date(mdy, "050724 x", err, rest);
  CHECK(!(err & B::failbit) && t.tm_mon == 4 && t.tm_mday == 7 && t.tm_year == 124 && rest == " x");
  t = date(none, "123199;", err, rest);
  CHECK(!(err & B::failbit) && t.tm_mon == 11 && t.tm_mday == 31 && t.tm_year == 99 && rest == ";");
  t = date(ymd, "690131 x", err, rest);
  CHECK(!(err & B::failbit) && t.tm_year == 69 && t.tm_mon == 0 && t.tm_mday == 31 && rest == " x");
  t = date(ydm, "683112 x", err, rest);
  CHECK(!(err & B::failbit) && t.tm_year == 168 && t.tm_mday == 31 && t.tm_mon == 11 && rest == " x");
  // Out of range for the order in use: day 13 is fine for dmy, month 13 is not for mdy.
  t = date(dmy, "130124", err, rest);
  CHECK(!(err & B::failbit) && t.tm_mday == 13 && t.tm_mon == 0);
  date(mdy, "130124", err, rest);
  CHECK((err & B::failbit) != 0);
  // The input ends in the middle of the date: failbit and eofbit.
  date(dmy, "0507", err, rest);
  CHECK((err & B::failbit) && (err & B::eofbit));

  return 0;
}
