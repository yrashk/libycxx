// libycxx hosted: parsing of <chrono> values ([time.parse]): the from_stream overloads of every
// parsable type and the parse manipulators.
//
// One scanner reads the input as the format directs and records each field it finds (year,
// century, ISO year, month, day, day of the year, week numbers, weekday, time of day, offset,
// abbreviation); each from_stream then builds its value from the fields, or sets failbit when a
// flag refers to information its type cannot represent ([time.parse]/16), when the input does
// not match, or when the fields do not determine a valid value ([time.parse]/17). Names (%a %b
// %p) and the representations %c %x %X %r are those of the "C" locale; white space is
// classified by the stream's ctype facet and the decimal point of %S is '.' or the stream
// locale's. The manipulators returned by parse are neither copyable nor movable and extract
// only as rvalues, so they cannot outlive the full-expression that holds the format string.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/chrono_cal.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/hosted/chrono_io.hpp>
#include <ycxx/hosted/chrono_tz.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __chrono_parsed {
  enum : unsigned {
    __has_Y = 1u << 0,
    __has_C = 1u << 1,
    __has_y = 1u << 2,
    __has_G = 1u << 3,
    __has_g = 1u << 4,
    __has_m = 1u << 5,
    __has_d = 1u << 6,
    __has_j = 1u << 7,
    __has_U = 1u << 8,
    __has_W = 1u << 9,
    __has_V = 1u << 10,
    __has_wd = 1u << 11,
    __has_H = 1u << 12,
    __has_I = 1u << 13,
    __has_p = 1u << 14,
    __has_M = 1u << 15,
    __has_S = 1u << 16,
    __has_z = 1u << 17,
    __has_Z = 1u << 18,
  };
  unsigned __have = 0;
  long long _Yp = 0, _Cp = 0, y = 0, _Gp = 0, __g = 0, m = 0, d = 0, __j = 0, _Up = 0, _Wp = 0, _Vp = 0, __wd = 0;
  long long _Hp = 0, _Ip = 0, _Mp = 0, _Sp = 0;
  unsigned long long __sub = 0; // fractional seconds in units of 10^-digits
  bool __pm = false;
  long long offset = 0; // minutes
  bool any(unsigned __f) const noexcept { return (__have & __f) != 0; }
};

constexpr bool __chrono_ieq(char a, char b) noexcept {
  return (a >= 'A' && a <= 'Z' ? a + 32 : a) == (b >= 'A' && b <= 'Z' ? b + 32 : b);
}

// The scanner: reads is (through its streambuf) as fmt directs.
template <class __charT, class __traits>
class __chrono_scanner {
  using int_type = typename __traits::int_type;
  std::basic_streambuf<__charT, __traits>* __sb_;
  const std::ctype<__charT>& __ct_;
  __charT __point_;
  bool __eof_ = false;

public:
  __chrono_parsed r;
  std::basic_string<__charT, __traits> abbrev;
  unsigned info;   // what the type can represent (ci_* of chrono_io.hpp)
  unsigned digits; // the fractional digits of %S

  __chrono_scanner(std::basic_istream<__charT, __traits>& is, unsigned __inf, unsigned __dig)
      : __sb_(is.rdbuf()), __ct_(std::use_facet<std::ctype<__charT>>(is.getloc())),
        __point_(std::use_facet<std::numpunct<__charT>>(is.getloc()).decimal_point()), info(__inf), digits(__dig) {}

  bool __at_eof() const noexcept { return __eof_; }

private:
  int_type peek() {
    if (__eof_)
      return __traits::eof();
    const int_type c = __sb_->sgetc();
    if (__traits::eq_int_type(c, __traits::eof()))
      __eof_ = true;
    return c;
  }
  void __bump() { __sb_->sbumpc(); }
  bool __is_space(int_type c) const {
    return !__traits::eq_int_type(c, __traits::eof()) && __ct_.is(std::ctype_base::space, __traits::to_char_type(c));
  }
  // The character as ASCII ('\0' for EOF or anything else).
  char narrow(int_type c) const {
    return __traits::eq_int_type(c, __traits::eof()) ? '\0' : __ct_.narrow(__traits::to_char_type(c), '\0');
  }

