// libycxx core: mutating sequence operations ([alg.modifying.operations]) other than those in
// algo_base.hpp: transform, replace, generate, remove, unique, reverse, rotate, shift, sample
// and shuffle.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/urbg.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Ops, class _Ip, class _Sp, class _Pp>
constexpr _Ip __remove_if_impl(_Ip first, _Sp last, _Pp pred) {
  first = ::__ycxx::__detail::__find_if_impl(static_cast<_Ip&&>(first), last, pred);
  if (first == last)
    return first;
  _Ip out = first;
  for (++first; first != last; ++first)
    if (!pred(*first)) {
      *out = _Ops::iter_move(first);
      ++out;
    }
  return out;
}

template <class _Ip, class _Sp, class _Op, class _Pp>
constexpr std::pair<_Ip, _Op> __remove_copy_if_impl(_Ip first, _Sp last, _Op result, _Pp pred) {
  for (; first != last; ++first)
    if (!pred(*first)) {
      *result = *first;
      ++result;
    }
  return {static_cast<_Ip&&>(first), static_cast<_Op&&>(result)};
}

// Keeps the first element of every group of consecutive equivalent elements.
template <class _Ops, class _Ip, class _Sp, class _Pp>
constexpr _Ip __unique_impl(_Ip first, _Sp last, _Pp eq) {
  first = ::__ycxx::__detail::__adjacent_find_impl(static_cast<_Ip&&>(first), last, eq);
  if (first == last)
    return first;
  _Ip out = first; // *out is the last element kept; *++first is known to be its duplicate
  ++first;
  for (++first; first != last; ++first)
    if (!eq(*out, *first))
      *++out = _Ops::iter_move(first);
  return ++out;
}

// unique_copy: Kind 0 compares with the last kept input element (forward input), 1 with the
// last element written (readable output of the same value type), 2 with a stored copy.
template <int _Kind, class _Ip, class _Sp, class _Op, class _Pp>
constexpr std::pair<_Ip, _Op> __unique_copy_impl(_Ip first, _Sp last, _Op result, _Pp eq) {
  if (first == last)
    return {static_cast<_Ip&&>(first), static_cast<_Op&&>(result)};
  if constexpr (_Kind == 0) {
    _Ip __kept = first;
    *result = *first;
    ++result;
    while (++first != last)
      if (!eq(*__kept, *first)) {
        __kept = first;
        *result = *first;
        ++result;
      }
  } else if constexpr (_Kind == 1) {
    _Op __written = result;
    *result = *first;
    ++result;
    while (++first != last)
      if (!eq(*__written, *first)) {
        __written = result;
        *result = *first;
        ++result;
      }
  } else {
    std::iter_value_t<_Ip> __kept(*first);
    *result = __kept;
    ++result;
    while (++first != last) {
      decltype(auto) __x = *first;
      if (!eq(__kept, __x)) {
        __kept = static_cast<decltype(__x)&&>(__x);
        *result = __kept;
        ++result;
      }
    }
  }
  return {static_cast<_Ip&&>(first), static_cast<_Op&&>(result)};
}

template <class _Ops, class _Ip>
constexpr void __reverse_impl(_Ip first, _Ip last) {
  if constexpr (std::random_access_iterator<_Ip> || __ra_iter<_Ip>) {
    if (first == last)
      return;
    for (--last; first < last; (void)++first, (void)--last)
      _Ops::iter_swap(first, last);
  } else {
    while (first != last && first != --last) {
      _Ops::iter_swap(first, last);
      ++first;
    }
  }
}

// Returns first + (last - middle). Bidirectional iterators: three reversals; forward iterators:
// block swapping. Either way at most last - first swaps.
template <class _Ops, class _Ip>
constexpr _Ip __rotate_impl(_Ip first, _Ip __middle, _Ip last) {
  if (first == __middle)
    return last;
  if (__middle == last)
    return first;
  if constexpr (std::bidirectional_iterator<_Ip> || __bidi_iter<_Ip>) {
    _Ip __ret = first;
    ::__ycxx::__detail::__iter_advance(__ret, ::__ycxx::__detail::__range_length(__middle, last));
    ::__ycxx::__detail::__reverse_impl<_Ops>(first, __middle);
    ::__ycxx::__detail::__reverse_impl<_Ops>(__middle, last);
    ::__ycxx::__detail::__reverse_impl<_Ops>(first, last);
    return __ret;
  } else {
    _Ip next = __middle;
    do {
      _Ops::iter_swap(first, next);
      ++first;
      ++next;
      if (first == __middle)
        __middle = next;
    } while (next != last);
    _Ip __ret = first;
    next = __middle;
    while (next != last) {
      _Ops::iter_swap(first, next);
      ++first;
      ++next;
      if (first == __middle)
        __middle = next;
      else if (next == last)
        next = __middle;
    }
    return __ret;
  }
}

