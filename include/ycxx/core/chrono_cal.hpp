// libycxx core: the civil calendar of <chrono> ([time.cal]), hh_mm_ss ([time.hms]), the 12/24
// hour functions ([time.12]) and the calendar parts of [time.hash]. Everything is constexpr and
// needs nothing from the OS; the stream operators, formatters and parsers are hosted
// (ycxx/hosted/chrono_io.hpp).
//
// Days and civil dates are converted with the era-based algorithm: a year starting on March 1st
// makes February the last month, so the day of the year is a linear function of the month, and
// 400-year eras (146097 days) make the conversion valid for every int year.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/compare.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Floored division and modulo.
constexpr long long __chrono_floor_div(long long a, long long b) noexcept {
  const long long __q = a / b;
  return (a % b != 0 && ((a < 0) != (b < 0))) ? __q - 1 : __q;
}
constexpr long long __chrono_modulo(long long a, long long b) noexcept {
  const long long r = a % b;
  return r != 0 && ((r < 0) != (b < 0)) ? r + b : r;
}
// a + b and a - b modulo 2^64: for results that are truncated to a narrow field anyway (day,
// year), and for counts at the ends of the range, without signed overflow.
constexpr unsigned long long __wrap_add(long long a, long long b) noexcept {
  return static_cast<unsigned long long>(a) + static_cast<unsigned long long>(b);
}
constexpr unsigned long long __wrap_sub(long long a, long long b) noexcept {
  return static_cast<unsigned long long>(a) - static_cast<unsigned long long>(b);
}

struct __civil_date {
  int y;
  unsigned m;
  unsigned d;
};

// Days since 1970-01-01 of the proleptic Gregorian date y-m-d (m in [1, 12], d any value).
constexpr int __days_from_civil(int y, unsigned m, unsigned d) noexcept {
  const long long __yy = static_cast<long long>(y) - (m <= 2 ? 1 : 0);
  const long long __era = ::__ycxx::__detail::__chrono_floor_div(__yy, 400);
  const long long __yoe = __yy - __era * 400;                                              // [0, 399]
  const long long __doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + static_cast<long long>(d) - 1; // [0, 365]
  const long long __doe = __yoe * 365 + __yoe / 4 - __yoe / 100 + __doy;                        // [0, 146096]
  return static_cast<int>(__era * 146097 + __doe - 719468);
}

// The proleptic Gregorian date of the day z days after 1970-01-01.
constexpr __civil_date __civil_from_days(long long __z) noexcept {
  // z + 719468 days from 0000-03-01, split into eras without overflow for any z.
  const long long __shifted = ::__ycxx::__detail::__chrono_modulo(__z, 146097) + 719468;
  const long long __era = ::__ycxx::__detail::__chrono_floor_div(__z, 146097) + ::__ycxx::__detail::__chrono_floor_div(__shifted, 146097);
  const long long __doe = ::__ycxx::__detail::__chrono_modulo(__shifted, 146097);      // [0, 146096]
  const long long __yoe = (__doe - __doe / 1460 + __doe / 36524 - __doe / 146096) / 365; // [0, 399]
  const long long __doy = __doe - (365 * __yoe + __yoe / 4 - __yoe / 100);              // [0, 365]
  const long long __mp = (5 * __doy + 2) / 153;                                   // [0, 11], March first
  const unsigned d = static_cast<unsigned>(__doy - (153 * __mp + 2) / 5 + 1);
  const unsigned m = static_cast<unsigned>(__mp < 10 ? __mp + 3 : __mp - 9);
  return {static_cast<int>(__yoe + __era * 400 + (m <= 2 ? 1 : 0)), m, d};
}

// 0 for Sunday.
constexpr unsigned __weekday_from_days(long long __z) noexcept {
  return static_cast<unsigned>(::__ycxx::__detail::__chrono_modulo(::__ycxx::__detail::__chrono_modulo(__z, 7) + 4, 7));
}

constexpr bool __is_leap_year(int y) noexcept { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }

// The number of days of month m (in [1, 12]) of year y.
constexpr unsigned __last_day_of(int y, unsigned m) noexcept {
  constexpr unsigned char table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return m == 2 && ::__ycxx::__detail::__is_leap_year(y) ? 29u : table[m - 1];
}

// hh_mm_ss::fractional_width ([time.hms.members]/1): the smallest w in [0, 18] such that 10^w is
// a multiple of the reduced period's denominator, or 6.
consteval unsigned __hms_fractional_width(std::intmax_t den) {
  unsigned __twos = 0, __fives = 0;
  while (den % 2 == 0)
    den /= 2, ++__twos;
  while (den % 5 == 0)
    den /= 5, ++__fives;
  const unsigned __w = __twos > __fives ? __twos : __fives;
  return den == 1 && __w <= 18 ? __w : 6;
}
consteval std::intmax_t __pow10(unsigned n) {
  std::intmax_t r = 1;
  while (n-- != 0)
    r *= 10;
  return r;
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.cal.last]
struct last_spec {
  explicit last_spec() = default;
};
inline constexpr last_spec last{};

// [time.cal.day]
class day {
  unsigned char __d_;

public:
  day() = default;
  constexpr explicit day(unsigned d) noexcept : __d_(static_cast<unsigned char>(d)) {}
  constexpr day& operator++() noexcept {
    ++__d_;
    return *this;
  }
  constexpr day operator++(int) noexcept {
    day t = *this;
    ++__d_;
    return t;
  }
  constexpr day& operator--() noexcept {
    --__d_;
    return *this;
  }
  constexpr day operator--(int) noexcept {
    day t = *this;
    --__d_;
    return t;
  }
  constexpr day& operator+=(const days& d) noexcept {
    *this = day(static_cast<unsigned>(::__ycxx::__detail::__wrap_add(__d_, d.count())));
    return *this;
  }
  constexpr day& operator-=(const days& d) noexcept {
    *this = day(static_cast<unsigned>(::__ycxx::__detail::__wrap_sub(__d_, d.count())));
    return *this;
  }
  constexpr explicit operator unsigned() const noexcept { return __d_; }
  constexpr bool ok() const noexcept { return __d_ >= 1 && __d_ <= 31; }
};

