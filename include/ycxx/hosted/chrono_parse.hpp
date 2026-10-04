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

namespace ycxx::detail {

struct chrono_parsed {
  enum : unsigned {
    has_Y = 1u << 0,
    has_C = 1u << 1,
    has_y = 1u << 2,
    has_G = 1u << 3,
    has_g = 1u << 4,
    has_m = 1u << 5,
    has_d = 1u << 6,
    has_j = 1u << 7,
    has_U = 1u << 8,
    has_W = 1u << 9,
    has_V = 1u << 10,
    has_wd = 1u << 11,
    has_H = 1u << 12,
    has_I = 1u << 13,
    has_p = 1u << 14,
    has_M = 1u << 15,
    has_S = 1u << 16,
    has_z = 1u << 17,
    has_Z = 1u << 18,
  };
  unsigned have = 0;
  long long Y = 0, C = 0, y = 0, G = 0, g = 0, m = 0, d = 0, j = 0, U = 0, W = 0, V = 0, wd = 0;
  long long H = 0, I = 0, M = 0, S = 0;
  unsigned long long sub = 0; // fractional seconds in units of 10^-digits
  bool pm = false;
  long long offset = 0; // minutes
  bool any(unsigned f) const noexcept { return (have & f) != 0; }
};

constexpr bool chrono_ieq(char a, char b) noexcept {
  return (a >= 'A' && a <= 'Z' ? a + 32 : a) == (b >= 'A' && b <= 'Z' ? b + 32 : b);
}

// The scanner: reads is (through its streambuf) as fmt directs.
template <class charT, class traits>
class chrono_scanner {
  using int_type = typename traits::int_type;
  std::basic_streambuf<charT, traits>* sb_;
  const std::ctype<charT>& ct_;
  charT point_;
  bool eof_ = false;

public:
  chrono_parsed r;
  std::basic_string<charT, traits> abbrev;
  unsigned info;   // what the type can represent (ci_* of chrono_io.hpp)
  unsigned digits; // the fractional digits of %S

  chrono_scanner(std::basic_istream<charT, traits>& is, unsigned inf, unsigned dig)
      : sb_(is.rdbuf()), ct_(std::use_facet<std::ctype<charT>>(is.getloc())),
        point_(std::use_facet<std::numpunct<charT>>(is.getloc()).decimal_point()), info(inf), digits(dig) {}

  bool at_eof() const noexcept { return eof_; }

private:
  int_type peek() {
    if (eof_)
      return traits::eof();
    const int_type c = sb_->sgetc();
    if (traits::eq_int_type(c, traits::eof()))
      eof_ = true;
    return c;
  }
  void bump() { sb_->sbumpc(); }
  bool is_space(int_type c) const {
    return !traits::eq_int_type(c, traits::eof()) && ct_.is(std::ctype_base::space, traits::to_char_type(c));
  }
  // The character as ASCII ('\0' for EOF or anything else).
  char narrow(int_type c) const {
    return traits::eq_int_type(c, traits::eof()) ? '\0' : ct_.narrow(traits::to_char_type(c), '\0');
  }

