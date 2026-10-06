// libycxx core: the ExecutionPolicy overloads of the ranges:: algorithms (P3179,
// [algorithm.syn], [algorithms.parallel.overloads]).
//
// Like the std:: ones (algo_parallel.hpp) they run sequentially on the calling thread, which
// every standard execution policy permits, and are noexcept: an exception leaving an element
// access function of a parallel algorithm calls terminate ([algorithms.parallel.exceptions]).
//
// Each ranges:: algorithm object with parallel overloads has the type
// ranges_par_algo<Fn, kind::name>: the sequential niebloid Fn plus the overloads taking a policy
// first, declared as in [algorithm.syn].
// - Most have the semantics and result of the overload without the policy
//   ([algorithms.parallel.overloads]/2), with random-access iterators, sized sentinels and sized
//   random-access ranges required; they forward to Fn.
// - for_each and for_each_n return the iterator; the algorithms writing an output range take its
//   end (result_last or result_r), which truncates the output ([alg.copy]/6-10 etc.).
// This header is included by the algorithm headers before they define the algorithm objects,
// so it needs only the concepts; the bodies call Fn and are instantiated at the call.
#pragma once

#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/algo_results.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/invoke.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
template <class _Ip, class _Op>
using reverse_copy_truncated_result = in_in_out_result<_Ip, _Ip, _Op>;
template <class _Ip, class _Op>
using rotate_copy_truncated_result = in_in_out_result<_Ip, _Ip, _Op>;
template <class _I1, class _I2, class _Op>
using set_difference_truncated_result = in_in_out_result<_I1, _I2, _Op>;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::par {

enum class kind {
  // forwarding to the overload without the policy
  all_of, any_of, none_of, contains, contains_subrange, find, find_if, find_if_not, find_last, find_last_if,
  find_last_if_not, find_end, find_first_of, adjacent_find, count, count_if, mismatch, equal, search,
  search_n, starts_with, ends_with, swap_ranges, replace, replace_if, fill, fill_n, remove, remove_if,
  unique, reverse, rotate, shift_left, shift_right, sort, stable_sort, partial_sort, partial_sort_copy,
  is_sorted, is_sorted_until, nth_element, is_partitioned, partition, stable_partition, inplace_merge,
  includes, is_heap, is_heap_until, min, max, minmax, min_element, max_element, minmax_element,
  lexicographical_compare,
  // with their own semantics
  for_each,
  for_each_n,
  copy,
  copy_n,
  copy_if,
  move,
  transform,
  replace_copy,
  replace_copy_if,
  remove_copy,
  remove_copy_if,
  unique_copy,
  reverse_copy,
  rotate_copy,
  partition_copy,
  merge,
  set_union,
  set_intersection,
  set_difference,
  set_symmetric_difference,
};

template <class _Rp>
concept __sized_random_access_range = std::ranges::random_access_range<_Rp> && std::ranges::sized_range<_Rp>;

// The iterator form's result in the range form: a borrowed iterator, or dangling.
template <class _Rp, class _Ip>
constexpr std::ranges::borrowed_iterator_t<_Rp> __borrowed(_Ip&& i) {
  return std::ranges::borrowed_iterator_t<_Rp>(static_cast<_Ip&&>(i));
}

template <class _Tp>
constexpr _Tp __min_of(_Tp a, _Tp b) {
  return b < a ? b : a;
}

template <class _Ip, class _Sp>
constexpr _Ip __end_iter(const _Ip& first, const _Sp& last) {
  return first + (last - first);
}

// The output loop of the filtering copies (copy_if, remove_copy(_if), unique_copy): copies each
// element for which keep(i) holds while the output has room, and stops at the first such element
// that does not fit ([alg.copy]/23, [alg.remove]/13, [alg.unique]/11).
template <class _Ip, class _Sp, class _Op, class _OS, class _Keep>
constexpr std::ranges::in_out_result<_Ip, _Op> __filter_copy(_Ip first, _Sp last, _Op result, _OS __result_last, _Keep __keep) {
  for (; first != last; ++first) {
    if (__keep(first)) {
      if (result == __result_last)
        break;
      *result = *first;
      ++result;
    }
  }
  if (first == last)
    return {__end_iter(first, last), std::move(result)};
  return {std::move(first), std::move(result)};
}

template <class _I1, class _S1, class _I2, class _S2, class _Op, class _OS, class _Cp, class _P1, class _P2>
constexpr std::ranges::in_in_out_result<_I1, _I2, _Op> __merge_bounded(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                                  _OS __result_last, _Cp& comp, _P1& __proj1, _P2& __proj2) {
  while (__first1 != __last1 && __first2 != __last2 && result != __result_last) {
    if (::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(__proj2, *__first2), ::__ycxx::__detail::invoke(__proj1, *__first1))) {
      *result = *__first2;
      ++__first2;
    } else {
      *result = *__first1;
      ++__first1;
    }
    ++result;
  }
  for (; __first1 != __last1 && result != __result_last; ++__first1, (void)++result)
    *result = *__first1;
  for (; __first2 != __last2 && result != __result_last; ++__first2, (void)++result)
    *result = *__first2;
  return {std::move(__first1), std::move(__first2), std::move(result)};
}