// [alg.shift]: returns NEW_LAST.
template <class _Ops, class _Ip, class _Sp>
constexpr _Ip __shift_left_impl(_Ip first, _Sp last, std::iter_difference_t<_Ip> n) {
  if (n <= 0)
    return ::__ycxx::__detail::__iter_at(first, last);
  _Ip __mid = first;
  if constexpr (std::sized_sentinel_for<_Sp, _Ip>) {
    if (n >= last - first)
      return first;
    ::__ycxx::__detail::__iter_advance(__mid, n);
  } else {
    for (; n > 0; --n, (void)++__mid)
      if (__mid == last)
        return first;
  }
  return ::__ycxx::__detail::__move_dispatch<_Ops>(static_cast<_Ip&&>(__mid), last, static_cast<_Ip&&>(first)).second;
}
// Returns {NEW_FIRST, end}.
template <class _Ops, class _Ip, class _Sp>
constexpr std::pair<_Ip, _Ip> __shift_right_impl(_Ip first, _Sp last, std::iter_difference_t<_Ip> n) {
  if (n <= 0)
    return {first, ::__ycxx::__detail::__iter_at(first, last)};
  if constexpr (std::bidirectional_iterator<_Ip> || (std::same_as<_Ip, _Sp> && __bidi_iter<_Ip>)) {
    _Ip end = ::__ycxx::__detail::__iter_at(first, last);
    if (::__ycxx::__detail::__range_length(first, end) <= n)
      return {end, end};
    _Ip __mid = end;
    ::__ycxx::__detail::__iter_advance(__mid, -n);
    ::__ycxx::__detail::__move_backward_dispatch<_Ops>(first, __mid, end);
    ::__ycxx::__detail::__iter_advance(first, n);
    return {first, end};
  } else {
    // Forward iterators: [first, result) is a ring of elements waiting to be placed; each
    // position from result on swaps its element with the oldest waiting one.
    _Ip result = first;
    for (std::iter_difference_t<_Ip> k = n; k > 0; --k, (void)++result)
      if (result == last)
        return {result, result};
    _Ip __slot = first;
    _Ip p = result;
    for (; p != last; ++p) {
      _Ops::iter_swap(p, __slot);
      if (++__slot == result)
        __slot = first;
    }
    return {result, p};
  }
}