  // An unsigned number of at most n digits (at least one).
  bool number(int n, long long& v) {
    v = 0;
    int k = 0;
    for (; k < n; ++k) {
      const char c = narrow(peek());
      if (c < '0' || c > '9')
        break;
      if (v > 99'999'999'999'999'999LL) // 18 digits: no field needs more
        return false;
      v = v * 10 + (c - '0');
      bump();
    }
    return k > 0;
  }
  // A number of exactly n digits.
  bool exact(int n, long long& v) {
    v = 0;
    for (int k = 0; k < n; ++k) {
      const char c = narrow(peek());
      if (c < '0' || c > '9')
        return false;
      v = v * 10 + (c - '0');
      bump();
    }
    return true;
  }
  // [+|-] and a number of at most n digits.
  bool signed_number(int n, long long& v) {
    const char c = narrow(peek());
    bool neg = false;
    if (c == '+' || c == '-') {
      neg = c == '-';
      bump();
    }
    if (!number(n, v))
      return false;
    if (neg)
      v = -v;
    return true;
  }
  // One of the names (full, or the first three letters), case-insensitively; the index.
  bool name(const char* const* names, int count, long long& index) {
    char text[16];
    int len = 0;
    for (;;) {
      const char c = narrow(peek());
      bool extends = false;
      for (int i = 0; i < count && !extends; ++i) {
        const char* n = names[i];
        int k = 0;
        while (k < len && n[k] != 0 && ::ycxx::detail::chrono_ieq(n[k], text[k]))
          ++k;
        extends = k == len && n[len] != 0 && ::ycxx::detail::chrono_ieq(n[len], c);
      }
      if (!extends || len == 15)
        break;
      text[len++] = c;
      bump();
    }
    for (int i = 0; i < count; ++i) {
      const char* n = names[i];
      int k = 0;
      while (k < len && n[k] != 0 && ::ycxx::detail::chrono_ieq(n[k], text[k]))
        ++k;
      if (k == len && (n[k] == 0 || len == 3)) {
        index = i;
        return true;
      }
    }
    return false;
  }
  bool need(unsigned what) const { return (info & what) == what; }

  bool seconds_field(int n) {
    // [time.parse]: %S as a decimal number, with the type's fractional digits when it has any.
    long long s;
    if (!number(n < 0 ? 2 : (n > 2 ? 2 : n), s))
      return false;
    r.S = s;
    r.sub = 0;
    r.have |= chrono_parsed::has_S;
    if (digits != 0 && traits::eq_int_type(peek(), traits::to_int_type(point_))) {
      bump();
      unsigned k = 0;
      for (; k < digits; ++k) {
        const char c = narrow(peek());
        if (c < '0' || c > '9')
          break;
        r.sub = r.sub * 10 + static_cast<unsigned>(c - '0');
        bump();
      }
      if (k == 0)
        return false;
      for (; k < digits; ++k)
        r.sub *= 10;
    }
    return true;
  }

  bool offset(bool modified) {
    const char c = narrow(peek());
    bool neg = false;
    if (c == '+' || c == '-') {
      neg = c == '-';
      bump();
    }
    long long h = 0, mins = 0;
    if (!modified) { // [+|-]hh[mm]
      if (!exact(2, h))
        return false;
      const char c2 = narrow(peek());
      if (c2 >= '0' && c2 <= '9' && !exact(2, mins))
        return false;
    } else { // [+|-]h[h][:mm]
      if (!number(2, h))
        return false;
      if (narrow(peek()) == ':') {
        bump();
        if (!exact(2, mins))
          return false;
      }
    }
    if (mins > 59)
      return false;
    r.offset = (neg ? -1 : 1) * (h * 60 + mins);
    r.have |= chrono_parsed::has_z;
    return true;
  }

  bool zone_name() {
    std::size_t n = 0;
    for (;;) {
      const char c = narrow(peek());
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '/' ||
            c == '-' || c == '+'))
        break;
      abbrev.push_back(traits::to_char_type(peek()));
      bump();
      ++n;
    }
    if (n == 0)
      return false;
    r.have |= chrono_parsed::has_Z;
    return true;
  }

