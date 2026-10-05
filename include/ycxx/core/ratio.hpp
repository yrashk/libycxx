// libycxx core: <ratio> ([ratio]).
//
// The arithmetic is exact: intermediate values are reduced by common factors first and computed
// in 128 bits where the target has them, so a result is ill-formed only when its reduced
// numerator or denominator does not fit in intmax_t ([ratio.arithmetic]/2). Without a 128-bit
// type, an intermediate overflow is diagnosed as well (allowed by the same paragraph).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

consteval std::intmax_t ratio_abs(std::intmax_t x) { return x < 0 ? -x : x; }
consteval std::intmax_t ratio_gcd(std::intmax_t a, std::intmax_t b) {
  a = ycxx::detail::ratio_abs(a);
  b = ycxx::detail::ratio_abs(b);
  while (b != 0) {
    std::intmax_t t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// A reduced fraction; `ok` is false when a value does not fit in intmax_t.
struct ratio_value {
  std::intmax_t num;
  std::intmax_t den;
  bool ok;
};

// a/b + c/d with b, d > 0 and both fractions reduced.
template <class Wide = ycxx::detail::int128>
consteval ratio_value ratio_add_values(std::intmax_t a, std::intmax_t b, std::intmax_t c, std::intmax_t d) {
  const std::intmax_t g = ycxx::detail::ratio_gcd(b, d);
  const std::intmax_t bg = b / g, dg = d / g;
  // a/b + c/d = (a*dg + c*bg) / (bg*d); gcd(a*dg + c*bg, bg*dg) == 1, so only g can be shared.
  if constexpr (cfg::has_int128) {
    using wide = Wide;
    const wide n = wide(a) * dg + wide(c) * bg;
    const wide g2 = n == 0 ? wide(g) : wide(ycxx::detail::ratio_gcd(static_cast<std::intmax_t>(n % g), g));
    const wide num = n / g2, den = wide(bg) * (d / static_cast<std::intmax_t>(g2));
    constexpr wide hi = wide(__INTMAX_MAX__);
    if (num > hi || num < -hi || den > hi) return {0, 1, false};
    return {static_cast<std::intmax_t>(num), static_cast<std::intmax_t>(den), true};
  } else {
    std::intmax_t x, y, n, den;
    if (__builtin_mul_overflow(a, dg, &x) || __builtin_mul_overflow(c, bg, &y) || __builtin_add_overflow(x, y, &n))
      return {0, 1, false};
    const std::intmax_t g2 = n == 0 ? g : ycxx::detail::ratio_gcd(n % g, g);
    if (__builtin_mul_overflow(bg, d / g2, &den) || n / g2 == -__INTMAX_MAX__ - 1) return {0, 1, false};
    return {n / g2, den, true};
  }
}

consteval ratio_value ratio_mul_values(std::intmax_t a, std::intmax_t b, std::intmax_t c, std::intmax_t d) {
  // (a/b) * (c/d) with both reduced: cancel across first, then the product is reduced.
  const std::intmax_t g1 = ycxx::detail::ratio_gcd(a, d), g2 = ycxx::detail::ratio_gcd(c, b);
  if (a == 0 || c == 0) return {0, 1, true};
  std::intmax_t num, den;
  if (__builtin_mul_overflow(a / g1, c / g2, &num) || __builtin_mul_overflow(b / g2, d / g1, &den) ||
      num == -__INTMAX_MAX__ - 1)
    return {0, 1, false};
  return {num, den, true};
}

// sign(a/b - c/d) for b, d > 0, without overflow (continued-fraction comparison).
consteval int ratio_compare(std::intmax_t a, std::intmax_t b, std::intmax_t c, std::intmax_t d) {
  if ((a < 0) != (c < 0)) return a < 0 ? -1 : 1;
  if (a < 0) return ycxx::detail::ratio_compare(-c, d, -a, b);
  // Both non-negative: compare integer parts, then the reciprocals of the remainders.
  for (int flip = 1;; flip = -flip) {
    const std::intmax_t qa = a / b, qc = c / d;
    if (qa != qc) return qa < qc ? -flip : flip;
    const std::intmax_t ra = a % b, rc = c % d;
    if (ra == 0 || rc == 0) return ra == rc ? 0 : (ra == 0 ? -flip : flip);
    // a/b - qa = ra/b; compare b/ra with d/rc in the opposite direction.
    a = b;
    b = ra;
    c = d;
    d = rc;
  }
}

template <class R>
inline constexpr bool is_ratio = false;

template <class R1, class R2>
struct ratio_check {
  static_assert(is_ratio<R1> && is_ratio<R2>,
                "[ratio.general]/2: R1 and R2 must be specializations of std::ratio");
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <intmax_t N, intmax_t D = 1>
class ratio {
  static_assert(D != 0, "[ratio.ratio]/1: the denominator of std::ratio must not be zero");
  static_assert(N != -__INTMAX_MAX__ - 1 && D != -__INTMAX_MAX__ - 1,
                "[ratio.ratio]/1: the absolute values of N and D must be representable by intmax_t");
  static constexpr intmax_t g = D == 0 ? 1 : ycxx::detail::ratio_gcd(N, D == 0 ? 1 : D);

public:
  static constexpr intmax_t num = (D < 0 ? -N : N) / g;
  static constexpr intmax_t den = (D < 0 ? -D : D) / g;
  using type = ratio<num, den>;
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <std::intmax_t N, std::intmax_t D>
inline constexpr bool is_ratio<std::ratio<N, D>> = true;

template <ratio_value V>
struct ratio_result {
  static_assert(V.ok, "[ratio.arithmetic]/2: the result of the std::ratio arithmetic is not representable by intmax_t");
  using type = std::ratio<V.ok ? V.num : 0, V.ok ? V.den : 1>;
};

template <class R1, class R2, bool Negate>
consteval ratio_value ratio_add_of() {
  (void)ratio_check<R1, R2>{};
  return ycxx::detail::ratio_add_values(R1::num, R1::den, Negate ? -R2::num : R2::num, R2::den);
}
template <class R1, class R2>
consteval ratio_value ratio_divide_of() {
  (void)ratio_check<R1, R2>{};
  static_assert(R2::num != 0, "[ratio.arithmetic]: std::ratio_divide by zero");
  if constexpr (R2::num == 0)
    return {0, 1, true};
  else
    return ycxx::detail::ratio_mul_values(R1::num, R1::den, R2::num < 0 ? -R2::den : R2::den,
                                          R2::num < 0 ? -R2::num : R2::num);
}
template <class R1, class R2>
consteval ratio_value ratio_multiply_of() {
  (void)ratio_check<R1, R2>{};
  return ycxx::detail::ratio_mul_values(R1::num, R1::den, R2::num, R2::den);
}
template <class R1, class R2>
consteval int ratio_compare_of() {
  (void)ratio_check<R1, R2>{};
  return ycxx::detail::ratio_compare(R1::num, R1::den, R2::num, R2::den);
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [ratio.arithmetic]
template <class R1, class R2>
using ratio_add = typename ycxx::detail::ratio_result<ycxx::detail::ratio_add_of<R1, R2, false>()>::type;
template <class R1, class R2>
using ratio_subtract = typename ycxx::detail::ratio_result<ycxx::detail::ratio_add_of<R1, R2, true>()>::type;
template <class R1, class R2>
using ratio_multiply = typename ycxx::detail::ratio_result<ycxx::detail::ratio_multiply_of<R1, R2>()>::type;
template <class R1, class R2>
using ratio_divide = typename ycxx::detail::ratio_result<ycxx::detail::ratio_divide_of<R1, R2>()>::type;

// [ratio.comparison]
template <class R1, class R2>
struct ratio_equal : bool_constant<ycxx::detail::ratio_compare_of<R1, R2>() == 0> {};
template <class R1, class R2>
struct ratio_not_equal : bool_constant<ycxx::detail::ratio_compare_of<R1, R2>() != 0> {};
template <class R1, class R2>
struct ratio_less : bool_constant<(ycxx::detail::ratio_compare_of<R1, R2>() < 0)> {};
template <class R1, class R2>
struct ratio_less_equal : bool_constant<(ycxx::detail::ratio_compare_of<R1, R2>() <= 0)> {};
template <class R1, class R2>
struct ratio_greater : bool_constant<(ycxx::detail::ratio_compare_of<R1, R2>() > 0)> {};
template <class R1, class R2>
struct ratio_greater_equal : bool_constant<(ycxx::detail::ratio_compare_of<R1, R2>() >= 0)> {};

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

// [ratio.si]: quecto ... zepto and zetta ... quetta need more than 64 bits, so they are declared
// only where intmax_t is wider (/1); there is no such target among those libycxx supports.
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