constexpr bool operator==(const day& __x, const day& y) noexcept {
  return static_cast<unsigned>(__x) == static_cast<unsigned>(y);
}
constexpr strong_ordering operator<=>(const day& __x, const day& y) noexcept {
  return static_cast<unsigned>(__x) <=> static_cast<unsigned>(y);
}
constexpr day operator+(const day& __x, const days& y) noexcept {
  return day(static_cast<unsigned>(::__ycxx::__detail::__wrap_add(static_cast<unsigned>(__x), y.count())));
}
constexpr day operator+(const days& __x, const day& y) noexcept { return y + __x; }
constexpr day operator-(const day& __x, const days& y) noexcept {
  return day(static_cast<unsigned>(::__ycxx::__detail::__wrap_sub(static_cast<unsigned>(__x), y.count())));
}
constexpr days operator-(const day& __x, const day& y) noexcept {
  return days(static_cast<int>(static_cast<unsigned>(__x)) - static_cast<int>(static_cast<unsigned>(y)));
}

// [time.cal.month]
class month {
  unsigned char __m_;

public:
  month() = default;
  constexpr explicit month(unsigned m) noexcept : __m_(static_cast<unsigned char>(m)) {}
  constexpr month& operator++() noexcept {
    *this += months(1);
    return *this;
  }
  constexpr month operator++(int) noexcept {
    month t = *this;
    ++*this;
    return t;
  }
  constexpr month& operator--() noexcept {
    *this -= months(1);
    return *this;
  }
  constexpr month operator--(int) noexcept {
    month t = *this;
    --*this;
    return t;
  }
  constexpr month& operator+=(const months& m) noexcept;
  constexpr month& operator-=(const months& m) noexcept;
  constexpr explicit operator unsigned() const noexcept { return __m_; }
  constexpr bool ok() const noexcept { return __m_ >= 1 && __m_ <= 12; }
};

constexpr bool operator==(const month& __x, const month& y) noexcept {
  return static_cast<unsigned>(__x) == static_cast<unsigned>(y);
}
constexpr strong_ordering operator<=>(const month& __x, const month& y) noexcept {
  return static_cast<unsigned>(__x) <=> static_cast<unsigned>(y);
}
constexpr month operator+(const month& __x, const months& y) noexcept {
  return month(static_cast<unsigned>(
      ::__ycxx::__detail::__chrono_modulo(static_cast<long long>(static_cast<unsigned>(__x)) - 1 +
                                        ::__ycxx::__detail::__chrono_modulo(y.count(), 12),
                                    12) +
      1));
}
constexpr month operator+(const months& __x, const month& y) noexcept { return y + __x; }
constexpr month operator-(const month& __x, const months& y) noexcept {
  return month(static_cast<unsigned>(
      ::__ycxx::__detail::__chrono_modulo(static_cast<long long>(static_cast<unsigned>(__x)) - 1 -
                                        ::__ycxx::__detail::__chrono_modulo(y.count(), 12),
                                    12) +
      1));
}
constexpr months operator-(const month& __x, const month& y) noexcept {
  return months(static_cast<int>(::__ycxx::__detail::__chrono_modulo(
      static_cast<long long>(static_cast<unsigned>(__x)) - static_cast<long long>(static_cast<unsigned>(y)), 12)));
}
constexpr month& month::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
constexpr month& month::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.year]
class year {
  short __y_;

public:
  year() = default;
  constexpr explicit year(int y) noexcept : __y_(static_cast<short>(y)) {}
  constexpr year& operator++() noexcept {
    ++__y_;
    return *this;
  }
  constexpr year operator++(int) noexcept {
    year t = *this;
    ++__y_;
    return t;
  }
  constexpr year& operator--() noexcept {
    --__y_;
    return *this;
  }
  constexpr year operator--(int) noexcept {
    year t = *this;
    --__y_;
    return t;
  }
  constexpr year& operator+=(const years& y) noexcept {
    *this = year(static_cast<int>(::__ycxx::__detail::__wrap_add(__y_, y.count())));
    return *this;
  }
  constexpr year& operator-=(const years& y) noexcept {
    *this = year(static_cast<int>(::__ycxx::__detail::__wrap_sub(__y_, y.count())));
    return *this;
  }
  constexpr year operator+() const noexcept { return *this; }
  constexpr year operator-() const noexcept { return year(-__y_); }
  constexpr bool is_leap() const noexcept { return ::__ycxx::__detail::__is_leap_year(__y_); }
  constexpr explicit operator int() const noexcept { return __y_; }
  constexpr bool ok() const noexcept { return __y_ != -32768; }
  static constexpr year min() noexcept { return year(-32767); }
  static constexpr year max() noexcept { return year(32767); }
};

constexpr bool operator==(const year& __x, const year& y) noexcept { return static_cast<int>(__x) == static_cast<int>(y); }
constexpr strong_ordering operator<=>(const year& __x, const year& y) noexcept {
  return static_cast<int>(__x) <=> static_cast<int>(y);
}
constexpr year operator+(const year& __x, const years& y) noexcept {
  return year(static_cast<int>(::__ycxx::__detail::__wrap_add(static_cast<int>(__x), y.count())));
}
constexpr year operator+(const years& __x, const year& y) noexcept { return y + __x; }
constexpr year operator-(const year& __x, const years& y) noexcept {
  return year(static_cast<int>(::__ycxx::__detail::__wrap_sub(static_cast<int>(__x), y.count())));
}
constexpr years operator-(const year& __x, const year& y) noexcept {
  return years(static_cast<int>(__x) - static_cast<int>(y));
}

class weekday_indexed;
class weekday_last;

// [time.cal.wd]
class weekday {
  unsigned char __wd_;

public:
  weekday() = default;
  constexpr explicit weekday(unsigned __wd) noexcept : __wd_(static_cast<unsigned char>(__wd == 7 ? 0 : __wd)) {}
  constexpr weekday(const sys_days& __dp) noexcept
      : __wd_(static_cast<unsigned char>(::__ycxx::__detail::__weekday_from_days(__dp.time_since_epoch().count()))) {}
  constexpr explicit weekday(const local_days& __dp) noexcept
      : __wd_(static_cast<unsigned char>(::__ycxx::__detail::__weekday_from_days(__dp.time_since_epoch().count()))) {}
  constexpr weekday& operator++() noexcept {
    *this += days(1);
    return *this;
  }
  constexpr weekday operator++(int) noexcept {
    weekday t = *this;
    ++*this;
    return t;
  }
  constexpr weekday& operator--() noexcept {
    *this -= days(1);
    return *this;
  }
  constexpr weekday operator--(int) noexcept {
    weekday t = *this;
    --*this;
    return t;
  }
  constexpr weekday& operator+=(const days& d) noexcept;
  constexpr weekday& operator-=(const days& d) noexcept;
  constexpr unsigned c_encoding() const noexcept { return __wd_; }
  constexpr unsigned iso_encoding() const noexcept { return __wd_ == 0 ? 7u : __wd_; }
  constexpr bool ok() const noexcept { return __wd_ <= 6; }
  constexpr weekday_indexed operator[](unsigned index) const noexcept;
  constexpr weekday_last operator[](last_spec) const noexcept;
};

