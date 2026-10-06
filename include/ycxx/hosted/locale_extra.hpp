// libycxx hosted: the time, monetary and message categories ([category.time],
// [category.monetary], [category.messages]), in the "C" locale.
//
// time_put formats as strftime does in the "C" locale, out of line (src/hosted/time.cpp; %z and
// %Z come from the C library's strftime). time_get parses the strptime conversions of the "C"
// locale; two-digit years (%y, and get_year with one or two digits) are 1969-2068.
// The base moneypunct: no decimal point or thousands separator (numeric_limits<charT>::max()),
// no grouping, empty currency symbol and positive sign, negative sign "-", frac_digits() 0,
// patterns { symbol, sign, none, value }. messages has no catalogs: open() returns -1.
#pragma once

#include <ctime>
#include <ycxx/hosted/locale_num.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// time_put stage: the characters strftime produces for "%<modifier><format>" in the "C"
// locale (src/hosted/time.cpp). Writes at most cap characters; returns the full length.
std::size_t __time_put_c(char* __buf, std::size_t __cap, const std::tm* t, char format, char __modifier) noexcept;

inline constexpr const char* __c_weekday_names[14] = {"Sunday",   "Monday", "Tuesday", "Wednesday", "Thursday",
                                                    "Friday",   "Saturday", "Sun",    "Mon",       "Tue",
                                                    "Wed",      "Thu",    "Fri",     "Sat"};
