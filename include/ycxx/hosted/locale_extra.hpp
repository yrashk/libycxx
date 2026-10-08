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

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// LC_TIME of a named locale as time_get_byname reads it (nl_langinfo_l; strings converted to
// charT through the name's LC_CTYPE).
// One era of the locale (a POSIX ERA segment "direction:offset:start:end:name:format"): the year
// numbered n in it is the Gregorian year __start_year + __step * (n - __offset); its name and
// its full-year format (%EY) are [pos, pos + len) of __time_data::__era_text.
struct __time_era {
  long long __start_year, __offset;
  int __step; // +1: the era's numbers grow with the Gregorian years; -1: they grow backwards
  std::size_t __name_pos, __name_len, __fmt_pos, __fmt_len;
};
template <class __charT>
struct __time_data {
  // weekdays (full Sunday-Saturday, then abbreviated), months (full, then abbreviated), AM, PM
  std::basic_string<__charT> __names[14 + 24 + 2];
  // the same as strftime_l writes them (%A %a %B %b %p), where that differs from the names above
  // (empty where it does not): Darwin's ja_JP, for one, writes %b as " 6" for an ABMON_6 of "6月"
  std::basic_string<__charT> __written[14 + 24 + 2];
  std::basic_string<__charT> __d_t_fmt, __d_fmt, __t_fmt, __t_fmt_ampm; // %c, %x, %X, %r
  std::basic_string<__charT> __era_d_t_fmt, __era_d_fmt, __era_t_fmt;    // %Ec, %Ex, %EX (empty: none)
  std::time_base::dateorder __order = std::time_base::mdy;      // from the order of %x's fields
  // the eras (ERA), at most 32
  __time_era __eras[32];
  int __neras = 0;
  std::basic_string<__charT> __era_text;
  // the alternative digits of 0-99 (the O forms): [__alt_pos[k], __alt_pos[k + 1]) of __alt_text;
  // none when __alt_text is empty
  std::basic_string<__charT> __alt_text;
  unsigned short __alt_pos[101] = {};

  bool __has_alt() const noexcept { return !__alt_text.empty(); }
  const __charT* __era_name(int k, std::size_t& __len) const noexcept {
    __len = __eras[k].__name_len;
    return __era_text.data() + __eras[k].__name_pos;
  }
};

// The longest case-insensitive match (ctype<charT>::tolower) among n <= 128 strings, given by
// get(k, len) -> pointer; an empty string never matches. Reads characters only while some string
// can still be extended. which: the index matched, else -1. eof: the end was reached.
template <class __charT, class _It, class _Get>
_It __time_match(_It s, _It end, const std::ctype<__charT>& __ct, int n, _Get get, int& __which, bool& __eof) {
  bool __alive[128];
  const __charT* __str[128];
  std::size_t __lens[128];
  for (int k = 0; k < n; ++k) {
    __str[k] = get(k, __lens[k]);
    __alive[k] = __lens[k] != 0;
  }
  __which = -1;
  __eof = false;
  std::size_t i = 0;
  for (;;) {
    for (int k = 0; k < n; ++k) // the longest complete match so far
      if (__alive[k] && __lens[k] == i && i != 0)
        __which = k;
    bool __more = false;
    for (int k = 0; k < n; ++k)
      __more = __more || (__alive[k] && i < __lens[k]);
    if (!__more)
      break;
    if (s == end) {
      __eof = true;
      break;
    }
    const __charT c = __ct.tolower(*s);
    bool any = false;
    for (int k = 0; k < n; ++k)
      any = any || (__alive[k] && i < __lens[k] && __ct.tolower(__str[k][i]) == c);
    if (!any)
      break;
    for (int k = 0; k < n; ++k)
      __alive[k] = __alive[k] && i < __lens[k] && __ct.tolower(__str[k][i]) == c;
    ++s;
    ++i;
  }
  // a shorter complete match followed by characters of a longer one that failed: the
  // characters read are gone (an input iterator), so only a match of everything read counts
  if (__which >= 0 && __lens[__which] != i)
    __which = -1;
  return s;
}

