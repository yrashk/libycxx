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

namespace [[gnu::visibility("hidden")]] std { namespace ranges {
template <class I, class O>
using reverse_copy_truncated_result = in_in_out_result<I, I, O>;
template <class I, class O>
using rotate_copy_truncated_result = in_in_out_result<I, I, O>;
template <class I1, class I2, class O>
using set_difference_truncated_result = in_in_out_result<I1, I2, O>;
}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::par {

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

template <class R>
concept sized_random_access_range = std::ranges::random_access_range<R> && std::ranges::sized_range<R>;

// The iterator form's result in the range form: a borrowed iterator, or dangling.
template <class R, class I>
constexpr std::ranges::borrowed_iterator_t<R> borrowed(I&& i) {
  return std::ranges::borrowed_iterator_t<R>(static_cast<I&&>(i));
}

template <class T>
constexpr T min_of(T a, T b) {
  return b < a ? b : a;
}

template <class I, class S>
constexpr I end_iter(const I& first, const S& last) {
  return first + (last - first);
}

// The output loop of the filtering copies (copy_if, remove_copy(_if), unique_copy): copies each
// element for which keep(i) holds while the output has room, and stops at the first such element
// that does not fit ([alg.copy]/23, [alg.remove]/13, [alg.unique]/11).
template <class I, class S, class O, class OS, class Keep>
constexpr std::ranges::in_out_result<I, O> filter_copy(I first, S last, O result, OS result_last, Keep keep) {
  for (; first != last; ++first) {
    if (keep(first)) {
      if (result == result_last)
        break;
      *result = *first;
      ++result;
    }
  }
  if (first == last)
    return {end_iter(first, last), std::move(result)};
  return {std::move(first), std::move(result)};
}

template <class I1, class S1, class I2, class S2, class O, class OS, class C, class P1, class P2>
constexpr std::ranges::in_in_out_result<I1, I2, O> merge_bounded(I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                                  OS result_last, C& comp, P1& proj1, P2& proj2) {
  while (first1 != last1 && first2 != last2 && result != result_last) {
    if (::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj2, *first2), ::ycxx::detail::invoke(proj1, *first1))) {
      *result = *first2;
      ++first2;
    } else {
      *result = *first1;
      ++first1;
    }
    ++result;
  }
  for (; first1 != last1 && result != result_last; ++first1, (void)++result)
    *result = *first1;
  for (; first2 != last2 && result != result_last; ++first2, (void)++result)
    *result = *first2;
  return {std::move(first1), std::move(first2), std::move(result)};
}

// The set operations stop at the first element of the result that does not fit; the input
// positions then count the copied and skipped elements ([alg.set.operations]).
template <kind K, class I1, class S1, class I2, class S2, class O, class OS, class C, class P1, class P2>
constexpr std::ranges::in_in_out_result<I1, I2, O> set_op_bounded(I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                                   OS result_last, C& comp, P1& proj1, P2& proj2) {
  constexpr bool keep1 = K != kind::set_intersection; // elements only in the first range
  constexpr bool keep2 = K == kind::set_union || K == kind::set_symmetric_difference;
  constexpr bool keep_both = K == kind::set_union || K == kind::set_intersection;
  // set_intersection with a complete output: the elements after the last one copied are not
  // skipped ([set.intersection]/3), so the result points just past it.
  I1 after_copy1 = first1;
  I2 after_copy2 = first2;
  while (first1 != last1 && first2 != last2) {
    if (::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj1, *first1), ::ycxx::detail::invoke(proj2, *first2))) {
      if constexpr (keep1) {
        if (result == result_last)
          return {std::move(first1), std::move(first2), std::move(result)};
        *result = *first1;
        ++result;
      }
      ++first1;
    } else if (::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj2, *first2),
                                      ::ycxx::detail::invoke(proj1, *first1))) {
      if constexpr (keep2) {
        if (result == result_last)
          return {std::move(first1), std::move(first2), std::move(result)};
        *result = *first2;
        ++result;
      }
      ++first2;
    } else {
      if constexpr (keep_both) {
        if (result == result_last)
          return {std::move(first1), std::move(first2), std::move(result)};
        *result = *first1;
        ++result;
      }
      ++first1;
      ++first2;
      if constexpr (K == kind::set_intersection) {
        after_copy1 = first1;
        after_copy2 = first2;
      }
    }
  }
  if constexpr (keep1) {
    for (; first1 != last1; ++first1, (void)++result) {
      if (result == result_last)
        return {std::move(first1), std::move(first2), std::move(result)};
      *result = *first1;
    }
  }
  if constexpr (keep2) {
    for (; first2 != last2; ++first2, (void)++result) {
      if (result == result_last)
        return {std::move(first1), std::move(first2), std::move(result)};
      *result = *first2;
    }
  }
  if constexpr (K == kind::set_intersection || K == kind::set_difference) {
    // The output is complete ([set.intersection]/4.3, [set.difference]/4.3.1).
    if constexpr (K == kind::set_intersection)
      return {std::move(after_copy1), std::move(after_copy2), std::move(result)};
    else
      return {end_iter(first1, last1), std::move(first2), std::move(result)};
  } else {
    return {end_iter(first1, last1), end_iter(first2, last2), std::move(result)};
  }
}

}} // namespace ycxx::detail::par

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The type of a ranges:: algorithm object with parallel overloads: specialized for each kind.
template <class Fn, ycxx::detail::par::kind K>
struct ranges_par_algo;

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

