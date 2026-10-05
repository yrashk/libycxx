// libycxx hosted runtime: time_put's conversions ([locale.time.put.virtuals]): what strftime
// produces in the "C" locale, computed here so the result does not depend on the C library's
// current locale. %z and %Z (the offset and zone name, which need the C library's time zone
// data) come from strftime itself. The E and O modifiers select the same output in the "C"
// locale. An invalid conversion is written as it appears in the pattern.
#include <ctime>
#include <locale>

namespace {

using std::size_t;

struct out {
  char* buf;
  size_t cap;
  size_t len = 0;
  void put(char c) {
    if (len < cap)
      buf[len] = c;
    ++len;
  }
  void put(const char* s) {
    while (*s)
      put(*s++);
  }
  // v in decimal, at least width digits, padded with pad
  void number(long long v, int width, char pad = '0') {
    char d[24];
    int n = 0;
    const bool neg = v < 0;
    unsigned long long u = neg ? 0ull - static_cast<unsigned long long>(v) : static_cast<unsigned long long>(v);
    do {
      d[n++] = static_cast<char>('0' + u % 10);
      u /= 10;
    } while (u != 0);
    if (neg)
      put('-');
    for (int i = n + (neg ? 1 : 0); i < width; ++i)
      put(pad);
    while (n != 0)
      put(d[--n]);
  }
};

long long floor_div(long long a, long long b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }

// ISO 8601 weeks: the number of weeks in year y and the week-based year/week of t.
int iso_weeks_in(long long y) {
  auto p = [](long long x) { return ((x + floor_div(x, 4) - floor_div(x, 100) + floor_div(x, 400)) % 7 + 7) % 7; };
  return 52 + (p(y) == 4 || p(y - 1) == 3 ? 1 : 0);
}
void iso_week(const std::tm* t, long long& year, int& week) {
  year = t->tm_year + 1900LL;
  const int wday_mon = (t->tm_wday + 6) % 7; // Monday = 0
  week = (t->tm_yday - wday_mon + 10) / 7;
  if (week < 1) {
    --year;
    week = iso_weeks_in(year);
  } else if (week > iso_weeks_in(year)) {
    ++year;
    week = 1;
  }
}

void format(out& o, const std::tm* t, char spec, char mod);

void format_pattern(out& o, const std::tm* t, const char* p) {
  for (; *p; ++p) {
    if (*p == '%' && p[1] != '\0') {
      format(o, t, p[1], 0);
      ++p;
    } else {
      o.put(*p);
    }
  }
}

const char* name_or(const char* const* names, int i, int n) { return i >= 0 && i < n ? names[i] : "?"; }

void format(out& o, const std::tm* t, char spec, char mod) {
  using ycxx::detail::c_month_names;
  using ycxx::detail::c_weekday_names;
  const long long year = t->tm_year + 1900LL;
  switch (spec) {
  case 'a':
    o.put(name_or(c_weekday_names + 7, t->tm_wday, 7));
    return;
  case 'A':
    o.put(name_or(c_weekday_names, t->tm_wday, 7));
    return;
  case 'b':
  case 'h':
    o.put(name_or(c_month_names + 12, t->tm_mon, 12));
    return;
  case 'B':
    o.put(name_or(c_month_names, t->tm_mon, 12));
    return;
  case 'c':
    format_pattern(o, t, "%a %b %e %H:%M:%S %Y");
    return;
  case 'C':
    o.number(floor_div(year, 100), 2);
    return;
  case 'd':
    o.number(t->tm_mday, 2);
    return;
  case 'D':
  case 'x':
    format_pattern(o, t, "%m/%d/%y");
    return;
  case 'e':
    o.number(t->tm_mday, 2, ' ');
    return;
  case 'F':
    format_pattern(o, t, "%Y-%m-%d");
    return;
  case 'g': {
    long long y;
    int w;
    iso_week(t, y, w);
    o.number((y < 0 ? -y : y) % 100, 2);
    return;
  }
  case 'G': {
    long long y;
    int w;
    iso_week(t, y, w);
    o.number(y, 1); // as %Y
    return;
  }
  case 'H':
    o.number(t->tm_hour, 2);
    return;
  case 'I':
    o.number(t->tm_hour % 12 == 0 ? 12 : t->tm_hour % 12, 2);
    return;
  case 'j':
    o.number(t->tm_yday + 1, 3);
    return;
  case 'm':
    o.number(t->tm_mon + 1, 2);
    return;
  case 'M':
    o.number(t->tm_min, 2);
    return;
  case 'n':
    o.put('\n');
    return;
  case 'p':
    o.put(t->tm_hour < 12 ? "AM" : "PM");
    return;
  case 'r':
    format_pattern(o, t, "%I:%M:%S %p");
    return;
  case 'R':
    format_pattern(o, t, "%H:%M");
    return;
  case 'S':
    o.number(t->tm_sec, 2);
    return;
  case 't':
    o.put('\t');
    return;
  case 'T':
  case 'X':
    format_pattern(o, t, "%H:%M:%S");
    return;
  case 'u':
    o.number(t->tm_wday == 0 ? 7 : t->tm_wday, 1);
    return;
  case 'U':
    o.number((t->tm_yday + 7 - t->tm_wday) / 7, 2);
    return;
  case 'V': {
    long long y;
    int w;
    iso_week(t, y, w);
    o.number(w, 2);
    return;
  }
  case 'w':
    o.number(t->tm_wday, 1);
    return;
  case 'W':
    o.number((t->tm_yday + 7 - (t->tm_wday + 6) % 7) / 7, 2);
    return;
  case 'y':
    o.number((year < 0 ? -year : year) % 100, 2);
    return;
  case 'Y':
    o.number(year, 1);
    return;
  case 'z':
  case 'Z': {
    char buf[64];
    const char f[3] = {'%', spec, '\0'};
    const size_t n = std::strftime(buf, sizeof buf, f, t);
    for (size_t i = 0; i < n; ++i)
      o.put(buf[i]);
    return;
  }
  case '%':
    o.put('%');
    return;
  default:
    o.put('%');
    if (mod)
      o.put(mod);
    o.put(spec);
    return;
  }
}

} // namespace

namespace ycxx::detail {

size_t time_put_c(char* buf, size_t cap, const std::tm* t, char format, char modifier) noexcept {
  out o{buf, cap};
  const bool e_ok = format == 'c' || format == 'C' || format == 'x' || format == 'X' || format == 'y' || format == 'Y';
  const bool o_ok = format == 'd' || format == 'e' || format == 'H' || format == 'I' || format == 'm' ||
                    format == 'M' || format == 'S' || format == 'u' || format == 'U' || format == 'V' ||
                    format == 'w' || format == 'W' || format == 'y';
  if (modifier != 0 && !((modifier == 'E' && e_ok) || (modifier == 'O' && o_ok))) {
    o.put('%');
    o.put(modifier);
    o.put(format);
    return o.len;
  }
  ::format(o, t, format, modifier);
  return o.len;
}

} // namespace ycxx::detail