  // Runs the NTBS pattern of a composite flag; a width n applies to its first field.
  bool pattern(const char* p, int n = -1) {
    for (; *p != 0; ++p) {
      if (*p == '%') {
        ++p;
        if (!flag(*p, 0, n))
          return false;
        n = -1;
      } else if (*p == ' ') {
        while (is_space(peek()))
          bump();
      } else {
        if (narrow(peek()) != *p)
          return false;
        bump();
      }
    }
    return true;
  }

public:
  // One flag; n is the width (-1: the default).
  bool flag(char f, char mod, int n) {
    using P = chrono_parsed;
    auto num = [&](int dflt, long long& v, unsigned bit, unsigned what) {
      if (!need(what))
        return false;
      if (!number(n < 0 ? dflt : n, v))
        return false;
      r.have |= bit;
      return true;
    };
    if (mod == 'E' && !(f == 'c' || f == 'C' || f == 'x' || f == 'X' || f == 'y' || f == 'Y' || f == 'z'))
      return false;
    if (mod == 'O' && !(f == 'd' || f == 'e' || f == 'H' || f == 'I' || f == 'm' || f == 'M' || f == 'S' ||
                        f == 'u' || f == 'U' || f == 'V' || f == 'w' || f == 'W' || f == 'y' || f == 'z'))
      return false;
    switch (f) {
    case 'a':
    case 'A':
      if (!need(ci_weekday) || !name(chrono_weekday_names, 7, r.wd))
        return false;
      r.have |= P::has_wd;
      return true;
    case 'b':
    case 'B':
    case 'h':
      if (!need(ci_month) || !name(chrono_month_names, 12, r.m))
        return false;
      ++r.m;
      r.have |= P::has_m;
      return true;
    case 'c': return need(ci_full_date | ci_time) && pattern("%a %b %e %H:%M:%S %Y");
    case 'C':
      if (!need(ci_year) || !signed_number(n < 0 ? 2 : n, r.C))
        return false;
      r.have |= P::has_C;
      return true;
    case 'e': // as written by %e: a leading space
      while (is_space(peek()))
        bump();
      return num(2, r.d, P::has_d, ci_day);
    case 'd': return num(2, r.d, P::has_d, ci_day);
    case 'D': return pattern("%m/%d/%y", n);
    case 'F': return pattern("%Y-%m-%d", n);
    case 'g': return num(2, r.g, P::has_g, ci_date);
    case 'G':
      if (!need(ci_date) || !signed_number(n < 0 ? 4 : n, r.G))
        return false;
      r.have |= P::has_G;
      return true;
    case 'H': return num(2, r.H, P::has_H, ci_time);
    case 'I': return num(2, r.I, P::has_I, ci_time);
    case 'j':
      if ((info & ci_duration) != 0) {
        if (!number(n < 0 ? 3 : n, r.j))
          return false;
        r.have |= P::has_j;
        return true;
      }
      return num(3, r.j, P::has_j, ci_date);
    case 'm': return num(2, r.m, P::has_m, ci_month);
    case 'M': return num(2, r.M, P::has_M, ci_time);
    case 'n':
      if (!is_space(peek()))
        return false;
      bump();
      return true;
    case 't':
      if (is_space(peek()))
        bump();
      return true;
    case 'p': {
      if (!need(ci_time))
        return false;
      const char a = narrow(peek());
      if (!::ycxx::detail::chrono_ieq(a, 'A') && !::ycxx::detail::chrono_ieq(a, 'P'))
        return false;
      bump();
      if (!::ycxx::detail::chrono_ieq(narrow(peek()), 'M'))
        return false;
      bump();
      r.pm = ::ycxx::detail::chrono_ieq(a, 'P');
      r.have |= P::has_p;
      return true;
    }
    case 'r': return need(ci_time) && pattern("%I:%M:%S %p");
    case 'R': return need(ci_time) && pattern("%H:%M");
    case 'S': return need(ci_time) && seconds_field(n);
    case 'T': return need(ci_time) && pattern("%H:%M:%S");
    case 'u':
      if (!num(1, r.wd, P::has_wd, ci_weekday))
        return false;
      if (r.wd < 1 || r.wd > 7)
        return false;
      r.wd %= 7;
      return true;
    case 'w':
      if (!num(1, r.wd, P::has_wd, ci_weekday))
        return false;
      return r.wd <= 6;
    case 'U': return num(2, r.U, P::has_U, ci_date);
    case 'W': return num(2, r.W, P::has_W, ci_date);
    case 'V': return num(2, r.V, P::has_V, ci_date);
    case 'x': return pattern("%m/%d/%y");
    case 'X': return need(ci_time) && pattern("%H:%M:%S");
    case 'y': return num(2, r.y, P::has_y, ci_year);
    case 'Y':
      if (!need(ci_year) || !signed_number(n < 0 ? 4 : n, r.Y))
        return false;
      r.have |= P::has_Y;
      return true;
    case 'z': return offset(mod != 0);
    case 'Z': return zone_name();
    case '%':
      if (narrow(peek()) != '%')
        return false;
      bump();
      return true;
    default: return false;
    }
  }

  // The whole format.
  bool run(const charT* fmt) {
    for (const charT* p = fmt; *p != charT(); ++p) {
      if (ct_.is(std::ctype_base::space, *p)) {
        while (is_space(peek()))
          bump();
        continue;
      }
      if (!traits::eq(*p, charT('%'))) {
        const int_type c = peek();
        if (traits::eq_int_type(c, traits::eof()) || !traits::eq(traits::to_char_type(c), *p))
          return false;
        bump();
        continue;
      }
      ++p;
      int n = -1;
      while (*p != charT() && ct_.narrow(*p, '\0') >= '0' && ct_.narrow(*p, '\0') <= '9') {
        n = (n < 0 ? 0 : n * 10) + (ct_.narrow(*p, '\0') - '0');
        if (n > 1000)
          return false;
        ++p;
      }
      if (n == 0)
        return false;
      char mod = 0;
      if (*p == charT('E') || *p == charT('O'))
        mod = ct_.narrow(*p++, '\0');
      if (*p == charT())
        return false;
      if (!flag(ct_.narrow(*p, '\0'), mod, n))
        return false;
    }
    return true;
  }
};

