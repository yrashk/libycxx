// libycxx core: compile-time rational arithmetic ([ratio]).
//
// Arithmetic reduces by the greatest common divisors before multiplying, so a result is
// computed whenever its reduced numerator and denominator fit in intmax_t; an overflow that
// cannot be avoided makes the program ill-formed ([ratio.arithmetic]/2). ratio_less compares by
// continued-fraction expansion, which never overflows.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/type_traits.hpp>

namespace std {
template <intmax_t N, intmax_t D = 1>
class ratio;
} // namespace std

namespace ycxx::detail {

inline constexpr std::intmax_t intmax_min = -__INTMAX_MAX__ - 1;

constexpr std::intmax_t ratio_abs(std::intmax_t v) noexcept { return v < 0 ? -v : v; }

// gcd of |a| and |b| (neither is the most negative value); gcd(0, 0) == 0.
constexpr std::intmax_t ratio_gcd(std::intmax_t a, std::intmax_t b) noexcept {
  a = ::ycxx::detail::ratio_abs(a);
  b = ::ycxx::detail::ratio_abs(b);
  while (b != 0) {
    std::intmax_t t = a % b;
    a = b;
    b = t;
  }
  return a;
}

template <class T>
inline constexpr bool is_ratio = false;
template <std::intmax_t N, std::intmax_t D>
inline constexpr bool is_ratio<std::ratio<N, D>> = true;

// A reduced fraction (den > 0) or the information that it overflows intmax_t.
struct ratio_value {
  std::intmax_t num;
  std::intmax_t den;
  bool ok;
};

constexpr bool ratio_mul_ok(std::intmax_t a, std::intmax_t b, std::intmax_t& r) noexcept {
  return !__builtin_mul_overflow(a, b, &r) && r != intmax_min;
}

// n1/d1 * n2/d2, both reduced with positive denominators.
constexpr ratio_value ratio_mul(std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2) noexcept {
  std::intmax_t g1 = ::ycxx::detail::ratio_gcd(n1, d2);
  std::intmax_t g2 = ::ycxx::detail::ratio_gcd(n2, d1);
  if (g1 == 0)
    g1 = 1;
  if (g2 == 0)
    g2 = 1;
  ratio_value r{0, 1, true};
  if (!::ycxx::detail::ratio_mul_ok(n1 / g1, n2 / g2, r.num) || !::ycxx::detail::ratio_mul_ok(d1 / g2, d2 / g1, r.den))
    r.ok = false;
  if (r.ok && r.num == 0)
    r.den = 1;
  return r;
}

// n1/d1 + n2/d2, both reduced with positive denominators.
constexpr ratio_value ratio_add(std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2) noexcept {
  const std::intmax_t g = ::ycxx::detail::ratio_gcd(d1, d2);
  ratio_value r{0, 1, false};
  std::intmax_t a = 0, b = 0, x = 0;
  if (!::ycxx::detail::ratio_mul_ok(n1, d2 / g, a) || !::ycxx::detail::ratio_mul_ok(n2, d1 / g, b) ||
      __builtin_add_overflow(a, b, &x) || x == intmax_min)
    return r;
  // x / (d1/g * d2): gcd(x, d1/g * d2) == gcd(x, g) because d1/g and d2/g are coprime.
  std::intmax_t g2 = ::ycxx::detail::ratio_gcd(x, g);
  if (g2 == 0)
    g2 = 1;
  if (x == 0) {
    r.ok = true;
    return r;
  }
  r.num = x / g2;
  r.ok = ::ycxx::detail::ratio_mul_ok(d1 / g, d2 / g2, r.den);
  return r;
}

constexpr ratio_value ratio_sub(std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2) noexcept {
  return ::ycxx::detail::ratio_add(n1, d1, -n2, d2);
}

// n1/d1 / (n2/d2), both reduced with positive denominators; ill-formed for n2 == 0.
constexpr ratio_value ratio_div(std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2) noexcept {
  if (n2 == 0)
    return {0, 1, false};
  return ::ycxx::detail::ratio_mul(n1, d1, n2 < 0 ? -d2 : d2, ::ycxx::detail::ratio_abs(n2));
}

// n1/d1 < n2/d2 (positive denominators), by comparing continued-fraction expansions.
constexpr bool ratio_less(std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2) noexcept {
  for (;;) {
    std::intmax_t q1 = n1 / d1, r1 = n1 % d1;
    if (r1 < 0) {
      --q1;
      r1 += d1;
    }
    std::intmax_t q2 = n2 / d2, r2 = n2 % d2;
    if (r2 < 0) {
      --q2;
      r2 += d2;
    }
    if (q1 != q2)
      return q1 < q2;
    if (r1 == 0 || r2 == 0)
      return r1 == 0 && r2 != 0;
    // q + r1/d1 < q + r2/d2  <=>  d2/r2 < d1/r1
    const std::intmax_t old_d1 = d1;
    n1 = d2;
    d1 = r2;
    n2 = old_d1;
    d2 = r1;
  }
}

using ratio_fn = ratio_value (*)(std::intmax_t, std::intmax_t, std::intmax_t, std::intmax_t) noexcept;

// [ratio.general]/2: R1 and R2 must be ratio specializations.
template <class R1, class R2>
struct ratio_args {
  static_assert(is_ratio<R1> && is_ratio<R2>, "std::ratio arithmetic and comparison need std::ratio arguments");
  static constexpr bool ok = true;
};

template <class R1, class R2, ratio_fn F, bool = ratio_args<R1, R2>::ok>
struct ratio_op {
  static constexpr ratio_value v = F(R1::num, R1::den, R2::num, R2::den);
  static_assert(v.ok, "std::ratio arithmetic: the result is not representable as intmax_t");
  using type = std::ratio<v.num, v.den>;
};

template <class R1, class R2, bool = ratio_args<R1, R2>::ok>
inline constexpr bool ratio_less_v = ::ycxx::detail::ratio_less(R1::num, R1::den, R2::num, R2::den);
template <class R1, class R2, bool = ratio_args<R1, R2>::ok>
inline constexpr bool ratio_equal_v = R1::num == R2::num && R1::den == R2::den;

} // namespace ycxx::detail

