// libycxx core: the time-arithmetic part of <chrono> ([time.traits], [time.duration],
// [time.point], [time.duration.literals], [time.duration.alg], the duration and time_point parts
// of [time.hash]), and the clock-independent aliases (sys_time, local_t, local_time, file_time).
//
// The clocks themselves need the OS and are hosted (ycxx/hosted/chrono_clocks.hpp); here they
// are only declared. Calendars, time zones, formatting and parsing are not part of this file.
//
// The integer duration literals (24h, 5ms, ...) are consteval: [time.duration.literals]/3 makes a
// literal whose value does not fit the result type ill-formed, and only an immediate function
// can diagnose that for a literal used to initialize a variable with dynamic initialization.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/ratio.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {
template <class _Rep, class _Period = ratio<1>>
class duration;
template <class _Clock, class _Duration = typename _Clock::duration>
class time_point;
}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_duration = false;
template <class _Rep, class _Period>
inline constexpr bool __is_duration<std::chrono::duration<_Rep, _Period>> = true;
template <class _Tp>
concept __duration_type = __is_duration<_Tp>;

// ratio_divide<P1, P2> is a valid ratio specialization (no overflow): the converting constructor
// of duration is constrained on it ([time.duration.cons]/3).
template <class _P1, class _P2>
concept __ratio_divide_valid =
    _P2::num != 0 && ::__ycxx::__detail::__ratio_mul_values(_P1::num, _P1::den, _P2::num < 0 ? -_P2::den : _P2::den,
                                                     _P2::num < 0 ? -_P2::num : _P2::num)
                        .ok;

template <std::intmax_t _Ap, std::intmax_t _Bp>
inline constexpr std::intmax_t __static_gcd = ::__ycxx::__detail::__ratio_gcd(_Ap, _Bp);

// [time.traits.specializations]/1: gcd(N1, N2) / lcm(D1, D2).
template <class _P1, class _P2>
using __ratio_gcd_t = std::ratio<__static_gcd<_P1::num, _P2::num>, (_P1::den / __static_gcd<_P1::den, _P2::den>) * _P2::den>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [time.traits.specializations]
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
  requires requires { typename common_type_t<_Rep1, _Rep2>; }
struct common_type<chrono::duration<_Rep1, _Period1>, chrono::duration<_Rep2, _Period2>> {
  using type = chrono::duration<common_type_t<_Rep1, _Rep2>,
                                __ycxx::__detail::__ratio_gcd_t<typename _Period1::type, typename _Period2::type>>;
};
template <class _Clock, class _Duration1, class _Duration2>
  requires requires { typename common_type_t<_Duration1, _Duration2>; }