// ---- building values from the fields -------------------------------------------------------

constexpr bool chrono_year_of(const chrono_parsed& r, long long& y) noexcept {
  using P = chrono_parsed;
  if (r.any(P::has_Y)) {
    y = r.Y;
  } else if (r.any(P::has_C | P::has_y)) {
    if (r.any(P::has_y) && (r.y < 0 || r.y > 99))
      return false;
    if (r.any(P::has_C))
      y = r.C < 0 && r.y != 0 ? (r.C + 1) * 100 - r.y : r.C * 100 + r.y;
    else
      y = r.y >= 69 ? 1900 + r.y : 2000 + r.y;
  } else {
    return false;
  }
  return y >= -32767 && y <= 32767;
}

// The date, from y/m/d, y + day of the year, ISO week date, or y + week number + weekday.
constexpr bool chrono_date_of(const chrono_parsed& r, long long& days) noexcept {
  using P = chrono_parsed;
  long long y = 0;
  if (r.any(P::has_G | P::has_g) && r.any(P::has_V) && r.any(P::has_wd)) {
    long long g = r.G;
    if (!r.any(P::has_G))
      g = r.any(P::has_C) ? r.C * 100 + r.g : (r.g >= 69 ? 1900 + r.g : 2000 + r.g);
    if (g < -32767 || g > 32767 || r.V < 1 ||
        r.V > static_cast<long long>(::ycxx::detail::chrono_iso_weeks(static_cast<int>(g))))
      return false;
    const long long jan4 = ::ycxx::detail::days_from_civil(static_cast<int>(g), 1, 4);
    const long long monday1 = jan4 - static_cast<long long>((::ycxx::detail::weekday_from_days(jan4) + 6) % 7);
    days = monday1 + (r.V - 1) * 7 + (r.wd + 6) % 7;
    if (!r.any(P::has_Y | P::has_C | P::has_y | P::has_m | P::has_d))
      return true;
  } else {
    if (!::ycxx::detail::chrono_year_of(r, y))
      return false;
    const int yi = static_cast<int>(y);
    const long long jan1 = ::ycxx::detail::days_from_civil(yi, 1, 1);
    if (r.any(P::has_m) && r.any(P::has_d)) {
      if (r.m < 1 || r.m > 12 || r.d < 1 || r.d > ::ycxx::detail::last_day_of(yi, static_cast<unsigned>(r.m)))
        return false;
      days = ::ycxx::detail::days_from_civil(yi, static_cast<unsigned>(r.m), static_cast<unsigned>(r.d));
    } else if (r.any(P::has_j)) {
      if (r.j < 1 || r.j > (::ycxx::detail::is_leap_year(yi) ? 366 : 365))
        return false;
      days = jan1 + r.j - 1;
    } else if (r.any(P::has_U | P::has_W) && r.any(P::has_wd)) {
      const long long w = r.any(P::has_U) ? r.U : r.W;
      if (w < 0 || w > 53)
        return false;
      // The first Sunday (%U) or Monday (%W) of the year starts week 01.
      const long long first_day = r.any(P::has_U) ? 0 : 1;
      const long long first =
          jan1 + (first_day - static_cast<long long>(::ycxx::detail::weekday_from_days(jan1)) + 7) % 7;
      days = first + (w - 1) * 7 + (r.any(P::has_U) ? r.wd : (r.wd + 6) % 7);
      if (days < jan1 || days >= jan1 + (::ycxx::detail::is_leap_year(yi) ? 366 : 365))
        return false;
    } else {
      return false;
    }
  }
  // Every other date field given must agree.
  const civil_date c = ::ycxx::detail::civil_from_days(days);
  if (r.any(P::has_m) && r.m != c.m)
    return false;
  if (r.any(P::has_d) && r.d != c.d)
    return false;
  if (r.any(P::has_wd) && r.wd != ::ycxx::detail::weekday_from_days(days))
    return false;
  return true;
}