// ---- uniform integers from a URBG ([rand.req.urng]) -------------------------------------------
// A uniformly distributed value in [0, n], by rejection; several draws are combined when the
// generator's range is smaller than n + 1.
template <class _Gp>
unsigned long long __uniform_upto(_Gp& __g, unsigned long long n) {
  using _Rp = std::remove_cvref_t<decltype(__g())>;
  using _Gr = std::remove_reference_t<_Gp>;
  // A generator whose range exceeds 64 bits (result_type unsigned __int128) is first reduced to
  // uniform 64-bit values: g() - min() is accepted below the largest multiple of 2^64 that fits
  // in its range, and its low 64 bits are used.
  constexpr bool __wide = static_cast<_Rp>(_Gr::max() - _Gr::min()) > static_cast<_Rp>(~0ull);
  constexpr unsigned long long __gmin = __wide ? 0 : static_cast<unsigned long long>(_Gr::min());
  constexpr unsigned long long range = __wide ? ~0ull : static_cast<unsigned long long>(_Gr::max()) - __gmin;
  auto __draw = [&__g] {
    if constexpr (__wide) {
      constexpr _Rp __wrange = static_cast<_Rp>(_Gr::max() - _Gr::min());
      constexpr _Rp __b64 = static_cast<_Rp>(~0ull) + 1;
      // the accepted values [0, __wlimit]: all of them when the range is a multiple of 2^64
      constexpr _Rp __wlimit = __wrange % __b64 == __b64 - 1 ? __wrange : __wrange / __b64 * __b64 - 1;
      for (;;) {
        _Rp __v = static_cast<_Rp>(static_cast<_Rp>(__g()) - _Gr::min());
        if (__v <= __wlimit)
          return static_cast<unsigned long long>(__v);
      }
    } else {
      return static_cast<unsigned long long>(static_cast<_Rp>(__g())) - __gmin;
    }
  };
  if (n == range)
    return __draw();
  if (n < range) {
    // Accept v below the largest multiple of n + 1 that fits in [0, range + 1).
    const unsigned long long m = n + 1;
    const unsigned long long __excess = range == ~0ull ? (~0ull % m + 1) % m : (range + 1) % m;
    const unsigned long long __limit = range - __excess; // values in [0, limit] are accepted
    for (;;) {
      unsigned long long __v = __draw();
      if (__v <= __limit)
        return __v % m;
    }
  }
  // n > range: v = hi * (range + 1) + lo with hi uniform in [0, n / (range + 1)].
  const unsigned long long base = range + 1;
  for (;;) {
    unsigned long long __hi = ::__ycxx::__detail::__uniform_upto(__g, n / base);
    unsigned long long __lo = __draw();
    if (__hi <= (n - __lo) / base)
      return __hi * base + __lo;
  }
}

}} // namespace __ycxx::__detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [alg.transform]
template <class _InputIterator, class _OutputIterator, class _UnaryOperation>
constexpr _OutputIterator transform(_InputIterator __first1, _InputIterator __last1, _OutputIterator result,
                                   _UnaryOperation op) {
  for (; __first1 != __last1; (void)++__first1, (void)++result)
    *result = op(*__first1);
  return result;
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator, class _BinaryOperation>
constexpr _OutputIterator transform(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                   _OutputIterator result, _BinaryOperation __binary_op) {
  for (; __first1 != __last1; (void)++__first1, (void)++__first2, (void)++result)
    *result = __binary_op(*__first1, *__first2);
  return result;
}

// [alg.replace]
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
constexpr void replace(_ForwardIterator first, _ForwardIterator last, const _Tp& __old_value, const _Tp& __new_value) {
  for (; first != last; ++first)
    if (*first == __old_value)
      *first = __new_value;
}
template <class _ForwardIterator, class _Predicate, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
constexpr void replace_if(_ForwardIterator first, _ForwardIterator last, _Predicate pred, const _Tp& __new_value) {
  for (; first != last; ++first)
    if (pred(*first))
      *first = __new_value;
}
template <class _InputIterator, class _OutputIterator, class _Tp>
constexpr _OutputIterator replace_copy(_InputIterator first, _InputIterator last, _OutputIterator result,
                                      const _Tp& __old_value, const _Tp& __new_value) {
  for (; first != last; (void)++first, (void)++result)
    if (*first == __old_value)
      *result = __new_value;
    else
      *result = *first;
  return result;
}
template <class _InputIterator, class _OutputIterator, class _Predicate,
          class _Tp = typename iterator_traits<_OutputIterator>::value_type>
constexpr _OutputIterator replace_copy_if(_InputIterator first, _InputIterator last, _OutputIterator result,
                                         _Predicate pred, const _Tp& __new_value) {
  for (; first != last; (void)++first, (void)++result)
    if (pred(*first))
      *result = __new_value;
    else
      *result = *first;
  return result;
}

// [alg.generate]
template <class _ForwardIterator, class _Generator>
constexpr void generate(_ForwardIterator first, _ForwardIterator last, _Generator __gen) {
  for (; first != last; ++first)
    *first = __gen();
}
template <class _OutputIterator, class _Size, class _Generator>
constexpr _OutputIterator generate_n(_OutputIterator first, _Size n, _Generator __gen) {
  for (auto count = ::__ycxx::__detail::__integral_count(n); count > 0; --count) {
    *first = __gen();
    ++first;
  }
  return first;
}

// [alg.remove]
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
[[nodiscard]] constexpr _ForwardIterator remove(_ForwardIterator first, _ForwardIterator last, const _Tp& value) {
  return ::__ycxx::__detail::__remove_if_impl<__ycxx::__detail::__classic_ops>(first, last,
                                                                   ::__ycxx::__detail::__equals_value_plain<_Tp>{value});
}
template <class _ForwardIterator, class _Predicate>
[[nodiscard]] constexpr _ForwardIterator remove_if(_ForwardIterator first, _ForwardIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__remove_if_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _InputIterator, class _OutputIterator, class _Tp = typename iterator_traits<_InputIterator>::value_type>
constexpr _OutputIterator remove_copy(_InputIterator first, _InputIterator last, _OutputIterator result, const _Tp& value) {
  return ::__ycxx::__detail::__remove_copy_if_impl(first, last, result, ::__ycxx::__detail::__equals_value_plain<_Tp>{value}).second;
}
template <class _InputIterator, class _OutputIterator, class _Predicate>
constexpr _OutputIterator remove_copy_if(_InputIterator first, _InputIterator last, _OutputIterator result,
                                        _Predicate pred) {
  return ::__ycxx::__detail::__remove_copy_if_impl(first, last, result, ::__ycxx::__detail::__ref_pred(pred)).second;
}

// [alg.unique]
template <class _ForwardIterator, class _BinaryPredicate>
[[nodiscard]] constexpr _ForwardIterator unique(_ForwardIterator first, _ForwardIterator last, _BinaryPredicate pred) {
  return ::__ycxx::__detail::__unique_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _ForwardIterator>
[[nodiscard]] constexpr _ForwardIterator unique(_ForwardIterator first, _ForwardIterator last) {
  return std::unique(first, last, equal_to<>{});
}
template <class _InputIterator, class _OutputIterator, class _BinaryPredicate>
constexpr _OutputIterator unique_copy(_InputIterator first, _InputIterator last, _OutputIterator result,
                                     _BinaryPredicate pred) {
  using _VI = typename iterator_traits<_InputIterator>::value_type;
  constexpr int kind = ::__ycxx::__detail::__fwd_iter<_InputIterator> ? 0
                       : (::__ycxx::__detail::__fwd_iter<_OutputIterator> &&
                          requires { requires is_same_v<typename iterator_traits<_OutputIterator>::value_type, _VI>; })
                           ? 1
                           : 2;
  return ::__ycxx::__detail::__unique_copy_impl<kind>(first, last, result, ::__ycxx::__detail::__ref_pred(pred)).second;
}
template <class _InputIterator, class _OutputIterator>
constexpr _OutputIterator unique_copy(_InputIterator first, _InputIterator last, _OutputIterator result) {
  return std::unique_copy(first, last, result, equal_to<>{});
}

// [alg.reverse]
template <class _BidirectionalIterator>
constexpr void reverse(_BidirectionalIterator first, _BidirectionalIterator last) {
  ::__ycxx::__detail::__reverse_impl<__ycxx::__detail::__classic_ops>(first, last);
}
template <class _BidirectionalIterator, class _OutputIterator>
constexpr _OutputIterator reverse_copy(_BidirectionalIterator first, _BidirectionalIterator last, _OutputIterator result) {
  for (; first != last; ++result)
    *result = *--last;
  return result;
}

// [alg.rotate]
template <class _ForwardIterator>
constexpr _ForwardIterator rotate(_ForwardIterator first, _ForwardIterator __middle, _ForwardIterator last) {
  return ::__ycxx::__detail::__rotate_impl<__ycxx::__detail::__classic_ops>(first, __middle, last);
}
template <class _ForwardIterator, class _OutputIterator>
constexpr _OutputIterator rotate_copy(_ForwardIterator first, _ForwardIterator __middle, _ForwardIterator last,
                                     _OutputIterator result) {
  result = ::__ycxx::__detail::__copy_dispatch(__middle, last, result).second;
  return ::__ycxx::__detail::__copy_dispatch(first, __middle, result).second;
}

// [alg.random.sample]
template <class _PopulationIterator, class _SampleIterator, class _Distance, class _UniformRandomBitGenerator>
_SampleIterator sample(_PopulationIterator first, _PopulationIterator last, _SampleIterator out, _Distance n,
                      _UniformRandomBitGenerator&& __g) {
  using _Dp = common_type_t<_Distance, typename iterator_traits<_PopulationIterator>::difference_type>;
  _Dp __want = static_cast<_Dp>(n);
  if (__want <= 0)
    return out;
  if constexpr (::__ycxx::__detail::__fwd_iter<_PopulationIterator>) {
    // Selection sampling: stable.
    _Dp left = static_cast<_Dp>(::__ycxx::__detail::__range_length(first, last));
    for (; __want > 0 && first != last; (void)++first, --left)
      if (static_cast<_Dp>(::__ycxx::__detail::__uniform_upto(__g, static_cast<unsigned long long>(left - 1))) < __want) {
        *out = *first;
        ++out;
        --__want;
      }
    return out;
  } else {
    // Reservoir sampling into the random-access output.
    _Dp k = 0;
    for (; k < __want && first != last; (void)++first, ++k)
      out[k] = *first;
    for (; first != last; (void)++first, ++k) {
      _Dp __j = static_cast<_Dp>(::__ycxx::__detail::__uniform_upto(__g, static_cast<unsigned long long>(k)));
      if (__j < __want)
        out[__j] = *first;
    }
    return out + (k < __want ? k : __want);
  }
}

// [alg.random.shuffle]
template <class _RandomAccessIterator, class _UniformRandomBitGenerator>
void shuffle(_RandomAccessIterator first, _RandomAccessIterator last, _UniformRandomBitGenerator&& __g) {
  using _Dp = typename iterator_traits<_RandomAccessIterator>::difference_type;
  _Dp n = last - first;
  for (_Dp i = 1; i < n; ++i) {
    _Dp __j = static_cast<_Dp>(::__ycxx::__detail::__uniform_upto(__g, static_cast<unsigned long long>(i)));
    if (__j != i) // no self-swap: it would move an element onto itself
      std::iter_swap(first + i, first + __j);
  }
}

// [alg.shift]
template <class _ForwardIterator>
constexpr _ForwardIterator shift_left(_ForwardIterator first, _ForwardIterator last,
                                     typename iterator_traits<_ForwardIterator>::difference_type n) {
  __ycxx::__detail::__precondition(n >= 0, "std::shift_left: negative n");
  return ::__ycxx::__detail::__shift_left_impl<__ycxx::__detail::__classic_ops>(first, last, n);
}
template <class _ForwardIterator>
constexpr _ForwardIterator shift_right(_ForwardIterator first, _ForwardIterator last,
                                      typename iterator_traits<_ForwardIterator>::difference_type n) {
  __ycxx::__detail::__precondition(n >= 0, "std::shift_right: negative n");
  return ::__ycxx::__detail::__shift_right_impl<__ycxx::__detail::__classic_ops>(first, last, n).first;
}

}} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {
template <class _Ip, class _Op>
using unary_transform_result = in_out_result<_Ip, _Op>;
template <class _I1, class _I2, class _Op>
using binary_transform_result = in_in_out_result<_I1, _I2, _Op>;
template <class _Ip, class _Op>
using replace_copy_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using replace_copy_if_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using remove_copy_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using remove_copy_if_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using unique_copy_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using reverse_copy_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using rotate_copy_result = in_out_result<_Ip, _Op>;
// reverse_copy_truncated_result, rotate_copy_truncated_result: algo_ranges_parallel.hpp.
}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::borrowed_subrange_t;
using std::ranges::iterator_t;

// [alg.transform]
struct __transform_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op, std::copy_constructible _Fp,
            class _Proj = std::identity>
    requires std::indirectly_writable<_Op, std::indirect_result_t<_Fp&, std::projected<_Ip, _Proj>>>
  constexpr std::ranges::unary_transform_result<_Ip, _Op> operator()(_Ip __first1, _Sp __last1, _Op result, _Fp op,
                                                                _Proj proj = {}) const {
    for (; __first1 != __last1; (void)++__first1, (void)++result)
      *result = ::__ycxx::__detail::invoke(op, ::__ycxx::__detail::invoke(proj, *__first1));
    return {std::move(__first1), std::move(result)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op, std::copy_constructible _Fp,
            class _Proj = std::identity>
    requires std::indirectly_writable<_Op, std::indirect_result_t<_Fp&, std::projected<iterator_t<_Rp>, _Proj>>>
  constexpr std::ranges::unary_transform_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result, _Fp op,
                                                                                     _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(op), std::move(proj));
  }
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            std::weakly_incrementable _Op, std::copy_constructible _Fp, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires std::indirectly_writable<_Op, std::indirect_result_t<_Fp&, std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>>>
  constexpr std::ranges::binary_transform_result<_I1, _I2, _Op> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2,
                                                                      _Op result, _Fp __binary_op, _Proj1 __proj1 = {},
                                                                      _Proj2 __proj2 = {}) const {
    for (; __first1 != __last1 && __first2 != __last2; (void)++__first1, (void)++__first2, (void)++result)
      *result = ::__ycxx::__detail::invoke(__binary_op, ::__ycxx::__detail::invoke(__proj1, *__first1),
                                       ::__ycxx::__detail::invoke(__proj2, *__first2));
    return {std::move(__first1), std::move(__first2), std::move(result)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, std::weakly_incrementable _Op,
            std::copy_constructible _Fp, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_writable<_Op, std::indirect_result_t<_Fp&, std::projected<iterator_t<_R1>, _Proj1>,
                                                                std::projected<iterator_t<_R2>, _Proj2>>>
  constexpr std::ranges::binary_transform_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>, _Op>
  operator()(_R1&& __r1, _R2&& __r2, _Op result, _Fp __binary_op, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(result), std::move(__binary_op), std::move(__proj1), std::move(__proj2));
  }
};