// The set operations stop at the first element of the result that does not fit; the input
// positions then count the copied and skipped elements ([alg.set.operations]).
template <kind _Kp, class _I1, class _S1, class _I2, class _S2, class _Op, class _OS, class _Cp, class _P1, class _P2>
constexpr std::ranges::in_in_out_result<_I1, _I2, _Op> __set_op_bounded(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                                   _OS __result_last, _Cp& comp, _P1& __proj1, _P2& __proj2) {
  constexpr bool __keep1 = _Kp != kind::set_intersection; // elements only in the first range
  constexpr bool __keep2 = _Kp == kind::set_union || _Kp == kind::set_symmetric_difference;
  constexpr bool __keep_both = _Kp == kind::set_union || _Kp == kind::set_intersection;
  // set_intersection with a complete output: the elements after the last one copied are not
  // skipped ([set.intersection]/3), so the result points just past it.
  _I1 __after_copy1 = __first1;
  _I2 __after_copy2 = __first2;
  while (__first1 != __last1 && __first2 != __last2) {
    if (::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(__proj1, *__first1), ::__ycxx::__detail::invoke(__proj2, *__first2))) {
      if constexpr (__keep1) {
        if (result == __result_last)
          return {std::move(__first1), std::move(__first2), std::move(result)};
        *result = *__first1;
        ++result;
      }
      ++__first1;
    } else if (::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(__proj2, *__first2),
                                      ::__ycxx::__detail::invoke(__proj1, *__first1))) {
      if constexpr (__keep2) {
        if (result == __result_last)
          return {std::move(__first1), std::move(__first2), std::move(result)};
        *result = *__first2;
        ++result;
      }
      ++__first2;
    } else {
      if constexpr (__keep_both) {
        if (result == __result_last)
          return {std::move(__first1), std::move(__first2), std::move(result)};
        *result = *__first1;
        ++result;
      }
      ++__first1;
      ++__first2;
      if constexpr (_Kp == kind::set_intersection) {
        __after_copy1 = __first1;
        __after_copy2 = __first2;
      }
    }
  }
  if constexpr (__keep1) {
    for (; __first1 != __last1; ++__first1, (void)++result) {
      if (result == __result_last)
        return {std::move(__first1), std::move(__first2), std::move(result)};
      *result = *__first1;
    }
  }
  if constexpr (__keep2) {
    for (; __first2 != __last2; ++__first2, (void)++result) {
      if (result == __result_last)
        return {std::move(__first1), std::move(__first2), std::move(result)};
      *result = *__first2;
    }
  }
  if constexpr (_Kp == kind::set_intersection || _Kp == kind::set_difference) {
    // The output is complete ([set.intersection]/4.3, [set.difference]/4.3.1).
    if constexpr (_Kp == kind::set_intersection)
      return {std::move(__after_copy1), std::move(__after_copy2), std::move(result)};
    else
      return {__end_iter(__first1, __last1), std::move(__first2), std::move(result)};
  } else {
    return {__end_iter(__first1, __last1), __end_iter(__first2, __last2), std::move(result)};
  }
}

}} // namespace __ycxx::__detail::par

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The type of a ranges:: algorithm object with parallel overloads: specialized for each kind.
template <class _Fn, __ycxx::__detail::par::kind _Kp>
struct __ranges_par_algo;

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