  // An unsigned number of at most n digits (at least one).
  bool __number(int n, long long& __v) {
    __v = 0;
    int k = 0;
    for (; k < n; ++k) {
      const char c = narrow(peek());
      if (c < '0' || c > '9')
        break;
      if (__v > 99'999'999'999'999'999LL) // 18 digits: no field needs more
        return false;
      __v = __v * 10 + (c - '0');
      __bump();
    }
    return k > 0;
  }
  // A number of exactly n digits.
  bool __exact(int n, long long& __v) {
    __v = 0;
    for (int k = 0; k < n; ++k) {
      const char c = narrow(peek());
      if (c < '0' || c > '9')
        return false;
      __v = __v * 10 + (c - '0');
      __bump();
    }
    return true;
  }
  // [+|-] and a number of at most n digits.
  bool __signed_number(int n, long long& __v) {
    const char c = narrow(peek());
    bool __neg = false;
    if (c == '+' || c == '-') {
      __neg = c == '-';
      __bump();
    }
    if (!__number(n, __v))
      return false;
    if (__neg)
      __v = -__v;
    return true;
  }
  // One of the names (full, or the first three letters), case-insensitively; the index.
  bool name(const char* const* __names, int count, long long& index) {
    char __text[16];
    int __len = 0;
    for (;;) {
      const char c = narrow(peek());
      bool __extends = false;
      for (int i = 0; i < count && !__extends; ++i) {
        const char* n = __names[i];
        int k = 0;
        while (k < __len && n[k] != 0 && ::__ycxx::__detail::__chrono_ieq(n[k], __text[k]))
          ++k;
        __extends = k == __len && n[__len] != 0 && ::__ycxx::__detail::__chrono_ieq(n[__len], c);
      }
      if (!__extends || __len == 15)
        break;
      __text[__len++] = c;
      __bump();
    }
    for (int i = 0; i < count; ++i) {
      const char* n = __names[i];
      int k = 0;
      while (k < __len && n[k] != 0 && ::__ycxx::__detail::__chrono_ieq(n[k], __text[k]))
        ++k;
      if (k == __len && (n[k] == 0 || __len == 3)) {
        index = i;
        return true;
      }
    }
    return false;
  }
  bool __need(unsigned what) const { return (info & what) == what; }

  bool __seconds_field(int n) {
    // [time.parse]: %S as a decimal number, with the type's fractional digits when it has any.
    long long s;
    if (!__number(n < 0 ? 2 : (n > 2 ? 2 : n), s))
      return false;
    r._Sp = s;
    r.__sub = 0;
    r.__have |= __chrono_parsed::__has_S;
    if (digits != 0 && __traits::eq_int_type(peek(), __traits::to_int_type(__point_))) {
      __bump();
      unsigned k = 0;
      for (; k < digits; ++k) {
        const char c = narrow(peek());
        if (c < '0' || c > '9')
          break;
        r.__sub = r.__sub * 10 + static_cast<unsigned>(c - '0');
        __bump();
      }
      if (k == 0)
        return false;
      for (; k < digits; ++k)
        r.__sub *= 10;
    }
    return true;
  }

  bool offset(bool __modified) {
    const char c = narrow(peek());
    bool __neg = false;
    if (c == '+' || c == '-') {
      __neg = c == '-';
      __bump();
    }
    long long h = 0, __mins = 0;
    if (!__modified) { // [+|-]hh[mm]
      if (!__exact(2, h))
        return false;
      const char __c2 = narrow(peek());
      if (__c2 >= '0' && __c2 <= '9' && !__exact(2, __mins))
        return false;
    } else { // [+|-]h[h][:mm]
      if (!__number(2, h))
        return false;
      if (narrow(peek()) == ':') {
        __bump();
        if (!__exact(2, __mins))
          return false;
      }
    }
    if (__mins > 59)
      return false;
    r.offset = (__neg ? -1 : 1) * (h * 60 + __mins);
    r.__have |= __chrono_parsed::__has_z;
    return true;
  }

  bool __zone_name() {
    std::size_t n = 0;
    for (;;) {
      const char c = narrow(peek());
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '/' ||
            c == '-' || c == '+'))
        break;
      abbrev.push_back(__traits::to_char_type(peek()));
      __bump();
      ++n;
    }
    if (n == 0)
      return false;
    r.__have |= __chrono_parsed::__has_Z;
    return true;
  }

  // Runs the NTBS pattern of a composite flag; a width n applies to its first field.
  bool pattern(const char* p, int n = -1) {
    for (; *p != 0; ++p) {
      if (*p == '%') {
        ++p;
        if (!__flag(*p, 0, n))
          return false;
        n = -1;
      } else if (*p == ' ') {
        while (__is_space(peek()))
          __bump();
      } else {
        if (narrow(peek()) != *p)
          return false;
        __bump();
      }
    }
    return true;
  }