using ycxx::detail::par::sized_random_access_range;
namespace par = ycxx::detail::par;

// ---- the forwarding overloads, as declared in [algorithm.syn] ----

// all_of
template <class Fn>
struct ranges_par_algo<Fn, par::kind::all_of> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// any_of
template <class Fn>
struct ranges_par_algo<Fn, par::kind::any_of> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// none_of
template <class Fn>
struct ranges_par_algo<Fn, par::kind::none_of> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// contains
template <class Fn>
struct ranges_par_algo<Fn, par::kind::contains> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  decltype(auto) operator()(Ep&&, I first, S last, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
  decltype(auto) operator()(Ep&&, R&& r, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), value, std::move(proj));
  }
};

// contains_subrange
template <class Fn>
struct ranges_par_algo<Fn, par::kind::contains_subrange> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// find
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  decltype(auto) operator()(Ep&&, I first, S last, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
  decltype(auto) operator()(Ep&&, R&& r, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), value, std::move(proj));
  }
};

// find_if
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// find_if_not
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_if_not> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// find_last
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_last> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  decltype(auto) operator()(Ep&&, I first, S last, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
  decltype(auto) operator()(Ep&&, R&& r, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), value, std::move(proj));
  }
};

// find_last_if
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_last_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// find_last_if_not
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_last_if_not> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// find_end
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_end> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// find_first_of
template <class Fn>
struct ranges_par_algo<Fn, par::kind::find_first_of> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// adjacent_find
template <class Fn>
struct ranges_par_algo<Fn, par::kind::adjacent_find> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_binary_predicate<std::projected<I, Proj>, std::projected<I, Proj>> Pred = std::ranges::equal_to>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_binary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>, std::projected<std::ranges::iterator_t<R>, Proj>> Pred = std::ranges::equal_to>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// count
template <class Fn>
struct ranges_par_algo<Fn, par::kind::count> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  decltype(auto) operator()(Ep&&, I first, S last, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
  decltype(auto) operator()(Ep&&, R&& r, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), value, std::move(proj));
  }
};

// count_if
template <class Fn>
struct ranges_par_algo<Fn, par::kind::count_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// mismatch
template <class Fn>
struct ranges_par_algo<Fn, par::kind::mismatch> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// equal
template <class Fn>
struct ranges_par_algo<Fn, par::kind::equal> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// search
template <class Fn>
struct ranges_par_algo<Fn, par::kind::search> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// search_n
template <class Fn>
struct ranges_par_algo<Fn, par::kind::search_n> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Pred = std::ranges::equal_to,
            class Proj = std::identity, class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I, const T*, Pred, Proj>
  decltype(auto) operator()(Ep&&, I first, S last, std::iter_difference_t<I> count, const T& value, Pred pred = {},
                            Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(count), value, std::move(pred),
                                         std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Pred = std::ranges::equal_to,
            class Proj = std::identity, class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R>, const T*, Pred, Proj>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::range_difference_t<R> count, const T& value, Pred pred = {},
                            Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(count), value, std::move(pred),
                                         std::move(proj));
  }
};

