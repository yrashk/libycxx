// libycxx core: <numeric> ([numeric.ops]): accumulate, reduce, inner_product, transform_reduce,
// the scans, adjacent_difference, iota and ranges::iota, gcd, lcm, midpoint, and saturation
// arithmetic ([numeric.sat]).
#pragma once

#include <ycxx/core/algo_results.hpp>
#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[gnu::visibility("hidden")]] std {

// [accumulate]
template <class InputIterator, class T>
constexpr T accumulate(InputIterator first, InputIterator last, T init) {
  for (; first != last; ++first)
    init = std::move(init) + *first;
  return init;
}
template <class InputIterator, class T, class BinaryOperation>
constexpr T accumulate(InputIterator first, InputIterator last, T init, BinaryOperation binary_op) {
  for (; first != last; ++first)
    init = binary_op(std::move(init), *first);
  return init;
}

// [reduce]: GENERALIZED_SUM may group and reorder freely; done left to right here.
template <class InputIterator, class T, class BinaryOperation>
constexpr T reduce(InputIterator first, InputIterator last, T init, BinaryOperation binary_op) {
  for (; first != last; ++first)
    init = binary_op(std::move(init), *first);
  return init;
}
template <class InputIterator, class T>
constexpr T reduce(InputIterator first, InputIterator last, T init) {
  return std::reduce(first, last, std::move(init), plus<>());
}
template <class InputIterator>
constexpr typename iterator_traits<InputIterator>::value_type reduce(InputIterator first, InputIterator last) {
  return std::reduce(first, last, typename iterator_traits<InputIterator>::value_type{});
}

// [inner.product]
template <class InputIterator1, class InputIterator2, class T>
constexpr T inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init) {
  for (; first1 != last1; (void)++first1, (void)++first2)
    init = std::move(init) + (*first1) * (*first2);
  return init;
}
template <class InputIterator1, class InputIterator2, class T, class BinaryOperation1, class BinaryOperation2>
constexpr T inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init,
                          BinaryOperation1 binary_op1, BinaryOperation2 binary_op2) {
  for (; first1 != last1; (void)++first1, (void)++first2)
    init = binary_op1(std::move(init), binary_op2(*first1, *first2));
  return init;
}

// [transform.reduce]
template <class InputIterator1, class InputIterator2, class T, class BinaryOperation1, class BinaryOperation2>
constexpr T transform_reduce(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init,
                             BinaryOperation1 binary_op1, BinaryOperation2 binary_op2) {
  for (; first1 != last1; (void)++first1, (void)++first2)
    init = binary_op1(std::move(init), binary_op2(*first1, *first2));
  return init;
}
template <class InputIterator1, class InputIterator2, class T>
constexpr T transform_reduce(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init) {
  return std::transform_reduce(first1, last1, first2, std::move(init), plus<>(), multiplies<>());
}
template <class InputIterator, class T, class BinaryOperation, class UnaryOperation>
constexpr T transform_reduce(InputIterator first, InputIterator last, T init, BinaryOperation binary_op,
                             UnaryOperation unary_op) {
  for (; first != last; ++first)
    init = binary_op(std::move(init), unary_op(*first));
  return init;
}

// [partial.sum]
template <class InputIterator, class OutputIterator, class BinaryOperation>
constexpr OutputIterator partial_sum(InputIterator first, InputIterator last, OutputIterator result,
                                     BinaryOperation binary_op) {
  if (first == last)
    return result;
  typename iterator_traits<InputIterator>::value_type acc(*first);
  *result = acc;
  while (++first != last) {
    acc = binary_op(std::move(acc), *first);
    *++result = acc;
  }
  return ++result;
}
template <class InputIterator, class OutputIterator>
constexpr OutputIterator partial_sum(InputIterator first, InputIterator last, OutputIterator result) {
  return std::partial_sum(first, last, result, plus<>());
}