public:
  // One flag; n is the width (-1: the default).
  bool __flag(char __f, char __mod, int n) {
    using _Pp = __chrono_parsed;
    auto num = [&](int __dflt, long long& __v, unsigned __bit, unsigned what) {
      if (!__need(what))
        return false;
      if (!__number(n < 0 ? __dflt : n, __v))
        return false;
      r.__have |= __bit;
      return true;
    };
    if (__mod == 'E' && !(__f == 'c' || __f == 'C' || __f == 'x' || __f == 'X' || __f == 'y' || __f == 'Y' || __f == 'z'))
      return false;
    if (__mod == 'O' && !(__f == 'd' || __f == 'e' || __f == 'H' || __f == 'I' || __f == 'm' || __f == 'M' || __f == 'S' ||
                        __f == 'u' || __f == 'U' || __f == 'V' || __f == 'w' || __f == 'W' || __f == 'y' || __f == 'z'))
      return false;
    switch (__f) {
    case 'a':
    case 'A':
      if (!__need(__ci_weekday) || !name(__chrono_weekday_names, 7, r.__wd))
        return false;
      r.__have |= _Pp::__has_wd;
      return true;
    case 'b':
    case 'B':
    case 'h':
      if (!__need(__ci_month) || !name(__chrono_month_names, 12, r.m))
        return false;
      ++r.m;
      r.__have |= _Pp::__has_m;
      return true;
    case 'c': return __need(__ci_full_date | __ci_time) && pattern("%a %b %e %H:%M:%S %Y");
    case 'C':
      if (!__need(__ci_year) || !__signed_number(n < 0 ? 2 : n, r._Cp))
        return false;
      r.__have |= _Pp::__has_C;
      return true;
    case 'e': // as written by %e: a leading space
      while (__is_space(peek()))
        __bump();
      return num(2, r.d, _Pp::__has_d, __ci_day);
    case 'd': return num(2, r.d, _Pp::__has_d, __ci_day);
    case 'D': return pattern("%m/%d/%y", n);
    case 'F': return pattern("%Y-%m-%d", n);
    case 'g': return num(2, r.__g, _Pp::__has_g, __ci_date);
    case 'G':
      if (!__need(__ci_date) || !__signed_number(n < 0 ? 4 : n, r._Gp))
        return false;
      r.__have |= _Pp::__has_G;
      return true;
    case 'H': return num(2, r._Hp, _Pp::__has_H, __ci_time);
    case 'I': return num(2, r._Ip, _Pp::__has_I, __ci_time);
    case 'j':
      if ((info & __ci_duration) != 0) {
        if (!__number(n < 0 ? 3 : n, r.__j))
          return false;
        r.__have |= _Pp::__has_j;
        return true;
      }
      return num(3, r.__j, _Pp::__has_j, __ci_date);
    case 'm': return num(2, r.m, _Pp::__has_m, __ci_month);
    case 'M': return num(2, r._Mp, _Pp::__has_M, __ci_time);
    case 'n':
      if (!__is_space(peek()))
        return false;
      __bump();
      return true;
    case 't':
      if (__is_space(peek()))
        __bump();
      return true;
    case 'p': {
      if (!__need(__ci_time))
        return false;
      const char a = narrow(peek());
      if (!::__ycxx::__detail::__chrono_ieq(a, 'A') && !::__ycxx::__detail::__chrono_ieq(a, 'P'))
        return false;
      __bump();
      if (!::__ycxx::__detail::__chrono_ieq(narrow(peek()), 'M'))
        return false;
      __bump();
      r.__pm = ::__ycxx::__detail::__chrono_ieq(a, 'P');
      r.__have |= _Pp::__has_p;
      return true;
    }
    case 'r': return __need(__ci_time) && pattern("%I:%M:%S %p");
    case 'R': return __need(__ci_time) && pattern("%H:%M");
    case 'S': return __need(__ci_time) && __seconds_field(n);
    case 'T': return __need(__ci_time) && pattern("%H:%M:%S");
    case 'u':
      if (!num(1, r.__wd, _Pp::__has_wd, __ci_weekday))
        return false;
      if (r.__wd < 1 || r.__wd > 7)
        return false;
      r.__wd %= 7;
      return true;
    case 'w':
      if (!num(1, r.__wd, _Pp::__has_wd, __ci_weekday))
        return false;
      return r.__wd <= 6;
    case 'U': return num(2, r._Up, _Pp::__has_U, __ci_date);
    case 'W': return num(2, r._Wp, _Pp::__has_W, __ci_date);
    case 'V': return num(2, r._Vp, _Pp::__has_V, __ci_date);
    case 'x': return pattern("%m/%d/%y");
    case 'X': return __need(__ci_time) && pattern("%H:%M:%S");
    case 'y': return num(2, r.y, _Pp::__has_y, __ci_year);
    case 'Y':
      if (!__need(__ci_year) || !__signed_number(n < 0 ? 4 : n, r._Yp))
        return false;
      r.__have |= _Pp::__has_Y;
      return true;
    case 'z': return offset(__mod != 0);
    case 'Z': return __zone_name();
    case '%':
      if (narrow(peek()) != '%')
        return false;
      __bump();
      return true;
    default: return false;
    }
  }

  // The whole format.
  bool run(const __charT* __fmt) {
    for (const __charT* p = __fmt; *p != __charT(); ++p) {
      if (__ct_.is(std::ctype_base::space, *p)) {
        while (__is_space(peek()))
          __bump();
        continue;
      }
      if (!__traits::__eq(*p, __charT('%'))) {
        const int_type c = peek();
        if (__traits::eq_int_type(c, __traits::eof()) || !__traits::__eq(__traits::to_char_type(c), *p))
          return false;
        __bump();
        continue;
      }
      ++p;
      int n = -1;
      while (*p != __charT() && __ct_.narrow(*p, '\0') >= '0' && __ct_.narrow(*p, '\0') <= '9') {
        n = (n < 0 ? 0 : n * 10) + (__ct_.narrow(*p, '\0') - '0');
        if (n > 1000)
          return false;
        ++p;
      }
      if (n == 0)
        return false;
      char __mod = 0;
      if (*p == __charT('E') || *p == __charT('O'))
        __mod = __ct_.narrow(*p++, '\0');
      if (*p == __charT())
        return false;
      if (!__flag(__ct_.narrow(*p, '\0'), __mod, n))
        return false;
    }
    return true;
  }
};