// [alg.replace]
struct __replace_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _T1 = std::projected_value_t<_Ip, _Proj>, class _T2 = std::iter_value_t<_Ip>>
    requires std::indirectly_writable<_Ip, const _T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _T1*>
  constexpr _Ip operator()(_Ip first, _Sp last, const _T1& __old_value, const _T2& __new_value, _Proj proj = {}) const {
    for (; first != last; ++first)
      if (::__ycxx::__detail::invoke(proj, *first) == __old_value)
        *first = __new_value;
    return first;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            class _T1 = std::projected_value_t<iterator_t<_Rp>, _Proj>, class _T2 = std::ranges::range_value_t<_Rp>>
    requires std::indirectly_writable<iterator_t<_Rp>, const _T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _T1*>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, const _T1& __old_value, const _T2& __new_value, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), __old_value, __new_value, std::move(proj));
  }
};
struct __replace_if_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity, class _Tp = std::iter_value_t<_Ip>,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires std::indirectly_writable<_Ip, const _Tp&>
  constexpr _Ip operator()(_Ip first, _Sp last, _Pred pred, const _Tp& __new_value, _Proj proj = {}) const {
    for (; first != last; ++first)
      if (::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, *first)))
        *first = __new_value;
    return first;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity, class _Tp = std::ranges::range_value_t<_Rp>,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::indirectly_writable<iterator_t<_Rp>, const _Tp&>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Pred pred, const _Tp& __new_value, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), __new_value, std::move(proj));
  }
};
struct __replace_copy_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Op, class _Proj = std::identity,
            class _T1 = std::projected_value_t<_Ip, _Proj>, class _T2 = std::iter_value_t<_Op>>
    requires std::indirectly_copyable<_Ip, _Op> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _T1*> &&
             std::output_iterator<_Op, const _T2&>
  constexpr std::ranges::replace_copy_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result, const _T1& __old_value,
                                                             const _T2& __new_value, _Proj proj = {}) const {
    for (; first != last; (void)++first, (void)++result)
      if (::__ycxx::__detail::invoke(proj, *first) == __old_value)
        *result = __new_value;
      else
        *result = *first;
    return {std::move(first), std::move(result)};
  }
  template <std::ranges::input_range _Rp, class _Op, class _Proj = std::identity,
            class _T1 = std::projected_value_t<iterator_t<_Rp>, _Proj>, class _T2 = std::iter_value_t<_Op>>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _T1*> &&
             std::output_iterator<_Op, const _T2&>
  constexpr std::ranges::replace_copy_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result,
                                                                                  const _T1& __old_value,
                                                                                  const _T2& __new_value,
                                                                                  _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), __old_value, __new_value, std::move(proj));
  }
};
struct __replace_copy_if_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Op, class _Tp = std::iter_value_t<_Op>,
            class _Proj = std::identity, std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires std::indirectly_copyable<_Ip, _Op> && std::output_iterator<_Op, const _Tp&>
  constexpr std::ranges::replace_copy_if_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result, _Pred pred,
                                                                const _Tp& __new_value, _Proj proj = {}) const {
    for (; first != last; (void)++first, (void)++result)
      if (::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, *first)))
        *result = __new_value;
      else
        *result = *first;
    return {std::move(first), std::move(result)};
  }
  template <std::ranges::input_range _Rp, class _Op, class _Tp = std::iter_value_t<_Op>, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op> && std::output_iterator<_Op, const _Tp&>
  constexpr std::ranges::replace_copy_if_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result, _Pred pred,
                                                                                     const _Tp& __new_value,
                                                                                     _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(pred), __new_value,
                   std::move(proj));
  }
};

