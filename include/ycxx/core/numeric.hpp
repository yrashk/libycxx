// libycxx core: <numeric> ([numeric.ops]): accumulate, reduce, inner_product, transform_reduce,
// the scans, adjacent_difference, iota and ranges::iota, gcd, lcm, midpoint, and saturation
// arithmetic ([numeric.sat]).
#pragma once

#include <ycxx/core/algo_results.hpp>
#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [accumulate]
template <class _InputIterator, class _Tp>
constexpr _Tp accumulate(_InputIterator first, _InputIterator last, _Tp init) {
  for (; first != last; ++first)
    init = std::move(init) + *first;
  return init;
}
template <class _InputIterator, class _Tp, class _BinaryOperation>
constexpr _Tp accumulate(_InputIterator first, _InputIterator last, _Tp init, _BinaryOperation __binary_op) {
  for (; first != last; ++first)
    init = __binary_op(std::move(init), *first);
  return init;
}

// [reduce]: GENERALIZED_SUM may group and reorder freely; done left to right here.
template <class _InputIterator, class _Tp, class _BinaryOperation>
constexpr _Tp reduce(_InputIterator first, _InputIterator last, _Tp init, _BinaryOperation __binary_op) {
  for (; first != last; ++first)
    init = __binary_op(std::move(init), *first);
  return init;
}
template <class _InputIterator, class _Tp>
constexpr _Tp reduce(_InputIterator first, _InputIterator last, _Tp init) {
  return std::reduce(first, last, std::move(init), plus<>());
}
template <class _InputIterator>
constexpr typename iterator_traits<_InputIterator>::value_type reduce(_InputIterator first, _InputIterator last) {
  return std::reduce(first, last, typename iterator_traits<_InputIterator>::value_type{});
}

// [inner.product]
template <class _InputIterator1, class _InputIterator2, class _Tp>
constexpr _Tp inner_product(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2, _Tp init) {
  for (; __first1 != __last1; (void)++__first1, (void)++__first2)
    init = std::move(init) + (*__first1) * (*__first2);
  return init;
}
template <class _InputIterator1, class _InputIterator2, class _Tp, class _BinaryOperation1, class _BinaryOperation2>
constexpr _Tp inner_product(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2, _Tp init,
                          _BinaryOperation1 __binary_op1, _BinaryOperation2 __binary_op2) {
  for (; __first1 != __last1; (void)++__first1, (void)++__first2)
    init = __binary_op1(std::move(init), __binary_op2(*__first1, *__first2));
  return init;
}

// [transform.reduce]
template <class _InputIterator1, class _InputIterator2, class _Tp, class _BinaryOperation1, class _BinaryOperation2>
constexpr _Tp transform_reduce(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2, _Tp init,
                             _BinaryOperation1 __binary_op1, _BinaryOperation2 __binary_op2) {
  for (; __first1 != __last1; (void)++__first1, (void)++__first2)
    init = __binary_op1(std::move(init), __binary_op2(*__first1, *__first2));
  return init;
}
template <class _InputIterator1, class _InputIterator2, class _Tp>
constexpr _Tp transform_reduce(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2, _Tp init) {
  return std::transform_reduce(__first1, __last1, __first2, std::move(init), plus<>(), multiplies<>());
}
template <class _InputIterator, class _Tp, class _BinaryOperation, class _UnaryOperation>
constexpr _Tp transform_reduce(_InputIterator first, _InputIterator last, _Tp init, _BinaryOperation __binary_op,
                             _UnaryOperation __unary_op) {
  for (; first != last; ++first)
    init = __binary_op(std::move(init), __unary_op(*first));
  return init;
}

// [partial.sum]
template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
constexpr _OutputIterator partial_sum(_InputIterator first, _InputIterator last, _OutputIterator result,
                                     _BinaryOperation __binary_op) {
  if (first == last)
    return result;
  typename iterator_traits<_InputIterator>::value_type __acc(*first);
  *result = __acc;
  while (++first != last) {
    __acc = __binary_op(std::move(__acc), *first);
    *++result = __acc;
  }
  return ++result;
}
template <class _InputIterator, class _OutputIterator>
constexpr _OutputIterator partial_sum(_InputIterator first, _InputIterator last, _OutputIterator result) {
  return std::partial_sum(first, last, result, plus<>());
}