// ---- building values from the fields -------------------------------------------------------

constexpr bool __chrono_year_of(const __chrono_parsed& r, long long& y) noexcept {
  using _Pp = __chrono_parsed;
  if (r.any(_Pp::__has_Y)) {
    y = r._Yp;
  } else if (r.any(_Pp::__has_C | _Pp::__has_y)) {
    if (r.any(_Pp::__has_y) && (r.y < 0 || r.y > 99))
      return false;
    if (r.any(_Pp::__has_C))
      y = r._Cp < 0 && r.y != 0 ? (r._Cp + 1) * 100 - r.y : r._Cp * 100 + r.y;
    else
      y = r.y >= 69 ? 1900 + r.y : 2000 + r.y;
  } else {
    return false;
  }
  return y >= -32767 && y <= 32767;
}

// The date, from y/m/d, y + day of the year, ISO week date, or y + week number + weekday.
constexpr bool __chrono_date_of(const __chrono_parsed& r, long long& days) noexcept {
  using _Pp = __chrono_parsed;
  long long y = 0;
  if (r.any(_Pp::__has_G | _Pp::__has_g) && r.any(_Pp::__has_V) && r.any(_Pp::__has_wd)) {
    long long __g = r._Gp;
    if (!r.any(_Pp::__has_G))
      __g = r.any(_Pp::__has_C) ? r._Cp * 100 + r.__g : (r.__g >= 69 ? 1900 + r.__g : 2000 + r.__g);
    if (__g < -32767 || __g > 32767 || r._Vp < 1 ||
        r._Vp > static_cast<long long>(::__ycxx::__detail::__chrono_iso_weeks(static_cast<int>(__g))))
      return false;
    const long long __jan4 = ::__ycxx::__detail::__days_from_civil(static_cast<int>(__g), 1, 4);
    const long long __monday1 = __jan4 - static_cast<long long>((::__ycxx::__detail::__weekday_from_days(__jan4) + 6) % 7);
    days = __monday1 + (r._Vp - 1) * 7 + (r.__wd + 6) % 7;
    if (!r.any(_Pp::__has_Y | _Pp::__has_C | _Pp::__has_y | _Pp::__has_m | _Pp::__has_d))
      return true;
  } else {
    if (!::__ycxx::__detail::__chrono_year_of(r, y))
      return false;
    const int __yi = static_cast<int>(y);
    const long long __jan1 = ::__ycxx::__detail::__days_from_civil(__yi, 1, 1);
    if (r.any(_Pp::__has_m) && r.any(_Pp::__has_d)) {
      if (r.m < 1 || r.m > 12 || r.d < 1 || r.d > ::__ycxx::__detail::__last_day_of(__yi, static_cast<unsigned>(r.m)))
        return false;
      days = ::__ycxx::__detail::__days_from_civil(__yi, static_cast<unsigned>(r.m), static_cast<unsigned>(r.d));
    } else if (r.any(_Pp::__has_j)) {
      if (r.__j < 1 || r.__j > (::__ycxx::__detail::__is_leap_year(__yi) ? 366 : 365))
        return false;
      days = __jan1 + r.__j - 1;
    } else if (r.any(_Pp::__has_U | _Pp::__has_W) && r.any(_Pp::__has_wd)) {
      const long long __w = r.any(_Pp::__has_U) ? r._Up : r._Wp;
      if (__w < 0 || __w > 53)
        return false;
      // The first Sunday (%U) or Monday (%W) of the year starts week 01.
      const long long __first_day = r.any(_Pp::__has_U) ? 0 : 1;
      const long long first =
          __jan1 + (__first_day - static_cast<long long>(::__ycxx::__detail::__weekday_from_days(__jan1)) + 7) % 7;
      days = first + (__w - 1) * 7 + (r.any(_Pp::__has_U) ? r.__wd : (r.__wd + 6) % 7);
      if (days < __jan1 || days >= __jan1 + (::__ycxx::__detail::__is_leap_year(__yi) ? 366 : 365))
        return false;
    } else {
      return false;
    }
  }
  // Every other date field given must agree.
  const __civil_date c = ::__ycxx::__detail::__civil_from_days(days);
  if (r.any(_Pp::__has_m) && r.m != c.m)
    return false;
  if (r.any(_Pp::__has_d) && r.d != c.d)
    return false;
  if (r.any(_Pp::__has_wd) && r.__wd != ::__ycxx::__detail::__weekday_from_days(days))
    return false;
  return true;
}

