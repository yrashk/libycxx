// [locale.time.get.virtuals]/11-14: do_get(s, end, f, err, t, format, modifier) "starts by
// evaluating err = ios_base::goodbit", reads one strptime conversion "formed by concatenating
// '%', the modifier character, when non-NUL, and the format character"; "When s == end
// evaluates to true after reading a character the function evaluates err |= ios_base::eofbit";
// it returns "an iterator pointing immediately beyond the last character recognized as possibly
// part of a valid input sequence". POSIX strptime: numeric fields have a maximum width (%H, %d,
// %m 2, %j 3; %Y stops at a non-digit), leading zeros are permitted but not required;
// %Od, %OH, %EY are the same fields with the alternative representation, which the "C" locale
// does not have; %n and %t match any white space.
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

static std::tm one(const char* in, char fmt, char mod, B::iostate& err, std::string& rest,
                   B::iostate initial = B::goodbit) {
  std::istringstream is(in);
  const TG& tg = std::use_facet<TG>(std::locale::classic());
  std::tm t = sentinel();
  err = initial;
  It it = tg.get(It(is), It(), is, err, &t, fmt, mod);
  rest.assign(it, It());
  return t;
}

int main() {
  B::iostate err;
  std::string rest;
  std::tm t;

  // do_get, one conversion at a time.
  t = one("2024x", 'Y', 0, err, rest);
  CHECK(err == B::goodbit && t.tm_year == 124 && rest == "x");
  t = one("2024", 'Y', 0, err, rest);
  CHECK(err == B::eofbit && t.tm_year == 124 && rest.empty());
  // err starts as goodbit whatever it held before.
  t = one("2024x", 'Y', 0, err, rest, B::failbit | B::eofbit);
  CHECK(err == B::goodbit && t.tm_year == 124);
  t = one("123", 'H', 0, err, rest);  // at most two digits
  CHECK(err == B::goodbit && t.tm_hour == 12 && rest == "3");
  t = one("7:", 'H', 0, err, rest);  // leading zero not required
  CHECK(err == B::goodbit && t.tm_hour == 7 && rest == ":");
  t = one("0012", 'j', 0, err, rest);  // at most three digits; day 1 is tm_yday 0
  CHECK(err == B::goodbit && t.tm_yday == 0 && rest == "2");
  t = one("366", 'j', 0, err, rest);
  CHECK(err == B::eofbit && t.tm_yday == 365);
  t = one("1/", 'm', 0, err, rest);
  CHECK(err == B::goodbit && t.tm_mon == 0 && rest == "/");
  t = one("09.", 'e', 0, err, rest);
  CHECK(err == B::goodbit && t.tm_mday == 9 && rest == ".");
  t = one("6x", 'w', 0, err, rest);
  CHECK(err == B::goodbit && t.tm_wday == 6 && rest == "x");
  t = one("Dec.", 'b', 0, err, rest);
  CHECK(err == B::goodbit && t.tm_mon == 11 && rest == ".");
  t = one("%x", '%', 0, err, rest);
  CHECK(err == B::goodbit && rest == "x");
  t = one(" \t\n5", 'n', 0, err, rest);
  CHECK(err == B::goodbit && rest == "5");
  t = one("  5", 't', 0, err, rest);
  CHECK(err == B::goodbit && rest == "5");
  // The O and E modifiers: the "C" locale has no alternative representation.
  t = one("07x", 'd', 'O', err, rest);
  CHECK(err == B::goodbit && t.tm_mday == 7 && rest == "x");
  t = one("23x", 'H', 'O', err, rest);
  CHECK(err == B::goodbit && t.tm_hour == 23 && rest == "x");
  t = one("1999x", 'Y', 'E', err, rest);
  CHECK(err == B::goodbit && t.tm_year == 99 && rest == "x");
  // Values outside the field: failbit.
  one("24", 'H', 0, err, rest);
  CHECK((err & B::failbit) != 0);
  one("0", 'd', 0, err, rest);
  CHECK((err & B::failbit) != 0);
  one("7", 'w', 0, err, rest);
  CHECK((err & B::failbit) != 0);
  one("", 'Y', 0, err, rest);
  CHECK((err & B::failbit) != 0);
  // A wrong literal for %%.
  one("x", '%', 0, err, rest);
  CHECK((err & B::failbit) != 0);
  return 0;
}