// starts_with
template <class Fn>
struct ranges_par_algo<Fn, par::kind::starts_with> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// ends_with
template <class Fn>
struct ranges_par_algo<Fn, par::kind::ends_with> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Pred = std::ranges::equal_to, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(pred), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_comparable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(pred),
                                         std::move(proj1), std::move(proj2));
  }
};

// swap_ranges
template <class Fn>
struct ranges_par_algo<Fn, par::kind::swap_ranges> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_swappable<I1, I2>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_swappable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2));
  }
};

// replace
template <class Fn>
struct ranges_par_algo<Fn, par::kind::replace> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T1 = std::projected_value_t<I, Proj>, class T2 = std::iter_value_t<I>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<I, const T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T1*>
  decltype(auto) operator()(Ep&&, I first, S last, const T1& old_value, const T2& new_value,
                            Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), old_value, new_value, std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T1 = std::projected_value_t<std::ranges::iterator_t<R>, Proj>,
            class T2 = std::ranges::range_value_t<R>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<std::ranges::iterator_t<R>, const T2&> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T1*>
  decltype(auto) operator()(Ep&&, R&& r, const T1& old_value, const T2& new_value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), old_value, new_value, std::move(proj));
  }
};

// replace_if
template <class Fn>
struct ranges_par_algo<Fn, par::kind::replace_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T = std::iter_value_t<I>, std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<I, const T&>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, const T& new_value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), new_value,
                                         std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T = std::ranges::range_value_t<R>,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<std::ranges::iterator_t<R>, const T&>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, const T& new_value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), new_value, std::move(proj));
  }
};

// fill
template <class Fn>
struct ranges_par_algo<Fn, par::kind::fill> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator O, std::sized_sentinel_for<O> S, class T = std::iter_value_t<O>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<O, const T&>
  decltype(auto) operator()(Ep&&, O first, S last, const T& value) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), value);
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class T = std::ranges::range_value_t<R>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<std::ranges::iterator_t<R>, const T&>
  decltype(auto) operator()(Ep&&, R&& r, const T& value) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), value);
  }
};

// fill_n
template <class Fn>
struct ranges_par_algo<Fn, par::kind::fill_n> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator O, class T = std::iter_value_t<O>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_writable<O, const T&>
  decltype(auto) operator()(Ep&&, O first, std::iter_difference_t<O> n, const T& value) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(n), value);
  }
};

// remove
template <class Fn>
struct ranges_par_algo<Fn, par::kind::remove> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  decltype(auto) operator()(Ep&&, I first, S last, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), value, std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
  decltype(auto) operator()(Ep&&, R&& r, const T& value, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), value, std::move(proj));
  }
};

// remove_if
template <class Fn>
struct ranges_par_algo<Fn, par::kind::remove_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// unique
template <class Fn>
struct ranges_par_algo<Fn, par::kind::unique> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<I, Proj>> C = std::ranges::equal_to>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last, C comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<std::ranges::iterator_t<R>, Proj>> C = std::ranges::equal_to>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, C comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// reverse
template <class Fn>
struct ranges_par_algo<Fn, par::kind::reverse> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r));
  }
};

// rotate
template <class Fn>
struct ranges_par_algo<Fn, par::kind::rotate> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, I middle, S last) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(middle), std::move(last));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::iterator_t<R> middle) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(middle));
  }
};

// shift_left
template <class Fn>
struct ranges_par_algo<Fn, par::kind::shift_left> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last, std::iter_difference_t<I> n) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(n));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::range_difference_t<R> n) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(n));
  }
};

// shift_right
template <class Fn>
struct ranges_par_algo<Fn, par::kind::shift_right> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last, std::iter_difference_t<I> n) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(n));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::range_difference_t<R> n) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(n));
  }
};