constexpr bool operator==(const weekday& __x, const weekday& y) noexcept { return __x.c_encoding() == y.c_encoding(); }
constexpr weekday operator+(const weekday& __x, const days& y) noexcept {
  return weekday(static_cast<unsigned>(::__ycxx::__detail::__chrono_modulo(
      static_cast<long long>(__x.c_encoding()) + ::__ycxx::__detail::__chrono_modulo(y.count(), 7), 7)));
}
constexpr weekday operator+(const days& __x, const weekday& y) noexcept { return y + __x; }
constexpr weekday operator-(const weekday& __x, const days& y) noexcept {
  return weekday(static_cast<unsigned>(::__ycxx::__detail::__chrono_modulo(
      static_cast<long long>(__x.c_encoding()) - ::__ycxx::__detail::__chrono_modulo(y.count(), 7), 7)));
}
constexpr days operator-(const weekday& __x, const weekday& y) noexcept {
  return days(static_cast<int>(::__ycxx::__detail::__chrono_modulo(
      static_cast<long long>(__x.c_encoding()) - static_cast<long long>(y.c_encoding()), 7)));
}
constexpr weekday& weekday::operator+=(const days& d) noexcept {
  *this = *this + d;
  return *this;
}
constexpr weekday& weekday::operator-=(const days& d) noexcept {
  *this = *this - d;
  return *this;
}

// [time.cal.wdidx]
class weekday_indexed {
  chrono::weekday __wd_;
  unsigned char __index_;

public:
  weekday_indexed() = default;
  constexpr weekday_indexed(const chrono::weekday& __wd, unsigned index) noexcept
      : __wd_(__wd), __index_(static_cast<unsigned char>(index)) {}
  constexpr chrono::weekday weekday() const noexcept { return __wd_; }
  constexpr unsigned index() const noexcept { return __index_; }
  constexpr bool ok() const noexcept { return __wd_.ok() && __index_ >= 1 && __index_ <= 5; }
};
constexpr bool operator==(const weekday_indexed& __x, const weekday_indexed& y) noexcept {
  return __x.weekday() == y.weekday() && __x.index() == y.index();
}

// [time.cal.wdlast]
class weekday_last {
  chrono::weekday __wd_;

public:
  constexpr explicit weekday_last(const chrono::weekday& __wd) noexcept : __wd_(__wd) {}
  constexpr chrono::weekday weekday() const noexcept { return __wd_; }
  constexpr bool ok() const noexcept { return __wd_.ok(); }
};
constexpr bool operator==(const weekday_last& __x, const weekday_last& y) noexcept { return __x.weekday() == y.weekday(); }

constexpr weekday_indexed weekday::operator[](unsigned index) const noexcept { return {*this, index}; }
constexpr weekday_last weekday::operator[](last_spec) const noexcept { return weekday_last(*this); }

// [time.cal.md]
class month_day {
  chrono::month __m_;
  chrono::day __d_;

public:
  month_day() = default;
  constexpr month_day(const chrono::month& m, const chrono::day& d) noexcept : __m_(m), __d_(d) {}
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr chrono::day day() const noexcept { return __d_; }
  // February has 29 days here.
  constexpr bool ok() const noexcept {
    return __m_.ok() && static_cast<unsigned>(__d_) >= 1 &&
           static_cast<unsigned>(__d_) <= ::__ycxx::__detail::__last_day_of(2000, static_cast<unsigned>(__m_));
  }
};
constexpr bool operator==(const month_day& __x, const month_day& y) noexcept {
  return __x.month() == y.month() && __x.day() == y.day();
}
constexpr strong_ordering operator<=>(const month_day& __x, const month_day& y) noexcept {
  if (auto c = __x.month() <=> y.month(); c != 0)
    return c;
  return __x.day() <=> y.day();
}

// [time.cal.mdlast]
class month_day_last {
  chrono::month __m_;

public:
  constexpr explicit month_day_last(const chrono::month& m) noexcept : __m_(m) {}
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr bool ok() const noexcept { return __m_.ok(); }
};
constexpr bool operator==(const month_day_last& __x, const month_day_last& y) noexcept { return __x.month() == y.month(); }
constexpr strong_ordering operator<=>(const month_day_last& __x, const month_day_last& y) noexcept {
  return __x.month() <=> y.month();
}

// [time.cal.mwd]
class month_weekday {
  chrono::month __m_;
  chrono::weekday_indexed __wdi_;

public:
  constexpr month_weekday(const chrono::month& m, const chrono::weekday_indexed& __wdi) noexcept : __m_(m), __wdi_(__wdi) {}
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr chrono::weekday_indexed weekday_indexed() const noexcept { return __wdi_; }
  constexpr bool ok() const noexcept { return __m_.ok() && __wdi_.ok(); }
};
constexpr bool operator==(const month_weekday& __x, const month_weekday& y) noexcept {
  return __x.month() == y.month() && __x.weekday_indexed() == y.weekday_indexed();
}

// [time.cal.mwdlast]
class month_weekday_last {
  chrono::month __m_;
  chrono::weekday_last __wdl_;

public:
  constexpr month_weekday_last(const chrono::month& m, const chrono::weekday_last& __wdl) noexcept : __m_(m), __wdl_(__wdl) {}
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr chrono::weekday_last weekday_last() const noexcept { return __wdl_; }
  constexpr bool ok() const noexcept { return __m_.ok() && __wdl_.ok(); }
};
constexpr bool operator==(const month_weekday_last& __x, const month_weekday_last& y) noexcept {
  return __x.month() == y.month() && __x.weekday_last() == y.weekday_last();
}

// The months overloads of year_month and the types built on it are templates (template <class =
// void>): for an argument convertible to both months and years, its conversion to years must be
// the better one ([time.cal.ym.members]/3 and friends), and a non-template wins the tie.

