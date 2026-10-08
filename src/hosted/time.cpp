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
  char* __buf;
  size_t __cap;
  size_t __len = 0;
  void put(char c) {
    if (__len < __cap)
      __buf[__len] = c;
    ++__len;
  }
  void put(const char* s) {
    while (*s)
      put(*s++);
  }
  // v in decimal, at least width digits, padded with pad
  void __number(long long __v, int width, char __pad = '0') {
    char d[24];
    int n = 0;
    const bool __neg = __v < 0;
    unsigned long long __u = __neg ? 0ull - static_cast<unsigned long long>(__v) : static_cast<unsigned long long>(__v);
    do {
      d[n++] = static_cast<char>('0' + __u % 10);
      __u /= 10;
    } while (__u != 0);
    if (__neg)
      put('-');
    for (int i = n + (__neg ? 1 : 0); i < width; ++i)
      put(__pad);
    while (n != 0)
      put(d[--n]);
  }
};

long long floor_div(long long a, long long b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }

// ISO 8601 weeks: the number of weeks in year y and the week-based year/week of t.
int iso_weeks_in(long long y) {
  auto p = [](long long __x) { return ((__x + floor_div(__x, 4) - floor_div(__x, 100) + floor_div(__x, 400)) % 7 + 7) % 7; };
  return 52 + (p(y) == 4 || p(y - 1) == 3 ? 1 : 0);
}
void iso_week(const std::tm* t, long long& year, int& __week) {
  year = t->tm_year + 1900LL;
  const int wday_mon = (t->tm_wday + 6) % 7; // Monday = 0
  __week = (t->tm_yday - wday_mon + 10) / 7;
  if (__week < 1) {
    --year;
    __week = iso_weeks_in(year);
  } else if (__week > iso_weeks_in(year)) {
    ++year;
    __week = 1;
  }
}

void format(out& __o, const std::tm* t, char __spec, char __mod);

void format_pattern(out& __o, const std::tm* t, const char* p) {
  for (; *p; ++p) {
    if (*p == '%' && p[1] != '\0') {
      format(__o, t, p[1], 0);
      ++p;
    } else {
      __o.put(*p);
    }
  }
}

const char* name_or(const char* const* __names, int i, int n) { return i >= 0 && i < n ? __names[i] : "?"; }

void format(out& __o, const std::tm* t, char __spec, char __mod) {
  using __ycxx::__detail::__c_month_names;
  using __ycxx::__detail::__c_weekday_names;
  const long long year = t->tm_year + 1900LL;
  switch (__spec) {
  case 'a':
    __o.put(name_or(__c_weekday_names + 7, t->tm_wday, 7));
    return;
  case 'A':
    __o.put(name_or(__c_weekday_names, t->tm_wday, 7));
    return;
  case 'b':
  case 'h':
    __o.put(name_or(__c_month_names + 12, t->tm_mon, 12));
    return;
  case 'B':
    __o.put(name_or(__c_month_names, t->tm_mon, 12));
    return;
  case 'c':
    format_pattern(__o, t, "%a %b %e %H:%M:%S %Y");
    return;
  case 'C':
    __o.__number(floor_div(year, 100), 2);
    return;
  case 'd':
    __o.__number(t->tm_mday, 2);
    return;
  case 'D':
  case 'x':
    format_pattern(__o, t, "%m/%d/%y");
    return;
  case 'e':
    __o.__number(t->tm_mday, 2, ' ');
    return;
  case 'F':
    format_pattern(__o, t, "%Y-%m-%d");
    return;
  case 'g': {
    long long y;
    int __w;
    iso_week(t, y, __w);
    __o.__number((y < 0 ? -y : y) % 100, 2);
    return;
  }
  case 'G': {
    long long y;
    int __w;
    iso_week(t, y, __w);
    __o.__number(y, 1); // as %Y
    return;
  }
  case 'H':
    __o.__number(t->tm_hour, 2);
    return;
  case 'I':
    __o.__number(t->tm_hour % 12 == 0 ? 12 : t->tm_hour % 12, 2);
    return;
  case 'j':
    __o.__number(t->tm_yday + 1, 3);
    return;
  case 'm':
    __o.__number(t->tm_mon + 1, 2);
    return;
  case 'M':
    __o.__number(t->tm_min, 2);
    return;
  case 'n':
    __o.put('\n');
    return;
  case 'p':
    __o.put(t->tm_hour < 12 ? "AM" : "PM");
    return;
  case 'r':
    format_pattern(__o, t, "%I:%M:%S %p");
    return;
  case 'R':
    format_pattern(__o, t, "%H:%M");
    return;
  case 'S':
    __o.__number(t->tm_sec, 2);
    return;
  case 't':
    __o.put('\t');
    return;
  case 'T':
  case 'X':
    format_pattern(__o, t, "%H:%M:%S");
    return;
  case 'u':
    __o.__number(t->tm_wday == 0 ? 7 : t->tm_wday, 1);
    return;
  case 'U':
    __o.__number((t->tm_yday + 7 - t->tm_wday) / 7, 2);
    return;
  case 'V': {
    long long y;
    int __w;
    iso_week(t, y, __w);
    __o.__number(__w, 2);
    return;
  }
  case 'w':
    __o.__number(t->tm_wday, 1);
    return;
  case 'W':
    __o.__number((t->tm_yday + 7 - (t->tm_wday + 6) % 7) / 7, 2);
    return;
  case 'y':
    __o.__number((year < 0 ? -year : year) % 100, 2);
    return;
  case 'Y':
    __o.__number(year, 1);
    return;
  case 'z':
  case 'Z': {
    char __buf[64];
    const char __f[3] = {'%', __spec, '\0'};
    const size_t n = std::strftime(__buf, sizeof __buf, __f, t);
    for (size_t i = 0; i < n; ++i)
      __o.put(__buf[i]);
    return;
  }
  case '%':
    __o.put('%');
    return;
  default:
    __o.put('%');
    if (__mod)
      __o.put(__mod);
    __o.put(__spec);
    return;
  }
}

} // namespace

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

size_t __time_put_c(char* __buf, size_t __cap, const std::tm* t, char format, char __modifier) noexcept {
  out __o{__buf, __cap};
  const bool __e_ok = format == 'c' || format == 'C' || format == 'x' || format == 'X' || format == 'y' || format == 'Y';
  const bool __o_ok = format == 'd' || format == 'e' || format == 'H' || format == 'I' || format == 'm' ||
                    format == 'M' || format == 'S' || format == 'u' || format == 'U' || format == 'V' ||
                    format == 'w' || format == 'W' || format == 'y';
  if (__modifier != 0 && !((__modifier == 'E' && __e_ok) || (__modifier == 'O' && __o_ok))) {
    __o.put('%');
    __o.put(__modifier);
    __o.put(format);
    return __o.__len;
  }
  ::format(__o, t, format, __modifier);
  return __o.__len;
}

}} // namespace __ycxx::__detail