using __ycxx::__detail::par::__sized_random_access_range;
namespace par = __ycxx::__detail::par;

// ---- the forwarding overloads, as declared in [algorithm.syn] ----

// all_of
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::all_of> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// any_of
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::any_of> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// none_of
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::none_of> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// contains
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::contains> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), value, std::move(proj));
  }
};

// contains_subrange
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::contains_subrange> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// find
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), value, std::move(proj));
  }
};

// find_if
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// find_if_not
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_if_not> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// find_last
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_last> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), value, std::move(proj));
  }
};

// find_last_if
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_last_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// find_last_if_not
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_last_if_not> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// find_end
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_end> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// find_first_of
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::find_first_of> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// adjacent_find
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::adjacent_find> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_binary_predicate<std::projected<_Ip, _Proj>, std::projected<_Ip, _Proj>> _Pred = std::ranges::equal_to>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_binary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>, std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred = std::ranges::equal_to>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// count
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::count> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), value, std::move(proj));
  }
};

// count_if
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::count_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// mismatch
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::mismatch> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// equal
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::equal> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// search
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::search> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// search_n
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::search_n> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Pred = std::ranges::equal_to,
            class _Proj = std::identity, class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_Ip, const _Tp*, _Pred, _Proj>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, std::iter_difference_t<_Ip> count, const _Tp& value, _Pred pred = {},
                            _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(count), value, std::move(pred),
                                         std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Pred = std::ranges::equal_to,
            class _Proj = std::identity, class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_Rp>, const _Tp*, _Pred, _Proj>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::range_difference_t<_Rp> count, const _Tp& value, _Pred pred = {},
                            _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(count), value, std::move(pred),
                                         std::move(proj));
  }
};

// starts_with
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::starts_with> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// ends_with
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::ends_with> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Pred = std::ranges::equal_to, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(pred), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_comparable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(pred),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// swap_ranges
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::swap_ranges> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_swappable<_I1, _I2>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_swappable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2));
  }
};

// replace
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::replace> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _T1 = std::projected_value_t<_Ip, _Proj>, class _T2 = std::iter_value_t<_Ip>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<_Ip, const _T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _T1*>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, const _T1& __old_value, const _T2& __new_value,
                            _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), __old_value, __new_value, std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _T1 = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>,
            class _T2 = std::ranges::range_value_t<_Rp>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<std::ranges::iterator_t<_Rp>, const _T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>, const _T1*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _T1& __old_value, const _T2& __new_value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), __old_value, __new_value, std::move(proj));
  }
};

// replace_if
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::replace_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::iter_value_t<_Ip>, std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<_Ip, const _Tp&>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, const _Tp& __new_value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), __new_value,
                                         std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _Tp = std::ranges::range_value_t<_Rp>,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<std::ranges::iterator_t<_Rp>, const _Tp&>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, const _Tp& __new_value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), __new_value, std::move(proj));
  }
};

// fill
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::fill> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Op, std::sized_sentinel_for<_Op> _Sp, class _Tp = std::iter_value_t<_Op>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<_Op, const _Tp&>
  decltype(auto) operator()(_Ep_&&, _Op first, _Sp last, const _Tp& value) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), value);
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Tp = std::ranges::range_value_t<_Rp>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<std::ranges::iterator_t<_Rp>, const _Tp&>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _Tp& value) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), value);
  }
};

// fill_n
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::fill_n> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Op, class _Tp = std::iter_value_t<_Op>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_writable<_Op, const _Tp&>
  decltype(auto) operator()(_Ep_&&, _Op first, std::iter_difference_t<_Op> n, const _Tp& value) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(n), value);
  }
};

// remove
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::remove> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>, const _Tp*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, const _Tp& value, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), value, std::move(proj));
  }
};

// remove_if
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::remove_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// unique
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::unique> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<_Ip, _Proj>> _Cp = std::ranges::equal_to>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Cp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Cp = std::ranges::equal_to>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Cp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// reverse
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::reverse> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r));
  }
};

// rotate
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::rotate> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Ip __middle, _Sp last) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(__middle), std::move(last));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::iterator_t<_Rp> __middle) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(__middle));
  }
};