// [time.cal.ym]
class year_month;
}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// ym + n months, or ym - n months with `__subtract`, without signed overflow for any n.
constexpr std::chrono::year_month __add_months(const std::chrono::year_month& __ym, long long n,
                                             bool __subtract = false) noexcept;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {
class year_month {
  chrono::year __y_;
  chrono::month __m_;

public:
  year_month() = default;
  constexpr year_month(const chrono::year& y, const chrono::month& m) noexcept : __y_(y), __m_(m) {}
  constexpr chrono::year year() const noexcept { return __y_; }
  constexpr chrono::month month() const noexcept { return __m_; }
  template <class = void>
  constexpr year_month& operator+=(const months& __dm) noexcept;
  template <class = void>
  constexpr year_month& operator-=(const months& __dm) noexcept;
  constexpr year_month& operator+=(const years& __dy) noexcept {
    __y_ += __dy;
    return *this;
  }
  constexpr year_month& operator-=(const years& __dy) noexcept {
    __y_ -= __dy;
    return *this;
  }
  constexpr bool ok() const noexcept { return __y_.ok() && __m_.ok(); }
};
constexpr bool operator==(const year_month& __x, const year_month& y) noexcept {
  return __x.year() == y.year() && __x.month() == y.month();
}
constexpr strong_ordering operator<=>(const year_month& __x, const year_month& y) noexcept {
  if (auto c = __x.year() <=> y.year(); c != 0)
    return c;
  return __x.month() <=> y.month();
}
}} // namespace std::chrono

constexpr std::chrono::year_month __ycxx::__detail::__add_months(const std::chrono::year_month& __ym, long long n,
                                                          bool __subtract) noexcept {
  // n = 12q + r with r in [0, 11]; the month moves by r, the year by q and the carry.
  const long long __q = ::__ycxx::__detail::__chrono_floor_div(n, 12);
  const long long r = ::__ycxx::__detail::__chrono_modulo(n, 12);
  const long long m = static_cast<long long>(static_cast<unsigned>(__ym.month())) - 1 + (__subtract ? -r : r);
  const long long __carry = ::__ycxx::__detail::__chrono_floor_div(m, 12);
  const unsigned long long y = (__subtract ? ::__ycxx::__detail::__wrap_sub(static_cast<int>(__ym.year()), __q)
                                         : ::__ycxx::__detail::__wrap_add(static_cast<int>(__ym.year()), __q)) +
                               static_cast<unsigned long long>(__carry);
  return {std::chrono::year(static_cast<int>(y)),
          std::chrono::month(static_cast<unsigned>(::__ycxx::__detail::__chrono_modulo(m, 12) + 1))};
}

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {
template <class = void>
constexpr year_month operator+(const year_month& __ym, const months& __dm) noexcept {
  return ::__ycxx::__detail::__add_months(__ym, __dm.count());
}
template <class = void>
constexpr year_month operator+(const months& __dm, const year_month& __ym) noexcept {
  return __ym + __dm;
}
template <class = void>
constexpr year_month operator-(const year_month& __ym, const months& __dm) noexcept {
  return ::__ycxx::__detail::__add_months(__ym, __dm.count(), true);
}
constexpr months operator-(const year_month& __x, const year_month& y) noexcept {
  return months(
      (static_cast<int>(__x.year()) - static_cast<int>(y.year())) * 12 +
      (static_cast<int>(static_cast<unsigned>(__x.month())) - static_cast<int>(static_cast<unsigned>(y.month()))));
}
constexpr year_month operator+(const year_month& __ym, const years& __dy) noexcept { return {__ym.year() + __dy, __ym.month()}; }
constexpr year_month operator+(const years& __dy, const year_month& __ym) noexcept { return __ym + __dy; }
constexpr year_month operator-(const year_month& __ym, const years& __dy) noexcept { return {__ym.year() - __dy, __ym.month()}; }
template <class _Vp>
constexpr year_month& year_month::operator+=(const months& __dm) noexcept {
  *this = *this + __dm;
  return *this;
}
template <class _Vp>
constexpr year_month& year_month::operator-=(const months& __dm) noexcept {
  *this = *this - __dm;
  return *this;
}

class year_month_day_last;

// [time.cal.ymd]
class year_month_day {
  chrono::year __y_;
  chrono::month __m_;
  chrono::day __d_;

  static constexpr year_month_day __from_days(long long __z) noexcept {
    const __ycxx::__detail::__civil_date c = ::__ycxx::__detail::__civil_from_days(__z);
    return {chrono::year(c.y), chrono::month(c.m), chrono::day(c.d)};
  }
  constexpr days __to_days() const noexcept {
    // For an invalid day of a valid year and month: sys_days{y/m/1d} + (d - 1d).
    return days(::__ycxx::__detail::__days_from_civil(static_cast<int>(__y_), static_cast<unsigned>(__m_), 1) +
                (static_cast<int>(static_cast<unsigned>(__d_)) - 1));
  }

public:
  year_month_day() = default;
  constexpr year_month_day(const chrono::year& y, const chrono::month& m, const chrono::day& d) noexcept
      : __y_(y), __m_(m), __d_(d) {}
  constexpr year_month_day(const year_month_day_last& __ymdl) noexcept;
  constexpr year_month_day(const sys_days& __dp) noexcept : year_month_day(__from_days(__dp.time_since_epoch().count())) {}
  constexpr explicit year_month_day(const local_days& __dp) noexcept
      : year_month_day(__from_days(__dp.time_since_epoch().count())) {}
  template <class = void>
  constexpr year_month_day& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_day& operator-=(const months& m) noexcept;
  constexpr year_month_day& operator+=(const years& y) noexcept {
    __y_ += y;
    return *this;
  }
  constexpr year_month_day& operator-=(const years& y) noexcept {
    __y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return __y_; }
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr chrono::day day() const noexcept { return __d_; }
  constexpr operator sys_days() const noexcept { return sys_days(__to_days()); }
  constexpr explicit operator local_days() const noexcept { return local_days(__to_days()); }
  constexpr bool ok() const noexcept {
    return __y_.ok() && __m_.ok() && static_cast<unsigned>(__d_) >= 1 &&
           static_cast<unsigned>(__d_) <= ::__ycxx::__detail::__last_day_of(static_cast<int>(__y_), static_cast<unsigned>(__m_));
  }
};
constexpr bool operator==(const year_month_day& __x, const year_month_day& y) noexcept {
  return __x.year() == y.year() && __x.month() == y.month() && __x.day() == y.day();
}
constexpr strong_ordering operator<=>(const year_month_day& __x, const year_month_day& y) noexcept {
  if (auto c = __x.year() <=> y.year(); c != 0)
    return c;
  if (auto c = __x.month() <=> y.month(); c != 0)
    return c;
  return __x.day() <=> y.day();
}
template <class = void>
constexpr year_month_day operator+(const year_month_day& ymd, const months& __dm) noexcept {
  const year_month __ym = year_month(ymd.year(), ymd.month()) + __dm;
  return {__ym.year(), __ym.month(), ymd.day()};
}
template <class = void>
constexpr year_month_day operator+(const months& __dm, const year_month_day& ymd) noexcept {
  return ymd + __dm;
}
template <class = void>
constexpr year_month_day operator-(const year_month_day& ymd, const months& __dm) noexcept {
  const year_month __ym = ::__ycxx::__detail::__add_months({ymd.year(), ymd.month()}, __dm.count(), true);
  return {__ym.year(), __ym.month(), ymd.day()};
}
constexpr year_month_day operator+(const year_month_day& ymd, const years& __dy) noexcept {
  return {ymd.year() + __dy, ymd.month(), ymd.day()};
}
constexpr year_month_day operator+(const years& __dy, const year_month_day& ymd) noexcept { return ymd + __dy; }
constexpr year_month_day operator-(const year_month_day& ymd, const years& __dy) noexcept {
  return {ymd.year() - __dy, ymd.month(), ymd.day()};
}
template <class _Vp>
constexpr year_month_day& year_month_day::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class _Vp>
constexpr year_month_day& year_month_day::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.ymdlast]
class year_month_day_last {
  chrono::year __y_;
  chrono::month_day_last __mdl_;

public:
  constexpr year_month_day_last(const chrono::year& y, const chrono::month_day_last& __mdl) noexcept
      : __y_(y), __mdl_(__mdl) {}
  template <class = void>
  constexpr year_month_day_last& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_day_last& operator-=(const months& m) noexcept;
  constexpr year_month_day_last& operator+=(const years& y) noexcept {
    __y_ += y;
    return *this;
  }
  constexpr year_month_day_last& operator-=(const years& y) noexcept {
    __y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return __y_; }
  constexpr chrono::month month() const noexcept { return __mdl_.month(); }
  constexpr chrono::month_day_last month_day_last() const noexcept { return __mdl_; }
  // For a month that is not ok() (the value is unspecified there): 31.
  constexpr chrono::day day() const noexcept {
    const unsigned m = static_cast<unsigned>(__mdl_.month());
    return chrono::day(m >= 1 && m <= 12 ? ::__ycxx::__detail::__last_day_of(static_cast<int>(__y_), m) : 31u);
  }
  constexpr operator sys_days() const noexcept { return sys_days(year_month_day(__y_, month(), day())); }
  constexpr explicit operator local_days() const noexcept {
    return local_days(sys_days(*this).time_since_epoch());
  }
  constexpr bool ok() const noexcept { return __y_.ok() && __mdl_.ok(); }
};
constexpr bool operator==(const year_month_day_last& __x, const year_month_day_last& y) noexcept {
  return __x.year() == y.year() && __x.month_day_last() == y.month_day_last();
}
constexpr strong_ordering operator<=>(const year_month_day_last& __x, const year_month_day_last& y) noexcept {
  if (auto c = __x.year() <=> y.year(); c != 0)
    return c;
  return __x.month_day_last() <=> y.month_day_last();
}
template <class = void>
constexpr year_month_day_last operator+(const year_month_day_last& __ymdl, const months& __dm) noexcept {
  const year_month __ym = year_month(__ymdl.year(), __ymdl.month()) + __dm;
  return {__ym.year(), month_day_last(__ym.month())};
}
template <class = void>
constexpr year_month_day_last operator+(const months& __dm, const year_month_day_last& __ymdl) noexcept {
  return __ymdl + __dm;
}
template <class = void>
constexpr year_month_day_last operator-(const year_month_day_last& __ymdl, const months& __dm) noexcept {
  const year_month __ym = ::__ycxx::__detail::__add_months({__ymdl.year(), __ymdl.month()}, __dm.count(), true);
  return {__ym.year(), month_day_last(__ym.month())};
}
constexpr year_month_day_last operator+(const year_month_day_last& __ymdl, const years& __dy) noexcept {
  return {__ymdl.year() + __dy, __ymdl.month_day_last()};
}
constexpr year_month_day_last operator+(const years& __dy, const year_month_day_last& __ymdl) noexcept {
  return __ymdl + __dy;
}
constexpr year_month_day_last operator-(const year_month_day_last& __ymdl, const years& __dy) noexcept {
  return {__ymdl.year() - __dy, __ymdl.month_day_last()};
}
template <class _Vp>
constexpr year_month_day_last& year_month_day_last::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class _Vp>
constexpr year_month_day_last& year_month_day_last::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