// sort
template <class Fn>
struct ranges_par_algo<Fn, par::kind::sort> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<I, Comp, Proj>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// stable_sort
template <class Fn>
struct ranges_par_algo<Fn, par::kind::stable_sort> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<I, Comp, Proj>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// partial_sort
template <class Fn>
struct ranges_par_algo<Fn, par::kind::partial_sort> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<I, Comp, Proj>
  decltype(auto) operator()(Ep&&, I first, I middle, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(middle), std::move(last), std::move(comp),
                                         std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::iterator_t<R> middle, Comp comp = {},
                            Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(middle), std::move(comp), std::move(proj));
  }
};

// partial_sort_copy
template <class Fn>
struct ranges_par_algo<Fn, par::kind::partial_sort_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Comp = std::ranges::less, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I1, I2> && std::sortable<I2, Comp, Proj2> &&
             std::indirect_strict_weak_order<Comp, std::projected<I1, Proj1>, std::projected<I2, Proj2>>
  decltype(auto) operator()(Ep&&, I1 first, S1 last, I2 result_first, S2 result_last, Comp comp = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(result_first),
                                         std::move(result_last), std::move(comp), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>> &&
             std::sortable<std::ranges::iterator_t<R2>, Comp, Proj2> &&
             std::indirect_strict_weak_order<Comp, std::projected<std::ranges::iterator_t<R1>, Proj1>, std::projected<std::ranges::iterator_t<R2>, Proj2>>
  decltype(auto) operator()(Ep&&, R1&& r, R2&& result_r, Comp comp = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r), static_cast<R2&&>(result_r), std::move(comp),
                                         std::move(proj1), std::move(proj2));
  }
};

// is_sorted
template <class Fn>
struct ranges_par_algo<Fn, par::kind::is_sorted> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// is_sorted_until
template <class Fn>
struct ranges_par_algo<Fn, par::kind::is_sorted_until> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// nth_element
template <class Fn>
struct ranges_par_algo<Fn, par::kind::nth_element> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<I, Comp, Proj>
  decltype(auto) operator()(Ep&&, I first, I nth, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(nth), std::move(last), std::move(comp),
                                         std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::iterator_t<R> nth, Comp comp = {},
                            Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(nth), std::move(comp), std::move(proj));
  }
};

// is_partitioned
template <class Fn>
struct ranges_par_algo<Fn, par::kind::is_partitioned> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// partition
template <class Fn>
struct ranges_par_algo<Fn, par::kind::partition> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// stable_partition
template <class Fn>
struct ranges_par_algo<Fn, par::kind::stable_partition> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<I>
  decltype(auto) operator()(Ep&&, I first, S last, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(pred), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::permutable<std::ranges::iterator_t<R>>
  decltype(auto) operator()(Ep&&, R&& r, Pred pred, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(pred), std::move(proj));
  }
};

// inplace_merge
template <class Fn>
struct ranges_par_algo<Fn, par::kind::inplace_merge> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<I, Comp, Proj>
  decltype(auto) operator()(Ep&&, I first, I middle, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(middle), std::move(last), std::move(comp),
                                         std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
  decltype(auto) operator()(Ep&&, R&& r, std::ranges::iterator_t<R> middle, Comp comp = {},
                            Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(middle), std::move(comp), std::move(proj));
  }
};

// includes
template <class Fn>
struct ranges_par_algo<Fn, par::kind::includes> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<I1, Proj1>, std::projected<I2, Proj2>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(comp), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R1>, Proj1>, std::projected<std::ranges::iterator_t<R2>, Proj2>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(comp),
                                         std::move(proj1), std::move(proj2));
  }
};

// is_heap
template <class Fn>
struct ranges_par_algo<Fn, par::kind::is_heap> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// is_heap_until
template <class Fn>
struct ranges_par_algo<Fn, par::kind::is_heap_until> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// min
template <class Fn>
struct ranges_par_algo<Fn, par::kind::min> : Fn {
  using Fn::operator();
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// max
template <class Fn>
struct ranges_par_algo<Fn, par::kind::max> : Fn {
  using Fn::operator();
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// minmax
template <class Fn>
struct ranges_par_algo<Fn, par::kind::minmax> : Fn {
  using Fn::operator();
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable_storable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>*>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// min_element
template <class Fn>
struct ranges_par_algo<Fn, par::kind::min_element> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// max_element
template <class Fn>
struct ranges_par_algo<Fn, par::kind::max_element> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// minmax_element
template <class Fn>
struct ranges_par_algo<Fn, par::kind::minmax_element> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I first, S last, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), std::move(last), std::move(comp), std::move(proj));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R&& r, Comp comp = {}, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(comp), std::move(proj));
  }
};