// [alg.generate]
struct __generate_fn {
  template <std::input_or_output_iterator _Op, std::sentinel_for<_Op> _Sp, std::copy_constructible _Fp>
    requires std::invocable<_Fp&> && std::indirectly_writable<_Op, std::invoke_result_t<_Fp&>>
  constexpr _Op operator()(_Op first, _Sp last, _Fp __gen) const {
    for (; first != last; ++first)
      *first = ::__ycxx::__detail::invoke(__gen);
    return first;
  }
  template <class _Rp, std::copy_constructible _Fp>
    requires std::invocable<_Fp&> && std::ranges::output_range<_Rp, std::invoke_result_t<_Fp&>>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Fp __gen) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(__gen));
  }
};
struct __generate_n_fn {
  template <std::input_or_output_iterator _Op, std::copy_constructible _Fp>
    requires std::invocable<_Fp&> && std::indirectly_writable<_Op, std::invoke_result_t<_Fp&>>
  constexpr _Op operator()(_Op first, std::iter_difference_t<_Op> n, _Fp __gen) const {
    for (; n > 0; --n) {
      *first = ::__ycxx::__detail::invoke(__gen);
      ++first;
    }
    return first;
  }
};

// [alg.remove]
struct __remove_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__remove_if_impl<__ranges_ops>(first, last, ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj});
    return {std::move(end), ::__ycxx::__detail::__iter_at(first, last)};
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::permutable<iterator_t<_Rp>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct __remove_if_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__remove_if_impl<__ranges_ops>(first, last, ::__ycxx::__detail::__make_pred(pred, proj));
    return {std::move(end), ::__ycxx::__detail::__iter_at(first, last)};
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct __remove_copy_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirectly_copyable<_Ip, _Op> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  constexpr std::ranges::remove_copy_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result, const _Tp& value,
                                                            _Proj proj = {}) const {
    auto r = ::__ycxx::__detail::__remove_copy_if_impl(std::move(first), std::move(last), std::move(result),
                                                 ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj});
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  constexpr std::ranges::remove_copy_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result, const _Tp& value,
                                                                                 _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), value, std::move(proj));
  }
};
struct __remove_copy_if_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires std::indirectly_copyable<_Ip, _Op>
  constexpr std::ranges::remove_copy_if_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result, _Pred pred,
                                                               _Proj proj = {}) const {
    auto r = ::__ycxx::__detail::__remove_copy_if_impl(std::move(first), std::move(last), std::move(result),
                                                 ::__ycxx::__detail::__make_pred(pred, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op>
  constexpr std::ranges::remove_copy_if_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result, _Pred pred,
                                                                                    _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(pred), std::move(proj));
  }
};