constexpr year_month_day::year_month_day(const year_month_day_last& __ymdl) noexcept
    : __y_(__ymdl.year()), __m_(__ymdl.month()), __d_(__ymdl.day()) {}

// [time.cal.ymwd]
class year_month_weekday {
  chrono::year __y_;
  chrono::month __m_;
  chrono::weekday_indexed __wdi_;

  static constexpr year_month_weekday __from_days(long long __z) noexcept {
    const __ycxx::__detail::__civil_date c = ::__ycxx::__detail::__civil_from_days(__z);
    return {chrono::year(c.y), chrono::month(c.m),
            chrono::weekday_indexed(chrono::weekday(::__ycxx::__detail::__weekday_from_days(__z)), (c.d - 1) / 7 + 1)};
  }
  constexpr days __to_days() const noexcept {
    const int first = ::__ycxx::__detail::__days_from_civil(static_cast<int>(__y_), static_cast<unsigned>(__m_), 1);
    const chrono::weekday __wd1(::__ycxx::__detail::__weekday_from_days(first));
    return days(first + (__wdi_.weekday() - __wd1).count() + (static_cast<int>(__wdi_.index()) - 1) * 7);
  }

public:
  year_month_weekday() = default;
  constexpr year_month_weekday(const chrono::year& y, const chrono::month& m,
                               const chrono::weekday_indexed& __wdi) noexcept
      : __y_(y), __m_(m), __wdi_(__wdi) {}
  constexpr year_month_weekday(const sys_days& __dp) noexcept
      : year_month_weekday(__from_days(__dp.time_since_epoch().count())) {}
  constexpr explicit year_month_weekday(const local_days& __dp) noexcept
      : year_month_weekday(__from_days(__dp.time_since_epoch().count())) {}
  template <class = void>
  constexpr year_month_weekday& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_weekday& operator-=(const months& m) noexcept;
  constexpr year_month_weekday& operator+=(const years& y) noexcept {
    __y_ += y;
    return *this;
  }
  constexpr year_month_weekday& operator-=(const years& y) noexcept {
    __y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return __y_; }
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr chrono::weekday weekday() const noexcept { return __wdi_.weekday(); }
  constexpr unsigned index() const noexcept { return __wdi_.index(); }
  constexpr chrono::weekday_indexed weekday_indexed() const noexcept { return __wdi_; }
  constexpr operator sys_days() const noexcept { return sys_days(__to_days()); }
  constexpr explicit operator local_days() const noexcept { return local_days(__to_days()); }
  constexpr bool ok() const noexcept {
    if (!__y_.ok() || !__m_.ok() || !__wdi_.ok())
      return false;
    if (__wdi_.index() <= 4)
      return true;
    const int first = ::__ycxx::__detail::__days_from_civil(static_cast<int>(__y_), static_cast<unsigned>(__m_), 1);
    const unsigned d = static_cast<unsigned>(__to_days().count() - first) + 1;
    return d <= ::__ycxx::__detail::__last_day_of(static_cast<int>(__y_), static_cast<unsigned>(__m_));
  }
};
constexpr bool operator==(const year_month_weekday& __x, const year_month_weekday& y) noexcept {
  return __x.year() == y.year() && __x.month() == y.month() && __x.weekday_indexed() == y.weekday_indexed();
}
template <class = void>
constexpr year_month_weekday operator+(const year_month_weekday& __ymwd, const months& __dm) noexcept {
  const year_month __ym = year_month(__ymwd.year(), __ymwd.month()) + __dm;
  return {__ym.year(), __ym.month(), __ymwd.weekday_indexed()};
}
template <class = void>
constexpr year_month_weekday operator+(const months& __dm, const year_month_weekday& __ymwd) noexcept {
  return __ymwd + __dm;
}
template <class = void>
constexpr year_month_weekday operator-(const year_month_weekday& __ymwd, const months& __dm) noexcept {
  const year_month __ym = ::__ycxx::__detail::__add_months({__ymwd.year(), __ymwd.month()}, __dm.count(), true);
  return {__ym.year(), __ym.month(), __ymwd.weekday_indexed()};
}
constexpr year_month_weekday operator+(const year_month_weekday& __ymwd, const years& __dy) noexcept {
  return {__ymwd.year() + __dy, __ymwd.month(), __ymwd.weekday_indexed()};
}
constexpr year_month_weekday operator+(const years& __dy, const year_month_weekday& __ymwd) noexcept { return __ymwd + __dy; }
constexpr year_month_weekday operator-(const year_month_weekday& __ymwd, const years& __dy) noexcept {
  return {__ymwd.year() - __dy, __ymwd.month(), __ymwd.weekday_indexed()};
}
template <class _Vp>
constexpr year_month_weekday& year_month_weekday::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class _Vp>
constexpr year_month_weekday& year_month_weekday::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.ymwdlast]
class year_month_weekday_last {
  chrono::year __y_;
  chrono::month __m_;
  chrono::weekday_last __wdl_;