// shift_left
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::shift_left> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, std::iter_difference_t<_Ip> n) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(n));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::range_difference_t<_Rp> n) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(n));
  }
};

// shift_right
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::shift_right> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, std::iter_difference_t<_Ip> n) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(n));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::range_difference_t<_Rp> n) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(n));
  }
};

// sort
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::sort> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<_Ip, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<std::ranges::iterator_t<_Rp>, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// stable_sort
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::stable_sort> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<_Ip, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<std::ranges::iterator_t<_Rp>, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// partial_sort
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::partial_sort> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<_Ip, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Ip __middle, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(__middle), std::move(last), std::move(comp),
                                         std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<std::ranges::iterator_t<_Rp>, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::iterator_t<_Rp> __middle, _Comp comp = {},
                            _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(__middle), std::move(comp), std::move(proj));
  }
};

// partial_sort_copy
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::partial_sort_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Comp = std::ranges::less, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_I1, _I2> && std::sortable<_I2, _Comp, _Proj2> &&
             std::indirect_strict_weak_order<_Comp, std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>>
  decltype(auto) operator()(_Ep_&&, _I1 first, _S1 last, _I2 __result_first, _S2 __result_last, _Comp comp = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(__result_first),
                                         std::move(__result_last), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>> &&
             std::sortable<std::ranges::iterator_t<_R2>, _Comp, _Proj2> &&
             std::indirect_strict_weak_order<_Comp, std::projected<std::ranges::iterator_t<_R1>, _Proj1>, std::projected<std::ranges::iterator_t<_R2>, _Proj2>>
  decltype(auto) operator()(_Ep_&&, _R1&& r, _R2&& __result_r, _Comp comp = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(r), static_cast<_R2&&>(__result_r), std::move(comp),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// is_sorted
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::is_sorted> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// is_sorted_until
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::is_sorted_until> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// nth_element
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::nth_element> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<_Ip, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Ip __nth, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(__nth), std::move(last), std::move(comp),
                                         std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<std::ranges::iterator_t<_Rp>, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::iterator_t<_Rp> __nth, _Comp comp = {},
                            _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(__nth), std::move(comp), std::move(proj));
  }
};

// is_partitioned
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::is_partitioned> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// partition
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::partition> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// stable_partition
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::stable_partition> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<_Ip>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::permutable<std::ranges::iterator_t<_Rp>>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Pred pred, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(pred), std::move(proj));
  }
};

// inplace_merge
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::inplace_merge> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<_Ip, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Ip __middle, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(__middle), std::move(last), std::move(comp),
                                         std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::sortable<std::ranges::iterator_t<_Rp>, _Comp, _Proj>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, std::ranges::iterator_t<_Rp> __middle, _Comp comp = {},
                            _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(__middle), std::move(comp), std::move(proj));
  }
};

// includes
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::includes> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Comp comp = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(comp), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_R1>, _Proj1>, std::projected<std::ranges::iterator_t<_R2>, _Proj2>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(comp),
                                         std::move(__proj1), std::move(__proj2));
  }
};

// is_heap
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::is_heap> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// is_heap_until
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::is_heap_until> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// min
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::min> : _Fn {
  using _Fn::operator();
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable_storable<std::ranges::iterator_t<_Rp>, std::ranges::range_value_t<_Rp>*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// max
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::max> : _Fn {
  using _Fn::operator();
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable_storable<std::ranges::iterator_t<_Rp>, std::ranges::range_value_t<_Rp>*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// minmax
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::minmax> : _Fn {
  using _Fn::operator();
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable_storable<std::ranges::iterator_t<_Rp>, std::ranges::range_value_t<_Rp>*>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// min_element
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::min_element> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// max_element
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::max_element> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// minmax_element
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::minmax_element> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _Rp&& r, _Comp comp = {}, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(comp), std::move(proj));
  }
};

// lexicographical_compare
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::lexicographical_compare> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Comp comp = {}, _Proj1 __proj1 = {},
                            _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2),
                                         std::move(comp), std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __ycxx::__detail::par::__sized_random_access_range _R1, __ycxx::__detail::par::__sized_random_access_range _R2,
            class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<_R1>, _Proj1>, std::projected<std::ranges::iterator_t<_R2>, _Proj2>> _Comp = std::ranges::less>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  decltype(auto) operator()(_Ep_&&, _R1&& __r1, _R2&& __r2, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_R1&&>(__r1), static_cast<_R2&&>(__r2), std::move(comp),
                                         std::move(__proj1), std::move(__proj2));
  }
};