// [exclusive.scan], [transform.exclusive.scan]. Each input is read before the output at the
// same position is written, so result may equal first.
template <class InputIterator, class OutputIterator, class T, class BinaryOperation, class UnaryOperation>
constexpr OutputIterator transform_exclusive_scan(InputIterator first, InputIterator last, OutputIterator result,
                                                  T init, BinaryOperation binary_op, UnaryOperation unary_op) {
  for (; first != last; (void)++first, (void)++result) {
    T next = binary_op(init, unary_op(*first));
    *result = std::move(init);
    init = std::move(next);
  }
  return result;
}
template <class InputIterator, class OutputIterator, class T, class BinaryOperation>
constexpr OutputIterator exclusive_scan(InputIterator first, InputIterator last, OutputIterator result, T init,
                                        BinaryOperation binary_op) {
  for (; first != last; (void)++first, (void)++result) {
    T next = binary_op(init, *first);
    *result = std::move(init);
    init = std::move(next);
  }
  return result;
}
template <class InputIterator, class OutputIterator, class T>
constexpr OutputIterator exclusive_scan(InputIterator first, InputIterator last, OutputIterator result, T init) {
  return std::exclusive_scan(first, last, result, std::move(init), plus<>());
}

// [inclusive.scan], [transform.inclusive.scan]
template <class InputIterator, class OutputIterator, class BinaryOperation, class UnaryOperation, class T>
constexpr OutputIterator transform_inclusive_scan(InputIterator first, InputIterator last, OutputIterator result,
                                                  BinaryOperation binary_op, UnaryOperation unary_op, T init) {
  for (; first != last; (void)++first, (void)++result) {
    init = binary_op(std::move(init), unary_op(*first));
    *result = init;
  }
  return result;
}
template <class InputIterator, class OutputIterator, class BinaryOperation, class UnaryOperation>
constexpr OutputIterator transform_inclusive_scan(InputIterator first, InputIterator last, OutputIterator result,
                                                  BinaryOperation binary_op, UnaryOperation unary_op) {
  if (first == last)
    return result;
  typename iterator_traits<InputIterator>::value_type acc(unary_op(*first));
  *result = acc;
  ++result;
  return std::transform_inclusive_scan(++first, last, result, binary_op, unary_op, std::move(acc));
}
template <class InputIterator, class OutputIterator, class BinaryOperation, class T>
constexpr OutputIterator inclusive_scan(InputIterator first, InputIterator last, OutputIterator result,
                                        BinaryOperation binary_op, T init) {
  for (; first != last; (void)++first, (void)++result) {
    init = binary_op(std::move(init), *first);
    *result = init;
  }
  return result;
}
template <class InputIterator, class OutputIterator, class BinaryOperation>
constexpr OutputIterator inclusive_scan(InputIterator first, InputIterator last, OutputIterator result,
                                        BinaryOperation binary_op) {
  if (first == last)
    return result;
  typename iterator_traits<InputIterator>::value_type acc(*first);
  *result = acc;
  ++result;
  return std::inclusive_scan(++first, last, result, binary_op, std::move(acc));
}
template <class InputIterator, class OutputIterator>
constexpr OutputIterator inclusive_scan(InputIterator first, InputIterator last, OutputIterator result) {
  return std::inclusive_scan(first, last, result, plus<>());
}

// [adjacent.difference]
template <class InputIterator, class OutputIterator, class BinaryOperation>
constexpr OutputIterator adjacent_difference(InputIterator first, InputIterator last, OutputIterator result,
                                             BinaryOperation binary_op) {
  if (first == last)
    return result;
  using T = typename iterator_traits<InputIterator>::value_type;
  T acc(*first);
  *result = acc;
  while (++first != last) {
    T val(*first);
    *++result = binary_op(val, std::move(acc));
    acc = std::move(val);
  }
  return ++result;
}
template <class InputIterator, class OutputIterator>
constexpr OutputIterator adjacent_difference(InputIterator first, InputIterator last, OutputIterator result) {
  return std::adjacent_difference(first, last, result, minus<>());
}