  constexpr days __to_days() const noexcept {
    const sys_days __l = sys_days(year_month_day_last(__y_, month_day_last(__m_)));
    return __l.time_since_epoch() - (chrono::weekday(__l) - __wdl_.weekday());
  }

public:
  constexpr year_month_weekday_last(const chrono::year& y, const chrono::month& m,
                                    const chrono::weekday_last& __wdl) noexcept
      : __y_(y), __m_(m), __wdl_(__wdl) {}
  template <class = void>
  constexpr year_month_weekday_last& operator+=(const months& m) noexcept;
  template <class = void>
  constexpr year_month_weekday_last& operator-=(const months& m) noexcept;
  constexpr year_month_weekday_last& operator+=(const years& y) noexcept {
    __y_ += y;
    return *this;
  }
  constexpr year_month_weekday_last& operator-=(const years& y) noexcept {
    __y_ -= y;
    return *this;
  }
  constexpr chrono::year year() const noexcept { return __y_; }
  constexpr chrono::month month() const noexcept { return __m_; }
  constexpr chrono::weekday weekday() const noexcept { return __wdl_.weekday(); }
  constexpr chrono::weekday_last weekday_last() const noexcept { return __wdl_; }
  constexpr operator sys_days() const noexcept { return sys_days(__to_days()); }
  constexpr explicit operator local_days() const noexcept { return local_days(__to_days()); }
  constexpr bool ok() const noexcept { return __y_.ok() && __m_.ok() && __wdl_.ok(); }
};
constexpr bool operator==(const year_month_weekday_last& __x, const year_month_weekday_last& y) noexcept {
  return __x.year() == y.year() && __x.month() == y.month() && __x.weekday_last() == y.weekday_last();
}
template <class = void>
constexpr year_month_weekday_last operator+(const year_month_weekday_last& __ymwdl, const months& __dm) noexcept {
  const year_month __ym = year_month(__ymwdl.year(), __ymwdl.month()) + __dm;
  return {__ym.year(), __ym.month(), __ymwdl.weekday_last()};
}
template <class = void>
constexpr year_month_weekday_last operator+(const months& __dm, const year_month_weekday_last& __ymwdl) noexcept {
  return __ymwdl + __dm;
}
template <class = void>
constexpr year_month_weekday_last operator-(const year_month_weekday_last& __ymwdl, const months& __dm) noexcept {
  const year_month __ym =
      ::__ycxx::__detail::__add_months({__ymwdl.year(), __ymwdl.month()}, __dm.count(), true);
  return {__ym.year(), __ym.month(), __ymwdl.weekday_last()};
}
constexpr year_month_weekday_last operator+(const year_month_weekday_last& __ymwdl, const years& __dy) noexcept {
  return {__ymwdl.year() + __dy, __ymwdl.month(), __ymwdl.weekday_last()};
}
constexpr year_month_weekday_last operator+(const years& __dy, const year_month_weekday_last& __ymwdl) noexcept {
  return __ymwdl + __dy;
}
constexpr year_month_weekday_last operator-(const year_month_weekday_last& __ymwdl, const years& __dy) noexcept {
  return {__ymwdl.year() - __dy, __ymwdl.month(), __ymwdl.weekday_last()};
}
template <class _Vp>
constexpr year_month_weekday_last& year_month_weekday_last::operator+=(const months& m) noexcept {
  *this = *this + m;
  return *this;
}
template <class _Vp>
constexpr year_month_weekday_last& year_month_weekday_last::operator-=(const months& m) noexcept {
  *this = *this - m;
  return *this;
}