inline constexpr const char* __c_month_names[24] = {
    "January", "February", "March", "April", "May", "June", "July", "August", "September", "October",
    "November", "December", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

constexpr char __ascii_lower(char c) noexcept { return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c; }

// Sets tm_mon and tm_mday from tm_year and tm_yday.
constexpr void __date_from_yday(std::tm& t) noexcept {
  const long long y = t.tm_year + 1900LL;
  const bool __leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
  if (t.tm_yday < 0 || t.tm_yday > (__leap ? 365 : 364))
    return;
  constexpr int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int d = t.tm_yday, m = 0;
  for (; m < 11; ++m) {
    const int __len = days[m] + (m == 1 && __leap ? 1 : 0);
    if (d < __len)
      break;
    d -= __len;
  }
  t.tm_mon = m;
  t.tm_mday = d + 1;
}

// Sets tm_wday (if wday) and tm_yday (if yday) from tm_year, tm_mon and tm_mday (proleptic
// Gregorian calendar); does nothing for a month or day out of range.
constexpr void __complete_date(std::tm& t, bool __wday, bool __yday) noexcept {
  if (t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31)
    return;
  const long long y = t.tm_year + 1900LL;
  const bool __leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
  constexpr int before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  const int __yd = before[t.tm_mon] + t.tm_mday - 1 + (__leap && t.tm_mon > 1 ? 1 : 0);
  if (__yday)
    t.tm_yday = __yd;
  if (__wday) {
    // the weekday of January 1 of y (Gauss), then forward
    const long long p = y - 1;
    const long long __jan1 = (1 + 5 * (((p % 4) + 4) % 4) + 4 * (((p % 100) + 100) % 100) + 6 * (((p % 400) + 400) % 400)) % 7;
    t.tm_wday = static_cast<int>((__jan1 + __yd) % 7);
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [locale.time.get] ------------------------------------------------------------------------
class time_base {
public:
  enum dateorder { no_order, dmy, mdy, ymd, ydm };
};

template <class __charT, class _InputIterator>
class time_get : public locale::facet, public time_base {
public:
  using char_type = __charT;
  using iter_type = _InputIterator;

  explicit time_get(size_t __refs = 0) : locale::facet(__refs) {}

  dateorder date_order() const { return do_date_order(); }
  iter_type get_time(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return do_get_time(s, end, __f, __err, t);
  }
  iter_type get_date(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return do_get_date(s, end, __f, __err, t);
  }
  iter_type get_weekday(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return do_get_weekday(s, end, __f, __err, t);
  }
  iter_type get_monthname(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return do_get_monthname(s, end, __f, __err, t);
  }
  iter_type get_year(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return do_get_year(s, end, __f, __err, t);
  }
  iter_type get(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t, char format,
                char __modifier = 0) const {
    return do_get(s, end, __f, __err, t, format, __modifier);
  }
  // [locale.time.get.members]/7-10
  iter_type get(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t, const char_type* __fmt,
                const char_type* __fmtend) const {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
    __err = ios_base::goodbit;
    // %C and %y together give the year (strptime): the last value of each is kept
    int __century = -1, __year_in_century = -1;
    // as strptime does, a complete date also gives the weekday and the day of the year
    bool __have_year = false, __have_mon = false, __have_mday = false, __have_wday = false, __have_yday = false;
    int __week = 0;
    char __week_kind = 0; // 'U' (weeks from the first Sunday) or 'W' (from the first Monday)
    while (__fmt != __fmtend && __err == ios_base::goodbit) {
      if (s == end) {
        __err = ios_base::eofbit | ios_base::failbit;
        break;
      }
      if (__ct.narrow(*__fmt, 0) == '%') {
        const __charT* p = __fmt + 1;
        if (p == __fmtend) {
          __err = ios_base::failbit;
          break;
        }
        char __mod = 0;
        char __spec = __ct.narrow(*p, 0);
        if (__spec == 'E' || __spec == 'O') {
          __mod = __spec;
          if (++p == __fmtend) {
            __err = ios_base::failbit;
            break;
          }
          __spec = __ct.narrow(*p, 0);
        }
        if (__spec == 'U' || __spec == 'W') {
          // read here (as do_get would) to keep the week number for the date below
          s = __read_ranged(s, end, __f, __err, 2, 0, 53, __week);
          __week_kind = __spec;
        } else {
          s = do_get(s, end, __f, __err, t, __spec, __mod);
        }
        if (!(__err & ios_base::failbit)) {
          if (__spec == 'C')
            __century = (t->tm_year + 1900) / 100;
          else if (__spec == 'y')
            __year_in_century = (t->tm_year + 1900) % 100;
          for (const char* __q = "CyYcDxF"; *__q; ++__q)
            __have_year = __have_year || __spec == *__q;
          for (const char* __q = "mbBhcDxF"; *__q; ++__q)
            __have_mon = __have_mon || __spec == *__q;
          for (const char* __q = "decDxF"; *__q; ++__q)
            __have_mday = __have_mday || __spec == *__q;
          for (const char* __q = "aAuwc"; *__q; ++__q)
            __have_wday = __have_wday || __spec == *__q;
          __have_yday = __have_yday || __spec == 'j';
        }
        if (__err == ios_base::goodbit)
          __fmt = p + 1;
      } else if (__ct.is(ctype_base::space, *__fmt)) {
        while (__fmt != __fmtend && __ct.is(ctype_base::space, *__fmt))
          ++__fmt;
        while (s != end && __ct.is(ctype_base::space, *s))
          ++s;
      } else if (__ct.toupper(*s) == __ct.toupper(*__fmt)) {
        ++__fmt;
        ++s;
      } else {
        __err = ios_base::failbit;
      }
    }
    if (!(__err & ios_base::failbit)) {
      if (__century >= 0 && __year_in_century >= 0)
        t->tm_year = __century * 100 + __year_in_century - 1900;
      if (__have_year && __week_kind != 0 && __have_wday && !__have_yday && !(__have_mon && __have_mday)) {
        // the day of the year from the week number and the weekday
        tm __jan1{};
        __jan1.tm_year = t->tm_year;
        __jan1.tm_mday = 1;
        ::__ycxx::__detail::__complete_date(__jan1, true, false);
        const int __j = __jan1.tm_wday;
        t->tm_yday = __week_kind == 'U' ? (7 - __j) % 7 + (__week - 1) * 7 + t->tm_wday
                                      : (8 - __j) % 7 + (__week - 1) * 7 + (t->tm_wday + 6) % 7;
        __have_yday = true;
      }
      if (__have_year && __have_yday && !(__have_mon && __have_mday))
        ::__ycxx::__detail::__date_from_yday(*t);
      if (__have_year && ((__have_mon && __have_mday) || __have_yday))
        ::__ycxx::__detail::__complete_date(*t, !__have_wday, !__have_yday);
    }
    return s;
  }

  static locale::id id;

protected:
  ~time_get() override {}
  virtual dateorder do_date_order() const { return mdy; }
  virtual iter_type do_get_time(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return parse(s, end, __f, __err, t, "%H:%M:%S");
  }
  virtual iter_type do_get_date(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    // [locale.time.get.virtuals]/4: what time_put produces for "%d%m%y" (or the order's
    // permutation), which has no separators; /5: other formats may be accepted too, here the
    // same fields separated by '/' ('?' in the pattern: an optional '/').
    switch (date_order()) {
    case dmy:
      return parse(s, end, __f, __err, t, "%d?%m?%y");
    case ymd:
      return parse(s, end, __f, __err, t, "%y?%m?%d");
    case ydm:
      return parse(s, end, __f, __err, t, "%y?%d?%m");
    default:
      return parse(s, end, __f, __err, t, "%m?%d?%y");
    }
  }
  virtual iter_type do_get_weekday(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    int __v;
    s = __match_name(s, end, __f, __err, __ycxx::__detail::__c_weekday_names, 14, __v);
    if (!(__err & ios_base::failbit))
      t->tm_wday = __v % 7;
    return s;
  }
  virtual iter_type do_get_monthname(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    int __v;
    s = __match_name(s, end, __f, __err, __ycxx::__detail::__c_month_names, 24, __v);
    if (!(__err & ios_base::failbit))
      t->tm_mon = __v % 12;
    return s;
  }
  virtual iter_type do_get_year(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    int y, digits;
    s = __read_number(s, end, __f, __err, 4, y, &digits);
    if (!(__err & ios_base::failbit))
      t->tm_year = (digits <= 2 ? (y < 69 ? y + 2000 : y + 1900) : y) - 1900;
    return s;
  }
  // [locale.time.get.virtuals]/11-15: one strptime conversion.
  virtual iter_type do_get(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t, char format,
                           char __modifier) const {
    __err = ios_base::goodbit;
    if (__modifier == 'E' && format != 'c' && format != 'C' && format != 'x' && format != 'X' && format != 'y' &&
        format != 'Y') {
      __err |= ios_base::failbit;
      return s;
    }
    if (__modifier == 'O' && !(format == 'd' || format == 'e' || format == 'H' || format == 'I' || format == 'm' ||
                             format == 'M' || format == 'S' || format == 'U' || format == 'w' || format == 'W' ||
                             format == 'y')) {
      __err |= ios_base::failbit;
      return s;
    }
    if (__modifier != 0 && __modifier != 'E' && __modifier != 'O') {
      __err |= ios_base::failbit;
      return s;
    }
    tm r = *t; // assigned to *t only on success
    int __v = 0;
    switch (format) {
    case 'a':
    case 'A':
      s = do_get_weekday(s, end, __f, __err, &r);
      break;
    case 'b':
    case 'B':
    case 'h':
      s = do_get_monthname(s, end, __f, __err, &r);
      break;
    case 'c':
      s = parse(s, end, __f, __err, &r, "%a %b %e %H:%M:%S %Y");
      break;
    case 'C':
      s = __read_number(s, end, __f, __err, 2, __v);
      if (!(__err & ios_base::failbit))
        r.tm_year = __v * 100 - 1900 + (r.tm_year + 1900) % 100;
      break;
    case 'd':
    case 'e':
      s = __skip_space(s, end, __f, __err);
      s = __read_ranged(s, end, __f, __err, 2, 1, 31, r.tm_mday);
      break;
    case 'D':
    case 'x':
      s = parse(s, end, __f, __err, &r, "%m/%d/%y");
      break;
    case 'F':
      s = parse(s, end, __f, __err, &r, "%Y-%m-%d");
      break;
    case 'H':
      s = __read_ranged(s, end, __f, __err, 2, 0, 23, r.tm_hour);
      break;
    case 'I':
      s = __read_ranged(s, end, __f, __err, 2, 1, 12, __v);
      if (!(__err & ios_base::failbit))
        r.tm_hour = __v % 12 + (r.tm_hour >= 12 ? 12 : 0);
      break;
    case 'j':
      s = __read_ranged(s, end, __f, __err, 3, 1, 366, __v);
      if (!(__err & ios_base::failbit))
        r.tm_yday = __v - 1;
      break;
    case 'm':
      s = __read_ranged(s, end, __f, __err, 2, 1, 12, __v);
      if (!(__err & ios_base::failbit))
        r.tm_mon = __v - 1;
      break;
    case 'M':
      s = __read_ranged(s, end, __f, __err, 2, 0, 59, r.tm_min);
      break;
    case 'n':
    case 't':
      s = __skip_space(s, end, __f, __err);
      break;
    case 'p':
      s = __read_ampm(s, end, __f, __err, r);
      break;
    case 'r':
      s = parse(s, end, __f, __err, &r, "%I:%M:%S %p");
      break;
    case 'R':
      s = parse(s, end, __f, __err, &r, "%H:%M");
      break;
    case 'S':
      s = __read_ranged(s, end, __f, __err, 2, 0, 60, r.tm_sec);
      break;
    case 'T':
    case 'X':
      s = parse(s, end, __f, __err, &r, "%H:%M:%S");
      break;
    case 'u':
      s = __read_ranged(s, end, __f, __err, 1, 1, 7, __v);
      if (!(__err & ios_base::failbit))
        r.tm_wday = __v % 7;
      break;
    case 'U':
    case 'W':
    case 'V':
      s = __read_ranged(s, end, __f, __err, 2, 0, 53, __v);
      break;
    case 'w':
      s = __read_ranged(s, end, __f, __err, 1, 0, 6, r.tm_wday);
      break;
    case 'y':
      s = __read_ranged(s, end, __f, __err, 2, 0, 99, __v);
      if (!(__err & ios_base::failbit))
        r.tm_year = __v < 69 ? __v + 100 : __v;
      break;
    case 'Y':
      s = __read_number(s, end, __f, __err, 4, __v);
      if (!(__err & ios_base::failbit))
        r.tm_year = __v - 1900;
      break;
    case '%':
      if (s == end)
        __err |= ios_base::eofbit | ios_base::failbit;
      else if (use_facet<ctype<__charT>>(__f.getloc()).narrow(*s, 0) == '%') {
        if (++s == end)
          __err |= ios_base::eofbit;
      } else
        __err |= ios_base::failbit;
      break;
    default:
      __err |= ios_base::failbit;
      break;
    }
    if (!(__err & ios_base::failbit))
      *t = r;
    return s;
  }

private:
  // Parses the char format fmt (conversions and literal characters) with do_get.
  iter_type parse(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t, const char* __fmt) const {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
    ios_base::iostate e = ios_base::goodbit;
    for (; *__fmt && !(e & ios_base::failbit); ++__fmt) {
      if (*__fmt == '%') {
        ++__fmt;
        ios_base::iostate __one = ios_base::goodbit;
        s = do_get(s, end, __f, __one, t, *__fmt, 0);
        e |= __one;
      } else if (*__fmt == ' ') {
        while (s != end && __ct.is(ctype_base::space, *s))
          ++s;
      } else if (*__fmt == '?') { // an optional '/' (do_get_date)
        if (s != end && __ct.narrow(*s, 0) == '/')
          ++s;
      } else if (s == end) {
        e |= ios_base::eofbit | ios_base::failbit;
      } else if (__ct.narrow(*s, 0) == *__fmt) {
        ++s;
      } else {
        e |= ios_base::failbit;
      }
    }
    if (s == end)
      e |= ios_base::eofbit;
    __err |= e;
    return s;
  }
  static iter_type __skip_space(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err) {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
    while (s != end && __ct.is(ctype_base::space, *s))
      ++s;
    if (s == end)
      __err |= ios_base::eofbit;
    return s;
  }
  // Up to max_digits decimal digits (at least one); with hi >= 0, a digit that would take the
  // value above hi is an error and is not read ("32" for a day of the month stops at "2").
  static iter_type __read_number(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, int __max_digits, int& __v,
                               int* __ndigits = nullptr, int __hi = -1) {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
    int n = 0;
    __v = 0;
    for (; n < __max_digits; ++n, static_cast<void>(++s)) {
      if (s == end) {
        __err |= ios_base::eofbit;
        break;
      }
      const char c = __ct.narrow(*s, 0);
      if (c < '0' || c > '9')
        break;
      if (__hi >= 0 && n != 0 && __v * 10 + (c - '0') > __hi) {
        __err |= ios_base::failbit;
        break;
      }
      __v = __v * 10 + (c - '0');
    }
    if (n == 0)
      __err |= ios_base::failbit;
    else if (n == __max_digits && s == end)
      __err |= ios_base::eofbit;
    if (__ndigits)
      *__ndigits = n;
    return s;
  }
  static iter_type __read_ranged(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, int __max_digits,
                               int __lo, int __hi, int& out) {
    int __v;
    s = __read_number(s, end, __f, __err, __max_digits, __v, nullptr, __hi);
    if (!(__err & ios_base::failbit)) {
      if (__v < __lo || __v > __hi)
        __err |= ios_base::failbit;
      else
        out = __v;
    }
    return s;
  }
  static iter_type __read_ampm(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm& r) {
    static constexpr const char* __names[2] = {"AM", "PM"};
    int __v;
    s = __match_name(s, end, __f, __err, __names, 2, __v);
    if (!(__err & ios_base::failbit)) {
      r.tm_hour %= 12;
      if (__v == 1)
        r.tm_hour += 12;
    }
    return s;
  }
  // Matches one of names[0..n) (case-insensitively), reading characters only while some name
  // can still be extended; the match is the name equal to everything read.
  static iter_type __match_name(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err,
                              const char* const* __names, int n, int& __which) {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
    bool __alive[24];
    for (int k = 0; k < n; ++k)
      __alive[k] = true;
    size_t i = 0;
    for (;;) {
      bool __more = false;
      for (int k = 0; k < n; ++k)
        __more = __more || (__alive[k] && __names[k][i] != '\0');
      if (!__more)
        break;
      if (s == end) {
        __err |= ios_base::eofbit;
        break;
      }
      const char c = __ycxx::__detail::__ascii_lower(__ct.narrow(*s, 0));
      bool any = false;
      for (int k = 0; k < n; ++k)
        any = any || (__alive[k] && __names[k][i] != '\0' && __ycxx::__detail::__ascii_lower(__names[k][i]) == c);
      if (!any)
        break; // nothing extends: the names alive so far that end here are the candidates
      for (int k = 0; k < n; ++k)
        __alive[k] = __alive[k] && __names[k][i] != '\0' && __ycxx::__detail::__ascii_lower(__names[k][i]) == c;
      ++s;
      ++i;
    }
    if (s == end)
      __err |= ios_base::eofbit;
    __which = -1;
    for (int k = 0; k < n; ++k)
      if (__alive[k] && __names[k][i] == '\0' && i != 0) {
        __which = k;
        break;
      }
    if (__which < 0)
      __err |= ios_base::failbit;
    return s;
  }
};
template <class __charT, class _InputIterator>
locale::id time_get<__charT, _InputIterator>::id;

// [locale.time.get.byname]
template <class __charT, class _InputIterator>
class time_get_byname : public time_get<__charT, _InputIterator> {
public:
  using dateorder = time_base::dateorder;
  using iter_type = _InputIterator;
  explicit time_get_byname(const char* name, size_t __refs = 0) : time_get<__charT, _InputIterator>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::time_get_byname");
  }
  explicit time_get_byname(const string& name, size_t __refs = 0) : time_get_byname(name.c_str(), __refs) {}

protected:
  ~time_get_byname() override {}
};

// ---- [locale.time.put] ------------------------------------------------------------------------
template <class __charT, class _OutputIterator>
class time_put : public locale::facet {
public:
  using char_type = __charT;
  using iter_type = _OutputIterator;

  explicit time_put(size_t __refs = 0) : locale::facet(__refs) {}

  // [locale.time.put.members]/1
  iter_type put(iter_type s, ios_base& str, char_type fill, const tm* t, const __charT* pattern,
                const __charT* __pat_end) const {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(str.getloc());
    while (pattern != __pat_end) {
      if (__ct.narrow(*pattern, 0) != '%' || pattern + 1 == __pat_end) {
        *s = *pattern++;
        ++s;
        continue;
      }
      const __charT* p = pattern + 1;
      char __mod = 0;
      char __spec = __ct.narrow(*p, 0);
      if ((__spec == 'E' || __spec == 'O') && p + 1 != __pat_end) {
        __mod = __spec;
        __spec = __ct.narrow(*++p, 0);
      }
      s = do_put(s, str, fill, t, __spec, __mod);
      pattern = p + 1;
    }
    return s;
  }
  iter_type put(iter_type s, ios_base& str, char_type fill, const tm* t, char format, char __modifier = 0) const {
    return do_put(s, str, fill, t, format, __modifier);
  }

  static locale::id id;

protected:
  ~time_put() override {}
  virtual iter_type do_put(iter_type s, ios_base& str, char_type, const tm* t, char format, char __modifier) const {
    char __y_local[128];
    size_t n = __ycxx::__detail::__time_put_c(__y_local, sizeof __y_local, t, format, __modifier);
    __ycxx::__detail::__small_buffer<char, 1> big(n > sizeof __y_local ? n : 0);
    const char* __text = __y_local;
    if (n > sizeof __y_local) {
      n = __ycxx::__detail::__time_put_c(big.get(), n, t, format, __modifier);
      __text = big.get();
    }
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(str.getloc());
    for (size_t i = 0; i < n; ++i, static_cast<void>(++s))
      *s = __ct.widen(__text[i]);
    return s;
  }
};
template <class __charT, class _OutputIterator>
locale::id time_put<__charT, _OutputIterator>::id;

// [locale.time.put.byname]
template <class __charT, class _OutputIterator>
class time_put_byname : public time_put<__charT, _OutputIterator> {
public:
  using char_type = __charT;
  using iter_type = _OutputIterator;
  explicit time_put_byname(const char* name, size_t __refs = 0) : time_put<__charT, _OutputIterator>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::time_put_byname");
  }
  explicit time_put_byname(const string& name, size_t __refs = 0) : time_put_byname(name.c_str(), __refs) {}

protected:
  ~time_put_byname() override {}
};

// ---- [locale.moneypunct] ----------------------------------------------------------------------
class money_base {
public:
  enum part { none, space, symbol, sign, value };
  struct pattern {
    char field[4];
  };
};

template <class __charT, bool _International>
class moneypunct : public locale::facet, public money_base {
public:
  using char_type = __charT;
  using string_type = basic_string<__charT>;

  explicit moneypunct(size_t __refs = 0) : locale::facet(__refs) {}

  __charT decimal_point() const { return do_decimal_point(); }
  __charT thousands_sep() const { return do_thousands_sep(); }
  string grouping() const { return do_grouping(); }
  string_type curr_symbol() const { return do_curr_symbol(); }
  string_type positive_sign() const { return do_positive_sign(); }
  string_type negative_sign() const { return do_negative_sign(); }
  int frac_digits() const { return do_frac_digits(); }
  pattern pos_format() const { return do_pos_format(); }
  pattern neg_format() const { return do_neg_format(); }

  static locale::id id;
  static const bool intl = _International;

protected:
  ~moneypunct() override {}
  virtual __charT do_decimal_point() const { return numeric_limits<__charT>::max(); }
  virtual __charT do_thousands_sep() const { return numeric_limits<__charT>::max(); }
  virtual string do_grouping() const { return string(); }
  virtual string_type do_curr_symbol() const { return string_type(); }
  virtual string_type do_positive_sign() const { return string_type(); }
  virtual string_type do_negative_sign() const { return string_type(1, __charT('-')); }
  virtual int do_frac_digits() const { return 0; }
  virtual pattern do_pos_format() const { return pattern{{symbol, sign, none, value}}; }
  virtual pattern do_neg_format() const { return pattern{{symbol, sign, none, value}}; }
};
template <class __charT, bool _International>
locale::id moneypunct<__charT, _International>::id;
template <class __charT, bool _International>
const bool moneypunct<__charT, _International>::intl;

// [locale.moneypunct.byname]
template <class __charT, bool _Intl>
class moneypunct_byname : public moneypunct<__charT, _Intl> {
public:
  using pattern = money_base::pattern;
  using string_type = basic_string<__charT>;
  explicit moneypunct_byname(const char* name, size_t __refs = 0) : moneypunct<__charT, _Intl>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::moneypunct_byname");
  }
  explicit moneypunct_byname(const string& name, size_t __refs = 0) : moneypunct_byname(name.c_str(), __refs) {}

protected:
  ~moneypunct_byname() override {}
};