// The time of day (or the duration) in units of 10^-digits seconds.
// A seconds field of 60 is accepted only with `leap_ok` (local_time, and utc_time's leap seconds).
constexpr bool chrono_time_of(const chrono_parsed& r, unsigned digits, bool duration, long long& units,
                              bool leap_ok = false) noexcept {
  using P = chrono_parsed;
  long long h = 0;
  if (r.any(P::has_I)) {
    if (r.I < 1 || r.I > 12)
      return false;
    h = r.I % 12 + (r.any(P::has_p) && r.pm ? 12 : 0);
    if (r.any(P::has_H) && r.H != h)
      return false;
  } else if (r.any(P::has_H)) {
    h = r.H;
    if (h > 23 && !duration)
      return false;
    if (r.any(P::has_p) && h <= 12) // %H with %p: a 12-hour value
      h = h % 12 + (r.pm ? 12 : 0);
  }
  if (r.M > 59 || r.S > (leap_ok ? 60 : 59))
    return false;
  long long scale = 1;
  for (unsigned i = 0; i < digits; ++i)
    scale *= 10;
  const long long d = r.any(P::has_j) && duration ? r.j : 0;
  long long secs = 0;
  return !__builtin_mul_overflow(d, 86400, &secs) && !__builtin_add_overflow(secs, (h * 60 + r.M) * 60 + r.S, &secs) &&
         !__builtin_mul_overflow(secs, scale, &units) &&
         !__builtin_add_overflow(units, static_cast<long long>(r.sub), &units);
}

template <class Duration>
inline constexpr unsigned chrono_parse_digits =
    std::chrono::hh_mm_ss<std::common_type_t<Duration, std::chrono::seconds>>::fractional_width;

template <class Duration>
using chrono_parse_unit =
    std::chrono::duration<long long, std::ratio<1, ::ycxx::detail::pow10(chrono_parse_digits<Duration>)>>;

// Scans and runs build(fields) -> bool; sets failbit on any failure. Stores abbrev and offset.
template <class charT, class traits, class Alloc, class Build>
std::basic_istream<charT, traits>& chrono_from_stream(std::basic_istream<charT, traits>& is, const charT* fmt,
                                                      std::basic_string<charT, traits, Alloc>* abbrev,
                                                      std::chrono::minutes* offset, unsigned info, unsigned digits,
                                                      Build&& build) {
  using istream_type = std::basic_istream<charT, traits>; // (ios_base is not declared here)
  const typename istream_type::sentry ok(is, true);
  if (!ok)
    return is;
  typename istream_type::iostate err = istream_type::goodbit;
  {
    chrono_scanner<charT, traits> sc(is, info, digits);
    if (!sc.run(fmt) || !build(sc.r)) {
      err |= istream_type::failbit;
    } else {
      if (abbrev != nullptr && sc.r.any(chrono_parsed::has_Z))
        abbrev->assign(sc.abbrev.data(), sc.abbrev.size());
      if (offset != nullptr && sc.r.any(chrono_parsed::has_z))
        *offset = std::chrono::minutes(sc.r.offset);
    }
    if (sc.at_eof())
      err |= istream_type::eofbit;
  }
  if (err != istream_type::goodbit)
    is.setstate(err);
  return is;
}

// A time point's local date and time as Duration units since the epoch.
template <class Duration>
constexpr bool chrono_point_of(const chrono_parsed& r, Duration& out, bool leap_ok = false) {
  long long dd = 0, units = 0;
  if (!::ycxx::detail::chrono_date_of(r, dd))
    return false;
  if (!::ycxx::detail::chrono_time_of(r, chrono_parse_digits<Duration>, false, units, leap_ok))
    return false;
  using unit = chrono_parse_unit<Duration>;
  const auto total = std::chrono::duration_cast<unit>(std::chrono::days(static_cast<int>(dd))) + unit(units);
  out = std::chrono::floor<Duration>(total);
  return true;
}

} // namespace ycxx::detail