namespace std {

// [ratio.ratio]
template <intmax_t N, intmax_t D>
class ratio {
  static_assert(D != 0, "std::ratio: the denominator is zero");
  static_assert(N != ycxx::detail::intmax_min && D != ycxx::detail::intmax_min,
                "std::ratio: the absolute value of an argument is not representable as intmax_t");

public:
  static constexpr intmax_t num =
      (N < 0) != (D < 0) ? -(ycxx::detail::ratio_abs(N) / ycxx::detail::ratio_gcd(N, D))
                         : ycxx::detail::ratio_abs(N) / ycxx::detail::ratio_gcd(N, D);
  static constexpr intmax_t den = ycxx::detail::ratio_abs(D) / ycxx::detail::ratio_gcd(N, D);
  using type = ratio<num, den>;
};

// [ratio.arithmetic]
template <class R1, class R2>
using ratio_add = typename ycxx::detail::ratio_op<R1, R2, ycxx::detail::ratio_add>::type;
template <class R1, class R2>
using ratio_subtract = typename ycxx::detail::ratio_op<R1, R2, ycxx::detail::ratio_sub>::type;
template <class R1, class R2>
using ratio_multiply = typename ycxx::detail::ratio_op<R1, R2, ycxx::detail::ratio_mul>::type;
template <class R1, class R2>
using ratio_divide = typename ycxx::detail::ratio_op<R1, R2, ycxx::detail::ratio_div>::type;

// [ratio.comparison]
template <class R1, class R2>
struct ratio_equal : bool_constant<ycxx::detail::ratio_equal_v<R1, R2>> {};
template <class R1, class R2>
struct ratio_not_equal : bool_constant<!ycxx::detail::ratio_equal_v<R1, R2>> {};
template <class R1, class R2>
struct ratio_less : bool_constant<ycxx::detail::ratio_less_v<R1, R2>> {};
template <class R1, class R2>
struct ratio_less_equal : bool_constant<!ycxx::detail::ratio_less_v<R2, R1>> {};
template <class R1, class R2>
struct ratio_greater : bool_constant<ycxx::detail::ratio_less_v<R2, R1>> {};
template <class R1, class R2>
struct ratio_greater_equal : bool_constant<!ycxx::detail::ratio_less_v<R1, R2>> {};
template <class R1, class R2>
constexpr bool ratio_equal_v = ratio_equal<R1, R2>::value;
template <class R1, class R2>
constexpr bool ratio_not_equal_v = ratio_not_equal<R1, R2>::value;
template <class R1, class R2>
constexpr bool ratio_less_v = ratio_less<R1, R2>::value;
template <class R1, class R2>
constexpr bool ratio_less_equal_v = ratio_less_equal<R1, R2>::value;
template <class R1, class R2>
constexpr bool ratio_greater_v = ratio_greater<R1, R2>::value;
template <class R1, class R2>
constexpr bool ratio_greater_equal_v = ratio_greater_equal<R1, R2>::value;

// [ratio.si]: quecto, ronto, yocto, zepto, zetta, yotta, ronna and quetta need more than the
// 64 bits of intmax_t and are not declared.
using atto = ratio<1, 1'000'000'000'000'000'000>;
using femto = ratio<1, 1'000'000'000'000'000>;
using pico = ratio<1, 1'000'000'000'000>;
using nano = ratio<1, 1'000'000'000>;
using micro = ratio<1, 1'000'000>;
using milli = ratio<1, 1'000>;
using centi = ratio<1, 100>;
using deci = ratio<1, 10>;
using deca = ratio<10, 1>;
using hecto = ratio<100, 1>;
using kilo = ratio<1'000, 1>;
using mega = ratio<1'000'000, 1>;
using giga = ratio<1'000'000'000, 1>;
using tera = ratio<1'000'000'000'000, 1>;
using peta = ratio<1'000'000'000'000'000, 1>;
using exa = ratio<1'000'000'000'000'000'000, 1>;

} // namespace std