// [exclusive.scan], [transform.exclusive.scan]. Each input is read before the output at the
// same position is written, so result may equal first.
template <class _InputIterator, class _OutputIterator, class _Tp, class _BinaryOperation, class _UnaryOperation>
constexpr _OutputIterator transform_exclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result,
                                                  _Tp init, _BinaryOperation __binary_op, _UnaryOperation __unary_op) {
  for (; first != last; (void)++first, (void)++result) {
    _Tp next = __binary_op(init, __unary_op(*first));
    *result = std::move(init);
    init = std::move(next);
  }
  return result;
}
template <class _InputIterator, class _OutputIterator, class _Tp, class _BinaryOperation>
constexpr _OutputIterator exclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result, _Tp init,
                                        _BinaryOperation __binary_op) {
  for (; first != last; (void)++first, (void)++result) {
    _Tp next = __binary_op(init, *first);
    *result = std::move(init);
    init = std::move(next);
  }
  return result;
}
template <class _InputIterator, class _OutputIterator, class _Tp>
constexpr _OutputIterator exclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result, _Tp init) {
  return std::exclusive_scan(first, last, result, std::move(init), plus<>());
}

// [inclusive.scan], [transform.inclusive.scan]
template <class _InputIterator, class _OutputIterator, class _BinaryOperation, class _UnaryOperation, class _Tp>
constexpr _OutputIterator transform_inclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result,
                                                  _BinaryOperation __binary_op, _UnaryOperation __unary_op, _Tp init) {
  for (; first != last; (void)++first, (void)++result) {
    init = __binary_op(std::move(init), __unary_op(*first));
    *result = init;
  }
  return result;
}
template <class _InputIterator, class _OutputIterator, class _BinaryOperation, class _UnaryOperation>
constexpr _OutputIterator transform_inclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result,
                                                  _BinaryOperation __binary_op, _UnaryOperation __unary_op) {
  if (first == last)
    return result;
  typename iterator_traits<_InputIterator>::value_type __acc(__unary_op(*first));
  *result = __acc;
  ++result;
  return std::transform_inclusive_scan(++first, last, result, __binary_op, __unary_op, std::move(__acc));
}
template <class _InputIterator, class _OutputIterator, class _BinaryOperation, class _Tp>
constexpr _OutputIterator inclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result,
                                        _BinaryOperation __binary_op, _Tp init) {
  for (; first != last; (void)++first, (void)++result) {
    init = __binary_op(std::move(init), *first);
    *result = init;
  }
  return result;
}
template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
constexpr _OutputIterator inclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result,
                                        _BinaryOperation __binary_op) {
  if (first == last)
    return result;
  typename iterator_traits<_InputIterator>::value_type __acc(*first);
  *result = __acc;
  ++result;
  return std::inclusive_scan(++first, last, result, __binary_op, std::move(__acc));
}
template <class _InputIterator, class _OutputIterator>
constexpr _OutputIterator inclusive_scan(_InputIterator first, _InputIterator last, _OutputIterator result) {
  return std::inclusive_scan(first, last, result, plus<>());
}

// [adjacent.difference]
template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
constexpr _OutputIterator adjacent_difference(_InputIterator first, _InputIterator last, _OutputIterator result,
                                             _BinaryOperation __binary_op) {
  if (first == last)
    return result;
  using _Tp = typename iterator_traits<_InputIterator>::value_type;
  _Tp __acc(*first);
  *result = __acc;
  while (++first != last) {
    _Tp __val(*first);
    *++result = __binary_op(__val, std::move(__acc));
    __acc = std::move(__val);
  }
  return ++result;
}
template <class _InputIterator, class _OutputIterator>
constexpr _OutputIterator adjacent_difference(_InputIterator first, _InputIterator last, _OutputIterator result) {
  return std::adjacent_difference(first, last, result, minus<>());
}

// [numeric.iota]
template <class _ForwardIterator, class _Tp>
constexpr void iota(_ForwardIterator first, _ForwardIterator last, _Tp value) {
  for (; first != last; ++first) {
    *first = value;
    ++value;
  }
}