// lexicographical_compare
template <class Fn>
struct ranges_par_algo<Fn, par::kind::lexicographical_compare> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<I1, Proj1>, std::projected<I2, Proj2>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {},
                            Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first1), std::move(last1), std::move(first2), std::move(last2),
                                         std::move(comp), std::move(proj1), std::move(proj2));
  }
  template <class Ep, ycxx::detail::par::sized_random_access_range R1, ycxx::detail::par::sized_random_access_range R2,
            class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<std::ranges::iterator_t<R1>, Proj1>, std::projected<std::ranges::iterator_t<R2>, Proj2>> Comp = std::ranges::less>
    requires ycxx::detail::execution_policy<Ep>
  decltype(auto) operator()(Ep&&, R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R1&&>(r1), static_cast<R2&&>(r2), std::move(comp),
                                         std::move(proj1), std::move(proj2));
  }
};


// [alg.foreach]: the iterator, not an in_fun_result.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::for_each> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, class Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>
    requires ycxx::detail::execution_policy<Ep>
  I operator()(Ep&&, I first, S last, Fun f, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), last, std::move(f), std::move(proj)).in;
  }
  template <class Ep, sized_random_access_range R, class Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<R>, Proj>> Fun>
    requires ycxx::detail::execution_policy<Ep>
  std::ranges::borrowed_iterator_t<R> operator()(Ep&&, R&& r, Fun f, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(static_cast<R&&>(r), std::move(f), std::move(proj)).in;
  }
};

template <class Fn>
struct ranges_par_algo<Fn, par::kind::for_each_n> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, class Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>
    requires ycxx::detail::execution_policy<Ep>
  I operator()(Ep&&, I first, std::iter_difference_t<I> n, Fun f, Proj proj = {}) const noexcept {
    return static_cast<const Fn&>(*this)(std::move(first), n, std::move(f), std::move(proj)).in;
  }
};

// [alg.copy], [alg.move]: N = min(last - first, result_last - result) elements.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last) const noexcept {
    auto n = par::min_of<std::iter_difference_t<I>>(last - first,
                                                    static_cast<std::iter_difference_t<I>>(result_last - result));
    return static_cast<const Fn&>(*this)(first, first + n, std::move(result));
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

template <class Fn>
struct ranges_par_algo<Fn, par::kind::move> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_movable<I, O>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last) const noexcept {
    auto n = par::min_of<std::iter_difference_t<I>>(last - first,
                                                    static_cast<std::iter_difference_t<I>>(result_last - result));
    return static_cast<const Fn&>(*this)(first, first + n, std::move(result));
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_movable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.copy]/12-18: N = min(result_last - result, max(0, n)).
template <class Fn>
struct ranges_par_algo<Fn, par::kind::copy_n> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::random_access_iterator O, std::sized_sentinel_for<O> OutS>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, std::iter_difference_t<I> n, O result,
                                              OutS result_last) const noexcept {
    std::iter_difference_t<I> m = n < 0 ? 0 : n;
    auto room = static_cast<std::iter_difference_t<I>>(result_last - result);
    return static_cast<const Fn&>(*this)(std::move(first), room < m ? room : m, std::move(result));
  }
};

// [alg.copy]/19-25.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::copy_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last, Pred pred,
                                               Proj proj = {}) const noexcept {
    return par::filter_copy(std::move(first), last, std::move(result), result_last, [&](const I& i) {
      return bool(::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, *i)));
    });
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, Pred pred, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), std::move(pred), std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.remove]/8-15.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::remove_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, class Proj = std::identity, class T = std::projected_value_t<I, Proj>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last, const T& value,
                                                   Proj proj = {}) const noexcept {
    return par::filter_copy(std::move(first), last, std::move(result), result_last,
                            [&](const I& i) { return !bool(::ycxx::detail::invoke(proj, *i) == value); });
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR, class Proj = std::identity,
            class T = std::projected_value_t<std::ranges::iterator_t<R>, Proj>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>,
                                            const T*>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, const T& value, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), value, std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