// [alg.unique]
struct __unique_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<_Ip, _Proj>> _Cp = std::ranges::equal_to>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, _Cp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__unique_impl<__ranges_ops>(first, last, ::__ycxx::__detail::__make_comp(comp, proj));
    return {std::move(end), ::__ycxx::__detail::__iter_at(first, last)};
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<iterator_t<_Rp>, _Proj>> _Cp = std::ranges::equal_to>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, _Cp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __unique_copy_fn {
  template <class _Ip, class _Op>
  static consteval int kind() {
    if constexpr (std::forward_iterator<_Ip>)
      return 0;
    else if constexpr (requires { requires std::input_iterator<_Op>; } &&
                       requires { requires std::same_as<std::iter_value_t<_Ip>, std::iter_value_t<_Op>>; })
      return 1;
    else
      return 2;
  }
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<_Ip, _Proj>> _Cp = std::ranges::equal_to>
    requires std::indirectly_copyable<_Ip, _Op> &&
             (std::forward_iterator<_Ip> || (std::input_iterator<_Op> && std::same_as<std::iter_value_t<_Ip>, std::iter_value_t<_Op>>) ||
              std::indirectly_copyable_storable<_Ip, _Op>)
  constexpr std::ranges::unique_copy_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result, _Cp comp = {},
                                                            _Proj proj = {}) const {
    auto r = ::__ycxx::__detail::__unique_copy_impl<kind<_Ip, _Op>()>(std::move(first), std::move(last), std::move(result),
                                                          ::__ycxx::__detail::__make_comp(comp, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<iterator_t<_Rp>, _Proj>> _Cp = std::ranges::equal_to>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op> &&
             (std::forward_iterator<iterator_t<_Rp>> ||
              (std::input_iterator<_Op> && std::same_as<std::ranges::range_value_t<_Rp>, std::iter_value_t<_Op>>) ||
              std::indirectly_copyable_storable<iterator_t<_Rp>, _Op>)
  constexpr std::ranges::unique_copy_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result, _Cp comp = {},
                                                                                 _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(comp), std::move(proj));
  }
};