// The time of day (or the duration) in units of 10^-digits seconds.
// A seconds field of 60 is accepted only with `__leap_ok` (local_time, and utc_time's leap seconds).
constexpr bool __chrono_time_of(const __chrono_parsed& r, unsigned digits, bool duration, long long& __units,
                              bool __leap_ok = false) noexcept {
  using _Pp = __chrono_parsed;
  long long h = 0;
  if (r.any(_Pp::__has_I)) {
    if (r._Ip < 1 || r._Ip > 12)
      return false;
    h = r._Ip % 12 + (r.any(_Pp::__has_p) && r.__pm ? 12 : 0);
    if (r.any(_Pp::__has_H) && r._Hp != h)
      return false;
  } else if (r.any(_Pp::__has_H)) {
    h = r._Hp;
    if (h > 23 && !duration)
      return false;
    if (r.any(_Pp::__has_p) && h <= 12) // %H with %p: a 12-hour value
      h = h % 12 + (r.__pm ? 12 : 0);
  }
  if (r._Mp > 59 || r._Sp > (__leap_ok ? 60 : 59))
    return false;
  long long scale = 1;
  for (unsigned i = 0; i < digits; ++i)
    scale *= 10;
  const long long d = r.any(_Pp::__has_j) && duration ? r.__j : 0;
  long long __secs = 0;
  return !__builtin_mul_overflow(d, 86400, &__secs) && !__builtin_add_overflow(__secs, (h * 60 + r._Mp) * 60 + r._Sp, &__secs) &&
         !__builtin_mul_overflow(__secs, scale, &__units) &&
         !__builtin_add_overflow(__units, static_cast<long long>(r.__sub), &__units);
}

template <class _Duration>
inline constexpr unsigned __chrono_parse_digits =
    std::chrono::hh_mm_ss<std::common_type_t<_Duration, std::chrono::seconds>>::fractional_width;

template <class _Duration>
using __chrono_parse_unit =
    std::chrono::duration<long long, std::ratio<1, ::__ycxx::__detail::__pow10(__chrono_parse_digits<_Duration>)>>;

// Scans and runs build(fields) -> bool; sets failbit on any failure. Stores abbrev and offset.
template <class __charT, class __traits, class _Alloc, class _Build>
std::basic_istream<__charT, __traits>& __chrono_from_stream(std::basic_istream<__charT, __traits>& is, const __charT* __fmt,
                                                      std::basic_string<__charT, __traits, _Alloc>* abbrev,
                                                      std::chrono::minutes* offset, unsigned info, unsigned digits,
                                                      _Build&& __build) {
  using istream_type = std::basic_istream<__charT, __traits>; // (ios_base is not declared here)
  const typename istream_type::sentry ok(is, true);
  if (!ok)
    return is;
  typename istream_type::iostate __err = istream_type::goodbit;
  {
    __chrono_scanner<__charT, __traits> __sc(is, info, digits);
    if (!__sc.run(__fmt) || !__build(__sc.r)) {
      __err |= istream_type::failbit;
    } else {
      if (abbrev != nullptr && __sc.r.any(__chrono_parsed::__has_Z))
        abbrev->assign(__sc.abbrev.data(), __sc.abbrev.size());
      if (offset != nullptr && __sc.r.any(__chrono_parsed::__has_z))
        *offset = std::chrono::minutes(__sc.r.offset);
    }
    if (__sc.__at_eof())
      __err |= istream_type::eofbit;
  }
  if (__err != istream_type::goodbit)
    is.setstate(__err);
  return is;
}