template <class Fn>
struct ranges_par_algo<Fn, par::kind::remove_copy_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last, Pred pred,
                                                      Proj proj = {}) const noexcept {
    return par::filter_copy(std::move(first), last, std::move(result), result_last, [&](const I& i) {
      return !bool(::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, *i)));
    });
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, Pred pred, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), std::move(pred), std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.unique]/6-12: element i (other than the first) is not copied when it is equivalent to
// the element before it in the input.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::unique_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<I, Proj>> C = std::ranges::equal_to>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last, C comp = {},
                                                   Proj proj = {}) const noexcept {
    const I start = first;
    return par::filter_copy(std::move(first), last, std::move(result), result_last, [&](const I& i) {
      return i == start || !bool(::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, *(i - 1)),
                                                         ::ycxx::detail::invoke(proj, *i)));
    });
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR, class Proj = std::identity,
            std::indirect_equivalence_relation<std::projected<std::ranges::iterator_t<R>, Proj>> C = std::ranges::equal_to>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, C comp = {}, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), std::move(comp), std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.transform]: N = min(M, result_last - result).
template <class Fn>
struct ranges_par_algo<Fn, par::kind::transform> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, std::copy_constructible F, class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_writable<O, std::indirect_result_t<F&, std::projected<I, Proj>>>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first1, S last1, O result, OutS result_last, F op,
                                                       Proj proj = {}) const noexcept {
    auto n = par::min_of<std::iter_difference_t<I>>(last1 - first1,
                                                         static_cast<std::iter_difference_t<I>>(result_last - result));
    return static_cast<const Fn&>(*this)(first1, first1 + n, std::move(result), std::move(op), std::move(proj));
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR, std::copy_constructible F,
            class Proj = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_writable<std::ranges::iterator_t<OutR>,
                                      std::indirect_result_t<F&, std::projected<std::ranges::iterator_t<R>, Proj>>>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, F op, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), std::move(op), std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, std::random_access_iterator O, std::sized_sentinel_for<O> OutS,
            std::copy_constructible F, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_writable<O,
             std::indirect_result_t<F&, std::projected<I1, Proj1>, std::projected<I2, Proj2>>>
  std::ranges::in_in_out_result<I1, I2, O> operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                             OutS result_last, F binary_op, Proj1 proj1 = {},
                                                             Proj2 proj2 = {}) const noexcept {
    using D = std::common_type_t<std::iter_difference_t<I1>, std::iter_difference_t<I2>, std::iter_difference_t<O>>;
    D n = par::min_of<D>(par::min_of<D>(D(last1 - first1), D(last2 - first2)), D(result_last - result));
    return static_cast<const Fn&>(*this)(first1, first1 + std::iter_difference_t<I1>(n), first2,
                                         first2 + std::iter_difference_t<I2>(n), std::move(result), std::move(binary_op),
                                         std::move(proj1), std::move(proj2));
  }
  template <class Ep, sized_random_access_range R1, sized_random_access_range R2, sized_random_access_range OutR,
            std::copy_constructible F, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_writable<std::ranges::iterator_t<OutR>,
                                      std::indirect_result_t<F&, std::projected<std::ranges::iterator_t<R1>, Proj1>,
                                                             std::projected<std::ranges::iterator_t<R2>, Proj2>>>
  std::ranges::in_in_out_result<std::ranges::borrowed_iterator_t<R1>, std::ranges::borrowed_iterator_t<R2>,
                                       std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R1&& r1, R2&& r2, OutR&& result_r, F binary_op, Proj1 proj1 = {},
             Proj2 proj2 = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                       std::ranges::begin(result_r), std::ranges::end(result_r), std::move(binary_op), std::move(proj1),
                       std::move(proj2));
    return {par::borrowed<R1>(std::move(res.in1)), par::borrowed<R2>(std::move(res.in2)),
            par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.replace]/7-12: N = min(last - first, result_last - result).