// ---- [locale.money.get] -----------------------------------------------------------------------
template <class __charT, class _InputIterator>
class money_get : public locale::facet {
public:
  using char_type = __charT;
  using iter_type = _InputIterator;
  using string_type = basic_string<__charT>;

  explicit money_get(size_t __refs = 0) : locale::facet(__refs) {}

  iter_type get(iter_type s, iter_type end, bool intl, ios_base& __f, ios_base::iostate& __err, long double& __units) const {
    return do_get(s, end, intl, __f, __err, __units);
  }
  iter_type get(iter_type s, iter_type end, bool intl, ios_base& __f, ios_base::iostate& __err,
                string_type& digits) const {
    return do_get(s, end, intl, __f, __err, digits);
  }

  static locale::id id;

protected:
  ~money_get() override {}
  virtual iter_type do_get(iter_type s, iter_type end, bool intl, ios_base& str, ios_base::iostate& __err,
                           long double& __units) const {
    string field; // '-'? digits, as char
    bool ok = false;
    s = intl ? parse<true>(s, end, str, __err, field, ok) : parse<false>(s, end, str, __err, field, ok);
    if (ok) {
      long double __v = 0;
      if (__ycxx::__detail::__num_get_float(field.data(), field.size(), &__v) == __ycxx::__detail::__num_parse::__not_converted)
        __err |= ios_base::failbit;
      else
        __units = __v;
    }
    return s;
  }
  virtual iter_type do_get(iter_type s, iter_type end, bool intl, ios_base& str, ios_base::iostate& __err,
                           string_type& digits) const {
    string field;
    bool ok = false;
    s = intl ? parse<true>(s, end, str, __err, field, ok) : parse<false>(s, end, str, __err, field, ok);
    if (ok) {
      const ctype<__charT>& __ct = use_facet<ctype<__charT>>(str.getloc());
      string_type r(field.size(), __charT());
      __ct.widen(field.data(), field.data() + field.size(), r.data());
      digits = static_cast<string_type&&>(r);
    }
    return s;
  }

private:
  // [locale.money.get.virtuals]/1-4: on success field holds the result as '-'? digits and ok is
  // true; otherwise failbit (and eofbit at the end of input) is set in err.
  template <bool _Intl>
  static iter_type parse(iter_type s, iter_type end, ios_base& str, ios_base::iostate& __err, string& field, bool& ok) {
    const locale __loc = str.getloc();
    const moneypunct<__charT, _Intl>& __mp = use_facet<moneypunct<__charT, _Intl>>(__loc);
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__loc);
    const money_base::pattern __pat = __mp.neg_format();
    const string_type __pos = __mp.positive_sign(), __neg = __mp.negative_sign(), __sym = __mp.curr_symbol();
    const string grouping = __mp.grouping();
    const __charT __point = __mp.decimal_point(), __sep = __mp.thousands_sep();
    const int __frac = __mp.frac_digits();
    const bool showbase = (str.flags() & ios_base::showbase) != 0;

