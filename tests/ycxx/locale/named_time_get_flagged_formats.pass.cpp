// A named locale whose own formats use the C library's strftime flags reads back what strftime
// writes with them: GNU and BSD strftime accept "%-d" (no padding), "%_m" (space padding), "%k"
// and "%l" (space-padded hours), and locales use them (glibc's aa_DJ: d_fmt "%-d/%-m/%y", t_fmt
// "%l:%M:%S %p"; Darwin's ja_JP: D_T_FMT "%a %_m/%e %T %Y").
// [locale.time.get.virtuals]: %c %x %X %r are "the locale's" representations; [time.parse]
// Table 134 likewise for chrono::parse.
#include <langinfo.h>
#include <time.h>
#include <chrono>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

using namespace std::chrono;

static std::string c_ftime(const char* name, const char* fmt, const std::tm& t) {
  return in_c_locale(name, [&] {
    char b[256];
    const size_t n = strftime(b, sizeof b, fmt, &t);
    return std::string(b, n);
  });
}

static void check_locale(const char* name) {
  const std::locale loc(name);
  const nl_item items[4] = {D_T_FMT, D_FMT, T_FMT, T_FMT_AMPM};
  const char* const convs[4] = {"%c", "%x", "%X", "%r"};
  for (int hour : {1, 13}) {
    std::tm t{};
    t.tm_year = 2026 - 1900;
    t.tm_mon = 2; // a one-digit month and day: the flags change their padding
    t.tm_mday = 7;
    t.tm_hour = hour;
    t.tm_min = 4;
    t.tm_sec = 5;
    t.tm_wday = 6;
    t.tm_yday = 65;
    for (int k = 0; k < 4; ++k) {
      const std::string own = in_c_locale(name, [&] { return std::string(nl_langinfo(items[k])); });
      if (own.empty())
        continue;
      const std::string text = c_ftime(name, convs[k], t);
      std::istringstream is(text);
      is.imbue(loc);
      std::tm r{};
      r.tm_hour = r.tm_min = r.tm_sec = -1;
      std::ios_base::iostate err{};
      const std::string f = convs[k];
      std::use_facet<std::time_get<char>>(loc).get(is, {}, is, err, &r, f.data(), f.data() + f.size());
      const bool date = k < 2, time = k != 1;
      CHECK_SAY((!(err & std::ios_base::failbit) && (!date || (r.tm_mon == 2 && r.tm_mday == 7)) &&
                 (!time || (r.tm_hour == hour && r.tm_min == 4 && r.tm_sec == 5))),
                "locale %s, %s is \"%s\", text \"%s\"", name, convs[k], own.c_str(), text.c_str());
      if (k == 1) { // chrono: the date of %x
        std::istringstream c(text);
        c.imbue(loc);
        year_month_day ymd{};
        c >> parse("%x", ymd);
        CHECK_SAY((!c.fail() && ymd == year{2026} / March / 7), "chrono: locale %s, %%x is \"%s\", text \"%s\"", name,
                  own.c_str(), text.c_str());
      }
      if (k == 2) { // chrono: the time of %X
        std::istringstream c(text);
        c.imbue(loc);
        seconds s{};
        c >> parse("%X", s);
        CHECK_SAY((!c.fail() && s == hours{hour} + 4min + 5s), "chrono: locale %s, %%X is \"%s\", text \"%s\"", name,
                  own.c_str(), text.c_str());
      }
    }
  }
}

int main() {
  check_locale(require_locale("en_US.UTF-8"));
  for (const char* other : {"aa_DJ.UTF-8", "ar_SA.UTF-8", "ja_JP.UTF-8", "de_DE.UTF-8", "bg_BG.UTF-8", "am_ET.UTF-8"})
    if (c_has_locale(other))
      check_locale(other);
}