namespace ranges {
template <class _Op, class _Tp>
using iota_result = out_value_result<_Op, _Tp>;
} // namespace ranges

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {
struct __iota_fn {
  template <std::input_or_output_iterator _Op, std::sentinel_for<_Op> _Sp, std::weakly_incrementable _Tp>
    requires std::indirectly_writable<_Op, const _Tp&>
  constexpr std::ranges::iota_result<_Op, _Tp> operator()(_Op first, _Sp last, _Tp value) const {
    for (; first != last; ++first) {
      *first = static_cast<const _Tp&>(value);
      ++value;
    }
    return {std::move(first), std::move(value)};
  }
  template <std::weakly_incrementable _Tp, std::ranges::output_range<const _Tp&> _Rp>
  constexpr std::ranges::iota_result<std::ranges::borrowed_iterator_t<_Rp>, _Tp> operator()(_Rp&& r, _Tp value) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(value));
  }
};
}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__detail::__ranges_algo::__iota_fn iota{};
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
concept __gcd_integer = std::is_integral_v<_Tp> && !std::is_same_v<std::remove_cv_t<_Tp>, bool>;

// |v| as the unsigned type U, computed in v's own type: |v| is representable in the common
// type even when v is negative and the common type unsigned.
template <class _Up, class _Tp>
constexpr _Up __unsigned_abs(_Tp __v) noexcept {
  if constexpr (std::is_signed_v<_Tp>) {
    using _UT = std::make_unsigned_t<_Tp>;
    return static_cast<_Up>(__v < 0 ? static_cast<_UT>(_UT(0) - static_cast<_UT>(__v)) : static_cast<_UT>(__v));
  } else {
    return static_cast<_Up>(__v);
  }
}
template <class _Up>
constexpr _Up __gcd_unsigned(_Up a, _Up b) noexcept {
  while (b != 0) {
    _Up t = static_cast<_Up>(a % b);
    a = b;
    b = t;
  }
  return a;
}

// [numeric.sat]: "a signed or unsigned integer type" (cv-qualified types are not).
template <class _Tp>
concept __sat_integer = __cmp_integer<_Tp> && std::same_as<_Tp, std::remove_cv_t<_Tp>>;

template <class _Tp>
concept __midpoint_arithmetic = std::is_arithmetic_v<_Tp> && !std::is_same_v<std::remove_cv_t<_Tp>, bool>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [numeric.ops.gcd], [numeric.ops.lcm]
// Both are noexcept (a strengthening): a violated precondition is undefined, not an exception.
template <class _Mp, class _Np>
constexpr common_type_t<_Mp, _Np> gcd(_Mp m, _Np n) noexcept {
  static_assert(__ycxx::__detail::__gcd_integer<_Mp> && __ycxx::__detail::__gcd_integer<_Np>,
                "std::gcd: M and N must be integer types other than bool");
  using _Cp = common_type_t<_Mp, _Np>;
  using _Up = make_unsigned_t<_Cp>;
  _Up a = __ycxx::__detail::__unsigned_abs<_Up>(m);
  _Up b = __ycxx::__detail::__unsigned_abs<_Up>(n);
  __ycxx::__detail::__precondition(a <= _Up(__ycxx::__detail::__int_max<_Cp>()) && b <= _Up(__ycxx::__detail::__int_max<_Cp>()),
                             "std::gcd: |m| or |n| is not representable in the common type");
  return static_cast<_Cp>(::__ycxx::__detail::__gcd_unsigned(a, b));
}
template <class _Mp, class _Np>
constexpr common_type_t<_Mp, _Np> lcm(_Mp m, _Np n) noexcept {
  static_assert(__ycxx::__detail::__gcd_integer<_Mp> && __ycxx::__detail::__gcd_integer<_Np>,
                "std::lcm: M and N must be integer types other than bool");
  using _Cp = common_type_t<_Mp, _Np>;
  using _Up = make_unsigned_t<_Cp>;
  _Up a = __ycxx::__detail::__unsigned_abs<_Up>(m);
  _Up b = __ycxx::__detail::__unsigned_abs<_Up>(n);
  __ycxx::__detail::__precondition(a <= _Up(__ycxx::__detail::__int_max<_Cp>()) && b <= _Up(__ycxx::__detail::__int_max<_Cp>()),
                             "std::lcm: |m| or |n| is not representable in the common type");
  if (a == 0 || b == 0)
    return 0;
  _Up r;
  bool overflow = __builtin_mul_overflow(static_cast<_Up>(a / ::__ycxx::__detail::__gcd_unsigned(a, b)), b, &r);
  __ycxx::__detail::__precondition(!overflow && r <= _Up(__ycxx::__detail::__int_max<_Cp>()),
                             "std::lcm: the result is not representable in the common type");
  return static_cast<_Cp>(r);
}