namespace std::chrono {

// [time.duration.io]/3
template <class charT, class traits, class Rep, class Period, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, duration<Rep, Period>& d,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  using D = duration<Rep, Period>;
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_time | ycxx::detail::ci_duration, ycxx::detail::chrono_parse_digits<D>,
      [&](const ycxx::detail::chrono_parsed& r) {
        using P = ycxx::detail::chrono_parsed;
        if (!r.any(P::has_j | P::has_H | P::has_I | P::has_M | P::has_S))
          return false;
        long long units = 0;
        if (!ycxx::detail::chrono_time_of(r, ycxx::detail::chrono_parse_digits<D>, true, units))
          return false;
        d = duration_cast<D>(ycxx::detail::chrono_parse_unit<D>(units));
        return true;
      });
}

// [time.clock.system.nonmembers]/6: the offset is subtracted.
template <class charT, class traits, class Duration, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, sys_time<Duration>& tp,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_full_date | ycxx::detail::ci_time,
      ycxx::detail::chrono_parse_digits<Duration>, [&](const ycxx::detail::chrono_parsed& r) {
        Duration d{};
        if (!ycxx::detail::chrono_point_of(r, d))
          return false;
        tp = sys_time<Duration>(d - duration_cast<Duration>(minutes(r.offset)));
        return true;
      });
}

// [time.clock.local]/5: the offset is only stored.
template <class charT, class traits, class Duration, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, local_time<Duration>& tp,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_full_date | ycxx::detail::ci_time,
      ycxx::detail::chrono_parse_digits<Duration>, [&](const ycxx::detail::chrono_parsed& r) {
        Duration d{};
        if (!ycxx::detail::chrono_point_of(r, d, true)) // 23:59:60 is accepted as a local time
          return false;
        tp = local_time<Duration>(d);
        return true;
      });
}

// [time.clock.utc.nonmembers]/3: 23:59:60 names the leap second.
template <class charT, class traits, class Duration, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, utc_time<Duration>& tp,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_full_date | ycxx::detail::ci_time,
      ycxx::detail::chrono_parse_digits<Duration>, [&](const ycxx::detail::chrono_parsed& r) {
        ycxx::detail::chrono_parsed r2 = r;
        const bool leap = r.S == 60;
        if (leap)
          r2.S = 59;
        Duration d{};
        if (!ycxx::detail::chrono_point_of(r2, d))
          return false;
        const sys_time<Duration> st(d - duration_cast<Duration>(minutes(r.offset)));
        auto u = utc_clock::from_sys(st);
        if (leap) {
          u += seconds(1);
          if (!get_leap_second_info(u).is_leap_second) // 60 seconds only during a leap second
            return false;
        }
        tp = time_point_cast<Duration>(u);
        return true;
      });
}

template <class charT, class traits, class Duration, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, tai_time<Duration>& tp,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_full_date | ycxx::detail::ci_time,
      ycxx::detail::chrono_parse_digits<Duration>, [&](const ycxx::detail::chrono_parsed& r) {
        Duration d{};
        if (!ycxx::detail::chrono_point_of(r, d))
          return false;
        // The inverse of [time.format]/12: TAI's epoch is the sys_time 1958-01-01.
        tp = tai_time<Duration>(d - duration_cast<Duration>(minutes(r.offset)) + duration_cast<Duration>(days(4383)));
        return true;
      });
}

template <class charT, class traits, class Duration, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, gps_time<Duration>& tp,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_full_date | ycxx::detail::ci_time,
      ycxx::detail::chrono_parse_digits<Duration>, [&](const ycxx::detail::chrono_parsed& r) {
        Duration d{};
        if (!ycxx::detail::chrono_point_of(r, d))
          return false;
        // The inverse of [time.format]/13: GPS's epoch is the sys_time 1980-01-06.
        tp = gps_time<Duration>(d - duration_cast<Duration>(minutes(r.offset)) - duration_cast<Duration>(days(3657)));
        return true;
      });
}

template <class charT, class traits, class Duration, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, file_time<Duration>& tp,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_full_date | ycxx::detail::ci_time,
      ycxx::detail::chrono_parse_digits<Duration>, [&](const ycxx::detail::chrono_parsed& r) {
        Duration d{};
        if (!ycxx::detail::chrono_point_of(r, d))
          return false;
        tp = time_point_cast<Duration>(
            clock_cast<file_clock>(sys_time<Duration>(d - duration_cast<Duration>(minutes(r.offset)))));
        return true;
      });
}