    const string_type* sign = nullptr; // the sign string matched (or implied)
    string digits;
    unsigned __groups[64];
    size_t __ngroups = 0;
    unsigned run = 0;
    bool __seen_sep = false;
    bool failed = false;

    auto __at_end = [&] { return s == end; };
    for (int i = 0; i < 4 && !failed; ++i) {
      switch (static_cast<money_base::part>(__pat.field[i])) {
      case money_base::none:
      case money_base::space:
        if (i == 3)
          break;
        if (__pat.field[i] == money_base::space) {
          if (__at_end() || !__ct.is(ctype_base::space, *s)) {
            failed = true;
            break;
          }
        }
        while (!__at_end() && __ct.is(ctype_base::space, *s))
          ++s;
        break;
      case money_base::symbol: {
        // without showbase the symbol is optional, consumed only if more of the format follows
        const bool __needed = showbase || (i < 3 && !(i == 2 && __pat.field[3] == money_base::none)) ||
                            (sign != nullptr && sign->size() > 1);
        if (__sym.empty())
          break;
        if (!__needed && !showbase)
          break;
        size_t k = 0;
        for (; k < __sym.size(); ++k, static_cast<void>(++s)) {
          if (__at_end() || !(*s == __sym[k]))
            break;
        }
        if (k != __sym.size() && (showbase || k != 0))
          failed = true;
        break;
      }
      case money_base::sign:
        if (!__pos.empty() && !__at_end() && *s == __pos[0]) {
          sign = &__pos;
          ++s;
        } else if (!__neg.empty() && !__at_end() && *s == __neg[0]) {
          sign = &__neg;
          ++s;
        } else if (__pos.empty()) {
          sign = &__pos;
        } else if (__neg.empty()) {
          sign = &__neg;
        } else {
          failed = true;
        }
        break;
      case money_base::value: {
        bool __point_seen = false;
        int __frac_got = 0;
        while (!__at_end()) {
          const __charT c = *s;
          if (__ct.is(ctype_base::digit, c)) {
            if (__point_seen) {
              if (__frac_got == __frac)
                break;
              ++__frac_got;
            } else {
              ++run;
            }
            digits.push_back(__ct.narrow(c, '0'));
          } else if (__frac > 0 && !__point_seen && c == __point) {
            __point_seen = true;
          } else if (!grouping.empty() && !__point_seen && c == __sep) {
            if (__ngroups < 64)
              __groups[__ngroups++] = run;
            run = 0;
            __seen_sep = true;
          } else {
            break;
          }
          ++s;
        }
        // [locale.moneypunct.general]/3: a decimal point must be followed by exactly
        // frac_digits() digits; without one, the digits are stored as they appear
        if (digits.empty() || (__point_seen && __frac_got != __frac)) {
          failed = true;
          break;
        }
        break;
      }
      }
    }
    // the rest of a multi-character sign
    if (!failed && sign != nullptr && sign->size() > 1) {
      for (size_t k = 1; k < sign->size(); ++k, static_cast<void>(++s)) {
        if (__at_end() || !(*s == (*sign)[k])) {
          failed = true;
          break;
        }
      }
    }
    if (!failed && __seen_sep) {
      if (__ngroups < 64) {
        __groups[__ngroups++] = run;
        failed = !__ycxx::__detail::__num_grouping_ok(grouping, __groups, __ngroups);
      } else {
        failed = true;
      }
    }
    if (failed) {
      __err |= ios_base::failbit;
      if (__at_end())
        __err |= ios_base::eofbit;
      return s;
    }
    // [locale.money.get.virtuals]/3: equal first characters, or both empty, mean positive
    const bool __negative =
        sign == &__neg && !(__pos.empty() && __neg.empty()) && !(!__pos.empty() && !__neg.empty() && __pos[0] == __neg[0]);
    size_t __lead = 0;
    while (__lead + 1 < digits.size() && digits[__lead] == '0')
      ++__lead;
    field.clear();
    if (__negative)
      field.push_back('-');
    field.append(digits, __lead);
    ok = true;
    if (__at_end())
      __err |= ios_base::eofbit;
    return s;
  }
};
template <class __charT, class _InputIterator>
locale::id money_get<__charT, _InputIterator>::id;