struct common_type<chrono::time_point<_Clock, _Duration1>, chrono::time_point<_Clock, _Duration2>> {
  using type = chrono::time_point<_Clock, common_type_t<_Duration1, _Duration2>>;
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {

// [time.traits.is.fp]
template <class _Rep>
struct treat_as_floating_point : is_floating_point<_Rep> {};
template <class _Rep>
constexpr bool treat_as_floating_point_v = treat_as_floating_point<_Rep>::value;

// [time.traits.duration.values]
template <class _Rep>
struct duration_values {
  static constexpr _Rep zero() noexcept { return _Rep(0); }
  static constexpr _Rep min() noexcept { return numeric_limits<_Rep>::lowest(); }
  static constexpr _Rep max() noexcept { return numeric_limits<_Rep>::max(); }
};

// [time.traits.is.clock]
template <class _Tp>
struct is_clock : bool_constant<requires {
  typename _Tp::rep;
  typename _Tp::period;
  typename _Tp::duration;
  typename _Tp::time_point;
  _Tp::is_steady;
  _Tp::now();
}> {};
template <class _Tp>
constexpr bool is_clock_v = is_clock<_Tp>::value;

// [time.duration.cast]
template <class _ToDuration, class _Rep, class _Period>
  requires __ycxx::__detail::__is_duration<_ToDuration>
constexpr _ToDuration duration_cast(const duration<_Rep, _Period>& d) {
  using __cf = ratio_divide<_Period, typename _ToDuration::period>;
  using __cr = common_type_t<typename _ToDuration::rep, _Rep, intmax_t>;
  using __to_rep = typename _ToDuration::rep;
  if constexpr (__cf::num == 1 && __cf::den == 1)
    return _ToDuration(static_cast<__to_rep>(d.count()));
  else if constexpr (__cf::den == 1)
    return _ToDuration(static_cast<__to_rep>(static_cast<__cr>(d.count()) * static_cast<__cr>(__cf::num)));
  else if constexpr (__cf::num == 1)
    return _ToDuration(static_cast<__to_rep>(static_cast<__cr>(d.count()) / static_cast<__cr>(__cf::den)));
  else
    return _ToDuration(
        static_cast<__to_rep>(static_cast<__cr>(d.count()) * static_cast<__cr>(__cf::num) / static_cast<__cr>(__cf::den)));
}

// [time.duration]
template <class _Rep, class _Period>
class duration {
  static_assert(!__ycxx::__detail::__is_duration<_Rep>, "duration: Rep must not be a duration ([time.duration.general]/2)");
  static_assert(!is_const_v<_Rep> && !is_volatile_v<_Rep>, "duration: Rep must not be cv-qualified");
  static_assert(__ycxx::__detail::__is_ratio<_Period>, "duration: Period must be a specialization of ratio");
  static_assert(_Period::num > 0, "duration: Period must be positive");

public:
  using rep = _Rep;
  using period = typename _Period::type;

private:
  rep __rep_;

public:
  constexpr duration() = default;
  template <class _Rep2>
    requires is_convertible_v<const _Rep2&, rep> &&
             (treat_as_floating_point_v<rep> || !treat_as_floating_point_v<_Rep2>)
  constexpr explicit duration(const _Rep2& r) : __rep_(r) {}
  template <class _Rep2, class _Period2>
    requires is_convertible_v<const _Rep2&, rep> && __ycxx::__detail::__ratio_divide_valid<typename _Period2::type, period> &&
             (treat_as_floating_point_v<rep> ||
              (ratio_divide<typename _Period2::type, period>::den == 1 && !treat_as_floating_point_v<_Rep2>))
  constexpr duration(const duration<_Rep2, _Period2>& d) : __rep_(chrono::duration_cast<duration>(d).count()) {}
  ~duration() = default;
  duration(const duration&) = default;
  duration& operator=(const duration&) = default;

  constexpr rep count() const { return __rep_; }

  constexpr common_type_t<duration> operator+() const { return common_type_t<duration>(*this); }
  constexpr common_type_t<duration> operator-() const { return common_type_t<duration>(-__rep_); }
  constexpr duration& operator++() {
    ++__rep_;
    return *this;
  }
  constexpr duration operator++(int) { return duration(__rep_++); }
  constexpr duration& operator--() {
    --__rep_;
    return *this;
  }
  constexpr duration operator--(int) { return duration(__rep_--); }
  constexpr duration& operator+=(const duration& d) {
    __rep_ += d.count();
    return *this;
  }
  constexpr duration& operator-=(const duration& d) {
    __rep_ -= d.count();
    return *this;
  }
  constexpr duration& operator*=(const rep& __rhs) {
    __rep_ *= __rhs;
    return *this;
  }
  constexpr duration& operator/=(const rep& __rhs) {
    __rep_ /= __rhs;
    return *this;
  }
  // Constrained, so that an explicit instantiation for a floating-point rep is well-formed.
  constexpr duration& operator%=(const rep& __rhs)
    requires requires(rep& r) { r %= __rhs; }
  {
    __rep_ %= __rhs;
    return *this;
  }
  constexpr duration& operator%=(const duration& __rhs)
    requires requires(rep& r) { r %= __rhs.count(); }
  {
    __rep_ %= __rhs.count();
    return *this;
  }

  static constexpr duration zero() noexcept { return duration(duration_values<rep>::zero()); }
  static constexpr duration min() noexcept { return duration(duration_values<rep>::min()); }
  static constexpr duration max() noexcept { return duration(duration_values<rep>::max()); }
};

using nanoseconds = duration<long long, nano>;
using microseconds = duration<long long, micro>;
using milliseconds = duration<long long, milli>;
using seconds = duration<long long>;
using minutes = duration<long long, ratio<60>>;
using hours = duration<long long, ratio<3600>>;
// The calendar durations count in long long too: sys_days + seconds then stays exact for every
// representable date (a 32-bit count overflows when converted to seconds before 1902).
using days = duration<long long, ratio_multiply<ratio<24>, hours::period>>;
using weeks = duration<long long, ratio_multiply<ratio<7>, days::period>>;
using years = duration<long long, ratio_multiply<ratio<146097, 400>, days::period>>;
using months = duration<long long, ratio_divide<years::period, ratio<12>>>;

// [time.duration.nonmember]
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>> operator+(const duration<_Rep1, _Period1>& __lhs,
                                                                                   const duration<_Rep2, _Period2>& __rhs) {
  using __cd = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __cd(__cd(__lhs).count() + __cd(__rhs).count());
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>> operator-(const duration<_Rep1, _Period1>& __lhs,
                                                                                   const duration<_Rep2, _Period2>& __rhs) {
  using __cd = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __cd(__cd(__lhs).count() - __cd(__rhs).count());
}
template <class _Rep1, class _Period, class _Rep2>
  requires is_convertible_v<const _Rep2&, common_type_t<_Rep1, _Rep2>>
constexpr duration<common_type_t<_Rep1, _Rep2>, _Period> operator*(const duration<_Rep1, _Period>& d, const _Rep2& s) {
  using __cd = duration<common_type_t<_Rep1, _Rep2>, _Period>;
  return __cd(__cd(d).count() * s);
}
template <class _Rep1, class _Rep2, class _Period>
  requires is_convertible_v<const _Rep1&, common_type_t<_Rep1, _Rep2>>
constexpr duration<common_type_t<_Rep1, _Rep2>, _Period> operator*(const _Rep1& s, const duration<_Rep2, _Period>& d) {
  return d * s;
}
template <class _Rep1, class _Period, class _Rep2>
  requires(!__ycxx::__detail::__is_duration<_Rep2>) && is_convertible_v<const _Rep2&, common_type_t<_Rep1, _Rep2>>
constexpr duration<common_type_t<_Rep1, _Rep2>, _Period> operator/(const duration<_Rep1, _Period>& d, const _Rep2& s) {
  using __cd = duration<common_type_t<_Rep1, _Rep2>, _Period>;
  return __cd(__cd(d).count() / s);
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr common_type_t<_Rep1, _Rep2> operator/(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  using __cd = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __cd(__lhs).count() / __cd(__rhs).count();
}
template <class _Rep1, class _Period, class _Rep2>
  requires(!__ycxx::__detail::__is_duration<_Rep2>) && is_convertible_v<const _Rep2&, common_type_t<_Rep1, _Rep2>>
constexpr duration<common_type_t<_Rep1, _Rep2>, _Period> operator%(const duration<_Rep1, _Period>& d, const _Rep2& s) {
  using __cd = duration<common_type_t<_Rep1, _Rep2>, _Period>;
  return __cd(__cd(d).count() % s);
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>> operator%(const duration<_Rep1, _Period1>& __lhs,
                                                                                   const duration<_Rep2, _Period2>& __rhs) {
  using __cd = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __cd(__cd(__lhs).count() % __cd(__rhs).count());
}

// [time.duration.comparisons]
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr bool operator==(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  using __ct = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __ct(__lhs).count() == __ct(__rhs).count();
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr bool operator<(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  using __ct = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __ct(__lhs).count() < __ct(__rhs).count();
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr bool operator>(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  return __rhs < __lhs;
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr bool operator<=(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  return !(__rhs < __lhs);
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
constexpr bool operator>=(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  return !(__lhs < __rhs);
}
template <class _Rep1, class _Period1, class _Rep2, class _Period2>
  requires three_way_comparable<typename common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>::rep>
constexpr auto operator<=>(const duration<_Rep1, _Period1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  using __ct = common_type_t<duration<_Rep1, _Period1>, duration<_Rep2, _Period2>>;
  return __ct(__lhs).count() <=> __ct(__rhs).count();
}

// [time.duration.cast]
template <class _ToDuration, class _Rep, class _Period>
  requires __ycxx::__detail::__is_duration<_ToDuration>
constexpr _ToDuration floor(const duration<_Rep, _Period>& d) {
  _ToDuration t = chrono::duration_cast<_ToDuration>(d);
  if (t > d)
    return t - _ToDuration{1};
  return t;
}
template <class _ToDuration, class _Rep, class _Period>
  requires __ycxx::__detail::__is_duration<_ToDuration>
constexpr _ToDuration ceil(const duration<_Rep, _Period>& d) {
  _ToDuration t = chrono::duration_cast<_ToDuration>(d);
  if (t < d)
    return t + _ToDuration{1};
  return t;
}
template <class _ToDuration, class _Rep, class _Period>
  requires __ycxx::__detail::__is_duration<_ToDuration> && (!treat_as_floating_point_v<typename _ToDuration::rep>)
constexpr _ToDuration round(const duration<_Rep, _Period>& d) {
  _ToDuration __t0 = chrono::floor<_ToDuration>(d);
  _ToDuration __t1 = __t0 + _ToDuration{1};
  auto __diff0 = d - __t0;
  auto __diff1 = __t1 - d;
  if (__diff0 == __diff1)
    return (__t0.count() & 1) ? __t1 : __t0;
  return __diff0 < __diff1 ? __t0 : __t1;
}

// [time.duration.alg]
template <class _Rep, class _Period>
  requires numeric_limits<_Rep>::is_signed
constexpr duration<_Rep, _Period> abs(duration<_Rep, _Period> d) {
  return d >= d.zero() ? d : static_cast<duration<_Rep, _Period>>(-d);
}

// [time.point]
template <class _Clock, class _Duration>
class time_point {
  static_assert(__ycxx::__detail::__is_duration<_Duration>, "time_point: Duration must be a specialization of duration");

public:
  using clock = _Clock;
  using duration = _Duration;
  using rep = typename duration::rep;
  using period = typename duration::period;

private:
  duration __d_;

public:
  // noexcept (a permitted strengthening): `noexcept(tai_clock::to_utc(tai_seconds()))` is then
  // true, as the noexcept conversions of [time.clock.tai] lead one to expect.
  constexpr time_point() noexcept(is_nothrow_copy_constructible_v<duration>) : __d_(duration::zero()) {}
  constexpr explicit time_point(const duration& d) : __d_(d) {}
  template <class _Duration2>
    requires is_convertible_v<_Duration2, duration>
  constexpr time_point(const time_point<clock, _Duration2>& t) : __d_(t.time_since_epoch()) {}

  constexpr duration time_since_epoch() const { return __d_; }

  constexpr time_point& operator++() {
    ++__d_;
    return *this;
  }
  constexpr time_point operator++(int) { return time_point{__d_++}; }
  constexpr time_point& operator--() {
    --__d_;
    return *this;
  }
  constexpr time_point operator--(int) { return time_point{__d_--}; }
  constexpr time_point& operator+=(const duration& d) {
    __d_ += d;
    return *this;
  }
  constexpr time_point& operator-=(const duration& d) {
    __d_ -= d;
    return *this;
  }

  static constexpr time_point min() noexcept { return time_point(duration::min()); }
  static constexpr time_point max() noexcept { return time_point(duration::max()); }
};

// [time.point.nonmember]
template <class _Clock, class _Duration1, class _Rep2, class _Period2>
constexpr time_point<_Clock, common_type_t<_Duration1, duration<_Rep2, _Period2>>>
operator+(const time_point<_Clock, _Duration1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  using __ct = time_point<_Clock, common_type_t<_Duration1, duration<_Rep2, _Period2>>>;
  return __ct(__lhs.time_since_epoch() + __rhs);
}
template <class _Rep1, class _Period1, class _Clock, class _Duration2>
constexpr time_point<_Clock, common_type_t<duration<_Rep1, _Period1>, _Duration2>>
operator+(const duration<_Rep1, _Period1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return __rhs + __lhs;
}
template <class _Clock, class _Duration1, class _Rep2, class _Period2>
constexpr time_point<_Clock, common_type_t<_Duration1, duration<_Rep2, _Period2>>>
operator-(const time_point<_Clock, _Duration1>& __lhs, const duration<_Rep2, _Period2>& __rhs) {
  using __ct = time_point<_Clock, common_type_t<_Duration1, duration<_Rep2, _Period2>>>;
  return __ct(__lhs.time_since_epoch() - __rhs);
}
template <class _Clock, class _Duration1, class _Duration2>
constexpr common_type_t<_Duration1, _Duration2> operator-(const time_point<_Clock, _Duration1>& __lhs,
                                                        const time_point<_Clock, _Duration2>& __rhs) {
  return __lhs.time_since_epoch() - __rhs.time_since_epoch();
}

// [time.point.comparisons]
template <class _Clock, class _Duration1, class _Duration2>
constexpr bool operator==(const time_point<_Clock, _Duration1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return __lhs.time_since_epoch() == __rhs.time_since_epoch();
}
template <class _Clock, class _Duration1, class _Duration2>
constexpr bool operator<(const time_point<_Clock, _Duration1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return __lhs.time_since_epoch() < __rhs.time_since_epoch();
}
template <class _Clock, class _Duration1, class _Duration2>
constexpr bool operator>(const time_point<_Clock, _Duration1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return __rhs < __lhs;
}
template <class _Clock, class _Duration1, class _Duration2>
constexpr bool operator<=(const time_point<_Clock, _Duration1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return !(__rhs < __lhs);
}
template <class _Clock, class _Duration1, class _Duration2>
constexpr bool operator>=(const time_point<_Clock, _Duration1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return !(__lhs < __rhs);
}
template <class _Clock, class _Duration1, three_way_comparable_with<_Duration1> _Duration2>
constexpr auto operator<=>(const time_point<_Clock, _Duration1>& __lhs, const time_point<_Clock, _Duration2>& __rhs) {
  return __lhs.time_since_epoch() <=> __rhs.time_since_epoch();
}

// [time.point.cast]
template <class _ToDuration, class _Clock, class _Duration>
  requires __ycxx::__detail::__is_duration<_ToDuration>
constexpr time_point<_Clock, _ToDuration> time_point_cast(const time_point<_Clock, _Duration>& t) {
  return time_point<_Clock, _ToDuration>(chrono::duration_cast<_ToDuration>(t.time_since_epoch()));
}
template <class _ToDuration, class _Clock, class _Duration>
  requires __ycxx::__detail::__is_duration<_ToDuration>
constexpr time_point<_Clock, _ToDuration> floor(const time_point<_Clock, _Duration>& __tp) {
  return time_point<_Clock, _ToDuration>(chrono::floor<_ToDuration>(__tp.time_since_epoch()));
}
template <class _ToDuration, class _Clock, class _Duration>
  requires __ycxx::__detail::__is_duration<_ToDuration>
constexpr time_point<_Clock, _ToDuration> ceil(const time_point<_Clock, _Duration>& __tp) {
  return time_point<_Clock, _ToDuration>(chrono::ceil<_ToDuration>(__tp.time_since_epoch()));
}
template <class _ToDuration, class _Clock, class _Duration>
  requires __ycxx::__detail::__is_duration<_ToDuration> && (!treat_as_floating_point_v<typename _ToDuration::rep>)
constexpr time_point<_Clock, _ToDuration> round(const time_point<_Clock, _Duration>& __tp) {
  return time_point<_Clock, _ToDuration>(chrono::round<_ToDuration>(__tp.time_since_epoch()));
}

// Clocks: declared here, defined in ycxx/hosted/chrono_clocks.hpp.
class system_clock;
class steady_clock;
class high_resolution_clock;

template <class _Duration>
using sys_time = time_point<system_clock, _Duration>;
using sys_seconds = sys_time<seconds>;
using sys_days = sys_time<days>;

// [time.clock.local]
struct local_t {};
template <class _Duration>
using local_time = time_point<local_t, _Duration>;
using local_seconds = local_time<seconds>;
using local_days = local_time<days>;

}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// [time.clock.file]: the type std::chrono::file_clock denotes (defined with the other clocks).
class file_clock;
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace chrono {
using file_clock = __ycxx::__adl_free::file_clock;
template <class _Duration>
using file_time = time_point<file_clock, _Duration>;
}} // namespace std::chrono

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Diagnoses an integer duration literal that overflows its type ([time.duration.literals]/3):
// not constexpr, so reaching it in the immediate literal operators is ill-formed.
inline void __duration_literal_overflows() noexcept {}
template <class _Dp>
consteval _Dp __duration_literal(unsigned long long __v) {
  if (__v > static_cast<unsigned long long>(std::numeric_limits<typename _Dp::rep>::max()))
    ::__ycxx::__detail::__duration_literal_overflows();
  return _Dp(static_cast<typename _Dp::rep>(__v));
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
inline namespace literals {
inline namespace chrono_literals {
// [time.duration.literals]
consteval chrono::hours operator""h(unsigned long long __v) {
  return __ycxx::__detail::__duration_literal<chrono::hours>(__v);
}
constexpr chrono::duration<long double, ratio<3600>> operator""h(long double __v) {
  return chrono::duration<long double, ratio<3600>>(__v);
}
consteval chrono::minutes operator""min(unsigned long long __v) {
  return __ycxx::__detail::__duration_literal<chrono::minutes>(__v);
}
constexpr chrono::duration<long double, ratio<60>> operator""min(long double __v) {
  return chrono::duration<long double, ratio<60>>(__v);
}
consteval chrono::seconds operator""s(unsigned long long __v) {
  return __ycxx::__detail::__duration_literal<chrono::seconds>(__v);
}
constexpr chrono::duration<long double> operator""s(long double __v) { return chrono::duration<long double>(__v); }
consteval chrono::milliseconds operator""ms(unsigned long long __v) {
  return __ycxx::__detail::__duration_literal<chrono::milliseconds>(__v);
}
constexpr chrono::duration<long double, milli> operator""ms(long double __v) {
  return chrono::duration<long double, milli>(__v);
}
consteval chrono::microseconds operator""us(unsigned long long __v) {
  return __ycxx::__detail::__duration_literal<chrono::microseconds>(__v);
}
constexpr chrono::duration<long double, micro> operator""us(long double __v) {
  return chrono::duration<long double, micro>(__v);
}
consteval chrono::nanoseconds operator""ns(unsigned long long __v) {
  return __ycxx::__detail::__duration_literal<chrono::nanoseconds>(__v);
}
constexpr chrono::duration<long double, nano> operator""ns(long double __v) {
  return chrono::duration<long double, nano>(__v);
}
} // namespace chrono_literals
} // namespace literals

namespace chrono {
using namespace literals::chrono_literals;
} // namespace chrono

// [time.hash]
template <class _Rep, class _Period>
  requires __ycxx::__detail::__hash_enabled<_Rep>
struct hash<chrono::duration<_Rep, _Period>> {
  size_t operator()(const chrono::duration<_Rep, _Period>& d) const { return hash<_Rep>{}(d.count()); }
};
template <class _Clock, class _Duration>
  requires __ycxx::__detail::__hash_enabled<_Duration>
struct hash<chrono::time_point<_Clock, _Duration>> {
  size_t operator()(const chrono::time_point<_Clock, _Duration>& t) const {
    return hash<_Duration>{}(t.time_since_epoch());
  }
};

} // namespace std
