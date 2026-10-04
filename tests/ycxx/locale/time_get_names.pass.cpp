// [locale.time.get.virtuals]/7: do_get_weekday / do_get_monthname: "Reads characters starting
// at s until it has extracted the (perhaps abbreviated) name of a weekday or month. If it
// finds an abbreviation that is followed by characters that can match a full name, it
// continues reading until it matches the full name or fails. It sets the appropriate tm member
// accordingly." /8: "Returns: An iterator pointing immediately beyond the last character
// recognized as part of a valid name." /1: date_order() describes the order of %x, which in
// the "C" locale is "%m/%d/%y" (ISO C 7.29.3.5), i.e. mdy (or no_order, footnote 207). /2-3: do_get_time reads the
// members used for "%H:%M:%S". /9-10: do_get_year reads a year (four digits are unambiguous).
// The names of the "C" locale (ISO C 7.29.3.5): "Sunday".."Saturday" / "Jan".."Dec" etc.
#include <locale>
#include <sstream>
#include <iterator>
#include <ctime>
#include <string>
#include "check.hpp"

using B = std::ios_base;
using It = std::istreambuf_iterator<char>;
using TG = std::time_get<char>;

enum Which { wday, mon, year, time_ };

static std::tm run(Which w, const char* in, B::iostate& err, std::string& rest) {
  std::istringstream is(in);
  is.imbue(std::locale::classic());
  const TG& tg = std::use_facet<TG>(is.getloc());
  std::tm t{};
  t.tm_wday = t.tm_mon = t.tm_year = t.tm_hour = t.tm_min = t.tm_sec = -1;
  err = B::goodbit;
  It it;
  switch (w) {
    case wday: it = tg.get_weekday(It(is), It(), is, err, &t); break;
    case mon: it = tg.get_monthname(It(is), It(), is, err, &t); break;
    case year: it = tg.get_year(It(is), It(), is, err, &t); break;
    case time_: it = tg.get_time(It(is), It(), is, err, &t); break;
  }
  rest.clear();
  for (; it != It(); ++it) rest.push_back(*it);
  return t;
}

int main() {
  B::iostate err;
  std::string rest;
  // Footnote 207: date_order "can return no_order in valid locales".
  const auto order = std::use_facet<TG>(std::locale::classic()).date_order();
  CHECK(order == std::time_base::mdy || order == std::time_base::no_order);

  CHECK(run(wday, "Thursday", err, rest).tm_wday == 4 && !(err & B::failbit) && rest.empty());
  CHECK(run(wday, "Thu", err, rest).tm_wday == 4 && !(err & B::failbit));
  CHECK(run(wday, "Thux", err, rest).tm_wday == 4 && err == B::goodbit && rest == "x");
  CHECK(run(wday, "Sun,", err, rest).tm_wday == 0 && err == B::goodbit && rest == ",");
  CHECK(run(wday, "Saturday!", err, rest).tm_wday == 6 && err == B::goodbit && rest == "!");
  run(wday, "Thurs", err, rest);  // continues towards "Thursday" and fails
  CHECK((err & B::failbit) != 0);
  run(wday, "Xyz", err, rest);
  CHECK((err & B::failbit) != 0);

  CHECK(run(mon, "May", err, rest).tm_mon == 4 && !(err & B::failbit));
  CHECK(run(mon, "Mayday", err, rest).tm_mon == 4 && err == B::goodbit && rest == "day");
  CHECK(run(mon, "Junx", err, rest).tm_mon == 5 && err == B::goodbit && rest == "x");
  CHECK(run(mon, "June", err, rest).tm_mon == 5 && !(err & B::failbit));
  CHECK(run(mon, "Jul 4", err, rest).tm_mon == 6 && err == B::goodbit && rest == " 4");
  CHECK(run(mon, "September", err, rest).tm_mon == 8 && !(err & B::failbit));
  CHECK(run(mon, "Sepx", err, rest).tm_mon == 8 && err == B::goodbit && rest == "x");
  run(mon, "Sept", err, rest);  // "Sep" then 't' can match "September": continues and fails
  CHECK((err & B::failbit) != 0);
  run(mon, "Ju", err, rest);
  CHECK((err & B::failbit) != 0);

  CHECK(run(year, "2024", err, rest).tm_year == 124 && !(err & B::failbit));
  CHECK(run(year, "1999-", err, rest).tm_year == 99 && err == B::goodbit && rest == "-");
  run(year, "x", err, rest);
  CHECK((err & B::failbit) != 0);

  std::tm t = run(time_, "23:59:58 rest", err, rest);
  CHECK(t.tm_hour == 23 && t.tm_min == 59 && t.tm_sec == 58 && err == B::goodbit && rest == " rest");
  run(time_, "23:59", err, rest);
  CHECK((err & B::failbit) != 0);
  run(time_, "25:00:00", err, rest);
  CHECK((err & B::failbit) != 0);
  return 0;
}