// ---- [locale.money.put] -----------------------------------------------------------------------
template <class __charT, class _OutputIterator>
class money_put : public locale::facet {
public:
  using char_type = __charT;
  using iter_type = _OutputIterator;
  using string_type = basic_string<__charT>;

  explicit money_put(size_t __refs = 0) : locale::facet(__refs) {}

  iter_type put(iter_type s, bool intl, ios_base& __f, char_type fill, long double __units) const {
    return do_put(s, intl, __f, fill, __units);
  }
  iter_type put(iter_type s, bool intl, ios_base& __f, char_type fill, const string_type& digits) const {
    return do_put(s, intl, __f, fill, digits);
  }

  static locale::id id;

protected:
  ~money_put() override {}
  virtual iter_type do_put(iter_type s, bool intl, ios_base& str, char_type fill, long double __units) const {
    // "%.0Lf"
    char __y_local[64];
    size_t __pad = 0;
    size_t n = __ycxx::__detail::__num_put_float(__y_local, sizeof __y_local, __units, ios_base::fixed, 0, &__pad);
    __ycxx::__detail::__small_buffer<char, 1> big(n > sizeof __y_local ? n : 0);
    const char* __text = __y_local;
    if (n > sizeof __y_local) {
      n = __ycxx::__detail::__num_put_float(big.get(), n, __units, ios_base::fixed, 0, &__pad);
      __text = big.get();
    }
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(str.getloc());
    string_type digits(n, __charT());
    __ct.widen(__text, __text + n, digits.data());
    return do_put(s, intl, str, fill, digits);
  }
  virtual iter_type do_put(iter_type s, bool intl, ios_base& str, char_type fill, const string_type& digits) const {
    return intl ? format<true>(s, str, fill, digits) : format<false>(s, str, fill, digits);
  }

private:
  template <bool _Intl>
  static iter_type format(iter_type s, ios_base& str, char_type fill, const string_type& digits) {
    const locale __loc = str.getloc();
    const moneypunct<__charT, _Intl>& __mp = use_facet<moneypunct<__charT, _Intl>>(__loc);
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__loc);
    const bool __negative = !digits.empty() && digits[0] == __ct.widen('-');
    // the digits: after the optional minus sign, up to the first non-digit
    size_t first = __negative ? 1 : 0, last = first;
    while (last < digits.size() && __ct.is(ctype_base::digit, digits[last]))
      ++last;
    const money_base::pattern __pat = __negative ? __mp.neg_format() : __mp.pos_format();
    const string_type sign = __negative ? __mp.negative_sign() : __mp.positive_sign();
    const string_type __sym = __mp.curr_symbol();
    const int __frac = __mp.frac_digits() > 0 ? __mp.frac_digits() : 0;
    const string grouping = __mp.grouping();