// [alg.foreach]: the iterator, not an in_fun_result.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::for_each> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<_Ip, _Proj>> _Fun>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  _Ip operator()(_Ep_&&, _Ip first, _Sp last, _Fun __f, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), last, std::move(__f), std::move(proj)).in;
  }
  template <class _Ep_, __sized_random_access_range _Rp, class _Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Fun>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  std::ranges::borrowed_iterator_t<_Rp> operator()(_Ep_&&, _Rp&& r, _Fun __f, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(static_cast<_Rp&&>(r), std::move(__f), std::move(proj)).in;
  }
};

template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::for_each_n> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, class _Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<_Ip, _Proj>> _Fun>
    requires __ycxx::__detail::__execution_policy<_Ep_>
  _Ip operator()(_Ep_&&, _Ip first, std::iter_difference_t<_Ip> n, _Fun __f, _Proj proj = {}) const noexcept {
    return static_cast<const _Fn&>(*this)(std::move(first), n, std::move(__f), std::move(proj)).in;
  }
};

// [alg.copy], [alg.move]: N = min(last - first, result_last - result) elements.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last) const noexcept {
    auto n = par::__min_of<std::iter_difference_t<_Ip>>(last - first,
                                                    static_cast<std::iter_difference_t<_Ip>>(__result_last - result));
    return static_cast<const _Fn&>(*this)(first, first + n, std::move(result));
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::move> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_movable<_Ip, _Op>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last) const noexcept {
    auto n = par::__min_of<std::iter_difference_t<_Ip>>(last - first,
                                                    static_cast<std::iter_difference_t<_Ip>>(__result_last - result));
    return static_cast<const _Fn&>(*this)(first, first + n, std::move(result));
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_movable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.copy]/12-18: N = min(result_last - result, max(0, n)).
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::copy_n> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::random_access_iterator _Op, std::sized_sentinel_for<_Op> _OutS>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, std::iter_difference_t<_Ip> n, _Op result,
                                              _OutS __result_last) const noexcept {
    std::iter_difference_t<_Ip> m = n < 0 ? 0 : n;
    auto __room = static_cast<std::iter_difference_t<_Ip>>(__result_last - result);
    return static_cast<const _Fn&>(*this)(std::move(first), __room < m ? __room : m, std::move(result));
  }
};