template <class Fn>
struct ranges_par_algo<Fn, par::kind::replace_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, class Proj = std::identity, class T1 = std::projected_value_t<I, Proj>,
            class T2 = std::iter_value_t<O>>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T1*> &&
             std::indirectly_writable<O, const T2&>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last,
                                                    const T1& old_value, const T2& new_value,
                                                    Proj proj = {}) const noexcept {
    auto n = par::min_of<std::iter_difference_t<I>>(last - first,
                                                         static_cast<std::iter_difference_t<I>>(result_last - result));
    return static_cast<const Fn&>(*this)(first, first + n, std::move(result), old_value, new_value, std::move(proj));
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR, class Proj = std::identity,
            class T1 = std::projected_value_t<std::ranges::iterator_t<R>, Proj>,
            class T2 = std::ranges::range_value_t<OutR>>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>> &&
             std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>,
                                            const T1*> &&
             std::indirectly_writable<std::ranges::iterator_t<OutR>, const T2&>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, const T1& old_value, const T2& new_value,
             Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), old_value, new_value, std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

template <class Fn>
struct ranges_par_algo<Fn, par::kind::replace_copy_if> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, class T = std::iter_value_t<O>, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O> &&
             std::indirectly_writable<O, const T&>
  std::ranges::in_out_result<I, O> operator()(Ep&&, I first, S last, O result, OutS result_last, Pred pred,
                                                       const T& new_value, Proj proj = {}) const noexcept {
    auto n = par::min_of<std::iter_difference_t<I>>(last - first,
                                                         static_cast<std::iter_difference_t<I>>(result_last - result));
    return static_cast<const Fn&>(*this)(first, first + n, std::move(result), std::move(pred), new_value,
                                         std::move(proj));
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR,
            class T = std::ranges::range_value_t<OutR>, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>> &&
             std::indirectly_writable<std::ranges::iterator_t<OutR>, const T&>
  std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r, Pred pred, const T& new_value, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r), std::move(pred), new_value, std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.reverse]/10-14: the last N elements, reversed.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::reverse_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::reverse_copy_truncated_result<I, O> operator()(Ep&&, I first, S last, O result,
                                                              OutS result_last) const noexcept {
    auto n = par::min_of<std::iter_difference_t<I>>(last - first,
                                                         static_cast<std::iter_difference_t<I>>(result_last - result));
    I end = par::end_iter(first, last);
    I new_first = end - n;
    auto res = static_cast<const Fn&>(*this)(new_first, end, std::move(result));
    return {std::move(end), std::move(new_first), std::move(res.out)};
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::reverse_copy_truncated_result<std::ranges::borrowed_iterator_t<R>,
  std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, OutR&& result_r) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                       std::ranges::end(result_r));
    return {par::borrowed<R>(std::move(res.in1)), par::borrowed<R>(std::move(res.in2)),
            par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.rotate]/12-18.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::rotate_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O>
  std::ranges::rotate_copy_truncated_result<I, O> operator()(Ep&&, I first, I middle, S last, O result,
                                                             OutS result_last) const noexcept {
    using D = std::iter_difference_t<I>;
    const D m = last - first;
    const D n = par::min_of<D>(m, static_cast<D>(result_last - result));
    const I end = par::end_iter(first, last);
    const D tail = end - middle;
    if (n < tail) {
      for (D i = 0; i < n; ++i, (void)++result)
        *result = *(middle + i);
      return {middle + n, std::move(first), std::move(result)};
    }
    for (I i = middle; i != end; ++i, (void)++result)
      *result = *i;
    for (D i = 0; i < n - tail; ++i, (void)++result)
      *result = *(first + i);
    return {end, first + (m == 0 ? 0 : (n + (middle - first)) % m), std::move(result)};
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR>>
  std::ranges::rotate_copy_truncated_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R&& r, std::ranges::iterator_t<R> middle, OutR&& result_r) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::move(middle), std::ranges::end(r),
                       std::ranges::begin(result_r),
                       std::ranges::end(result_r));
    return {par::borrowed<R>(std::move(res.in1)), par::borrowed<R>(std::move(res.in2)),
            par::borrowed<OutR>(std::move(res.out))};
  }
};