// [alg.reverse]
struct __reverse_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp>
    requires std::permutable<_Ip>
  constexpr _Ip operator()(_Ip first, _Sp last) const {
    _Ip end = ::__ycxx::__detail::__iter_at(std::move(first), last);
    ::__ycxx::__detail::__reverse_impl<__ranges_ops>(first, end);
    return end;
  }
  template <std::ranges::bidirectional_range _Rp>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r));
  }
};
struct __reverse_copy_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<_Ip, _Op>
  constexpr std::ranges::reverse_copy_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, last);
    for (_Ip __it = end; __it != first; ++result)
      *result = *--__it;
    return {std::move(end), std::move(result)};
  }
  template <std::ranges::bidirectional_range _Rp, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op>
  constexpr std::ranges::reverse_copy_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};

// [alg.rotate]
struct __rotate_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Ip __middle, _Sp last) const {
    _Ip end = ::__ycxx::__detail::__iter_at(__middle, std::move(last));
    _Ip r = ::__ycxx::__detail::__rotate_impl<__ranges_ops>(std::move(first), std::move(__middle), end);
    return {std::move(r), std::move(end)};
  }
  template <std::ranges::forward_range _Rp>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, iterator_t<_Rp> __middle) const {
    return (*this)(std::ranges::begin(r), std::move(__middle), std::ranges::end(r));
  }
};
struct __rotate_copy_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<_Ip, _Op>
  constexpr std::ranges::rotate_copy_result<_Ip, _Op> operator()(_Ip first, _Ip __middle, _Sp last, _Op result) const {
    auto r = ::__ycxx::__detail::__copy_dispatch(__middle, std::move(last), std::move(result));
    auto __r2 = ::__ycxx::__detail::__copy_dispatch(std::move(first), std::move(__middle), std::move(r.second));
    return {std::move(r.first), std::move(__r2.second)};
  }
  template <std::ranges::forward_range _Rp, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op>
  constexpr std::ranges::rotate_copy_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, iterator_t<_Rp> __middle,
                                                                                 _Op result) const {
    return (*this)(std::ranges::begin(r), std::move(__middle), std::ranges::end(r), std::move(result));
  }
};

// [alg.shift]
struct __shift_left_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, std::iter_difference_t<_Ip> n) const {
    ::__ycxx::__detail::__precondition(n >= 0, "ranges::shift_left: negative n");
    _Ip end = ::__ycxx::__detail::__shift_left_impl<__ranges_ops>(first, std::move(last), n);
    return {std::move(first), std::move(end)};
  }
  template <std::ranges::forward_range _Rp>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, std::ranges::range_difference_t<_Rp> n) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), n);
  }
};
struct __shift_right_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, std::iter_difference_t<_Ip> n) const {
    ::__ycxx::__detail::__precondition(n >= 0, "ranges::shift_right: negative n");
    auto r = ::__ycxx::__detail::__shift_right_impl<__ranges_ops>(std::move(first), std::move(last), n);
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range _Rp>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, std::ranges::range_difference_t<_Rp> n) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), n);
  }
};