    // the value: integer digits (grouped), the decimal point and frac fraction digits
    const size_t __ndig = last - first;
    const size_t __nint = __ndig > static_cast<size_t>(__frac) ? __ndig - __frac : 0;
    size_t __marks[64]; // separator positions, counted in digits from the right
    size_t __nmarks = 0;
    for (size_t i = 0, at = 0; !grouping.empty() && __nint != 0; ++i) {
      const char __g = grouping[i < grouping.size() ? i : grouping.size() - 1];
      if (static_cast<signed char>(__g) <= 0 || __g == numeric_limits<char>::max())
        break;
      at += static_cast<unsigned char>(__g);
      if (at >= __nint || __nmarks == 64)
        break;
      __marks[__nmarks++] = at;
    }
    const size_t __value_len = (__nint == 0 ? 1 : __nint + __nmarks) + (__frac > 0 ? 1 + static_cast<size_t>(__frac) : 0);
    const size_t __cap = __value_len + __sym.size() + sign.size() + 2;
    __ycxx::__detail::__small_buffer<__charT, 128> __buffer(__cap);
    __charT* const out = __buffer.get();
    size_t __len = 0;
    size_t __pad_at = __cap; // where internal padding goes (cap: none)
    for (int __f = 0; __f < 4; ++__f) {
      switch (static_cast<money_base::part>(__pat.field[__f])) {
      case money_base::none:
        __pad_at = __len;
        break;
      case money_base::space:
        __pad_at = __len;
        out[__len++] = __ct.widen(' ');
        break;
      case money_base::symbol:
        if (str.flags() & ios_base::showbase)
          for (__charT c : __sym)
            out[__len++] = c;
        break;
      case money_base::sign:
        if (!sign.empty())
          out[__len++] = sign[0];
        break;
      case money_base::value:
        if (__nint == 0) {
          out[__len++] = __ct.widen('0');
        } else {
          for (size_t i = 0; i < __nint; ++i) {
            for (size_t m = 0; m < __nmarks; ++m)
              if (i != 0 && __marks[m] == __nint - i)
                out[__len++] = __mp.thousands_sep();
            out[__len++] = digits[first + i];
          }
        }
        if (__frac > 0) {
          out[__len++] = __mp.decimal_point();
          for (size_t k = static_cast<size_t>(__frac); k > __ndig; --k)
            out[__len++] = __ct.widen('0');
          for (size_t i = __nint; i < __ndig; ++i)
            out[__len++] = digits[first + i];
        }
        break;
      }
    }
    for (size_t k = 1; k < sign.size(); ++k)
      out[__len++] = sign[k];

