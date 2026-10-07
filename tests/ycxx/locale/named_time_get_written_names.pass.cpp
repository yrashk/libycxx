// time_get_byname and chrono::parse read the names as strftime writes them, wherever they stand
// in the format: a name padded with white space (glibc's and Darwin's ja_JP write %b as " 6月"
// or " 6") reads after a non-white-space character of the format as well
// ([locale.time.get.virtuals]: %b "the locale's ... month name"; [time.parse] Table 134).
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
  for (int m = 0; m < 12; ++m) {
    std::tm t{};
    t.tm_year = 2019 - 1900;
    t.tm_mon = m;
    t.tm_mday = 7;
    t.tm_hour = 13;
    t.tm_min = 4;
    t.tm_sec = 5;
    t.tm_wday = 3;
    const sys_seconds tp = sys_days(year{2019} / month(unsigned(m + 1)) / 7) + 13h + 4min + 5s;
    for (const char* f : {"%a %b/%e %T %Y", "x%b/%e %T %Y", "%B/%e %T %Y", "x%B/%e %T %Y"}) {
      const std::string text = c_ftime(name, f, t);
      std::istringstream is(text);
      is.imbue(loc);
      std::tm r{};
      std::ios_base::iostate err{};
      const std::string fs = f;
      std::use_facet<std::time_get<char>>(loc).get(is, {}, is, err, &r, fs.data(), fs.data() + fs.size());
      CHECK_SAY((!(err & std::ios_base::failbit) && r.tm_mon == m && r.tm_mday == 7 && r.tm_year == 119),
                "locale %s, format \"%s\", text \"%s\"", name, f, text.c_str());
      if (f[0] == '%' && f[1] == 'a')
        continue; // chrono checks the weekday against the date: the tm's is arbitrary
      std::istringstream c(text);
      c.imbue(loc);
      sys_seconds got{};
      c >> parse(f, got);
      CHECK_SAY((!c.fail() && got == tp), "chrono: locale %s, format \"%s\", text \"%s\"", name, f, text.c_str());
    }
  }
}

int main() {
  check_locale(require_locale("en_US.UTF-8"));
  for (const char* other : {"ja_JP.UTF-8", "ko_KR.UTF-8", "zh_CN.UTF-8", "de_DE.UTF-8", "fr_FR.UTF-8"})
    if (c_has_locale(other))
      check_locale(other);
}
