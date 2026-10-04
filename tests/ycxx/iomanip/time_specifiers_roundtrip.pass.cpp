// [locale.time.put.virtuals]/1: do_put formats "interpreted identically as the format
// specifiers in the string argument to the standard library function strftime(), except that
// the sequence of characters produced for those specifiers that are described as depending on
// the C locale are instead implementation-defined" (so %a %A %b %B %h %p %c %x %X %r are only
// checked by reading them back). ISO C 7.29.3.5 (strftime): %C year/100, %y year%100, %e day
// with a leading space, %j day of year 001-366, %I 12-hour clock 01-12, %u ISO weekday 1-7
// (Monday 1), %w weekday 0-6, %U / %W week of the year (first Sunday / Monday starts week 1),
// %V / %G / %g ISO 8601 week and week-based year, %D = "%m/%d/%y", %F = "%Y-%m-%d", %T =
// "%H:%M:%S", %R = "%H:%M", %n / %t / %%; the E and O modifiers use the alternative
// representation, if any, which the C locale does not have.
// [locale.time.get.virtuals]/12: do_get reads "those tm members ... corresponding to a
// conversion specification appropriate for the POSIX function strptime"; "When the
// concatenation fails to yield a complete valid directive the function leaves the object
// pointed to by t unchanged and evaluates err |= ios_base::failbit." [locale.time.get.members]
// /8: get(s, end, f, err, t, fmt, fmtend) loops: s == end before fmt is exhausted -> eofbit |
// failbit; '%' directives through do_get; whitespace in fmt skips any whitespace in the input;
// other characters must match case-insensitively, else failbit. POSIX strptime: %b / %B / %h
// accept the abbreviated or the full month name, %a / %A either weekday name, case ignored;
// %y 69-99 -> 1969-1999, 00-68 -> 2000-2068; %I with %p; %S accepts [00,60]; %j sets tm_yday;
// numbers outside the field's range do not match.
// [ext.manip]/8-10: get_time / put_time call time_get::get / time_put::put.
#include <iomanip>
#include <locale>
#include <sstream>
#include <ctime>
#include <string>
#include "check.hpp"

static std::tm make(int y, int mon, int d, int h, int mi, int s, int wday, int yday) {
  std::tm t{};
  t.tm_year = y - 1900;
  t.tm_mon = mon - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  t.tm_wday = wday;
  t.tm_yday = yday;
  return t;
}

static std::string put(const std::tm& t, const char* f) {
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os << std::put_time(&t, f);
  CHECK(os.good());
  return os.str();
}

static std::tm sentinel() {
  std::tm t{};
  t.tm_year = t.tm_mon = t.tm_mday = t.tm_hour = t.tm_min = t.tm_sec = t.tm_wday = t.tm_yday = -99;
  return t;
}

static bool get(const std::string& in, const char* f, std::tm& t, std::ios_base::iostate* st = nullptr) {
  std::istringstream is(in);
  is.imbue(std::locale::classic());
  t = sentinel();
  is >> std::get_time(&t, f);
  if (st) *st = is.rdstate();
  return !is.fail();
}