    const streamsize width = str.width();
    str.width(0);
    size_t __fill_count = width > 0 && static_cast<size_t>(width) > __len ? static_cast<size_t>(width) - __len : 0;
    const ios_base::fmtflags __adjust = str.flags() & ios_base::adjustfield;
    size_t where = 0;
    if (__adjust == ios_base::internal && __pad_at != __cap)
      where = __pad_at;
    else if (__adjust == ios_base::left)
      where = __len;
    for (size_t i = 0; i < where; ++i, static_cast<void>(++s))
      *s = out[i];
    for (; __fill_count != 0; --__fill_count, static_cast<void>(++s))
      *s = fill;
    for (size_t i = where; i < __len; ++i, static_cast<void>(++s))
      *s = out[i];
    return s;
  }
};
template <class __charT, class _OutputIterator>
locale::id money_put<__charT, _OutputIterator>::id;

// ---- [locale.messages] ------------------------------------------------------------------------
class messages_base {
public:
  using catalog = int;
};

template <class __charT>
class messages : public locale::facet, public messages_base {
public:
  using char_type = __charT;
  using string_type = basic_string<__charT>;

  explicit messages(size_t __refs = 0) : locale::facet(__refs) {}

  catalog open(const string& __fn, const locale& __loc) const { return do_open(__fn, __loc); }
  string_type get(catalog c, int set, int __msgid, const string_type& __dfault) const {
    return do_get(c, set, __msgid, __dfault);
  }
  void close(catalog c) const { do_close(c); }

  static locale::id id;

protected:
  ~messages() override {}
  virtual catalog do_open(const string&, const locale&) const { return -1; }
  virtual string_type do_get(catalog, int, int, const string_type& __dfault) const { return __dfault; }
  virtual void do_close(catalog) const {}
};
template <class __charT>
locale::id messages<__charT>::id;

// [locale.messages.byname]
template <class __charT>
class messages_byname : public messages<__charT> {
public:
  using catalog = messages_base::catalog;
  using string_type = basic_string<__charT>;
  explicit messages_byname(const char* name, size_t __refs = 0) : messages<__charT>(__refs) {
    ::__ycxx::__detail::__check_locale_name(name, "std::messages_byname");
  }
  explicit messages_byname(const string& name, size_t __refs = 0) : messages_byname(name.c_str(), __refs) {}

protected:
  ~messages_byname() override {}
};

} // namespace std