// A time point's local date and time as Duration units since the epoch.
template <class _Duration>
constexpr bool __chrono_point_of(const __chrono_parsed& r, _Duration& out, bool __leap_ok = false) {
  long long __dd = 0, __units = 0;
  if (!::__ycxx::__detail::__chrono_date_of(r, __dd))
    return false;
  if (!::__ycxx::__detail::__chrono_time_of(r, __chrono_parse_digits<_Duration>, false, __units, __leap_ok))
    return false;
  using __unit = __chrono_parse_unit<_Duration>;
  const auto __total = std::chrono::duration_cast<__unit>(std::chrono::days(static_cast<int>(__dd))) + __unit(__units);
  out = std::chrono::floor<_Duration>(__total);
  return true;
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.duration.io]/3
template <class __charT, class __traits, class _Rep, class _Period, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, duration<_Rep, _Period>& d,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  using _Dp = duration<_Rep, _Period>;
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_time | __ycxx::__detail::__ci_duration, __ycxx::__detail::__chrono_parse_digits<_Dp>,
      [&](const __ycxx::__detail::__chrono_parsed& r) {
        using _Pp = __ycxx::__detail::__chrono_parsed;
        if (!r.any(_Pp::__has_j | _Pp::__has_H | _Pp::__has_I | _Pp::__has_M | _Pp::__has_S))
          return false;
        long long __units = 0;
        if (!__ycxx::__detail::__chrono_time_of(r, __ycxx::__detail::__chrono_parse_digits<_Dp>, true, __units))
          return false;
        d = duration_cast<_Dp>(__ycxx::__detail::__chrono_parse_unit<_Dp>(__units));
        return true;
      });
}

// [time.clock.system.nonmembers]/6: the offset is subtracted.
template <class __charT, class __traits, class _Duration, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, sys_time<_Duration>& __tp,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date | __ycxx::__detail::__ci_time,
      __ycxx::__detail::__chrono_parse_digits<_Duration>, [&](const __ycxx::__detail::__chrono_parsed& r) {
        _Duration d{};
        if (!__ycxx::__detail::__chrono_point_of(r, d))
          return false;
        __tp = sys_time<_Duration>(d - duration_cast<_Duration>(minutes(r.offset)));
        return true;
      });
}

// [time.clock.local]/5: the offset is only stored.
template <class __charT, class __traits, class _Duration, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, local_time<_Duration>& __tp,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date | __ycxx::__detail::__ci_time,
      __ycxx::__detail::__chrono_parse_digits<_Duration>, [&](const __ycxx::__detail::__chrono_parsed& r) {
        _Duration d{};
        if (!__ycxx::__detail::__chrono_point_of(r, d, true)) // 23:59:60 is accepted as a local time
          return false;
        __tp = local_time<_Duration>(d);
        return true;
      });
}

// [time.clock.utc.nonmembers]/3: 23:59:60 names the leap second.
template <class __charT, class __traits, class _Duration, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, utc_time<_Duration>& __tp,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date | __ycxx::__detail::__ci_time,
      __ycxx::__detail::__chrono_parse_digits<_Duration>, [&](const __ycxx::__detail::__chrono_parsed& r) {
        __ycxx::__detail::__chrono_parsed __r2 = r;
        const bool __leap = r._Sp == 60;
        if (__leap)
          __r2._Sp = 59;
        _Duration d{};
        if (!__ycxx::__detail::__chrono_point_of(__r2, d))
          return false;
        const sys_time<_Duration> __st(d - duration_cast<_Duration>(minutes(r.offset)));
        auto __u = utc_clock::from_sys(__st);
        if (__leap) {
          __u += seconds(1);
          if (!get_leap_second_info(__u).is_leap_second) // 60 seconds only during a leap second
            return false;
        }
        __tp = time_point_cast<_Duration>(__u);
        return true;
      });
}

template <class __charT, class __traits, class _Duration, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, tai_time<_Duration>& __tp,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date | __ycxx::__detail::__ci_time,
      __ycxx::__detail::__chrono_parse_digits<_Duration>, [&](const __ycxx::__detail::__chrono_parsed& r) {
        _Duration d{};
        if (!__ycxx::__detail::__chrono_point_of(r, d))
          return false;
        // The inverse of [time.format]/12: TAI's epoch is the sys_time 1958-01-01.
        __tp = tai_time<_Duration>(d - duration_cast<_Duration>(minutes(r.offset)) + duration_cast<_Duration>(days(4383)));
        return true;
      });
}

template <class __charT, class __traits, class _Duration, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, gps_time<_Duration>& __tp,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date | __ycxx::__detail::__ci_time,
      __ycxx::__detail::__chrono_parse_digits<_Duration>, [&](const __ycxx::__detail::__chrono_parsed& r) {
        _Duration d{};
        if (!__ycxx::__detail::__chrono_point_of(r, d))
          return false;
        // The inverse of [time.format]/13: GPS's epoch is the sys_time 1980-01-06.
        __tp = gps_time<_Duration>(d - duration_cast<_Duration>(minutes(r.offset)) - duration_cast<_Duration>(days(3657)));
        return true;
      });
}

template <class __charT, class __traits, class _Duration, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, file_time<_Duration>& __tp,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date | __ycxx::__detail::__ci_time,
      __ycxx::__detail::__chrono_parse_digits<_Duration>, [&](const __ycxx::__detail::__chrono_parsed& r) {
        _Duration d{};
        if (!__ycxx::__detail::__chrono_point_of(r, d))
          return false;
        __tp = time_point_cast<_Duration>(
            clock_cast<file_clock>(sys_time<_Duration>(d - duration_cast<_Duration>(minutes(r.offset)))));
        return true;
      });
}