// [alg.partitions]/14-22: stops at the first element whose output range is full.
template <class Fn>
struct ranges_par_algo<Fn, par::kind::partition_copy> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I, std::sized_sentinel_for<I> S, std::random_access_iterator O1,
            std::sized_sentinel_for<O1> OutS1, std::random_access_iterator O2, std::sized_sentinel_for<O2> OutS2,
            class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> && std::indirectly_copyable<I, O1> && std::indirectly_copyable<I, O2>
  std::ranges::in_out_out_result<I, O1, O2> operator()(Ep&&, I first, S last, O1 out_true, OutS1 last_true,
                                                           O2 out_false, OutS2 last_false, Pred pred,
                                                           Proj proj = {}) const noexcept {
    for (; first != last; ++first) {
      if (::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, *first))) {
        if (out_true == last_true)
          break;
        *out_true = *first;
        ++out_true;
      } else {
        if (out_false == last_false)
          break;
        *out_false = *first;
        ++out_false;
      }
    }
    return {std::move(first), std::move(out_true), std::move(out_false)};
  }
  template <class Ep, sized_random_access_range R, sized_random_access_range OutR1, sized_random_access_range OutR2,
            class Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred>
    requires ycxx::detail::execution_policy<Ep> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR1>> &&
             std::indirectly_copyable<std::ranges::iterator_t<R>, std::ranges::iterator_t<OutR2>>
  std::ranges::in_out_out_result<std::ranges::borrowed_iterator_t<R>, std::ranges::borrowed_iterator_t<OutR1>,
                                     std::ranges::borrowed_iterator_t<OutR2>>
  operator()(Ep&& exec, R&& r, OutR1&& out_true_r, OutR2&& out_false_r, Pred pred, Proj proj = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(out_true_r),
                       std::ranges::end(out_true_r), std::ranges::begin(out_false_r), std::ranges::end(out_false_r),
                       std::move(pred), std::move(proj));
    return {par::borrowed<R>(std::move(res.in)), par::borrowed<OutR1>(std::move(res.out1)),
            par::borrowed<OutR2>(std::move(res.out2))};
  }
};

// [alg.merge] and the set operations ([alg.set.operations]): the sequential loops, stopped when
// the output range is full.
template <class Fn, par::kind K>
  requires(K == par::kind::merge || K == par::kind::set_union || K == par::kind::set_intersection ||
           K == par::kind::set_difference || K == par::kind::set_symmetric_difference)
struct ranges_par_algo<Fn, K> : Fn {
  using Fn::operator();
  template <class Ep, std::random_access_iterator I1, std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, std::random_access_iterator O, std::sized_sentinel_for<O> OutS,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> && std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
  std::ranges::in_in_out_result<I1, I2, O> operator()(Ep&&, I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                      OutS result_last, Comp comp = {}, Proj1 proj1 = {},
                                                      Proj2 proj2 = {}) const noexcept {
    if constexpr (K == par::kind::merge)
      return par::merge_bounded(std::move(first1), last1, std::move(first2), last2, std::move(result), result_last,
                                comp, proj1, proj2);
    else
      return par::set_op_bounded<K>(std::move(first1), last1, std::move(first2), last2, std::move(result), result_last,
                                    comp, proj1, proj2);
  }
  template <class Ep, sized_random_access_range R1, sized_random_access_range R2, sized_random_access_range OutR,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires ycxx::detail::execution_policy<Ep> &&
             std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>, std::ranges::iterator_t<OutR>,
                            Comp, Proj1, Proj2>
  std::ranges::in_in_out_result<std::ranges::borrowed_iterator_t<R1>, std::ranges::borrowed_iterator_t<R2>,
                                std::ranges::borrowed_iterator_t<OutR>>
  operator()(Ep&& exec, R1&& r1, R2&& r2, OutR&& result_r, Comp comp = {}, Proj1 proj1 = {},
             Proj2 proj2 = {}) const noexcept {
    auto res = (*this)(exec, std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                       std::ranges::begin(result_r), std::ranges::end(result_r), std::move(comp), std::move(proj1),
                       std::move(proj2));
    return {par::borrowed<R1>(std::move(res.in1)), par::borrowed<R2>(std::move(res.in2)),
            par::borrowed<OutR>(std::move(res.out))};
  }
};

}} // namespace ycxx::adl_free