// [numeric.ops.midpoint]
template <class _Tp>
  requires __ycxx::__detail::__midpoint_arithmetic<_Tp>
constexpr _Tp midpoint(_Tp a, _Tp b) noexcept {
  if constexpr (is_integral_v<_Tp>) {
    using _Up = make_unsigned_t<_Tp>;
    // The distance as an unsigned value, halved towards a.
    if (a > b)
      return static_cast<_Tp>(a - static_cast<_Tp>(static_cast<_Up>(static_cast<_Up>(a) - static_cast<_Up>(b)) / 2));
    return static_cast<_Tp>(a + static_cast<_Tp>(static_cast<_Up>(static_cast<_Up>(b) - static_cast<_Up>(a)) / 2));
  } else {
    // At most one inexact operation: halve the sum unless it could overflow; halve an operand
    // first only when it is too large, and the other one only when it is not tiny.
    constexpr _Tp __lo = numeric_limits<_Tp>::min() * 2;
    constexpr _Tp __hi = numeric_limits<_Tp>::max() / 2;
    const _Tp __abs_a = a < 0 ? -a : a;
    const _Tp __abs_b = b < 0 ? -b : b;
    if (__abs_a <= __hi && __abs_b <= __hi)
      return (a + b) / 2;
    if (__abs_a < __lo)
      return a + b / 2;
    if (__abs_b < __lo)
      return a / 2 + b;
    return a / 2 + b / 2;
  }
}
template <class _Tp>
  requires is_object_v<_Tp>
constexpr _Tp* midpoint(_Tp* a, _Tp* b) noexcept {
  static_assert(sizeof(_Tp) != 0, "std::midpoint: T must be a complete type");
  return a + (b - a) / 2;
}

// [numeric.sat.func]
template <__ycxx::__detail::__sat_integer _Tp>
constexpr _Tp saturating_add(_Tp __x, _Tp y) noexcept {
  _Tp r;
  if (!__builtin_add_overflow(__x, y, &r))
    return r;
  // Overflow only happens with y on the side of the bound that was crossed.
  if constexpr (is_signed_v<_Tp>)
    return y < 0 ? __ycxx::__detail::__int_min<_Tp>() : __ycxx::__detail::__int_max<_Tp>();
  else
    return __ycxx::__detail::__int_max<_Tp>();
}
template <__ycxx::__detail::__sat_integer _Tp>
constexpr _Tp saturating_sub(_Tp __x, _Tp y) noexcept {
  _Tp r;
  if (!__builtin_sub_overflow(__x, y, &r))
    return r;
  if constexpr (is_signed_v<_Tp>)
    return y < 0 ? __ycxx::__detail::__int_max<_Tp>() : __ycxx::__detail::__int_min<_Tp>();
  else
    return _Tp(0);
}
template <__ycxx::__detail::__sat_integer _Tp>
constexpr _Tp saturating_mul(_Tp __x, _Tp y) noexcept {
  _Tp r;
  if (!__builtin_mul_overflow(__x, y, &r))
    return r;
  if constexpr (is_signed_v<_Tp>)
    return (__x < 0) != (y < 0) ? __ycxx::__detail::__int_min<_Tp>() : __ycxx::__detail::__int_max<_Tp>();
  else
    return __ycxx::__detail::__int_max<_Tp>();
}
template <__ycxx::__detail::__sat_integer _Tp>
constexpr _Tp saturating_div(_Tp __x, _Tp y) noexcept {
  __ycxx::__detail::__precondition(y != 0, "std::saturating_div: division by zero");
  if constexpr (is_signed_v<_Tp>) {
    if (__x == __ycxx::__detail::__int_min<_Tp>() && y == _Tp(-1))
      return __ycxx::__detail::__int_max<_Tp>();
  }
  return static_cast<_Tp>(__x / y);
}

// [numeric.sat.cast]
template <__ycxx::__detail::__sat_integer _Rp, __ycxx::__detail::__sat_integer _Tp>
constexpr _Rp saturating_cast(_Tp __x) noexcept {
  if (std::cmp_less(__x, __ycxx::__detail::__int_min<_Rp>()))
    return __ycxx::__detail::__int_min<_Rp>();
  if (std::cmp_greater(__x, __ycxx::__detail::__int_max<_Rp>()))
    return __ycxx::__detail::__int_max<_Rp>();
  return static_cast<_Rp>(__x);
}

} // namespace std