int main() {
  using B = std::ios_base;
  // Thursday 2024-03-07 09:05:03, day of year 66 (0-based).
  const std::tm t = make(2024, 3, 7, 9, 5, 3, 4, 66);
  CHECK(put(t, "%Y|%C|%y|%m|%d|%e|%j|%H|%I|%M|%S|%u|%w|%U|%W|%V|%G|%g") ==
        "2024|20|24|03|07| 7|067|09|09|05|03|4|4|09|10|10|2024|24");
  CHECK(put(t, "%D|%F|%T|%R|%%|%n|%t|x") == "03/07/24|2024-03-07|09:05:03|09:05|%|\n|\t|x");
  CHECK(put(t, "%EY|%Ey|%EC|%Od|%Oe|%OH|%OI|%Om|%OM|%OS|%Ou|%Ow|%OU|%OW|%OV|%Oy") ==
        "2024|24|20|07| 7|09|09|03|05|03|4|4|09|10|10|24");
  CHECK(put(t, "") == "");

  // 12-hour clock edges.
  CHECK(put(make(2024, 1, 1, 0, 0, 0, 1, 0), "%I") == "12");
  CHECK(put(make(2024, 1, 1, 12, 0, 0, 1, 0), "%I") == "12");
  CHECK(put(make(2024, 1, 1, 23, 0, 0, 1, 0), "%I") == "11");
  // ISO 8601 week-based years at year boundaries: Friday 2021-01-01 is in week 53 of 2020;
  // Monday 2024-12-30 is in week 1 of 2025 (and in %U week 52, %W week 53 of 2024).
  CHECK(put(make(2021, 1, 1, 0, 0, 0, 5, 0), "%G %g %V %U %W %u") == "2020 20 53 00 00 5");
  CHECK(put(make(2024, 12, 30, 0, 0, 0, 1, 364), "%G %g %V %U %W %j") == "2025 25 01 52 53 365");
  CHECK(put(make(10000, 1, 1, 0, 0, 0, 6, 0), "%Y %C") == "10000 100");

  std::tm r;
  B::iostate st;
  // Locale-dependent specifiers: what put produces in the classic locale is read back.
  CHECK(get(put(t, "%a %A %b %B %h"), "%a %A %b %B %h", r) && r.tm_wday == 4 && r.tm_mon == 2);
  CHECK(get(put(t, "%c"), "%c", r) && r.tm_year == 124 && r.tm_mon == 2 && r.tm_mday == 7 &&
        r.tm_hour == 9 && r.tm_min == 5 && r.tm_sec == 3);
  CHECK(get(put(t, "%x"), "%x", r) && r.tm_year == 124 && r.tm_mon == 2 && r.tm_mday == 7);
  CHECK(get(put(t, "%X"), "%X", r) && r.tm_hour == 9 && r.tm_min == 5 && r.tm_sec == 3);
  CHECK(get(put(t, "%r"), "%r", r) && r.tm_hour == 9 && r.tm_min == 5 && r.tm_sec == 3);
  for (int h : {0, 1, 11, 12, 13, 23}) {
    const std::tm th = make(2024, 3, 7, h, 30, 0, 4, 66);
    CHECK(get(put(th, "%I:%M %p"), "%I:%M %p", r) && r.tm_hour == h && r.tm_min == 30);
  }

  // Numeric specifiers.
  CHECK(get(put(t, "%Y-%m-%d %H:%M:%S"), "%Y-%m-%d %H:%M:%S", r) && r.tm_year == 124 &&
        r.tm_mon == 2 && r.tm_mday == 7 && r.tm_hour == 9 && r.tm_min == 5 && r.tm_sec == 3);
  CHECK(get(put(t, "%D %T"), "%D %T", r) && r.tm_year == 124 && r.tm_mon == 2 && r.tm_mday == 7 &&
        r.tm_hour == 9 && r.tm_min == 5 && r.tm_sec == 3);
  CHECK(get(put(t, "%R"), "%R", r) && r.tm_hour == 9 && r.tm_min == 5);
  CHECK(get(put(t, "%j"), "%j", r) && r.tm_yday == 66);
  CHECK(get(put(t, "%w"), "%w", r) && r.tm_wday == 4);
  CHECK(get("69", "%y", r) && r.tm_year == 69);
  CHECK(get("68", "%y", r) && r.tm_year == 168);
  CHECK(get("00", "%y", r) && r.tm_year == 100);
  CHECK(get("23:59:60", "%H:%M:%S", r) && r.tm_sec == 60);
  CHECK(get("7", "%d", r) && r.tm_mday == 7);
  CHECK(get("100%", "%j%%", r) && r.tm_yday == 99);
  CHECK(get("2024\n\t  03", "%Y %m", r) && r.tm_year == 124 && r.tm_mon == 2);
  CHECK(get("2024T03", "%Yt%m", r) && r.tm_mon == 2);  // literal: case-insensitive

  // Names: either form, case ignored.
  CHECK(get("MARCH", "%b", r) && r.tm_mon == 2);
  CHECK(get("mar", "%B", r) && r.tm_mon == 2);
  CHECK(get("thursday", "%a", r) && r.tm_wday == 4);
  CHECK(get("Sun", "%A", r) && r.tm_wday == 0);
  CHECK(get("12:00 am", "%I:%M %p", r) && r.tm_hour == 0);
  CHECK(get("12:00 PM", "%I:%M %p", r) && r.tm_hour == 12);

  // Failures.
  CHECK(!get("13", "%m", r));
  CHECK(!get("00", "%m", r));
  CHECK(!get("32", "%d", r));
  CHECK(!get("24", "%H", r));
  CHECK(!get("60", "%M", r));
  CHECK(!get("13", "%I", r));
  CHECK(!get("367", "%j", r));
  CHECK(!get("Mxr", "%b", r));
  CHECK(!get("2024/03", "%Y-%m", r));
  CHECK(!get("2024-", "%Y-%m", r, &st) && st == (B::eofbit | B::failbit));  // 8.3
  CHECK(!get("", "%Y", r, &st) && (st & B::failbit));
  // ISO C: %C is "the year divided by 100 and truncated to an integer, as a decimal number
  // (00-99)".
  CHECK(put(make(905, 6, 1, 0, 0, 0, 0, 151), "%C %y %Y") == "09 05 905");
  return 0;
}