// A number in the locale's alternative digits (the longest match among those of 0-99), or in
// ASCII digits (at most __max_digits); ok false when neither starts here.
template <class __charT, class _It>
_It __time_read_alt(_It s, _It end, const std::ctype<__charT>& __ct, const __time_data<__charT>* d, int __max_digits,
                    long long& __v, bool& ok, bool& __eof) {
  ok = false;
  __eof = false;
  __v = 0;
  if (s == end) {
    __eof = true;
    return s;
  }
  const char c = __ct.narrow(*s, 0);
  if (c >= '0' && c <= '9') {
    int n = 0;
    for (; n < __max_digits; ++n) {
      if (s == end) {
        __eof = true;
        break;
      }
      const char __d = __ct.narrow(*s, 0);
      if (__d < '0' || __d > '9')
        break;
      __v = __v * 10 + (__d - '0');
      ++s;
    }
    ok = n != 0;
    return s;
  }
  if (d == nullptr || !d->__has_alt())
    return s;
  int __which;
  s = ::__ycxx::__detail::__time_match(s, end, __ct, 100,
                                       [d](int k, std::size_t& __len) {
                                         __len = static_cast<std::size_t>(d->__alt_pos[k + 1] - d->__alt_pos[k]);
                                         return d->__alt_text.data() + d->__alt_pos[k];
                                       },
                                       __which, __eof);
  ok = __which >= 0;
  __v = __which;
  return s;
}

// %EY: a full year in one of the eras' formats (their %EC, %Ey and %Y conversions and literal
// characters), matched against all eras at once without going back. ok false when none matches.
template <class __charT, class _It>
_It __time_read_era_year(_It s, _It end, const std::ctype<__charT>& __ct, const __time_data<__charT>& d, long long& year,
                         bool& ok, bool& __eof) {
  struct __cand {
    std::size_t i;     // position in the format
    long long __name;  // position in the era's name while inside %EC, else -1
    long long __num;   // the number read for %Ey (-1: none) or for %Y
    bool __full;       // the number is %Y's
    bool __alive;
  } __c[32];
  const int n = d.__neras;
  const __charT* const __text = d.__era_text.data();
  for (int k = 0; k < n; ++k)
    __c[k] = {0, -1, -1, false, d.__eras[k].__fmt_len != 0};
  ok = false;
  __eof = false;
  int __done = -1;
  auto __fmt = [&](int k) { return __text + d.__eras[k].__fmt_pos; };
  for (;;) {
    // normalize: enter and leave %EC, find the candidates that are complete
    int __want_num = 0, __want_char = 0;
    for (int k = 0; k < n; ++k) {
      __cand& c = __c[k];
      if (!c.__alive)
        continue;
      const __time_era& e = d.__eras[k];
      const __charT* __f = __fmt(k);
      for (;;) {
        if (c.__name >= 0 && static_cast<std::size_t>(c.__name) == e.__name_len) {
          c.__name = -1;
          c.i += 3;
          continue;
        }
        if (c.__name < 0 && c.i + 2 < e.__fmt_len && __ct.narrow(__f[c.i], 0) == '%' &&
            __ct.narrow(__f[c.i + 1], 0) == 'E' && __ct.narrow(__f[c.i + 2], 0) == 'C') {
          c.__name = 0;
          continue;
        }
        break;
      }
      if (c.__name < 0 && c.i == e.__fmt_len) {
        __done = k; // complete here
        c.__alive = false;
        continue;
      }
      if (c.__name < 0 && __ct.narrow(__f[c.i], 0) == '%') {
        const bool __ey = c.i + 2 < e.__fmt_len && __ct.narrow(__f[c.i + 1], 0) == 'E' && __ct.narrow(__f[c.i + 2], 0) == 'y';
        const bool __y = c.i + 1 < e.__fmt_len && __ct.narrow(__f[c.i + 1], 0) == 'Y';
        if (!__ey && !__y) {
          c.__alive = false; // a conversion an era format does not use
          continue;
        }
        ++__want_num;
      } else {
        ++__want_char;
      }
    }
    if (__want_num + __want_char == 0)
      break;
    if (s == end) {
      __eof = true;
      break;
    }
    const char __a = __ct.narrow(*s, 0);
    bool __num_here = __want_num != 0 && ((__a >= '0' && __a <= '9') || __a == '-' || d.__has_alt());
    if (__num_here && __want_char != 0) {
      // a character some candidate expects literally decides against a number
      const __charT __lc = __ct.tolower(*s);
      for (int k = 0; k < n && __num_here; ++k) {
        const __cand& c = __c[k];
        if (!c.__alive)
          continue;
        const __charT* __f = __fmt(k);
        const bool __lit = c.__name >= 0 || __ct.narrow(__f[c.i], 0) != '%';
        const __charT __e = c.__name >= 0 ? __text[d.__eras[k].__name_pos + static_cast<std::size_t>(c.__name)] : __f[c.i];
        if (__lit && __ct.tolower(__e) == __lc)
          __num_here = false;
      }
    }
    if (__num_here) {
      bool __neg = false;
      if (__a == '-') {
        __neg = true;
        ++s;
      }
      long long __v;
      bool __nok, __e2;
      s = ::__ycxx::__detail::__time_read_alt(s, end, __ct, &d, 6, __v, __nok, __e2);
      if (__neg)
        __v = -__v;
      for (int k = 0; k < n; ++k) {
        __cand& c = __c[k];
        if (!c.__alive)
          continue;
        const __charT* __f = __fmt(k);
        const bool __lit = c.__name >= 0 || __ct.narrow(__f[c.i], 0) != '%';
        if (__lit || !__nok) {
          c.__alive = false;
          continue;
        }
        c.__full = __ct.narrow(__f[c.i + 1], 0) == 'Y';
        if (!c.__full && __neg)
          c.__alive = false;
        c.__num = __v;
        c.i += c.__full ? 2 : 3;
      }
      __done = -1; // a completion before the number no longer covers everything read
      __eof = __eof || __e2;
      continue; // the candidates now complete are found by the next normalization
    }
    const __charT __lc = __ct.tolower(*s);
    bool any = false;
    for (int k = 0; k < n; ++k) {
      __cand& c = __c[k];
      if (!c.__alive)
        continue;
      const __charT* __f = __fmt(k);
      const bool __lit = c.__name >= 0 || __ct.narrow(__f[c.i], 0) != '%';
      const __charT __e = c.__name >= 0 ? __text[d.__eras[k].__name_pos + static_cast<std::size_t>(c.__name)] : __f[c.i];
      if (!__lit || __ct.tolower(__e) != __lc) {
        c.__alive = false;
        continue;
      }
      any = true;
      if (c.__name >= 0)
        ++c.__name;
      else
        ++c.i;
    }
    if (!any)
      break;
    ++s;
    __done = -1;
  }
  if (__done >= 0) {
    const __cand& c = __c[__done];
    const __time_era& e = d.__eras[__done];
    year = c.__full ? c.__num : e.__start_year + e.__step * ((c.__num < 0 ? e.__offset : c.__num) - e.__offset);
    ok = true;
  }
  return s;
}