// [time.cal.operators]
constexpr year_month operator/(const year& y, const month& m) noexcept { return {y, m}; }
constexpr year_month operator/(const year& y, int m) noexcept { return {y, month(static_cast<unsigned>(m))}; }
constexpr month_day operator/(const month& m, const day& d) noexcept { return {m, d}; }
constexpr month_day operator/(const month& m, int d) noexcept { return {m, day(static_cast<unsigned>(d))}; }
constexpr month_day operator/(int m, const day& d) noexcept { return {month(static_cast<unsigned>(m)), d}; }
constexpr month_day operator/(const day& d, const month& m) noexcept { return {m, d}; }
constexpr month_day operator/(const day& d, int m) noexcept { return {month(static_cast<unsigned>(m)), d}; }
constexpr month_day_last operator/(const month& m, last_spec) noexcept { return month_day_last(m); }
constexpr month_day_last operator/(int m, last_spec) noexcept {
  return month_day_last(month(static_cast<unsigned>(m)));
}
constexpr month_day_last operator/(last_spec, const month& m) noexcept { return month_day_last(m); }
constexpr month_day_last operator/(last_spec, int m) noexcept {
  return month_day_last(month(static_cast<unsigned>(m)));
}
constexpr month_weekday operator/(const month& m, const weekday_indexed& __wdi) noexcept { return {m, __wdi}; }
constexpr month_weekday operator/(int m, const weekday_indexed& __wdi) noexcept {
  return {month(static_cast<unsigned>(m)), __wdi};
}
constexpr month_weekday operator/(const weekday_indexed& __wdi, const month& m) noexcept { return {m, __wdi}; }
constexpr month_weekday operator/(const weekday_indexed& __wdi, int m) noexcept {
  return {month(static_cast<unsigned>(m)), __wdi};
}
constexpr month_weekday_last operator/(const month& m, const weekday_last& __wdl) noexcept { return {m, __wdl}; }
constexpr month_weekday_last operator/(int m, const weekday_last& __wdl) noexcept {
  return {month(static_cast<unsigned>(m)), __wdl};
}
constexpr month_weekday_last operator/(const weekday_last& __wdl, const month& m) noexcept { return {m, __wdl}; }
constexpr month_weekday_last operator/(const weekday_last& __wdl, int m) noexcept {
  return {month(static_cast<unsigned>(m)), __wdl};
}
constexpr year_month_day operator/(const year_month& __ym, const day& d) noexcept { return {__ym.year(), __ym.month(), d}; }
constexpr year_month_day operator/(const year_month& __ym, int d) noexcept {
  return {__ym.year(), __ym.month(), day(static_cast<unsigned>(d))};
}
constexpr year_month_day operator/(const year& y, const month_day& __md) noexcept { return {y, __md.month(), __md.day()}; }
constexpr year_month_day operator/(int y, const month_day& __md) noexcept { return year(y) / __md; }
constexpr year_month_day operator/(const month_day& __md, const year& y) noexcept { return y / __md; }
constexpr year_month_day operator/(const month_day& __md, int y) noexcept { return year(y) / __md; }
constexpr year_month_day_last operator/(const year_month& __ym, last_spec) noexcept {
  return {__ym.year(), month_day_last(__ym.month())};
}
constexpr year_month_day_last operator/(const year& y, const month_day_last& __mdl) noexcept { return {y, __mdl}; }
constexpr year_month_day_last operator/(int y, const month_day_last& __mdl) noexcept { return {year(y), __mdl}; }
constexpr year_month_day_last operator/(const month_day_last& __mdl, const year& y) noexcept { return {y, __mdl}; }
constexpr year_month_day_last operator/(const month_day_last& __mdl, int y) noexcept { return {year(y), __mdl}; }
constexpr year_month_weekday operator/(const year_month& __ym, const weekday_indexed& __wdi) noexcept {
  return {__ym.year(), __ym.month(), __wdi};
}
constexpr year_month_weekday operator/(const year& y, const month_weekday& __mwd) noexcept {
  return {y, __mwd.month(), __mwd.weekday_indexed()};
}
constexpr year_month_weekday operator/(int y, const month_weekday& __mwd) noexcept { return year(y) / __mwd; }
constexpr year_month_weekday operator/(const month_weekday& __mwd, const year& y) noexcept { return y / __mwd; }
constexpr year_month_weekday operator/(const month_weekday& __mwd, int y) noexcept { return year(y) / __mwd; }
constexpr year_month_weekday_last operator/(const year_month& __ym, const weekday_last& __wdl) noexcept {
  return {__ym.year(), __ym.month(), __wdl};
}
constexpr year_month_weekday_last operator/(const year& y, const month_weekday_last& __mwdl) noexcept {
  return {y, __mwdl.month(), __mwdl.weekday_last()};
}
constexpr year_month_weekday_last operator/(int y, const month_weekday_last& __mwdl) noexcept { return year(y) / __mwdl; }
constexpr year_month_weekday_last operator/(const month_weekday_last& __mwdl, const year& y) noexcept {
  return y / __mwdl;
}
constexpr year_month_weekday_last operator/(const month_weekday_last& __mwdl, int y) noexcept { return year(y) / __mwdl; }

// Calendrical constants ([time.syn]).
inline constexpr weekday Sunday{0};
inline constexpr weekday Monday{1};
inline constexpr weekday Tuesday{2};
inline constexpr weekday Wednesday{3};
inline constexpr weekday Thursday{4};
inline constexpr weekday Friday{5};
inline constexpr weekday Saturday{6};
inline constexpr month January{1};
inline constexpr month February{2};
inline constexpr month March{3};
inline constexpr month April{4};
inline constexpr month May{5};
inline constexpr month June{6};
inline constexpr month July{7};
inline constexpr month August{8};
inline constexpr month September{9};
inline constexpr month October{10};
inline constexpr month November{11};
inline constexpr month December{12};

// [time.hms]
template <class _Duration>
class hh_mm_ss {
  static_assert(__ycxx::__detail::__is_duration<_Duration>, "hh_mm_ss: Duration must be a specialization of duration");

public:
  static constexpr unsigned fractional_width = __ycxx::__detail::__hms_fractional_width(_Duration::__period::den);
  using precision = chrono::duration<common_type_t<typename _Duration::rep, seconds::rep>,
                                     ratio<1, __ycxx::__detail::__pow10(fractional_width)>>;

private:
  bool __neg_;
  chrono::hours __h_;
  chrono::minutes __m_;
  chrono::seconds __s_;
  precision __ss_;

