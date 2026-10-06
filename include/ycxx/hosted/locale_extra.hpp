// libycxx hosted: the time, monetary and message categories ([category.time],
// [category.monetary], [category.messages]), in the "C" locale; the _byname facets of named
// locales read the C library's (DECISIONS §7).
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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// time_put stage: the characters strftime produces for "%<modifier><format>" in the "C"
// locale (src/hosted/time.cpp). Writes at most cap characters; returns the full length.
std::size_t time_put_c(char* buf, std::size_t cap, const std::tm* t, char format, char modifier) noexcept;

inline constexpr const char* c_weekday_names[14] = {"Sunday",   "Monday", "Tuesday", "Wednesday", "Thursday",
                                                    "Friday",   "Saturday", "Sun",    "Mon",       "Tue",
                                                    "Wed",      "Thu",    "Fri",     "Sat"};
inline constexpr const char* c_month_names[24] = {
    "January", "February", "March", "April", "May", "June", "July", "August", "September", "October",
    "November", "December", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

constexpr char ascii_lower(char c) noexcept { return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c; }

// Sets tm_mon and tm_mday from tm_year and tm_yday.
constexpr void date_from_yday(std::tm& t) noexcept {
  const long long y = t.tm_year + 1900LL;
  const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
  if (t.tm_yday < 0 || t.tm_yday > (leap ? 365 : 364))
    return;
  constexpr int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int d = t.tm_yday, m = 0;
  for (; m < 11; ++m) {
    const int len = days[m] + (m == 1 && leap ? 1 : 0);
    if (d < len)
      break;
    d -= len;
  }
  t.tm_mon = m;
  t.tm_mday = d + 1;
}

// Sets tm_wday (if wday) and tm_yday (if yday) from tm_year, tm_mon and tm_mday (proleptic
// Gregorian calendar); does nothing for a month or day out of range.
constexpr void complete_date(std::tm& t, bool wday, bool yday) noexcept {
  if (t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31)
    return;
  const long long y = t.tm_year + 1900LL;
  const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
  constexpr int before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  const int yd = before[t.tm_mon] + t.tm_mday - 1 + (leap && t.tm_mon > 1 ? 1 : 0);
  if (yday)
    t.tm_yday = yd;
  if (wday) {
    // the weekday of January 1 of y (Gauss), then forward
    const long long p = y - 1;
    const long long jan1 = (1 + 5 * (((p % 4) + 4) % 4) + 4 * (((p % 100) + 100) % 100) + 6 * (((p % 400) + 400) % 400)) % 7;
    t.tm_wday = static_cast<int>((jan1 + yd) % 7);
  }
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// ---- [locale.time.get] ------------------------------------------------------------------------
class time_base {
public:
  enum dateorder { no_order, dmy, mdy, ymd, ydm };
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// LC_TIME of a named locale as time_get_byname reads it (nl_langinfo_l; strings converted to
// charT through the name's LC_CTYPE).
template <class charT>
struct time_data {
  // weekdays (full Sunday-Saturday, then abbreviated), months (full, then abbreviated), AM, PM
  std::basic_string<charT> names[14 + 24 + 2];
  std::basic_string<charT> d_t_fmt, d_fmt, t_fmt, t_fmt_ampm; // %c, %x, %X, %r
  std::time_base::dateorder order = std::time_base::mdy;      // from the order of %x's fields
};
// Fills d for the locale `name`; false (d untouched) for the names with classic semantics.
bool named_time_data(const char* name, time_data<char>& d);
bool named_time_data(const char* name, time_data<wchar_t>& d);
// time_put_byname: strftime_l (wcsftime_l) of "%<modifier><format>" in the locale. Writes at
// most cap characters; returns the full length.
std::size_t named_strftime(const named_locale* h, char* buf, std::size_t cap, const std::tm* t, char format,
                           char modifier);
std::size_t named_strftime(const named_locale* h, wchar_t* buf, std::size_t cap, const std::tm* t, char format,
                           char modifier);

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class charT, class InputIterator>
class time_get : public locale::facet, public time_base {
public:
  using char_type = charT;
  using iter_type = InputIterator;

  explicit time_get(size_t refs = 0) : locale::facet(refs) {}

  dateorder date_order() const { return do_date_order(); }
  iter_type get_time(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    return do_get_time(s, end, f, err, t);
  }
  iter_type get_date(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    return do_get_date(s, end, f, err, t);
  }
  iter_type get_weekday(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    return do_get_weekday(s, end, f, err, t);
  }
  iter_type get_monthname(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    return do_get_monthname(s, end, f, err, t);
  }
  iter_type get_year(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    return do_get_year(s, end, f, err, t);
  }
  iter_type get(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t, char format,
                char modifier = 0) const {
    return do_get(s, end, f, err, t, format, modifier);
  }
  // [locale.time.get.members]/7-10
  iter_type get(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t, const char_type* fmt,
                const char_type* fmtend) const {
    const ctype<charT>& ct = use_facet<ctype<charT>>(f.getloc());
    err = ios_base::goodbit;
    // %C and %y together give the year (strptime): the last value of each is kept
    int century = -1, year_in_century = -1;
    // as strptime does, a complete date also gives the weekday and the day of the year
    bool have_year = false, have_mon = false, have_mday = false, have_wday = false, have_yday = false;
    int week = 0;
    char week_kind = 0; // 'U' (weeks from the first Sunday) or 'W' (from the first Monday)
    while (fmt != fmtend && err == ios_base::goodbit) {
      if (s == end) {
        err = ios_base::eofbit | ios_base::failbit;
        break;
      }
      if (ct.narrow(*fmt, 0) == '%') {
        const charT* p = fmt + 1;
        if (p == fmtend) {
          err = ios_base::failbit;
          break;
        }
        char mod = 0;
        char spec = ct.narrow(*p, 0);
        if (spec == 'E' || spec == 'O') {
          mod = spec;
          if (++p == fmtend) {
            err = ios_base::failbit;
            break;
          }
          spec = ct.narrow(*p, 0);
        }
        if (spec == 'U' || spec == 'W') {
          // read here (as do_get would) to keep the week number for the date below
          s = read_ranged(s, end, f, err, 2, 0, 53, week);
          week_kind = spec;
        } else {
          s = do_get(s, end, f, err, t, spec, mod);
        }
        if (!(err & ios_base::failbit)) {
          if (spec == 'C')
            century = (t->tm_year + 1900) / 100;
          else if (spec == 'y')
            year_in_century = (t->tm_year + 1900) % 100;
          for (const char* q = "CyYcDxF"; *q; ++q)
            have_year = have_year || spec == *q;
          for (const char* q = "mbBhcDxF"; *q; ++q)
            have_mon = have_mon || spec == *q;
          for (const char* q = "decDxF"; *q; ++q)
            have_mday = have_mday || spec == *q;
          for (const char* q = "aAuwc"; *q; ++q)
            have_wday = have_wday || spec == *q;
          have_yday = have_yday || spec == 'j';
        }
        if (err == ios_base::goodbit)
          fmt = p + 1;
      } else if (ct.is(ctype_base::space, *fmt)) {
        while (fmt != fmtend && ct.is(ctype_base::space, *fmt))
          ++fmt;
        while (s != end && ct.is(ctype_base::space, *s))
          ++s;
      } else if (ct.toupper(*s) == ct.toupper(*fmt)) {
        ++fmt;
        ++s;
      } else {
        err = ios_base::failbit;
      }
    }
    if (!(err & ios_base::failbit)) {
      if (century >= 0 && year_in_century >= 0)
        t->tm_year = century * 100 + year_in_century - 1900;
      if (have_year && week_kind != 0 && have_wday && !have_yday && !(have_mon && have_mday)) {
        // the day of the year from the week number and the weekday
        tm jan1{};
        jan1.tm_year = t->tm_year;
        jan1.tm_mday = 1;
        ::ycxx::detail::complete_date(jan1, true, false);
        const int j = jan1.tm_wday;
        t->tm_yday = week_kind == 'U' ? (7 - j) % 7 + (week - 1) * 7 + t->tm_wday
                                      : (8 - j) % 7 + (week - 1) * 7 + (t->tm_wday + 6) % 7;
        have_yday = true;
      }
      if (have_year && have_yday && !(have_mon && have_mday))
        ::ycxx::detail::date_from_yday(*t);
      if (have_year && ((have_mon && have_mday) || have_yday))
        ::ycxx::detail::complete_date(*t, !have_wday, !have_yday);
    }
    return s;
  }

  static locale::id id;

protected:
  ~time_get() override {}
  virtual dateorder do_date_order() const { return named_ != nullptr ? named_->order : mdy; }
  virtual iter_type do_get_time(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    return parse(s, end, f, err, t, "%H:%M:%S");
  }
  virtual iter_type do_get_date(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    // [locale.time.get.virtuals]/4: what time_put produces for "%d%m%y" (or the order's
    // permutation), which has no separators; /5: other formats may be accepted too, here the
    // same fields separated by '/' ('?' in the pattern: an optional '/'). A named locale's facet
    // reads its %x format instead (the one its time_put writes for %x).
    if (named_ != nullptr && !named_->d_fmt.empty())
      return parse_format(s, end, f, err, t, named_->d_fmt);
    switch (date_order()) {
    case dmy:
      return parse(s, end, f, err, t, "%d?%m?%y");
    case ymd:
      return parse(s, end, f, err, t, "%y?%m?%d");
    case ydm:
      return parse(s, end, f, err, t, "%y?%d?%m");
    default:
      return parse(s, end, f, err, t, "%m?%d?%y");
    }
  }
  virtual iter_type do_get_weekday(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    // e: this call's state (err may hold failbit already on entry; it is only or-ed into)
    int v;
    ios_base::iostate e = ios_base::goodbit;
    if (named_ != nullptr)
      s = match_name(s, end, f, e, named_->names, 14, v);
    else
      s = match_name(s, end, f, e, ycxx::detail::c_weekday_names, 14, v);
    if (!(e & ios_base::failbit))
      t->tm_wday = v % 7;
    err |= e;
    return s;
  }
  virtual iter_type do_get_monthname(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    int v;
    ios_base::iostate e = ios_base::goodbit;
    if (named_ != nullptr)
      s = match_name(s, end, f, e, named_->names + 14, 24, v);
    else
      s = match_name(s, end, f, e, ycxx::detail::c_month_names, 24, v);
    if (!(e & ios_base::failbit))
      t->tm_mon = v % 12;
    err |= e;
    return s;
  }
  virtual iter_type do_get_year(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t) const {
    int y, digits;
    ios_base::iostate e = ios_base::goodbit;
    s = read_number(s, end, f, e, 4, y, &digits);
    if (!(e & ios_base::failbit))
      t->tm_year = (digits <= 2 ? (y < 69 ? y + 2000 : y + 1900) : y) - 1900;
    err |= e;
    return s;
  }
  // [locale.time.get.virtuals]/11-15: one strptime conversion.
  virtual iter_type do_get(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t, char format,
                           char modifier) const {
    err = ios_base::goodbit;
    if (modifier == 'E' && format != 'c' && format != 'C' && format != 'x' && format != 'X' && format != 'y' &&
        format != 'Y') {
      err |= ios_base::failbit;
      return s;
    }
    if (modifier == 'O' && !(format == 'd' || format == 'e' || format == 'H' || format == 'I' || format == 'm' ||
                             format == 'M' || format == 'S' || format == 'U' || format == 'w' || format == 'W' ||
                             format == 'y')) {
      err |= ios_base::failbit;
      return s;
    }
    if (modifier != 0 && modifier != 'E' && modifier != 'O') {
      err |= ios_base::failbit;
      return s;
    }
    tm r = *t; // assigned to *t only on success
    int v = 0;
    // a named locale's %c, %x, %X and %r are its own formats
    if (named_ != nullptr) {
      const basic_string<charT>* own = format == 'c'   ? &named_->d_t_fmt
                                       : format == 'x' ? &named_->d_fmt
                                       : format == 'X' ? &named_->t_fmt
                                       : format == 'r' ? &named_->t_fmt_ampm
                                                       : nullptr;
      if (own != nullptr && !own->empty()) {
        s = parse_format(s, end, f, err, &r, *own);
        if (!(err & ios_base::failbit))
          *t = r;
        return s;
      }
    }
    switch (format) {
    case 'a':
    case 'A':
      s = do_get_weekday(s, end, f, err, &r);
      break;
    case 'b':
    case 'B':
    case 'h':
      s = do_get_monthname(s, end, f, err, &r);
      break;
    case 'c':
      s = parse(s, end, f, err, &r, "%a %b %e %H:%M:%S %Y");
      break;
    case 'C':
      s = read_number(s, end, f, err, 2, v);
      if (!(err & ios_base::failbit))
        r.tm_year = v * 100 - 1900 + (r.tm_year + 1900) % 100;
      break;
    case 'd':
    case 'e':
      s = skip_space(s, end, f, err);
      s = read_ranged(s, end, f, err, 2, 1, 31, r.tm_mday);
      break;
    case 'D':
    case 'x':
      s = parse(s, end, f, err, &r, "%m/%d/%y");
      break;
    case 'F':
      s = parse(s, end, f, err, &r, "%Y-%m-%d");
      break;
    case 'H':
      s = read_ranged(s, end, f, err, 2, 0, 23, r.tm_hour);
      break;
    case 'I':
      s = read_ranged(s, end, f, err, 2, 1, 12, v);
      if (!(err & ios_base::failbit))
        r.tm_hour = v % 12 + (r.tm_hour >= 12 ? 12 : 0);
      break;
    case 'j':
      s = read_ranged(s, end, f, err, 3, 1, 366, v);
      if (!(err & ios_base::failbit))
        r.tm_yday = v - 1;
      break;
    case 'm':
      s = read_ranged(s, end, f, err, 2, 1, 12, v);
      if (!(err & ios_base::failbit))
        r.tm_mon = v - 1;
      break;
    case 'M':
      s = read_ranged(s, end, f, err, 2, 0, 59, r.tm_min);
      break;
    case 'n':
    case 't':
      s = skip_space(s, end, f, err);
      break;
    case 'p':
      s = read_ampm(s, end, f, err, r);
      break;
    case 'r':
      s = parse(s, end, f, err, &r, "%I:%M:%S %p");
      break;
    case 'R':
      s = parse(s, end, f, err, &r, "%H:%M");
      break;
    case 'S':
      s = read_ranged(s, end, f, err, 2, 0, 60, r.tm_sec);
      break;
    case 'T':
    case 'X':
      s = parse(s, end, f, err, &r, "%H:%M:%S");
      break;
    case 'u':
      s = read_ranged(s, end, f, err, 1, 1, 7, v);
      if (!(err & ios_base::failbit))
        r.tm_wday = v % 7;
      break;
    case 'U':
    case 'W':
    case 'V':
      s = read_ranged(s, end, f, err, 2, 0, 53, v);
      break;
    case 'w':
      s = read_ranged(s, end, f, err, 1, 0, 6, r.tm_wday);
      break;
    case 'y':
      s = read_ranged(s, end, f, err, 2, 0, 99, v);
      if (!(err & ios_base::failbit))
        r.tm_year = v < 69 ? v + 100 : v;
      break;
    case 'Y':
      s = read_number(s, end, f, err, 4, v);
      if (!(err & ios_base::failbit))
        r.tm_year = v - 1900;
      break;
    case 'Z': // a time zone name: read, not converted (as strptime)
      s = skip_space(s, end, f, err);
      while (s != end && !use_facet<ctype<charT>>(f.getloc()).is(ctype_base::space, *s))
        ++s;
      if (s == end)
        err |= ios_base::eofbit;
      break;
    case '%':
      if (s == end)
        err |= ios_base::eofbit | ios_base::failbit;
      else if (use_facet<ctype<charT>>(f.getloc()).narrow(*s, 0) == '%') {
        if (++s == end)
          err |= ios_base::eofbit;
      } else
        err |= ios_base::failbit;
      break;
    default:
      err |= ios_base::failbit;
      break;
    }
    if (!(err & ios_base::failbit))
      *t = r;
    return s;
  }

private:
  template <class, class>
  friend class time_get_byname;
  // A named locale's names and formats (time_get_byname's; null: the "C" locale's).
  const ycxx::detail::time_data<charT>* named_ = nullptr;

  // Parses a named locale's format with get() (its conversions with do_get).
  iter_type parse_format(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t,
                         const basic_string<charT>& fmt) const {
    ios_base::iostate e = ios_base::goodbit;
    s = get(s, end, f, e, t, fmt.data(), fmt.data() + fmt.size());
    if (s == end) // as parse() reports the end of the input
      e |= ios_base::eofbit;
    err |= e;
    return s;
  }
  // Parses the char format fmt (conversions and literal characters) with do_get.
  iter_type parse(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm* t, const char* fmt) const {
    const ctype<charT>& ct = use_facet<ctype<charT>>(f.getloc());
    ios_base::iostate e = ios_base::goodbit;
    for (; *fmt && !(e & ios_base::failbit); ++fmt) {
      if (*fmt == '%') {
        ++fmt;
        ios_base::iostate one = ios_base::goodbit;
        s = do_get(s, end, f, one, t, *fmt, 0);
        e |= one;
      } else if (*fmt == ' ') {
        while (s != end && ct.is(ctype_base::space, *s))
          ++s;
      } else if (*fmt == '?') { // an optional '/' (do_get_date)
        if (s != end && ct.narrow(*s, 0) == '/')
          ++s;
      } else if (s == end) {
        e |= ios_base::eofbit | ios_base::failbit;
      } else if (ct.narrow(*s, 0) == *fmt) {
        ++s;
      } else {
        e |= ios_base::failbit;
      }
    }
    if (s == end)
      e |= ios_base::eofbit;
    err |= e;
    return s;
  }
  static iter_type skip_space(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err) {
    const ctype<charT>& ct = use_facet<ctype<charT>>(f.getloc());
    while (s != end && ct.is(ctype_base::space, *s))
      ++s;
    if (s == end)
      err |= ios_base::eofbit;
    return s;
  }
  // Up to max_digits decimal digits (at least one); with hi >= 0, a digit that would take the
  // value above hi is an error and is not read ("32" for a day of the month stops at "2").
  static iter_type read_number(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, int max_digits, int& v,
                               int* ndigits = nullptr, int hi = -1) {
    const ctype<charT>& ct = use_facet<ctype<charT>>(f.getloc());
    int n = 0;
    v = 0;
    for (; n < max_digits; ++n, static_cast<void>(++s)) {
      if (s == end) {
        err |= ios_base::eofbit;
        break;
      }
      const char c = ct.narrow(*s, 0);
      if (c < '0' || c > '9')
        break;
      if (hi >= 0 && n != 0 && v * 10 + (c - '0') > hi) {
        err |= ios_base::failbit;
        break;
      }
      v = v * 10 + (c - '0');
    }
    if (n == 0)
      err |= ios_base::failbit;
    else if (n == max_digits && s == end)
      err |= ios_base::eofbit;
    if (ndigits)
      *ndigits = n;
    return s;
  }
  static iter_type read_ranged(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, int max_digits,
                               int lo, int hi, int& out) {
    int v;
    s = read_number(s, end, f, err, max_digits, v, nullptr, hi);
    if (!(err & ios_base::failbit)) {
      if (v < lo || v > hi)
        err |= ios_base::failbit;
      else
        out = v;
    }
    return s;
  }
  iter_type read_ampm(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err, tm& r) const {
    static constexpr const char* names[2] = {"AM", "PM"};
    int v;
    if (named_ != nullptr) {
      // a locale without AM/PM strings (most 24-hour locales) has nothing to read
      if (named_->names[38].empty() && named_->names[39].empty())
        return s;
      s = match_name(s, end, f, err, named_->names + 38, 2, v);
    } else {
      s = match_name(s, end, f, err, names, 2, v);
    }
    if (!(err & ios_base::failbit)) {
      r.tm_hour %= 12;
      if (v == 1)
        r.tm_hour += 12;
    }
    return s;
  }
  // Matches one of names[0..n) (case-insensitively), reading characters only while some name
  // can still be extended; the match is the name equal to everything read.
  static iter_type match_name(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err,
                              const char* const* names, int n, int& which) {
    const ctype<charT>& ct = use_facet<ctype<charT>>(f.getloc());
    bool alive[24];
    for (int k = 0; k < n; ++k)
      alive[k] = true;
    size_t i = 0;
    for (;;) {
      bool more = false;
      for (int k = 0; k < n; ++k)
        more = more || (alive[k] && names[k][i] != '\0');
      if (!more)
        break;
      if (s == end) {
        err |= ios_base::eofbit;
        break;
      }
      const char c = ycxx::detail::ascii_lower(ct.narrow(*s, 0));
      bool any = false;
      for (int k = 0; k < n; ++k)
        any = any || (alive[k] && names[k][i] != '\0' && ycxx::detail::ascii_lower(names[k][i]) == c);
      if (!any)
        break; // nothing extends: the names alive so far that end here are the candidates
      for (int k = 0; k < n; ++k)
        alive[k] = alive[k] && names[k][i] != '\0' && ycxx::detail::ascii_lower(names[k][i]) == c;
      ++s;
      ++i;
    }
    if (s == end)
      err |= ios_base::eofbit;
    which = -1;
    for (int k = 0; k < n; ++k)
      if (alive[k] && names[k][i] == '\0' && i != 0) {
        which = k;
        break;
      }
    if (which < 0)
      err |= ios_base::failbit;
    return s;
  }
  // The same for a named locale's names, compared through the stream's ctype<charT>::tolower;
  // an empty name never matches.
  static iter_type match_name(iter_type s, iter_type end, ios_base& f, ios_base::iostate& err,
                              const basic_string<charT>* names, int n, int& which) {
    const ctype<charT>& ct = use_facet<ctype<charT>>(f.getloc());
    bool alive[24];
    for (int k = 0; k < n; ++k)
      alive[k] = !names[k].empty();
    size_t i = 0;
    for (;;) {
      bool more = false;
      for (int k = 0; k < n; ++k)
        more = more || (alive[k] && i < names[k].size());
      if (!more)
        break;
      if (s == end) {
        err |= ios_base::eofbit;
        break;
      }
      const charT c = ct.tolower(*s);
      bool any = false;
      for (int k = 0; k < n; ++k)
        any = any || (alive[k] && i < names[k].size() && ct.tolower(names[k][i]) == c);
      if (!any)
        break;
      for (int k = 0; k < n; ++k)
        alive[k] = alive[k] && i < names[k].size() && ct.tolower(names[k][i]) == c;
      ++s;
      ++i;
    }
    if (s == end)
      err |= ios_base::eofbit;
    which = -1;
    for (int k = 0; k < n; ++k)
      if (alive[k] && names[k].size() == i && i != 0) {
        which = k;
        break;
      }
    if (which < 0)
      err |= ios_base::failbit;
    return s;
  }
};
template <class charT, class InputIterator>
locale::id time_get<charT, InputIterator>::id;

// [locale.time.get.byname]
template <class charT, class InputIterator>
class time_get_byname : public time_get<charT, InputIterator> {
public:
  using dateorder = time_base::dateorder;
  using iter_type = InputIterator;
  // char and wchar_t: LC_TIME's names and formats, read here; other character types (and the
  // names with classic semantics): the "C" locale's.
  explicit time_get_byname(const char* name, size_t refs = 0) : time_get<charT, InputIterator>(refs) {
    if constexpr (is_same_v<charT, char> || is_same_v<charT, wchar_t>) {
      if (::ycxx::detail::named_time_data(name, data_))
        this->named_ = &data_;
    } else {
      ::ycxx::detail::check_locale_name(name, "std::time_get_byname");
    }
  }
  explicit time_get_byname(const string& name, size_t refs = 0) : time_get_byname(name.c_str(), refs) {}

protected:
  ~time_get_byname() override {}

private:
  ycxx::detail::time_data<charT> data_;
};

// ---- [locale.time.put] ------------------------------------------------------------------------
template <class charT, class OutputIterator>
class time_put : public locale::facet {
public:
  using char_type = charT;
  using iter_type = OutputIterator;

  explicit time_put(size_t refs = 0) : locale::facet(refs) {}

  // [locale.time.put.members]/1
  iter_type put(iter_type s, ios_base& str, char_type fill, const tm* t, const charT* pattern,
                const charT* pat_end) const {
    const ctype<charT>& ct = use_facet<ctype<charT>>(str.getloc());
    while (pattern != pat_end) {
      if (ct.narrow(*pattern, 0) != '%' || pattern + 1 == pat_end) {
        *s = *pattern++;
        ++s;
        continue;
      }
      const charT* p = pattern + 1;
      char mod = 0;
      char spec = ct.narrow(*p, 0);
      if ((spec == 'E' || spec == 'O') && p + 1 != pat_end) {
        mod = spec;
        spec = ct.narrow(*++p, 0);
      }
      s = do_put(s, str, fill, t, spec, mod);
      pattern = p + 1;
    }
    return s;
  }
  iter_type put(iter_type s, ios_base& str, char_type fill, const tm* t, char format, char modifier = 0) const {
    return do_put(s, str, fill, t, format, modifier);
  }

  static locale::id id;

protected:
  ~time_put() override {}
  virtual iter_type do_put(iter_type s, ios_base& str, char_type, const tm* t, char format, char modifier) const {
    char local[128];
    size_t n = ycxx::detail::time_put_c(local, sizeof local, t, format, modifier);
    ycxx::detail::small_buffer<char, 1> big(n > sizeof local ? n : 0);
    const char* text = local;
    if (n > sizeof local) {
      n = ycxx::detail::time_put_c(big.get(), n, t, format, modifier);
      text = big.get();
    }
    const ctype<charT>& ct = use_facet<ctype<charT>>(str.getloc());
    for (size_t i = 0; i < n; ++i, static_cast<void>(++s))
      *s = ct.widen(text[i]);
    return s;
  }
};
template <class charT, class OutputIterator>
locale::id time_put<charT, OutputIterator>::id;

// [locale.time.put.byname]
template <class charT, class OutputIterator>
class time_put_byname : public time_put<charT, OutputIterator> {
public:
  using char_type = charT;
  using iter_type = OutputIterator;
  // char and wchar_t: strftime_l / wcsftime_l in the name's LC_TIME; other character types (and
  // the names with classic semantics): the "C" locale's conversions.
  explicit time_put_byname(const char* name, size_t refs = 0) : time_put<charT, OutputIterator>(refs) {
    if constexpr (is_same_v<charT, char> || is_same_v<charT, wchar_t>)
      named_ = ::ycxx::detail::named_open(name, locale::time, "std::time_put_byname");
    else
      ::ycxx::detail::check_locale_name(name, "std::time_put_byname");
  }
  explicit time_put_byname(const string& name, size_t refs = 0) : time_put_byname(name.c_str(), refs) {}

protected:
  ~time_put_byname() override { ::ycxx::detail::named_release(named_); }
  iter_type do_put(iter_type s, ios_base& str, char_type fill, const tm* t, char format, char modifier) const override {
    if constexpr (is_same_v<charT, char> || is_same_v<charT, wchar_t>) {
      if (named_ != nullptr) {
        charT local[128];
        size_t n = ycxx::detail::named_strftime(named_, local, 128, t, format, modifier);
        ycxx::detail::small_buffer<charT, 1> big(n > 128 ? n : 0);
        const charT* text = local;
        if (n > 128) {
          n = ycxx::detail::named_strftime(named_, big.get(), n, t, format, modifier);
          text = big.get();
        }
        for (size_t i = 0; i < n; ++i, static_cast<void>(++s))
          *s = text[i];
        return s;
      }
    }
    return time_put<charT, OutputIterator>::do_put(s, str, fill, t, format, modifier);
  }

private:
  ycxx::detail::named_locale* named_ = nullptr;
};

// ---- [locale.moneypunct] ----------------------------------------------------------------------
class money_base {
public:
  enum part { none, space, symbol, sign, value };
  struct pattern {
    char field[4];
  };
};

template <class charT, bool International>
class moneypunct : public locale::facet, public money_base {
public:
  using char_type = charT;
  using string_type = basic_string<charT>;

  explicit moneypunct(size_t refs = 0) : locale::facet(refs) {}

  charT decimal_point() const { return do_decimal_point(); }
  charT thousands_sep() const { return do_thousands_sep(); }
  string grouping() const { return do_grouping(); }
  string_type curr_symbol() const { return do_curr_symbol(); }
  string_type positive_sign() const { return do_positive_sign(); }
  string_type negative_sign() const { return do_negative_sign(); }
  int frac_digits() const { return do_frac_digits(); }
  pattern pos_format() const { return do_pos_format(); }
  pattern neg_format() const { return do_neg_format(); }

  static locale::id id;
  static const bool intl = International;

protected:
  ~moneypunct() override {}
  virtual charT do_decimal_point() const { return numeric_limits<charT>::max(); }
  virtual charT do_thousands_sep() const { return numeric_limits<charT>::max(); }
  virtual string do_grouping() const { return string(); }
  virtual string_type do_curr_symbol() const { return string_type(); }
  virtual string_type do_positive_sign() const { return string_type(); }
  virtual string_type do_negative_sign() const { return string_type(1, charT('-')); }
  virtual int do_frac_digits() const { return 0; }
  virtual pattern do_pos_format() const { return pattern{{symbol, sign, none, value}}; }
  virtual pattern do_neg_format() const { return pattern{{symbol, sign, none, value}}; }
};
template <class charT, bool International>
locale::id moneypunct<charT, International>::id;
template <class charT, bool International>
const bool moneypunct<charT, International>::intl;

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// LC_MONETARY of a named locale as moneypunct_byname reads it; initialized to the values of the
// base moneypunct.
template <class charT>
struct money_data {
  charT point = std::numeric_limits<charT>::max();
  charT sep = std::numeric_limits<charT>::max();
  std::string grouping;
  std::basic_string<charT> symbol, positive, negative = std::basic_string<charT>(1, charT('-'));
  int frac_digits = 0;
  std::money_base::pattern pos{{std::money_base::symbol, std::money_base::sign, std::money_base::none,
                                std::money_base::value}};
  std::money_base::pattern neg = pos;
};
// Fills d for the locale `name` from localeconv (the int_ members for intl); d is untouched for
// the names with classic semantics. Separators as for named_numpunct.
void named_money_data(const char* name, bool intl, money_data<char>& d);
void named_money_data(const char* name, bool intl, money_data<wchar_t>& d);

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [locale.moneypunct.byname]: for char and wchar_t, the C library's LC_MONETARY of the name, read
// once, here. The patterns follow POSIX's p_cs_precedes, p_sep_by_space and p_sign_posn (n_ for
// neg_format(), int_p_/int_n_ for Intl): a separating space is the pattern's space field, and a
// sign position of 0 (parentheses) gives the sign string "()". Other character types: the base's
// values.
template <class charT, bool Intl>
class moneypunct_byname : public moneypunct<charT, Intl> {
public:
  using pattern = money_base::pattern;
  using string_type = basic_string<charT>;
  explicit moneypunct_byname(const char* name, size_t refs = 0) : moneypunct<charT, Intl>(refs) {
    if constexpr (is_same_v<charT, char> || is_same_v<charT, wchar_t>)
      ::ycxx::detail::named_money_data(name, Intl, data_);
    else
      ::ycxx::detail::check_locale_name(name, "std::moneypunct_byname");
  }
  explicit moneypunct_byname(const string& name, size_t refs = 0) : moneypunct_byname(name.c_str(), refs) {}

protected:
  ~moneypunct_byname() override {}
  charT do_decimal_point() const override { return data_.point; }
  charT do_thousands_sep() const override { return data_.sep; }
  string do_grouping() const override { return data_.grouping; }
  string_type do_curr_symbol() const override { return data_.symbol; }
  string_type do_positive_sign() const override { return data_.positive; }
  string_type do_negative_sign() const override { return data_.negative; }
  int do_frac_digits() const override { return data_.frac_digits; }
  pattern do_pos_format() const override { return data_.pos; }
  pattern do_neg_format() const override { return data_.neg; }

private:
  ycxx::detail::money_data<charT> data_;
};

// ---- [locale.money.get] -----------------------------------------------------------------------
template <class charT, class InputIterator>
class money_get : public locale::facet {
public:
  using char_type = charT;
  using iter_type = InputIterator;
  using string_type = basic_string<charT>;

  explicit money_get(size_t refs = 0) : locale::facet(refs) {}

  iter_type get(iter_type s, iter_type end, bool intl, ios_base& f, ios_base::iostate& err, long double& units) const {
    return do_get(s, end, intl, f, err, units);
  }
  iter_type get(iter_type s, iter_type end, bool intl, ios_base& f, ios_base::iostate& err,
                string_type& digits) const {
    return do_get(s, end, intl, f, err, digits);
  }

  static locale::id id;

protected:
  ~money_get() override {}
  virtual iter_type do_get(iter_type s, iter_type end, bool intl, ios_base& str, ios_base::iostate& err,
                           long double& units) const {
    string field; // '-'? digits, as char
    bool ok = false;
    s = intl ? parse<true>(s, end, str, err, field, ok) : parse<false>(s, end, str, err, field, ok);
    if (ok) {
      long double v = 0;
      if (ycxx::detail::num_get_float(field.data(), field.size(), &v) == ycxx::detail::num_parse::not_converted)
        err |= ios_base::failbit;
      else
        units = v;
    }
    return s;
  }
  virtual iter_type do_get(iter_type s, iter_type end, bool intl, ios_base& str, ios_base::iostate& err,
                           string_type& digits) const {
    string field;
    bool ok = false;
    s = intl ? parse<true>(s, end, str, err, field, ok) : parse<false>(s, end, str, err, field, ok);
    if (ok) {
      const ctype<charT>& ct = use_facet<ctype<charT>>(str.getloc());
      string_type r(field.size(), charT());
      ct.widen(field.data(), field.data() + field.size(), r.data());
      digits = static_cast<string_type&&>(r);
    }
    return s;
  }

private:
  // [locale.money.get.virtuals]/1-4: on success field holds the result as '-'? digits and ok is
  // true; otherwise failbit (and eofbit at the end of input) is set in err.
  template <bool Intl>
  static iter_type parse(iter_type s, iter_type end, ios_base& str, ios_base::iostate& err, string& field, bool& ok) {
    const locale loc = str.getloc();
    const moneypunct<charT, Intl>& mp = use_facet<moneypunct<charT, Intl>>(loc);
    const ctype<charT>& ct = use_facet<ctype<charT>>(loc);
    const money_base::pattern pat = mp.neg_format();
    const string_type pos = mp.positive_sign(), neg = mp.negative_sign(), sym = mp.curr_symbol();
    const string grouping = mp.grouping();
    const charT point = mp.decimal_point(), sep = mp.thousands_sep();
    const int frac = mp.frac_digits();
    const bool showbase = (str.flags() & ios_base::showbase) != 0;

    const string_type* sign = nullptr; // the sign string matched (or implied)
    string digits;
    unsigned groups[64];
    size_t ngroups = 0;
    unsigned run = 0;
    bool seen_sep = false;
    bool failed = false;

    auto at_end = [&] { return s == end; };
    for (int i = 0; i < 4 && !failed; ++i) {
      switch (static_cast<money_base::part>(pat.field[i])) {
      case money_base::none:
      case money_base::space:
        if (i == 3)
          break;
        if (pat.field[i] == money_base::space) {
          if (at_end() || !ct.is(ctype_base::space, *s)) {
            failed = true;
            break;
          }
        }
        while (!at_end() && ct.is(ctype_base::space, *s))
          ++s;
        break;
      case money_base::symbol: {
        // without showbase the symbol is optional, consumed only if other characters are needed
        // to complete the format: a later value, a later space that is not last, a later sign
        // when neither sign string is empty, or the rest of a sign already begun
        bool needed = showbase || (sign != nullptr && sign->size() > 1);
        for (int j = i + 1; j < 4; ++j) {
          const auto f = static_cast<money_base::part>(pat.field[j]);
          needed = needed || f == money_base::value || (f == money_base::space && j < 3) ||
                   (f == money_base::sign && !pos.empty() && !neg.empty());
        }
        if (sym.empty())
          break;
        if (!needed && !showbase)
          break;
        size_t k = 0;
        for (; k < sym.size(); ++k, static_cast<void>(++s)) {
          if (at_end() || !(*s == sym[k]))
            break;
        }
        if (k != sym.size() && (showbase || k != 0))
          failed = true;
        break;
      }
      case money_base::sign:
        if (!pos.empty() && !at_end() && *s == pos[0]) {
          sign = &pos;
          ++s;
        } else if (!neg.empty() && !at_end() && *s == neg[0]) {
          sign = &neg;
          ++s;
        } else if (pos.empty()) {
          sign = &pos;
        } else if (neg.empty()) {
          sign = &neg;
        } else {
          failed = true;
        }
        break;
      case money_base::value: {
        bool point_seen = false;
        int frac_got = 0;
        while (!at_end()) {
          const charT c = *s;
          if (ct.is(ctype_base::digit, c)) {
            if (point_seen) {
              if (frac_got == frac)
                break;
              ++frac_got;
            } else {
              ++run;
            }
            digits.push_back(ct.narrow(c, '0'));
          } else if (frac > 0 && !point_seen && c == point) {
            point_seen = true;
          } else if (!grouping.empty() && !point_seen && c == sep) {
            if (ngroups < 64)
              groups[ngroups++] = run;
            run = 0;
            seen_sep = true;
          } else {
            break;
          }
          ++s;
        }
        // [locale.moneypunct.general]/3: a decimal point must be followed by exactly
        // frac_digits() digits; without one, the digits are stored as they appear
        if (digits.empty() || (point_seen && frac_got != frac)) {
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
        if (at_end() || !(*s == (*sign)[k])) {
          failed = true;
          break;
        }
      }
    }
    if (!failed && seen_sep) {
      if (ngroups < 64) {
        groups[ngroups++] = run;
        failed = !ycxx::detail::num_grouping_ok(grouping, groups, ngroups);
      } else {
        failed = true;
      }
    }
    if (failed) {
      err |= ios_base::failbit;
      if (at_end())
        err |= ios_base::eofbit;
      return s;
    }
    // [locale.money.get.virtuals]/3: equal first characters, or both empty, mean positive
    const bool negative =
        sign == &neg && !(pos.empty() && neg.empty()) && !(!pos.empty() && !neg.empty() && pos[0] == neg[0]);
    size_t lead = 0;
    while (lead + 1 < digits.size() && digits[lead] == '0')
      ++lead;
    field.clear();
    if (negative)
      field.push_back('-');
    field.append(digits, lead);
    ok = true;
    if (at_end())
      err |= ios_base::eofbit;
    return s;
  }
};
template <class charT, class InputIterator>
locale::id money_get<charT, InputIterator>::id;

// ---- [locale.money.put] -----------------------------------------------------------------------
template <class charT, class OutputIterator>
class money_put : public locale::facet {
public:
  using char_type = charT;
  using iter_type = OutputIterator;
  using string_type = basic_string<charT>;

  explicit money_put(size_t refs = 0) : locale::facet(refs) {}

  iter_type put(iter_type s, bool intl, ios_base& f, char_type fill, long double units) const {
    return do_put(s, intl, f, fill, units);
  }
  iter_type put(iter_type s, bool intl, ios_base& f, char_type fill, const string_type& digits) const {
    return do_put(s, intl, f, fill, digits);
  }

  static locale::id id;

protected:
  ~money_put() override {}
  virtual iter_type do_put(iter_type s, bool intl, ios_base& str, char_type fill, long double units) const {
    // "%.0Lf"
    char local[64];
    size_t pad = 0;
    size_t n = ycxx::detail::num_put_float(local, sizeof local, units, ios_base::fixed, 0, &pad);
    ycxx::detail::small_buffer<char, 1> big(n > sizeof local ? n : 0);
    const char* text = local;
    if (n > sizeof local) {
      n = ycxx::detail::num_put_float(big.get(), n, units, ios_base::fixed, 0, &pad);
      text = big.get();
    }
    const ctype<charT>& ct = use_facet<ctype<charT>>(str.getloc());
    string_type digits(n, charT());
    ct.widen(text, text + n, digits.data());
    return do_put(s, intl, str, fill, digits);
  }
  virtual iter_type do_put(iter_type s, bool intl, ios_base& str, char_type fill, const string_type& digits) const {
    return intl ? format<true>(s, str, fill, digits) : format<false>(s, str, fill, digits);
  }

private:
  template <bool Intl>
  static iter_type format(iter_type s, ios_base& str, char_type fill, const string_type& digits) {
    const locale loc = str.getloc();
    const moneypunct<charT, Intl>& mp = use_facet<moneypunct<charT, Intl>>(loc);
    const ctype<charT>& ct = use_facet<ctype<charT>>(loc);
    const bool negative = !digits.empty() && digits[0] == ct.widen('-');
    // the digits: after the optional minus sign, up to the first non-digit
    size_t first = negative ? 1 : 0, last = first;
    while (last < digits.size() && ct.is(ctype_base::digit, digits[last]))
      ++last;
    const money_base::pattern pat = negative ? mp.neg_format() : mp.pos_format();
    const string_type sign = negative ? mp.negative_sign() : mp.positive_sign();
    const string_type sym = mp.curr_symbol();
    const int frac = mp.frac_digits() > 0 ? mp.frac_digits() : 0;
    const string grouping = mp.grouping();

    // the value: integer digits (grouped), the decimal point and frac fraction digits
    const size_t ndig = last - first;
    const size_t nint = ndig > static_cast<size_t>(frac) ? ndig - frac : 0;
    size_t marks[64]; // separator positions, counted in digits from the right
    size_t nmarks = 0;
    for (size_t i = 0, at = 0; !grouping.empty() && nint != 0; ++i) {
      const char g = grouping[i < grouping.size() ? i : grouping.size() - 1];
      if (static_cast<signed char>(g) <= 0 || g == numeric_limits<char>::max())
        break;
      at += static_cast<unsigned char>(g);
      if (at >= nint || nmarks == 64)
        break;
      marks[nmarks++] = at;
    }
    const size_t value_len = (nint == 0 ? 1 : nint + nmarks) + (frac > 0 ? 1 + static_cast<size_t>(frac) : 0);
    const size_t cap = value_len + sym.size() + sign.size() + 2;
    ycxx::detail::small_buffer<charT, 128> buffer(cap);
    charT* const out = buffer.get();
    size_t len = 0;
    size_t pad_at = cap; // where internal padding goes (cap: none)
    for (int f = 0; f < 4; ++f) {
      switch (static_cast<money_base::part>(pat.field[f])) {
      case money_base::none:
        pad_at = len;
        break;
      case money_base::space:
        pad_at = len;
        out[len++] = ct.widen(' ');
        break;
      case money_base::symbol:
        if (str.flags() & ios_base::showbase)
          for (charT c : sym)
            out[len++] = c;
        break;
      case money_base::sign:
        if (!sign.empty())
          out[len++] = sign[0];
        break;
      case money_base::value:
        if (nint == 0) {
          out[len++] = ct.widen('0');
        } else {
          for (size_t i = 0; i < nint; ++i) {
            for (size_t m = 0; m < nmarks; ++m)
              if (i != 0 && marks[m] == nint - i)
                out[len++] = mp.thousands_sep();
            out[len++] = digits[first + i];
          }
        }
        if (frac > 0) {
          out[len++] = mp.decimal_point();
          for (size_t k = static_cast<size_t>(frac); k > ndig; --k)
            out[len++] = ct.widen('0');
          for (size_t i = nint; i < ndig; ++i)
            out[len++] = digits[first + i];
        }
        break;
      }
    }
    for (size_t k = 1; k < sign.size(); ++k)
      out[len++] = sign[k];

    const streamsize width = str.width();
    str.width(0);
    size_t fill_count = width > 0 && static_cast<size_t>(width) > len ? static_cast<size_t>(width) - len : 0;
    const ios_base::fmtflags adjust = str.flags() & ios_base::adjustfield;
    size_t where = 0;
    if (adjust == ios_base::internal && pad_at != cap)
      where = pad_at;
    else if (adjust == ios_base::left)
      where = len;
    for (size_t i = 0; i < where; ++i, static_cast<void>(++s))
      *s = out[i];
    for (; fill_count != 0; --fill_count, static_cast<void>(++s))
      *s = fill;
    for (size_t i = where; i < len; ++i, static_cast<void>(++s))
      *s = out[i];
    return s;
  }
};
template <class charT, class OutputIterator>
locale::id money_put<charT, OutputIterator>::id;

// ---- [locale.messages] ------------------------------------------------------------------------
class messages_base {
public:
  using catalog = int;
};

template <class charT>
class messages : public locale::facet, public messages_base {
public:
  using char_type = charT;
  using string_type = basic_string<charT>;

  explicit messages(size_t refs = 0) : locale::facet(refs) {}

  catalog open(const string& fn, const locale& loc) const { return do_open(fn, loc); }
  string_type get(catalog c, int set, int msgid, const string_type& dfault) const {
    return do_get(c, set, msgid, dfault);
  }
  void close(catalog c) const { do_close(c); }

  static locale::id id;

protected:
  ~messages() override {}
  virtual catalog do_open(const string&, const locale&) const { return -1; }
  virtual string_type do_get(catalog, int, int, const string_type& dfault) const { return dfault; }
  virtual void do_close(catalog) const {}
};
template <class charT>
locale::id messages<charT>::id;

// [locale.messages.byname]
template <class charT>
class messages_byname : public messages<charT> {
public:
  using catalog = messages_base::catalog;
  using string_type = basic_string<charT>;
  explicit messages_byname(const char* name, size_t refs = 0) : messages<charT>(refs) {
    ::ycxx::detail::check_locale_name(name, "std::messages_byname");
  }
  explicit messages_byname(const string& name, size_t refs = 0) : messages_byname(name.c_str(), refs) {}

protected:
  ~messages_byname() override {}
};
// The C library's message catalogs (catopen with NL_CAT_LOCALE in the name's LC_MESSAGES; catgets,
// whose text is converted through the name's LC_CTYPE for wchar_t); src/hosted/locale_named.cpp.
// For the names with classic semantics, the base messages (no catalogs).
template <>
class messages_byname<char> : public messages<char> {
public:
  using catalog = messages_base::catalog;
  using string_type = string;
  explicit messages_byname(const char* name, size_t refs = 0)
      : messages(refs), named_(::ycxx::detail::named_open(name, locale::messages, "std::messages_byname")) {}
  explicit messages_byname(const string& name, size_t refs = 0) : messages_byname(name.c_str(), refs) {}

protected:
  ~messages_byname() override;
  catalog do_open(const string& fn, const locale& loc) const override;
  string_type do_get(catalog c, int set, int msgid, const string_type& dfault) const override;
  void do_close(catalog c) const override;

private:
  ycxx::detail::named_locale* named_;
};
template <>
class messages_byname<wchar_t> : public messages<wchar_t> {
public:
  using catalog = messages_base::catalog;
  using string_type = wstring;
  explicit messages_byname(const char* name, size_t refs = 0)
      : messages(refs), named_(::ycxx::detail::named_open(name, locale::messages, "std::messages_byname")) {}
  explicit messages_byname(const string& name, size_t refs = 0) : messages_byname(name.c_str(), refs) {}

protected:
  ~messages_byname() override;
  catalog do_open(const string& fn, const locale& loc) const override;
  string_type do_get(catalog c, int set, int msgid, const string_type& dfault) const override;
  void do_close(catalog c) const override;

private:
  ycxx::detail::named_locale* named_;
};

} // namespace std