// The named data of a time_get facet (null: the classic conventions or a program's own facet
// not derived from time_get_byname); for the chrono parser.
struct __time_get_access {
  template <class _Facet>
  static auto __data(const _Facet& __f) noexcept {
    return __f.__named_;
  }
};
// Fills d for the locale `name`; false (d untouched) for the names with classic semantics.
bool __named_time_data(const char* name, __time_data<char>& d);
bool __named_time_data(const char* name, __time_data<wchar_t>& d);
// time_put_byname: strftime_l (wcsftime_l) of "%<modifier><format>" in the locale. Writes at
// most cap characters; returns the full length.
std::size_t __named_strftime(const __named_locale* h, char* __buf, std::size_t __cap, const std::tm* t, char format,
                           char __modifier);
std::size_t __named_strftime(const __named_locale* h, wchar_t* __buf, std::size_t __cap, const std::tm* t, char format,
                           char __modifier);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

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
    // %EC and %Ey of a locale with eras: the era and the year within it (-1: not given)
    int __era = -1;
    long long __era_year = -1;
    const bool __eras = __named_ != nullptr && __named_->__neras != 0;
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
        const bool __era_part = __eras && __mod == 'E' && (__spec == 'C' || __spec == 'y');
        if (__spec == 'U' || __spec == 'W') {
          // read here (as do_get would) to keep the week number for the date below
          s = __read_field(__mod == 'O' && __named_ != nullptr && __named_->__has_alt(), s, end, __f, __err, 2, 0, 53,
                           __week);
          __week_kind = __spec;
        } else if (__era_part) {
          // read here: the era and its year combine below
          bool __eof = false, ok = false;
          if (__spec == 'C') {
            s = __match_era(s, end, __ct, __era, __eof);
            ok = __era >= 0;
          } else {
            s = ::__ycxx::__detail::__time_read_alt(s, end, __ct, __named_, 6, __era_year, ok, __eof);
          }
          if (__eof)
            __err |= ios_base::eofbit;
          if (!ok)
            __err |= ios_base::failbit;
        } else {
          s = do_get(s, end, __f, __err, t, __spec, __mod);
        }
        if (!(__err & ios_base::failbit) && !__era_part) {
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
    if (!(__err & ios_base::failbit) && (__era >= 0 || __era_year >= 0)) {
      long long y;
      if (__era >= 0) {
        const __ycxx::__detail::__time_era& e = __named_->__eras[__era];
        y = e.__start_year + e.__step * ((__era_year >= 0 ? __era_year : e.__offset) - e.__offset);
      } else { // a year of an unnamed era: as %y
        y = __era_year + (__era_year < 69 ? 2000 : 1900);
        if (__era_year > 99)
          __err |= ios_base::failbit;
      }
      if (y - 1900 < -__INT_MAX__ || y - 1900 > __INT_MAX__)
        __err |= ios_base::failbit;
      else
        t->tm_year = static_cast<int>(y - 1900);
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
  virtual dateorder do_date_order() const { return __named_ != nullptr ? __named_->__order : mdy; }
  virtual iter_type do_get_time(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    return parse(s, end, __f, __err, t, "%H:%M:%S");
  }
  virtual iter_type do_get_date(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    // [locale.time.get.virtuals]/4: what time_put produces for "%d%m%y" (or the order's
    // permutation), which has no separators; /5: other formats may be accepted too, here the
    // same fields separated by '/' ('?' in the pattern: an optional '/'). A named locale's facet
    // reads its %x format instead (the one its time_put writes for %x).
    if (__named_ != nullptr && !__named_->__d_fmt.empty())
      return __parse_format(s, end, __f, __err, t, __named_->__d_fmt);
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
    // e: this call's state (err may hold failbit already on entry; it is only or-ed into)
    int __v;
    ios_base::iostate e = ios_base::goodbit;
    if (__named_ != nullptr)
      s = __match_name(s, end, __f, e, __named_->__names, __named_->__written, 14, __v);
    else
      s = __match_name(s, end, __f, e, __ycxx::__detail::__c_weekday_names, 14, __v);
    if (!(e & ios_base::failbit))
      t->tm_wday = __v % 7;
    __err |= e;
    return s;
  }
  virtual iter_type do_get_monthname(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    int __v;
    ios_base::iostate e = ios_base::goodbit;
    if (__named_ != nullptr)
      s = __match_name(s, end, __f, e, __named_->__names + 14, __named_->__written + 14, 24, __v);
    else
      s = __match_name(s, end, __f, e, __ycxx::__detail::__c_month_names, 24, __v);
    if (!(e & ios_base::failbit))
      t->tm_mon = __v % 12;
    __err |= e;
    return s;
  }
  virtual iter_type do_get_year(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t) const {
    int y, digits;
    ios_base::iostate e = ios_base::goodbit;
    s = __read_number(s, end, __f, e, 4, y, &digits);
    if (!(e & ios_base::failbit))
      t->tm_year = (digits <= 2 ? (y < 69 ? y + 2000 : y + 1900) : y) - 1900;
    __err |= e;
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
    // POSIX's O forms, and %OC, which locales use in their formats (glibc's my_MM: "%OC%Oy")
    if (__modifier == 'O' && !(format == 'd' || format == 'e' || format == 'H' || format == 'I' || format == 'm' ||
                             format == 'M' || format == 'S' || format == 'U' || format == 'w' || format == 'W' ||
                             format == 'y' || format == 'C')) {
      __err |= ios_base::failbit;
      return s;
    }
    if (__modifier != 0 && __modifier != 'E' && __modifier != 'O') {
      __err |= ios_base::failbit;
      return s;
    }
    tm r = *t; // assigned to *t only on success
    int __v = 0;
    // a named locale's %c, %x, %X and %r are its own formats; %Ec, %Ex and %EX its era formats
    // where it has them
    if (__named_ != nullptr) {
      const bool __e = __modifier == 'E';
      const basic_string<__charT>* __own =
          format == 'c'   ? (__e && !__named_->__era_d_t_fmt.empty() ? &__named_->__era_d_t_fmt : &__named_->__d_t_fmt)
          : format == 'x' ? (__e && !__named_->__era_d_fmt.empty() ? &__named_->__era_d_fmt : &__named_->__d_fmt)
          : format == 'X' ? (__e && !__named_->__era_t_fmt.empty() ? &__named_->__era_t_fmt : &__named_->__t_fmt)
          : format == 'r' ? &__named_->__t_fmt_ampm
                          : nullptr;
      if (__own != nullptr && !__own->empty()) {
        s = __parse_format(s, end, __f, __err, &r, *__own);
        if (!(__err & ios_base::failbit))
          *t = r;
        return s;
      }
    }
    // %EC, %Ey and %EY of a locale with eras (strptime's): an era name (the year is the era's
    // first), a year of an unnamed era (as %y), a full era year (an era's format)
    if (__modifier == 'E' && __named_ != nullptr && __named_->__neras != 0 &&
        (format == 'C' || format == 'y' || format == 'Y')) {
      const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
      bool ok = false, __eof = false;
      long long y = 0;
      if (format == 'Y') {
        s = ::__ycxx::__detail::__time_read_era_year(s, end, __ct, *__named_, y, ok, __eof);
      } else if (format == 'C') {
        int __k;
        s = __match_era(s, end, __ct, __k, __eof);
        if ((ok = __k >= 0))
          y = __named_->__eras[__k].__start_year;
      } else {
        s = ::__ycxx::__detail::__time_read_alt(s, end, __ct, __named_, 2, y, ok, __eof);
        ok = ok && y <= 99;
        y += y < 69 ? 2000 : 1900;
      }
      if (__eof)
        __err |= ios_base::eofbit;
      if (!ok || y - 1900 < -__INT_MAX__ || y - 1900 > __INT_MAX__)
        __err |= ios_base::failbit;
      else
        t->tm_year = static_cast<int>(y - 1900);
      return s;
    }
    // the O forms read the locale's alternative digits too
    const bool __alt = __modifier == 'O' && __named_ != nullptr && __named_->__has_alt();
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
      if (__alt)
        s = __read_field(true, s, end, __f, __err, 2, 0, 99, __v);
      else
        s = __read_number(s, end, __f, __err, 2, __v);
      if (!(__err & ios_base::failbit))
        r.tm_year = __v * 100 - 1900 + (r.tm_year + 1900) % 100;
      break;
    case 'd':
    case 'e':
      s = __skip_space(s, end, __f, __err);
      s = __read_field(__alt, s, end, __f, __err, 2, 1, 31, r.tm_mday);
      break;
    case 'D':
    case 'x':
      s = parse(s, end, __f, __err, &r, "%m/%d/%y");
      break;
    case 'F':
      s = parse(s, end, __f, __err, &r, "%Y-%m-%d");
      break;
    case 'H':
      s = __read_field(__alt, s, end, __f, __err, 2, 0, 23, r.tm_hour);
      break;
    case 'I':
      s = __read_field(__alt, s, end, __f, __err, 2, 1, 12, __v);
      if (!(__err & ios_base::failbit))
        r.tm_hour = __v % 12 + (r.tm_hour >= 12 ? 12 : 0);
      break;
    case 'j':
      s = __read_field(__alt, s, end, __f, __err, 3, 1, 366, __v);
      if (!(__err & ios_base::failbit))
        r.tm_yday = __v - 1;
      break;
    case 'm':
      s = __read_field(__alt, s, end, __f, __err, 2, 1, 12, __v);
      if (!(__err & ios_base::failbit))
        r.tm_mon = __v - 1;
      break;
    case 'M':
      s = __read_field(__alt, s, end, __f, __err, 2, 0, 59, r.tm_min);
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
      s = __read_field(__alt, s, end, __f, __err, 2, 0, 60, r.tm_sec);
      break;
    case 'T':
    case 'X':
      s = parse(s, end, __f, __err, &r, "%H:%M:%S");
      break;
    case 'u':
      s = __read_field(__alt, s, end, __f, __err, 1, 1, 7, __v);
      if (!(__err & ios_base::failbit))
        r.tm_wday = __v % 7;
      break;
    case 'U':
    case 'W':
    case 'V':
      s = __read_field(__alt, s, end, __f, __err, 2, 0, 53, __v);
      break;
    case 'w':
      s = __read_field(__alt, s, end, __f, __err, 1, 0, 6, r.tm_wday);
      break;
    case 'y':
      s = __read_field(__alt, s, end, __f, __err, 2, 0, 99, __v);
      if (!(__err & ios_base::failbit))
        r.tm_year = __v < 69 ? __v + 100 : __v;
      break;
    case 'Y':
      s = __read_number(s, end, __f, __err, 4, __v);
      if (!(__err & ios_base::failbit))
        r.tm_year = __v - 1900;
      break;
    case 'Z': // a time zone name: read, not converted (as strptime)
      s = __skip_space(s, end, __f, __err);
      while (s != end && !use_facet<ctype<__charT>>(__f.getloc()).is(ctype_base::space, *s))
        ++s;
      if (s == end)
        __err |= ios_base::eofbit;
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
  template <class, class>
  friend class time_get_byname;
  friend struct ::__ycxx::__detail::__time_get_access;
  // A named locale's names and formats (time_get_byname's; null: the "C" locale's).
  const __ycxx::__detail::__time_data<__charT>* __named_ = nullptr;

  // An era name of the named locale (k: its era, else -1).
  iter_type __match_era(iter_type s, iter_type end, const ctype<__charT>& __ct, int& __k, bool& __eof) const {
    const __ycxx::__detail::__time_data<__charT>* d = __named_;
    return ::__ycxx::__detail::__time_match(s, end, __ct, d->__neras,
                                            [d](int k, size_t& __len) { return d->__era_name(k, __len); }, __k, __eof);
  }
  // A number field: with alt (an O form in a locale with alternative digits), also one of those.
  iter_type __read_field(bool __alt, iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, int __max_digits,
                         int __lo, int __hi, int& out) const {
    if (__alt && s != end) {
      const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
      const char c = __ct.narrow(*s, 0);
      if (c < '0' || c > '9') {
        long long __v;
        bool ok, __eof;
        s = ::__ycxx::__detail::__time_read_alt(s, end, __ct, __named_, __max_digits, __v, ok, __eof);
        if (__eof)
          __err |= ios_base::eofbit;
        if (!ok || __v < __lo || __v > __hi)
          __err |= ios_base::failbit;
        else
          out = static_cast<int>(__v);
        return s;
      }
    }
    return __read_ranged(s, end, __f, __err, __max_digits, __lo, __hi, out);
  }

  // Parses a named locale's format with get() (its conversions with do_get).
  iter_type __parse_format(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm* t,
                         const basic_string<__charT>& __fmt) const {
    ios_base::iostate e = ios_base::goodbit;
    s = get(s, end, __f, e, t, __fmt.data(), __fmt.data() + __fmt.size());
    if (s == end) // as parse() reports the end of the input
      e |= ios_base::eofbit;
    __err |= e;
    return s;
  }
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
  iter_type __read_ampm(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err, tm& r) const {
    static constexpr const char* __names[2] = {"AM", "PM"};
    int __v;
    if (__named_ != nullptr) {
      // a locale without AM/PM strings (most 24-hour locales) has nothing to read
      if (__named_->__names[38].empty() && __named_->__names[39].empty())
        return s;
      s = __match_name(s, end, __f, __err, __named_->__names + 38, __named_->__written + 38, 2, __v);
    } else {
      s = __match_name(s, end, __f, __err, __names, 2, __v);
    }
    if (!(__err & ios_base::failbit)) {
      // an hour not read yet (out of range: a %p before the %I, as in ja_JP's "%p%I時%M分%S秒")
      // is 0, so the %I that follows keeps the half of the day
      r.tm_hour = (r.tm_hour >= 0 && r.tm_hour <= 23 ? r.tm_hour % 12 : 0) + (__v == 1 ? 12 : 0);
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
  // The same for a named locale's names and their written forms (n of each; name k or written k
  // is index k), compared through the stream's ctype<charT>::tolower, after the white space
  // before the name (both are stored without theirs); an empty string never matches.
  static iter_type __match_name(iter_type s, iter_type end, ios_base& __f, ios_base::iostate& __err,
                              const basic_string<__charT>* __names, const basic_string<__charT>* __written, int n,
                              int& __which) {
    const ctype<__charT>& __ct = use_facet<ctype<__charT>>(__f.getloc());
    while (s != end && __ct.is(ctype_base::space, *s))
      ++s;
    auto __str = [&](int k) -> const basic_string<__charT>& { return k < n ? __names[k] : __written[k - n]; };
    bool __alive[48];
    for (int k = 0; k < 2 * n; ++k)
      __alive[k] = !__str(k).empty();
    size_t i = 0;
    for (;;) {
      bool __more = false;
      for (int k = 0; k < 2 * n; ++k)
        __more = __more || (__alive[k] && i < __str(k).size());
      if (!__more)
        break;
      if (s == end) {
        __err |= ios_base::eofbit;
        break;
      }
      const __charT c = __ct.tolower(*s);
      bool any = false;
      for (int k = 0; k < 2 * n; ++k)
        any = any || (__alive[k] && i < __str(k).size() && __ct.tolower(__str(k)[i]) == c);
      if (!any)
        break;
      for (int k = 0; k < 2 * n; ++k)
        __alive[k] = __alive[k] && i < __str(k).size() && __ct.tolower(__str(k)[i]) == c;
      ++s;
      ++i;
    }
    if (s == end)
      __err |= ios_base::eofbit;
    __which = -1;
    for (int k = 0; k < 2 * n; ++k)
      if (__alive[k] && __str(k).size() == i && i != 0) {
        __which = k % n;
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
  // char and wchar_t: LC_TIME's names and formats, read here; other character types (and the
  // names with classic semantics): the "C" locale's.
  explicit time_get_byname(const char* name, size_t __refs = 0) : time_get<__charT, _InputIterator>(__refs) {
    if constexpr (is_same_v<__charT, char> || is_same_v<__charT, wchar_t>) {
      if (::__ycxx::__detail::__named_time_data(name, __data_))
        this->__named_ = &__data_;
    } else {
      ::__ycxx::__detail::__check_locale_name(name, "std::time_get_byname");
    }
  }
  explicit time_get_byname(const string& name, size_t __refs = 0) : time_get_byname(name.c_str(), __refs) {}

protected:
  ~time_get_byname() override {}

private:
  __ycxx::__detail::__time_data<__charT> __data_;
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
  // char and wchar_t: strftime_l / wcsftime_l in the name's LC_TIME; other character types (and
  // the names with classic semantics): the "C" locale's conversions.
  explicit time_put_byname(const char* name, size_t __refs = 0) : time_put<__charT, _OutputIterator>(__refs) {
    if constexpr (is_same_v<__charT, char> || is_same_v<__charT, wchar_t>)
      __named_ = ::__ycxx::__detail::__named_open(name, locale::time, "std::time_put_byname");
    else
      ::__ycxx::__detail::__check_locale_name(name, "std::time_put_byname");
  }
  explicit time_put_byname(const string& name, size_t __refs = 0) : time_put_byname(name.c_str(), __refs) {}

protected:
  ~time_put_byname() override { ::__ycxx::__detail::__named_release(__named_); }
  iter_type do_put(iter_type s, ios_base& str, char_type fill, const tm* t, char format, char __modifier) const override {
    if constexpr (is_same_v<__charT, char> || is_same_v<__charT, wchar_t>) {
      if (__named_ != nullptr) {
        __charT __y_local[128];
        size_t n = __ycxx::__detail::__named_strftime(__named_, __y_local, 128, t, format, __modifier);
        __ycxx::__detail::__small_buffer<__charT, 1> big(n > 128 ? n : 0);
        const __charT* __text = __y_local;
        if (n > 128) {
          n = __ycxx::__detail::__named_strftime(__named_, big.get(), n, t, format, __modifier);
          __text = big.get();
        }
        for (size_t i = 0; i < n; ++i, static_cast<void>(++s))
          *s = __text[i];
        return s;
      }
    }
    return time_put<__charT, _OutputIterator>::do_put(s, str, fill, t, format, __modifier);
  }

private:
  __ycxx::__detail::__named_locale* __named_ = nullptr;
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

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// LC_MONETARY of a named locale as moneypunct_byname reads it; initialized to the values of the
// base moneypunct.
template <class __charT>
struct __money_data {
  __charT __point = std::numeric_limits<__charT>::max();
  __charT __sep = std::numeric_limits<__charT>::max();
  std::string grouping;
  std::basic_string<__charT> symbol, __positive, __negative = std::basic_string<__charT>(1, __charT('-'));
  int frac_digits = 0;
  std::money_base::pattern __pos{{std::money_base::symbol, std::money_base::sign, std::money_base::none,
                                std::money_base::value}};
  std::money_base::pattern __neg = __pos;
};
// Fills d for the locale `name` from localeconv (the int_ members for intl); d is untouched for
// the names with classic semantics. Separators as for named_numpunct.
void __named_money_data(const char* name, bool intl, __money_data<char>& d);
void __named_money_data(const char* name, bool intl, __money_data<wchar_t>& d);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [locale.moneypunct.byname]: for char and wchar_t, the C library's LC_MONETARY of the name, read
// once, here. The patterns follow POSIX's p_cs_precedes, p_sep_by_space and p_sign_posn (n_ for
// neg_format(), int_p_/int_n_ for Intl): a separating space is the pattern's space field, and a
// sign position of 0 (parentheses) gives the sign string "()". Other character types: the base's
// values.
template <class __charT, bool _Intl>
class moneypunct_byname : public moneypunct<__charT, _Intl> {
public:
  using pattern = money_base::pattern;
  using string_type = basic_string<__charT>;
  explicit moneypunct_byname(const char* name, size_t __refs = 0) : moneypunct<__charT, _Intl>(__refs) {
    if constexpr (is_same_v<__charT, char> || is_same_v<__charT, wchar_t>)
      ::__ycxx::__detail::__named_money_data(name, _Intl, __data_);
    else
      ::__ycxx::__detail::__check_locale_name(name, "std::moneypunct_byname");
  }
  explicit moneypunct_byname(const string& name, size_t __refs = 0) : moneypunct_byname(name.c_str(), __refs) {}

protected:
  ~moneypunct_byname() override {}
  __charT do_decimal_point() const override { return __data_.__point; }
  __charT do_thousands_sep() const override { return __data_.__sep; }
  string do_grouping() const override { return __data_.grouping; }
  string_type do_curr_symbol() const override { return __data_.symbol; }
  string_type do_positive_sign() const override { return __data_.__positive; }
  string_type do_negative_sign() const override { return __data_.__negative; }
  int do_frac_digits() const override { return __data_.frac_digits; }
  pattern do_pos_format() const override { return __data_.__pos; }
  pattern do_neg_format() const override { return __data_.__neg; }

private:
  __ycxx::__detail::__money_data<__charT> __data_;
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
        // without showbase the symbol is optional, consumed only if other characters are needed
        // to complete the format: a later value, a later space that is not last, a later sign
        // when neither sign string is empty, or the rest of a sign already begun
        bool __needed = showbase || (sign != nullptr && sign->size() > 1);
        for (int __j = i + 1; __j < 4; ++__j) {
          const auto __f = static_cast<money_base::part>(__pat.field[__j]);
          __needed = __needed || __f == money_base::value || (__f == money_base::space && __j < 3) ||
                   (__f == money_base::sign && !__pos.empty() && !__neg.empty());
        }
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
        // [locale.moneypunct.general]/3: units ::= digits [thousands-sep units], so a separator
        // only follows a digit (not first, not doubled, not before the decimal point); one that
        // does not ends the value there
        bool __point_seen = false;
        bool __after_sep = false;
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
            __after_sep = false;
            digits.push_back(__ct.narrow(c, '0'));
          } else if (__frac > 0 && !__point_seen && !__after_sep && c == __point) {
            __point_seen = true;
          } else if (!grouping.empty() && !__point_seen && run > 0 && c == __sep) {
            if (__ngroups < 64)
              __groups[__ngroups++] = run;
            run = 0;
            __seen_sep = true;
            __after_sep = true;
          } else {
            break;
          }
          ++s;
        }
        // a decimal point must be followed by exactly frac_digits() digits; without one, the
        // digits are stored as they appear
        if (digits.empty() || __after_sep || (__point_seen && __frac_got != __frac)) {
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
// The C library's message catalogs (catopen with NL_CAT_LOCALE in the name's LC_MESSAGES; catgets,
// whose text is converted through the name's LC_CTYPE for wchar_t); src/hosted/locale_named.cpp.
// For the names with classic semantics, the base messages (no catalogs).
template <>
class messages_byname<char> : public messages<char> {
public:
  using catalog = messages_base::catalog;
  using string_type = string;
  explicit messages_byname(const char* name, size_t __refs = 0)
      : messages(__refs), __named_(::__ycxx::__detail::__named_open(name, locale::messages, "std::messages_byname")) {}
  explicit messages_byname(const string& name, size_t __refs = 0) : messages_byname(name.c_str(), __refs) {}

protected:
  ~messages_byname() override;
  catalog do_open(const string& __fn, const locale& __loc) const override;
  string_type do_get(catalog c, int set, int __msgid, const string_type& __dfault) const override;
  void do_close(catalog c) const override;

private:
  __ycxx::__detail::__named_locale* __named_;
};
template <>
class messages_byname<wchar_t> : public messages<wchar_t> {
public:
  using catalog = messages_base::catalog;
  using string_type = wstring;
  explicit messages_byname(const char* name, size_t __refs = 0)
      : messages(__refs), __named_(::__ycxx::__detail::__named_open(name, locale::messages, "std::messages_byname")) {}
  explicit messages_byname(const string& name, size_t __refs = 0) : messages_byname(name.c_str(), __refs) {}

protected:
  ~messages_byname() override;
  catalog do_open(const string& __fn, const locale& __loc) const override;
  string_type do_get(catalog c, int set, int __msgid, const string_type& __dfault) const override;
  void do_close(catalog c) const override;

private:
  __ycxx::__detail::__named_locale* __named_;
};

} // namespace std