// [time.cal.*.nonmembers]
template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, day& d,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(is, fmt, abbrev, offset, ycxx::detail::ci_day, 0,
                                          [&](const ycxx::detail::chrono_parsed& r) {
                                            if (!r.any(ycxx::detail::chrono_parsed::has_d) || r.d < 1 || r.d > 31)
                                              return false;
                                            d = day(static_cast<unsigned>(r.d));
                                            return true;
                                          });
}

template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, month& m,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(is, fmt, abbrev, offset, ycxx::detail::ci_month, 0,
                                          [&](const ycxx::detail::chrono_parsed& r) {
                                            if (!r.any(ycxx::detail::chrono_parsed::has_m) || r.m < 1 || r.m > 12)
                                              return false;
                                            m = month(static_cast<unsigned>(r.m));
                                            return true;
                                          });
}

template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, year& y,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(is, fmt, abbrev, offset, ycxx::detail::ci_year, 0,
                                          [&](const ycxx::detail::chrono_parsed& r) {
                                            long long v = 0;
                                            if (!ycxx::detail::chrono_year_of(r, v))
                                              return false;
                                            y = year(static_cast<int>(v));
                                            return true;
                                          });
}

template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, weekday& wd,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(is, fmt, abbrev, offset, ycxx::detail::ci_weekday, 0,
                                          [&](const ycxx::detail::chrono_parsed& r) {
                                            if (!r.any(ycxx::detail::chrono_parsed::has_wd))
                                              return false;
                                            wd = weekday(static_cast<unsigned>(r.wd));
                                            return true;
                                          });
}

template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, month_day& md,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(
      is, fmt, abbrev, offset, ycxx::detail::ci_month | ycxx::detail::ci_day, 0,
      [&](const ycxx::detail::chrono_parsed& r) {
        using P = ycxx::detail::chrono_parsed;
        if (!r.any(P::has_m) || !r.any(P::has_d))
          return false;
        const month_day v(month(static_cast<unsigned>(r.m)), day(static_cast<unsigned>(r.d)));
        if (r.m > 12 || r.d > 31 || !v.ok())
          return false;
        md = v;
        return true;
      });
}

template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, year_month& ym,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(is, fmt, abbrev, offset, ycxx::detail::ci_year | ycxx::detail::ci_month,
                                          0, [&](const ycxx::detail::chrono_parsed& r) {
                                            long long y = 0;
                                            if (!ycxx::detail::chrono_year_of(r, y) ||
                                                !r.any(ycxx::detail::chrono_parsed::has_m) || r.m < 1 || r.m > 12)
                                              return false;
                                            ym = year_month(year(static_cast<int>(y)),
                                                            month(static_cast<unsigned>(r.m)));
                                            return true;
                                          });
}

template <class charT, class traits, class Alloc = allocator<charT>>
basic_istream<charT, traits>& from_stream(basic_istream<charT, traits>& is, const charT* fmt, year_month_day& ymd,
                                          basic_string<charT, traits, Alloc>* abbrev = nullptr,
                                          minutes* offset = nullptr) {
  return ycxx::detail::chrono_from_stream(is, fmt, abbrev, offset, ycxx::detail::ci_full_date, 0,
                                          [&](const ycxx::detail::chrono_parsed& r) {
                                            long long dd = 0;
                                            if (!ycxx::detail::chrono_date_of(r, dd))
                                              return false;
                                            ymd = year_month_day(sys_days(days(static_cast<int>(dd))));
                                            return true;
                                          });
}

} // namespace std::chrono

namespace ycxx::adl_free {

// [time.parse]: the manipulator; `Mode` 0: (fmt, tp), 1: + abbrev, 2: + offset, 3: + both.
template <int Mode, class charT, class traits, class Alloc, class Parsable>
class chrono_parse_manip {
  const charT* fmt_;
  Parsable& tp_;
  std::basic_string<charT, traits, Alloc>* abbrev_;
  std::chrono::minutes* offset_;

public:
  chrono_parse_manip(const charT* fmt, Parsable& tp, std::basic_string<charT, traits, Alloc>* abbrev,
                     std::chrono::minutes* offset) noexcept
      : fmt_(fmt), tp_(tp), abbrev_(abbrev), offset_(offset) {}
  chrono_parse_manip(const chrono_parse_manip&) = delete;
  chrono_parse_manip& operator=(const chrono_parse_manip&) = delete;