// [alg.random.sample], [alg.random.shuffle]
struct __sample_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op, class _Gen>
    requires(std::forward_iterator<_Ip> || std::random_access_iterator<_Op>) && std::indirectly_copyable<_Ip, _Op> &&
            std::uniform_random_bit_generator<std::remove_reference_t<_Gen>>
  _Op operator()(_Ip first, _Sp last, _Op out, std::iter_difference_t<_Ip> n, _Gen&& __g) const {
    using _Dp = std::iter_difference_t<_Ip>;
    if (n <= 0)
      return out;
    if constexpr (std::forward_iterator<_Ip>) {
      _Dp left = std::ranges::distance(first, last);
      for (; n > 0 && first != last; (void)++first, --left)
        if (static_cast<_Dp>(::__ycxx::__detail::__uniform_upto(__g, static_cast<unsigned long long>(left - 1))) < n) {
          *out = *first;
          ++out;
          --n;
        }
      return out;
    } else {
      using _OD = std::iter_difference_t<_Op>;
      _Dp k = 0;
      for (; k < n && first != last; (void)++first, ++k)
        out[static_cast<_OD>(k)] = *first;
      for (; first != last; (void)++first, ++k) {
        _Dp __j = static_cast<_Dp>(::__ycxx::__detail::__uniform_upto(__g, static_cast<unsigned long long>(k)));
        if (__j < n)
          out[static_cast<_OD>(__j)] = *first;
      }
      return out + static_cast<_OD>(k < n ? k : n);
    }
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op, class _Gen>
    requires(std::ranges::forward_range<_Rp> || std::random_access_iterator<_Op>) &&
            std::indirectly_copyable<iterator_t<_Rp>, _Op> && std::uniform_random_bit_generator<std::remove_reference_t<_Gen>>
  _Op operator()(_Rp&& r, _Op out, std::ranges::range_difference_t<_Rp> n, _Gen&& __g) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(out), n, static_cast<_Gen&&>(__g));
  }
};
struct __shuffle_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Gen>
    requires std::permutable<_Ip> && std::uniform_random_bit_generator<std::remove_reference_t<_Gen>>
  _Ip operator()(_Ip first, _Sp last, _Gen&& __g) const {
    using _Dp = std::iter_difference_t<_Ip>;
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    _Dp n = end - first;
    for (_Dp i = 1; i < n; ++i) {
      _Dp __j = static_cast<_Dp>(::__ycxx::__detail::__uniform_upto(__g, static_cast<unsigned long long>(i)));
      if (__j != i)
        std::ranges::iter_swap(first + i, first + __j);
    }
    return end;
  }
  template <std::ranges::random_access_range _Rp, class _Gen>
    requires std::permutable<iterator_t<_Rp>> && std::uniform_random_bit_generator<std::remove_reference_t<_Gen>>
  borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Gen&& __g) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), static_cast<_Gen&&>(__g));
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__transform_fn, __ycxx::__detail::par::kind::transform> transform{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__replace_fn, __ycxx::__detail::par::kind::replace> replace{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__replace_if_fn, __ycxx::__detail::par::kind::replace_if> replace_if{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__replace_copy_fn, __ycxx::__detail::par::kind::replace_copy> replace_copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__replace_copy_if_fn, __ycxx::__detail::par::kind::replace_copy_if> replace_copy_if{};
inline constexpr __ycxx::__detail::__ranges_algo::__generate_fn generate{};
inline constexpr __ycxx::__detail::__ranges_algo::__generate_n_fn generate_n{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__remove_fn, __ycxx::__detail::par::kind::remove> remove{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__remove_if_fn, __ycxx::__detail::par::kind::remove_if> remove_if{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__remove_copy_fn, __ycxx::__detail::par::kind::remove_copy> remove_copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__remove_copy_if_fn, __ycxx::__detail::par::kind::remove_copy_if> remove_copy_if{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__unique_fn, __ycxx::__detail::par::kind::unique> unique{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__unique_copy_fn, __ycxx::__detail::par::kind::unique_copy> unique_copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__reverse_fn, __ycxx::__detail::par::kind::reverse> reverse{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__reverse_copy_fn, __ycxx::__detail::par::kind::reverse_copy> reverse_copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__rotate_fn, __ycxx::__detail::par::kind::rotate> rotate{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__rotate_copy_fn, __ycxx::__detail::par::kind::rotate_copy> rotate_copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__shift_left_fn, __ycxx::__detail::par::kind::shift_left> shift_left{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__shift_right_fn, __ycxx::__detail::par::kind::shift_right> shift_right{};
inline constexpr __ycxx::__detail::__ranges_algo::__sample_fn sample{};
inline constexpr __ycxx::__detail::__ranges_algo::__shuffle_fn shuffle{};
}}} // namespace std::ranges