// [time.cal.*.nonmembers]
template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, day& d,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(is, __fmt, abbrev, offset, __ycxx::__detail::__ci_day, 0,
                                          [&](const __ycxx::__detail::__chrono_parsed& r) {
                                            if (!r.any(__ycxx::__detail::__chrono_parsed::__has_d) || r.d < 1 || r.d > 31)
                                              return false;
                                            d = day(static_cast<unsigned>(r.d));
                                            return true;
                                          });
}

template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, month& m,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(is, __fmt, abbrev, offset, __ycxx::__detail::__ci_month, 0,
                                          [&](const __ycxx::__detail::__chrono_parsed& r) {
                                            if (!r.any(__ycxx::__detail::__chrono_parsed::__has_m) || r.m < 1 || r.m > 12)
                                              return false;
                                            m = month(static_cast<unsigned>(r.m));
                                            return true;
                                          });
}

template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, year& y,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(is, __fmt, abbrev, offset, __ycxx::__detail::__ci_year, 0,
                                          [&](const __ycxx::__detail::__chrono_parsed& r) {
                                            long long __v = 0;
                                            if (!__ycxx::__detail::__chrono_year_of(r, __v))
                                              return false;
                                            y = year(static_cast<int>(__v));
                                            return true;
                                          });
}

template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, weekday& __wd,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(is, __fmt, abbrev, offset, __ycxx::__detail::__ci_weekday, 0,
                                          [&](const __ycxx::__detail::__chrono_parsed& r) {
                                            if (!r.any(__ycxx::__detail::__chrono_parsed::__has_wd))
                                              return false;
                                            __wd = weekday(static_cast<unsigned>(r.__wd));
                                            return true;
                                          });
}

template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, month_day& __md,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(
      is, __fmt, abbrev, offset, __ycxx::__detail::__ci_month | __ycxx::__detail::__ci_day, 0,
      [&](const __ycxx::__detail::__chrono_parsed& r) {
        using _Pp = __ycxx::__detail::__chrono_parsed;
        if (!r.any(_Pp::__has_m) || !r.any(_Pp::__has_d))
          return false;
        const month_day __v(month(static_cast<unsigned>(r.m)), day(static_cast<unsigned>(r.d)));
        if (r.m > 12 || r.d > 31 || !__v.ok())
          return false;
        __md = __v;
        return true;
      });
}

template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, year_month& __ym,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(is, __fmt, abbrev, offset, __ycxx::__detail::__ci_year | __ycxx::__detail::__ci_month,
                                          0, [&](const __ycxx::__detail::__chrono_parsed& r) {
                                            long long y = 0;
                                            if (!__ycxx::__detail::__chrono_year_of(r, y) ||
                                                !r.any(__ycxx::__detail::__chrono_parsed::__has_m) || r.m < 1 || r.m > 12)
                                              return false;
                                            __ym = year_month(year(static_cast<int>(y)),
                                                            month(static_cast<unsigned>(r.m)));
                                            return true;
                                          });
}

template <class __charT, class __traits, class _Alloc = allocator<__charT>>
basic_istream<__charT, __traits>& from_stream(basic_istream<__charT, __traits>& is, const __charT* __fmt, year_month_day& ymd,
                                          basic_string<__charT, __traits, _Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return __ycxx::__detail::__chrono_from_stream(is, __fmt, abbrev, offset, __ycxx::__detail::__ci_full_date, 0,
                                          [&](const __ycxx::__detail::__chrono_parsed& r) {
                                            long long __dd = 0;
                                            if (!__ycxx::__detail::__chrono_date_of(r, __dd))
                                              return false;
                                            ymd = year_month_day(sys_days(days(static_cast<int>(__dd))));
                                            return true;
                                          });
}

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// [time.parse]: the manipulator; `_Mode` 0: (fmt, tp), 1: + abbrev, 2: + offset, 3: + both.
template <int _Mode, class __charT, class __traits, class _Alloc, class _Parsable>
class __chrono_parse_manip {
  const __charT* __fmt_;
  _Parsable& __tp_;
  std::basic_string<__charT, __traits, _Alloc>* __abbrev_;
  std::chrono::minutes* __offset_;

public:
  __chrono_parse_manip(const __charT* __fmt, _Parsable& __tp, std::basic_string<__charT, __traits, _Alloc>* abbrev,
                     std::chrono::minutes* offset) noexcept
      : __fmt_(__fmt), __tp_(__tp), __abbrev_(abbrev), __offset_(offset) {}
  __chrono_parse_manip(const __chrono_parse_manip&) = delete;
  __chrono_parse_manip& operator=(const __chrono_parse_manip&) = delete;