  // abs(d) ([time.hms.members]/2) in precision's rep, which is at least long long: negating the
  // most negative count of a narrower rep in Duration's own rep would overflow.
  using __wide = chrono::duration<typename precision::rep, typename _Duration::__period>;
  static constexpr __wide __abs_of(_Duration d) noexcept { return d < _Duration::zero() ? -__wide(d) : __wide(d); }

public:
  constexpr hh_mm_ss() noexcept : hh_mm_ss(_Duration::zero()) {}
  constexpr explicit hh_mm_ss(_Duration d)
      : __neg_(d < _Duration::zero()), __h_(chrono::duration_cast<chrono::hours>(__abs_of(d))),
        __m_(chrono::duration_cast<chrono::minutes>(__abs_of(d) - __h_)),
        __s_(chrono::duration_cast<chrono::seconds>(__abs_of(d) - __h_ - __m_)), __ss_() {
    if constexpr (treat_as_floating_point_v<typename precision::rep>)
      __ss_ = __abs_of(d) - __h_ - __m_ - __s_;
    else
      __ss_ = chrono::duration_cast<precision>(__abs_of(d) - __h_ - __m_ - __s_);
  }
  constexpr bool is_negative() const noexcept { return __neg_; }
  constexpr chrono::hours hours() const noexcept { return __h_; }
  constexpr chrono::minutes minutes() const noexcept { return __m_; }
  constexpr chrono::seconds seconds() const noexcept { return __s_; }
  constexpr precision subseconds() const noexcept { return __ss_; }
  constexpr explicit operator precision() const noexcept { return to_duration(); }
  constexpr precision to_duration() const noexcept {
    const precision p = __h_ + __m_ + __s_ + __ss_;
    return __neg_ ? -p : p;
  }
};

// [time.12]
constexpr bool is_am(const hours& h) noexcept { return hours(0) <= h && h <= hours(11); }
constexpr bool is_pm(const hours& h) noexcept { return hours(12) <= h && h <= hours(23); }
constexpr hours make12(const hours& h) noexcept {
  if (h == hours(0))
    return hours(12);
  return h > hours(12) ? h - hours(12) : h;
}
constexpr hours make24(const hours& h, bool is_pm) noexcept {
  if (is_pm)
    return h == hours(12) ? h : h + hours(12);
  return h == hours(12) ? hours(0) : h;
}

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] std {
inline namespace literals {
inline namespace chrono_literals {
// [time.cal.day.nonmembers], [time.cal.year.nonmembers]
constexpr chrono::day operator""d(unsigned long long d) noexcept { return chrono::day(static_cast<unsigned>(d)); }
constexpr chrono::year operator""y(unsigned long long y) noexcept { return chrono::year(static_cast<int>(y)); }
} // namespace chrono_literals
} // namespace literals

// [time.hash]/3: enabled for every value, ok() or not.
template <>
struct hash<chrono::day> {
  size_t operator()(const chrono::day& d) const noexcept { return static_cast<unsigned>(d); }
};
template <>
struct hash<chrono::month> {
  size_t operator()(const chrono::month& m) const noexcept { return static_cast<unsigned>(m); }
};
template <>
struct hash<chrono::year> {
  size_t operator()(const chrono::year& y) const noexcept {
    return static_cast<size_t>(static_cast<unsigned short>(static_cast<int>(y)));
  }
};
template <>
struct hash<chrono::weekday> {
  size_t operator()(const chrono::weekday& __w) const noexcept { return __w.c_encoding(); }
};
template <>
struct hash<chrono::weekday_indexed> {
  size_t operator()(const chrono::weekday_indexed& __w) const noexcept {
    return __w.weekday().c_encoding() << 8 | (__w.index() & 0xff);
  }
};
template <>
struct hash<chrono::weekday_last> {
  size_t operator()(const chrono::weekday_last& __w) const noexcept { return __w.weekday().c_encoding(); }
};
template <>
struct hash<chrono::month_day> {
  size_t operator()(const chrono::month_day& __md) const noexcept {
    return static_cast<unsigned>(__md.month()) << 8 | static_cast<unsigned>(__md.day());
  }
};
template <>
struct hash<chrono::month_day_last> {
  size_t operator()(const chrono::month_day_last& __mdl) const noexcept { return static_cast<unsigned>(__mdl.month()); }
};
template <>
struct hash<chrono::month_weekday> {
  size_t operator()(const chrono::month_weekday& __mwd) const noexcept {
    return static_cast<unsigned>(__mwd.month()) << 16 | hash<chrono::weekday_indexed>{}(__mwd.weekday_indexed());
  }
};
template <>
struct hash<chrono::month_weekday_last> {
  size_t operator()(const chrono::month_weekday_last& __mwdl) const noexcept {
    return static_cast<unsigned>(__mwdl.month()) << 8 | __mwdl.weekday_last().weekday().c_encoding();
  }
};
template <>
struct hash<chrono::year_month> {
  size_t operator()(const chrono::year_month& __ym) const noexcept {
    return hash<chrono::year>{}(__ym.year()) << 8 | static_cast<unsigned>(__ym.month());
  }
};
template <>
struct hash<chrono::year_month_day> {
  size_t operator()(const chrono::year_month_day& ymd) const noexcept {
    return hash<chrono::year>{}(ymd.year()) << 16 | static_cast<unsigned>(ymd.month()) << 8 |
           static_cast<unsigned>(ymd.day());
  }
};
template <>
struct hash<chrono::year_month_day_last> {
  size_t operator()(const chrono::year_month_day_last& __ymdl) const noexcept {
    return hash<chrono::year>{}(__ymdl.year()) << 8 | static_cast<unsigned>(__ymdl.month());
  }
};
template <>
struct hash<chrono::year_month_weekday> {
  size_t operator()(const chrono::year_month_weekday& __ymwd) const noexcept {
    return hash<chrono::year>{}(__ymwd.year()) << 24 | static_cast<unsigned>(__ymwd.month()) << 16 |
           hash<chrono::weekday_indexed>{}(__ymwd.weekday_indexed());
  }
};
template <>
struct hash<chrono::year_month_weekday_last> {
  size_t operator()(const chrono::year_month_weekday_last& __ymwdl) const noexcept {
    return hash<chrono::year>{}(__ymwdl.year()) << 16 | static_cast<unsigned>(__ymwdl.month()) << 8 |
           __ymwdl.weekday().c_encoding();
  }
};

} // namespace std