// [alg.copy]/19-25.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::copy_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last, _Pred pred,
                                               _Proj proj = {}) const noexcept {
    return par::__filter_copy(std::move(first), last, std::move(result), __result_last, [&](const _Ip& i) {
      return bool(::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, *i)));
    });
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, _Pred pred, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), std::move(pred), std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.remove]/8-15.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::remove_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, class _Proj = std::identity, class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last, const _Tp& value,
                                                   _Proj proj = {}) const noexcept {
    return par::__filter_copy(std::move(first), last, std::move(result), __result_last,
                            [&](const _Ip& i) { return !bool(::__ycxx::__detail::invoke(proj, *i) == value); });
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR, class _Proj = std::identity,
            class _Tp = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>,
                                            const _Tp*>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, const _Tp& value, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), value, std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::remove_copy_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last, _Pred pred,
                                                      _Proj proj = {}) const noexcept {
    return par::__filter_copy(std::move(first), last, std::move(result), __result_last, [&](const _Ip& i) {
      return !bool(::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, *i)));
    });
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, _Pred pred, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), std::move(pred), std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.unique]/6-12: element i (other than the first) is not copied when it is equivalent to
// the element before it in the input.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::unique_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<_Ip, _Proj>> _Cp = std::ranges::equal_to>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last, _Cp comp = {},
                                                   _Proj proj = {}) const noexcept {
    const _Ip start = first;
    return par::__filter_copy(std::move(first), last, std::move(result), __result_last, [&](const _Ip& i) {
      return i == start || !bool(::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, *(i - 1)),
                                                         ::__ycxx::__detail::invoke(proj, *i)));
    });
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR, class _Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Cp = std::ranges::equal_to>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, _Cp comp = {}, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), std::move(comp), std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.transform]: N = min(M, result_last - result).
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::transform> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, std::copy_constructible _Fp, class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_writable<_Op, std::indirect_result_t<_Fp&, std::projected<_Ip, _Proj>>>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip __first1, _Sp __last1, _Op result, _OutS __result_last, _Fp op,
                                                       _Proj proj = {}) const noexcept {
    auto n = par::__min_of<std::iter_difference_t<_Ip>>(__last1 - __first1,
                                                         static_cast<std::iter_difference_t<_Ip>>(__result_last - result));
    return static_cast<const _Fn&>(*this)(__first1, __first1 + n, std::move(result), std::move(op), std::move(proj));
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR, std::copy_constructible _Fp,
            class _Proj = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_writable<std::ranges::iterator_t<_OutR>,
                                      std::indirect_result_t<_Fp&, std::projected<std::ranges::iterator_t<_Rp>, _Proj>>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, _Fp op, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), std::move(op), std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, std::random_access_iterator _Op, std::sized_sentinel_for<_Op> _OutS,
            std::copy_constructible _Fp, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_writable<_Op,
             std::indirect_result_t<_Fp&, std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>>>
  std::ranges::in_in_out_result<_I1, _I2, _Op> operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                             _OutS __result_last, _Fp __binary_op, _Proj1 __proj1 = {},
                                                             _Proj2 __proj2 = {}) const noexcept {
    using _Dp = std::common_type_t<std::iter_difference_t<_I1>, std::iter_difference_t<_I2>, std::iter_difference_t<_Op>>;
    _Dp n = par::__min_of<_Dp>(par::__min_of<_Dp>(_Dp(__last1 - __first1), _Dp(__last2 - __first2)), _Dp(__result_last - result));
    return static_cast<const _Fn&>(*this)(__first1, __first1 + std::iter_difference_t<_I1>(n), __first2,
                                         __first2 + std::iter_difference_t<_I2>(n), std::move(result), std::move(__binary_op),
                                         std::move(__proj1), std::move(__proj2));
  }
  template <class _Ep_, __sized_random_access_range _R1, __sized_random_access_range _R2, __sized_random_access_range _OutR,
            std::copy_constructible _Fp, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_writable<std::ranges::iterator_t<_OutR>,
                                      std::indirect_result_t<_Fp&, std::projected<std::ranges::iterator_t<_R1>, _Proj1>,
                                                             std::projected<std::ranges::iterator_t<_R2>, _Proj2>>>
  std::ranges::in_in_out_result<std::ranges::borrowed_iterator_t<_R1>, std::ranges::borrowed_iterator_t<_R2>,
                                       std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _R1&& __r1, _R2&& __r2, _OutR&& __result_r, _Fp __binary_op, _Proj1 __proj1 = {},
             _Proj2 __proj2 = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                       std::ranges::begin(__result_r), std::ranges::end(__result_r), std::move(__binary_op), std::move(__proj1),
                       std::move(__proj2));
    return {par::__borrowed<_R1>(std::move(__res.in1)), par::__borrowed<_R2>(std::move(__res.in2)),
            par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.replace]/7-12: N = min(last - first, result_last - result).
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::replace_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, class _Proj = std::identity, class _T1 = std::projected_value_t<_Ip, _Proj>,
            class _T2 = std::iter_value_t<_Op>>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _T1*> &&
             std::indirectly_writable<_Op, const _T2&>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last,
                                                    const _T1& __old_value, const _T2& __new_value,
                                                    _Proj proj = {}) const noexcept {
    auto n = par::__min_of<std::iter_difference_t<_Ip>>(last - first,
                                                         static_cast<std::iter_difference_t<_Ip>>(__result_last - result));
    return static_cast<const _Fn&>(*this)(first, first + n, std::move(result), __old_value, __new_value, std::move(proj));
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR, class _Proj = std::identity,
            class _T1 = std::projected_value_t<std::ranges::iterator_t<_Rp>, _Proj>,
            class _T2 = std::ranges::range_value_t<_OutR>>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rp>, _Proj>,
                                            const _T1*> &&
             std::indirectly_writable<std::ranges::iterator_t<_OutR>, const _T2&>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, const _T1& __old_value, const _T2& __new_value,
             _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), __old_value, __new_value, std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::replace_copy_if> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS, class _Tp = std::iter_value_t<_Op>, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op> &&
             std::indirectly_writable<_Op, const _Tp&>
  std::ranges::in_out_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result, _OutS __result_last, _Pred pred,
                                                       const _Tp& __new_value, _Proj proj = {}) const noexcept {
    auto n = par::__min_of<std::iter_difference_t<_Ip>>(last - first,
                                                         static_cast<std::iter_difference_t<_Ip>>(__result_last - result));
    return static_cast<const _Fn&>(*this)(first, first + n, std::move(result), std::move(pred), __new_value,
                                         std::move(proj));
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR,
            class _Tp = std::ranges::range_value_t<_OutR>, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>> &&
             std::indirectly_writable<std::ranges::iterator_t<_OutR>, const _Tp&>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r, _Pred pred, const _Tp& __new_value, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r), std::move(pred), __new_value, std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.reverse]/10-14: the last N elements, reversed.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::reverse_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::reverse_copy_truncated_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Sp last, _Op result,
                                                              _OutS __result_last) const noexcept {
    auto n = par::__min_of<std::iter_difference_t<_Ip>>(last - first,
                                                         static_cast<std::iter_difference_t<_Ip>>(__result_last - result));
    _Ip end = par::__end_iter(first, last);
    _Ip __new_first = end - n;
    auto __res = static_cast<const _Fn&>(*this)(__new_first, end, std::move(result));
    return {std::move(end), std::move(__new_first), std::move(__res.out)};
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::reverse_copy_truncated_result<std::ranges::borrowed_iterator_t<_Rp>,
  std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR&& __result_r) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                       std::ranges::end(__result_r));
    return {par::__borrowed<_Rp>(std::move(__res.in1)), par::__borrowed<_Rp>(std::move(__res.in2)),
            par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.rotate]/12-18.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::rotate_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _Op,
            std::sized_sentinel_for<_Op> _OutS>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _Op>
  std::ranges::rotate_copy_truncated_result<_Ip, _Op> operator()(_Ep_&&, _Ip first, _Ip __middle, _Sp last, _Op result,
                                                             _OutS __result_last) const noexcept {
    using _Dp = std::iter_difference_t<_Ip>;
    const _Dp m = last - first;
    const _Dp n = par::__min_of<_Dp>(m, static_cast<_Dp>(__result_last - result));
    const _Ip end = par::__end_iter(first, last);
    const _Dp __tail = end - __middle;
    if (n < __tail) {
      for (_Dp i = 0; i < n; ++i, (void)++result)
        *result = *(__middle + i);
      return {__middle + n, std::move(first), std::move(result)};
    }
    for (_Ip i = __middle; i != end; ++i, (void)++result)
      *result = *i;
    for (_Dp i = 0; i < n - __tail; ++i, (void)++result)
      *result = *(first + i);
    return {end, first + (m == 0 ? 0 : (n + (__middle - first)) % m), std::move(result)};
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR>>
  std::ranges::rotate_copy_truncated_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _Rp&& r, std::ranges::iterator_t<_Rp> __middle, _OutR&& __result_r) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::move(__middle), std::ranges::end(r),
                       std::ranges::begin(__result_r),
                       std::ranges::end(__result_r));
    return {par::__borrowed<_Rp>(std::move(__res.in1)), par::__borrowed<_Rp>(std::move(__res.in2)),
            par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

// [alg.partitions]/14-22: stops at the first element whose output range is full.
template <class _Fn>
struct __ranges_par_algo<_Fn, par::kind::partition_copy> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _Sp, std::random_access_iterator _O1,
            std::sized_sentinel_for<_O1> _OutS1, std::random_access_iterator _O2, std::sized_sentinel_for<_O2> _OutS2,
            class _Proj = std::identity, std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::indirectly_copyable<_Ip, _O1> && std::indirectly_copyable<_Ip, _O2>
  std::ranges::in_out_out_result<_Ip, _O1, _O2> operator()(_Ep_&&, _Ip first, _Sp last, _O1 __out_true, _OutS1 __last_true,
                                                           _O2 __out_false, _OutS2 __last_false, _Pred pred,
                                                           _Proj proj = {}) const noexcept {
    for (; first != last; ++first) {
      if (::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, *first))) {
        if (__out_true == __last_true)
          break;
        *__out_true = *first;
        ++__out_true;
      } else {
        if (__out_false == __last_false)
          break;
        *__out_false = *first;
        ++__out_false;
      }
    }
    return {std::move(first), std::move(__out_true), std::move(__out_false)};
  }
  template <class _Ep_, __sized_random_access_range _Rp, __sized_random_access_range _OutR1, __sized_random_access_range _OutR2,
            class _Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<_Rp>, _Proj>> _Pred>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR1>> &&
             std::indirectly_copyable<std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_OutR2>>
  std::ranges::in_out_out_result<std::ranges::borrowed_iterator_t<_Rp>, std::ranges::borrowed_iterator_t<_OutR1>,
                                     std::ranges::borrowed_iterator_t<_OutR2>>
  operator()(_Ep_&& __exec, _Rp&& r, _OutR1&& __out_true_r, _OutR2&& __out_false_r, _Pred pred, _Proj proj = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__out_true_r),
                       std::ranges::end(__out_true_r), std::ranges::begin(__out_false_r), std::ranges::end(__out_false_r),
                       std::move(pred), std::move(proj));
    return {par::__borrowed<_Rp>(std::move(__res.in)), par::__borrowed<_OutR1>(std::move(__res.out1)),
            par::__borrowed<_OutR2>(std::move(__res.out2))};
  }
};