  friend std::basic_istream<__charT, __traits>& operator>>(std::basic_istream<__charT, __traits>& is,
                                                       __chrono_parse_manip&& m) {
    if constexpr (_Mode == 0)
      from_stream(is, m.__fmt_, m.__tp_);
    else if constexpr (_Mode == 1)
      from_stream(is, m.__fmt_, m.__tp_, m.__abbrev_);
    else if constexpr (_Mode == 2)
      from_stream(is, m.__fmt_, m.__tp_, static_cast<std::basic_string<__charT, __traits, _Alloc>*>(nullptr), m.__offset_);
    else
      from_stream(is, m.__fmt_, m.__tp_, m.__abbrev_, m.__offset_);
    return is;
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class __charT, class __traits, class _Parsable, class... _Extra>
concept __chrono_parsable = requires(std::basic_istream<__charT, __traits>& is, const __charT* __fmt, _Parsable& __tp,
                                   _Extra... __extra) { from_stream(is, __fmt, __tp, __extra...); };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

template <class __charT, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, char_traits<__charT>, _Parsable>
auto parse(const __charT* __fmt, _Parsable& __tp) {
  return __ycxx::__adl_free::__chrono_parse_manip<0, __charT, char_traits<__charT>, allocator<__charT>, _Parsable>(__fmt, __tp, nullptr,
                                                                                                       nullptr);
}
template <class __charT, class __traits, class _Alloc, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, __traits, _Parsable>
auto parse(const basic_string<__charT, __traits, _Alloc>& __fmt, _Parsable& __tp) {
  return __ycxx::__adl_free::__chrono_parse_manip<0, __charT, __traits, _Alloc, _Parsable>(__fmt.c_str(), __tp, nullptr, nullptr);
}
template <class __charT, class __traits, class _Alloc, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, __traits, _Parsable, basic_string<__charT, __traits, _Alloc>*>
auto parse(const __charT* __fmt, _Parsable& __tp, basic_string<__charT, __traits, _Alloc>& abbrev) {
  return __ycxx::__adl_free::__chrono_parse_manip<1, __charT, __traits, _Alloc, _Parsable>(__fmt, __tp, __builtin_addressof(abbrev),
                                                                              nullptr);
}
template <class __charT, class __traits, class _Alloc, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, __traits, _Parsable, basic_string<__charT, __traits, _Alloc>*>
auto parse(const basic_string<__charT, __traits, _Alloc>& __fmt, _Parsable& __tp, basic_string<__charT, __traits, _Alloc>& abbrev) {
  return __ycxx::__adl_free::__chrono_parse_manip<1, __charT, __traits, _Alloc, _Parsable>(__fmt.c_str(), __tp,
                                                                              __builtin_addressof(abbrev), nullptr);
}
template <class __charT, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, char_traits<__charT>, _Parsable, basic_string<__charT>*, minutes*>
auto parse(const __charT* __fmt, _Parsable& __tp, minutes& offset) {
  return __ycxx::__adl_free::__chrono_parse_manip<2, __charT, char_traits<__charT>, allocator<__charT>, _Parsable>(
      __fmt, __tp, nullptr, __builtin_addressof(offset));
}
template <class __charT, class __traits, class _Alloc, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, __traits, _Parsable, basic_string<__charT, __traits, _Alloc>*, minutes*>
auto parse(const basic_string<__charT, __traits, _Alloc>& __fmt, _Parsable& __tp, minutes& offset) {
  return __ycxx::__adl_free::__chrono_parse_manip<2, __charT, __traits, _Alloc, _Parsable>(__fmt.c_str(), __tp, nullptr,
                                                                              __builtin_addressof(offset));
}
template <class __charT, class __traits, class _Alloc, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, __traits, _Parsable, basic_string<__charT, __traits, _Alloc>*, minutes*>
auto parse(const __charT* __fmt, _Parsable& __tp, basic_string<__charT, __traits, _Alloc>& abbrev, minutes& offset) {
  return __ycxx::__adl_free::__chrono_parse_manip<3, __charT, __traits, _Alloc, _Parsable>(__fmt, __tp, __builtin_addressof(abbrev),
                                                                              __builtin_addressof(offset));
}
template <class __charT, class __traits, class _Alloc, class _Parsable>
  requires __ycxx::__detail::__chrono_parsable<__charT, __traits, _Parsable, basic_string<__charT, __traits, _Alloc>*, minutes*>
auto parse(const basic_string<__charT, __traits, _Alloc>& __fmt, _Parsable& __tp, basic_string<__charT, __traits, _Alloc>& abbrev,
           minutes& offset) {
  return __ycxx::__adl_free::__chrono_parse_manip<3, __charT, __traits, _Alloc, _Parsable>(
      __fmt.c_str(), __tp, __builtin_addressof(abbrev), __builtin_addressof(offset));
}

}} // namespace std::chrono