// [numeric.iota]
template <class ForwardIterator, class T>
constexpr void iota(ForwardIterator first, ForwardIterator last, T value) {
  for (; first != last; ++first) {
    *first = value;
    ++value;
  }
}

namespace ranges {
template <class O, class T>
using iota_result = out_value_result<O, T>;
} // namespace ranges

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::ranges_algo {
struct iota_fn {
  template <std::input_or_output_iterator O, std::sentinel_for<O> S, std::weakly_incrementable T>
    requires std::indirectly_writable<O, const T&>
  constexpr std::ranges::iota_result<O, T> operator()(O first, S last, T value) const {
    for (; first != last; ++first) {
      *first = static_cast<const T&>(value);
      ++value;
    }
    return {std::move(first), std::move(value)};
  }
  template <std::weakly_incrementable T, std::ranges::output_range<const T&> R>
  constexpr std::ranges::iota_result<std::ranges::borrowed_iterator_t<R>, T> operator()(R&& r, T value) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(value));
  }
};
}} // namespace ycxx::detail::ranges_algo

namespace [[gnu::visibility("hidden")]] std { namespace ranges {
inline constexpr ycxx::detail::ranges_algo::iota_fn iota{};
}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
concept gcd_integer = std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;

// |v| as the unsigned type U, computed in v's own type: |v| is representable in the common
// type even when v is negative and the common type unsigned.
template <class U, class T>
constexpr U unsigned_abs(T v) noexcept {
  if constexpr (std::is_signed_v<T>) {
    using UT = std::make_unsigned_t<T>;
    return static_cast<U>(v < 0 ? static_cast<UT>(UT(0) - static_cast<UT>(v)) : static_cast<UT>(v));
  } else {
    return static_cast<U>(v);
  }
}
template <class U>
constexpr U gcd_unsigned(U a, U b) noexcept {
  while (b != 0) {
    U t = static_cast<U>(a % b);
    a = b;
    b = t;
  }
  return a;
}

// [numeric.sat]: "a signed or unsigned integer type" (cv-qualified types are not).
template <class T>
concept sat_integer = cmp_integer<T> && std::same_as<T, std::remove_cv_t<T>>;

template <class T>
concept midpoint_arithmetic = std::is_arithmetic_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [numeric.ops.gcd], [numeric.ops.lcm]
// Both are noexcept (a strengthening): a violated precondition is undefined, not an exception.
template <class M, class N>
constexpr common_type_t<M, N> gcd(M m, N n) noexcept {
  static_assert(ycxx::detail::gcd_integer<M> && ycxx::detail::gcd_integer<N>,
                "std::gcd: M and N must be integer types other than bool");
  using C = common_type_t<M, N>;
  using U = make_unsigned_t<C>;
  U a = ycxx::detail::unsigned_abs<U>(m);
  U b = ycxx::detail::unsigned_abs<U>(n);
  ycxx::detail::precondition(a <= U(ycxx::detail::int_max<C>()) && b <= U(ycxx::detail::int_max<C>()),
                             "std::gcd: |m| or |n| is not representable in the common type");
  return static_cast<C>(::ycxx::detail::gcd_unsigned(a, b));
}
template <class M, class N>
constexpr common_type_t<M, N> lcm(M m, N n) noexcept {
  static_assert(ycxx::detail::gcd_integer<M> && ycxx::detail::gcd_integer<N>,
                "std::lcm: M and N must be integer types other than bool");
  using C = common_type_t<M, N>;
  using U = make_unsigned_t<C>;
  U a = ycxx::detail::unsigned_abs<U>(m);
  U b = ycxx::detail::unsigned_abs<U>(n);
  ycxx::detail::precondition(a <= U(ycxx::detail::int_max<C>()) && b <= U(ycxx::detail::int_max<C>()),
                             "std::lcm: |m| or |n| is not representable in the common type");
  if (a == 0 || b == 0)
    return 0;
  U r;
  bool overflow = __builtin_mul_overflow(static_cast<U>(a / ::ycxx::detail::gcd_unsigned(a, b)), b, &r);
  ycxx::detail::precondition(!overflow && r <= U(ycxx::detail::int_max<C>()),
                             "std::lcm: the result is not representable in the common type");
  return static_cast<C>(r);
}

// [numeric.ops.midpoint]
template <class T>
  requires ycxx::detail::midpoint_arithmetic<T>
constexpr T midpoint(T a, T b) noexcept {
  if constexpr (is_integral_v<T>) {
    using U = make_unsigned_t<T>;
    // The distance as an unsigned value, halved towards a.
    if (a > b)
      return static_cast<T>(a - static_cast<T>(static_cast<U>(static_cast<U>(a) - static_cast<U>(b)) / 2));
    return static_cast<T>(a + static_cast<T>(static_cast<U>(static_cast<U>(b) - static_cast<U>(a)) / 2));
  } else {
    // At most one inexact operation: halve the sum unless it could overflow; halve an operand
    // first only when it is too large, and the other one only when it is not tiny.
    constexpr T lo = numeric_limits<T>::min() * 2;
    constexpr T hi = numeric_limits<T>::max() / 2;
    const T abs_a = a < 0 ? -a : a;
    const T abs_b = b < 0 ? -b : b;
    if (abs_a <= hi && abs_b <= hi)
      return (a + b) / 2;
    if (abs_a < lo)
      return a + b / 2;
    if (abs_b < lo)
      return a / 2 + b;
    return a / 2 + b / 2;
  }
}
template <class T>
  requires is_object_v<T>
constexpr T* midpoint(T* a, T* b) noexcept {
  static_assert(sizeof(T) != 0, "std::midpoint: T must be a complete type");
  return a + (b - a) / 2;
}

// [numeric.sat.func]
template <ycxx::detail::sat_integer T>
constexpr T saturating_add(T x, T y) noexcept {
  T r;
  if (!__builtin_add_overflow(x, y, &r))
    return r;
  // Overflow only happens with y on the side of the bound that was crossed.
  if constexpr (is_signed_v<T>)
    return y < 0 ? ycxx::detail::int_min<T>() : ycxx::detail::int_max<T>();
  else
    return ycxx::detail::int_max<T>();
}
template <ycxx::detail::sat_integer T>
constexpr T saturating_sub(T x, T y) noexcept {
  T r;
  if (!__builtin_sub_overflow(x, y, &r))
    return r;
  if constexpr (is_signed_v<T>)
    return y < 0 ? ycxx::detail::int_max<T>() : ycxx::detail::int_min<T>();
  else
    return T(0);
}
template <ycxx::detail::sat_integer T>
constexpr T saturating_mul(T x, T y) noexcept {
  T r;
  if (!__builtin_mul_overflow(x, y, &r))
    return r;
  if constexpr (is_signed_v<T>)
    return (x < 0) != (y < 0) ? ycxx::detail::int_min<T>() : ycxx::detail::int_max<T>();
  else
    return ycxx::detail::int_max<T>();
}
template <ycxx::detail::sat_integer T>
constexpr T saturating_div(T x, T y) noexcept {
  ycxx::detail::precondition(y != 0, "std::saturating_div: division by zero");
  if constexpr (is_signed_v<T>) {
    if (x == ycxx::detail::int_min<T>() && y == T(-1))
      return ycxx::detail::int_max<T>();
  }
  return static_cast<T>(x / y);
}

// [numeric.sat.cast]
template <ycxx::detail::sat_integer R, ycxx::detail::sat_integer T>
constexpr R saturating_cast(T x) noexcept {
  if (std::cmp_less(x, ycxx::detail::int_min<R>()))
    return ycxx::detail::int_min<R>();
  if (std::cmp_greater(x, ycxx::detail::int_max<R>()))
    return ycxx::detail::int_max<R>();
  return static_cast<R>(x);
}

} // namespace std