  friend std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& is,
                                                       chrono_parse_manip&& m) {
    if constexpr (Mode == 0)
      from_stream(is, m.fmt_, m.tp_);
    else if constexpr (Mode == 1)
      from_stream(is, m.fmt_, m.tp_, m.abbrev_);
    else if constexpr (Mode == 2)
      from_stream(is, m.fmt_, m.tp_, static_cast<std::basic_string<charT, traits, Alloc>*>(nullptr), m.offset_);
    else
      from_stream(is, m.fmt_, m.tp_, m.abbrev_, m.offset_);
    return is;
  }
};

} // namespace ycxx::adl_free

namespace ycxx::detail {
template <class charT, class traits, class Parsable, class... Extra>
concept chrono_parsable = requires(std::basic_istream<charT, traits>& is, const charT* fmt, Parsable& tp,
                                   Extra... extra) { from_stream(is, fmt, tp, extra...); };
} // namespace ycxx::detail

namespace std::chrono {

template <class charT, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, char_traits<charT>, Parsable>
auto parse(const charT* fmt, Parsable& tp) {
  return ycxx::adl_free::chrono_parse_manip<0, charT, char_traits<charT>, allocator<charT>, Parsable>(fmt, tp, nullptr,
                                                                                                       nullptr);
}
template <class charT, class traits, class Alloc, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, traits, Parsable>
auto parse(const basic_string<charT, traits, Alloc>& fmt, Parsable& tp) {
  return ycxx::adl_free::chrono_parse_manip<0, charT, traits, Alloc, Parsable>(fmt.c_str(), tp, nullptr, nullptr);
}
template <class charT, class traits, class Alloc, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, traits, Parsable, basic_string<charT, traits, Alloc>*>
auto parse(const charT* fmt, Parsable& tp, basic_string<charT, traits, Alloc>& abbrev) {
  return ycxx::adl_free::chrono_parse_manip<1, charT, traits, Alloc, Parsable>(fmt, tp, __builtin_addressof(abbrev),
                                                                              nullptr);
}
template <class charT, class traits, class Alloc, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, traits, Parsable, basic_string<charT, traits, Alloc>*>
auto parse(const basic_string<charT, traits, Alloc>& fmt, Parsable& tp, basic_string<charT, traits, Alloc>& abbrev) {
  return ycxx::adl_free::chrono_parse_manip<1, charT, traits, Alloc, Parsable>(fmt.c_str(), tp,
                                                                              __builtin_addressof(abbrev), nullptr);
}
template <class charT, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, char_traits<charT>, Parsable, basic_string<charT>*, minutes*>
auto parse(const charT* fmt, Parsable& tp, minutes& offset) {
  return ycxx::adl_free::chrono_parse_manip<2, charT, char_traits<charT>, allocator<charT>, Parsable>(
      fmt, tp, nullptr, __builtin_addressof(offset));
}
template <class charT, class traits, class Alloc, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, traits, Parsable, basic_string<charT, traits, Alloc>*, minutes*>
auto parse(const basic_string<charT, traits, Alloc>& fmt, Parsable& tp, minutes& offset) {
  return ycxx::adl_free::chrono_parse_manip<2, charT, traits, Alloc, Parsable>(fmt.c_str(), tp, nullptr,
                                                                              __builtin_addressof(offset));
}
template <class charT, class traits, class Alloc, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, traits, Parsable, basic_string<charT, traits, Alloc>*, minutes*>
auto parse(const charT* fmt, Parsable& tp, basic_string<charT, traits, Alloc>& abbrev, minutes& offset) {
  return ycxx::adl_free::chrono_parse_manip<3, charT, traits, Alloc, Parsable>(fmt, tp, __builtin_addressof(abbrev),
                                                                              __builtin_addressof(offset));
}
template <class charT, class traits, class Alloc, class Parsable>
  requires ycxx::detail::chrono_parsable<charT, traits, Parsable, basic_string<charT, traits, Alloc>*, minutes*>
auto parse(const basic_string<charT, traits, Alloc>& fmt, Parsable& tp, basic_string<charT, traits, Alloc>& abbrev,
           minutes& offset) {
  return ycxx::adl_free::chrono_parse_manip<3, charT, traits, Alloc, Parsable>(
      fmt.c_str(), tp, __builtin_addressof(abbrev), __builtin_addressof(offset));
}

} // namespace std::chrono