// [alg.merge] and the set operations ([alg.set.operations]): the sequential loops, stopped when
// the output range is full.
template <class _Fn, par::kind _Kp>
  requires(_Kp == par::kind::merge || _Kp == par::kind::set_union || _Kp == par::kind::set_intersection ||
           _Kp == par::kind::set_difference || _Kp == par::kind::set_symmetric_difference)
struct __ranges_par_algo<_Fn, _Kp> : _Fn {
  using _Fn::operator();
  template <class _Ep_, std::random_access_iterator _I1, std::sized_sentinel_for<_I1> _S1, std::random_access_iterator _I2,
            std::sized_sentinel_for<_I2> _S2, std::random_access_iterator _Op, std::sized_sentinel_for<_Op> _OutS,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> && std::mergeable<_I1, _I2, _Op, _Comp, _Proj1, _Proj2>
  std::ranges::in_in_out_result<_I1, _I2, _Op> operator()(_Ep_&&, _I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                      _OutS __result_last, _Comp comp = {}, _Proj1 __proj1 = {},
                                                      _Proj2 __proj2 = {}) const noexcept {
    if constexpr (_Kp == par::kind::merge)
      return par::__merge_bounded(std::move(__first1), __last1, std::move(__first2), __last2, std::move(result), __result_last,
                                comp, __proj1, __proj2);
    else
      return par::__set_op_bounded<_Kp>(std::move(__first1), __last1, std::move(__first2), __last2, std::move(result), __result_last,
                                    comp, __proj1, __proj2);
  }
  template <class _Ep_, __sized_random_access_range _R1, __sized_random_access_range _R2, __sized_random_access_range _OutR,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires __ycxx::__detail::__execution_policy<_Ep_> &&
             std::mergeable<std::ranges::iterator_t<_R1>, std::ranges::iterator_t<_R2>, std::ranges::iterator_t<_OutR>,
                            _Comp, _Proj1, _Proj2>
  std::ranges::in_in_out_result<std::ranges::borrowed_iterator_t<_R1>, std::ranges::borrowed_iterator_t<_R2>,
                                std::ranges::borrowed_iterator_t<_OutR>>
  operator()(_Ep_&& __exec, _R1&& __r1, _R2&& __r2, _OutR&& __result_r, _Comp comp = {}, _Proj1 __proj1 = {},
             _Proj2 __proj2 = {}) const noexcept {
    auto __res = (*this)(__exec, std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                       std::ranges::begin(__result_r), std::ranges::end(__result_r), std::move(comp), std::move(__proj1),
                       std::move(__proj2));
    return {par::__borrowed<_R1>(std::move(__res.in1)), par::__borrowed<_R2>(std::move(__res.in2)),
            par::__borrowed<_OutR>(std::move(__res.out))};
  }
};

}} // namespace __ycxx::__adl_free
