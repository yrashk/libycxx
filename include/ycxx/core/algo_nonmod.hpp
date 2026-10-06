// libycxx core: non-modifying sequence operations ([alg.nonmodifying]) other than those in
// algo_base.hpp: all_of / any_of / none_of, contains, for_each, find_last, find_end,
// find_first_of, adjacent_find, count, is_permutation, search, search_n, starts_with,
// ends_with and the folds.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/optional.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Ip, class _Sp, class _Pp>
constexpr std::iter_difference_t<_Ip> __count_if_impl(_Ip first, _Sp last, _Pp pred) {
  std::iter_difference_t<_Ip> n = 0;
  for (; first != last; ++first)
    if (pred(*first))
      ++n;
  return n;
}

template <class _I1, class _S1, class _I2, class _S2, class _Pp>
constexpr _I1 __find_first_of_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pp __eq) {
  for (; __first1 != __last1; ++__first1)
    for (_I2 __j = __first2; __j != __last2; ++__j)
      if (__eq(*__first1, *__j))
        return __first1;
  return __first1;
}

// The first occurrence of [first2, last2) in [first1, last1): {match, match end}, or
// {last1, last1}. An empty pattern matches at first1.
template <class _I1, class _S1, class _I2, class _S2, class _Pp>
constexpr std::pair<_I1, _I1> __search_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pp __eq) {
  if constexpr (std::sized_sentinel_for<_S1, _I1> && std::sized_sentinel_for<_S2, _I2>) {
    // Lengths known: never start a match that cannot fit.
    auto __n1 = __last1 - __first1;
    const auto __n2 = __last2 - __first2;
    for (; __n1 >= __n2; (void)++__first1, --__n1) {
      _I1 i = __first1;
      _I2 __j = __first2;
      for (;; (void)++i, (void)++__j) {
        if (__j == __last2)
          return {__first1, i};
        if (!__eq(*i, *__j))
          break;
      }
    }
    _I1 end = ::__ycxx::__detail::__iter_at(__first1, __last1);
    return {end, end};
  } else {
    for (;; ++__first1) {
      _I1 i = __first1;
      _I2 __j = __first2;
      for (;; (void)++i, (void)++__j) {
        if (__j == __last2)
          return {__first1, i};
        if (i == __last1)
          return {i, i};
        if (!__eq(*i, *__j))
          break;
      }
    }
  }
}

// count consecutive elements e with eq(e) ({match, match end} or {last, last}).
template <class _Ip, class _Sp, class _Pp>
constexpr std::pair<_Ip, _Ip> __search_n_impl(_Ip first, _Sp last, std::iter_difference_t<_Ip> count, _Pp __eq) {
  if (count <= 0)
    return {first, first};
  for (; first != last; ++first) {
    if (!__eq(*first))
      continue;
    _Ip start = first;
    std::iter_difference_t<_Ip> n = 1;
    for (;;) {
      if (n == count)
        return {start, ++first};
      if (++first == last)
        return {first, first};
      if (!__eq(*first))
        break;
      ++n;
    }
  }
  return {first, first};
}

// The last occurrence; an empty pattern gives {last1, last1}. [alg.find.end]/3 allows
// M * (N - M + 1) comparisons (M, N the pattern and sequence lengths): each of the N - M + 1
// candidate positions is compared at most once, the pattern in order. With bidirectional
// iterators the candidates are tried from the back and the first match ends the search;
// otherwise a window of M elements slides forward and the last match is kept.
template <class _I1, class _S1, class _I2, class _S2, class _Pp>
constexpr std::pair<_I1, _I1> __find_end_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pp __eq) {
  // Whether the pattern occurs at start; on success stop is the end of the match.
  auto __match_at = [&](_I1 start, _I1& __stop) {
    for (_I2 __j = __first2; __j != __last2; (void)++start, (void)++__j)
      if (!__eq(*start, *__j))
        return false;
    __stop = start;
    return true;
  };
  if (__first2 == __last2) {
    _I1 end = ::__ycxx::__detail::__iter_at(std::move(__first1), std::move(__last1));
    return {end, end};
  }
  if constexpr (__bidi_iter<_I1>) {
    const _I1 end = ::__ycxx::__detail::__iter_at(__first1, std::move(__last1));
    // the last candidate: M elements before the end
    _I1 start = end;
    for (_I2 __j = __first2; __j != __last2; ++__j) {
      if (start == __first1)
        return {end, end}; // the pattern is longer than the sequence
      --start;
    }
    for (;;) {
      _I1 __stop = end;
      if (__match_at(start, __stop))
        return {std::move(start), std::move(__stop)};
      if (start == __first1)
        return {end, end};
      --start;
    }
  } else {
    // [start, probe) is the window of M elements under test
    _I1 __probe = __first1;
    for (_I2 __j = __first2; __j != __last2; (void)++__j, (void)++__probe)
      if (__probe == __last1)
        return {__probe, __probe}; // the pattern is longer than the sequence
    _I1 start = std::move(__first1);
    _I1 found = __probe, __found_end = __probe;
    bool any = false;
    for (;;) {
      if (__match_at(start, __found_end)) {
        found = start;
        any = true;
      }
      if (__probe == __last1)
        break;
      ++start;
      ++__probe;
    }
    if (!any)
      return {__probe, __probe};
    return {std::move(found), std::move(__found_end)};
  }
}

template <class _I1, class _S1, class _I2, class _S2, class _Pp>
constexpr bool __is_permutation_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pp __eq) {
  if constexpr ((std::sized_sentinel_for<_S1, _I1> || (std::same_as<_I1, _S1> && __ra_iter<_I1>)) &&
                (std::sized_sentinel_for<_S2, _I2> || (std::same_as<_I2, _S2> && __ra_iter<_I2>))) {
    if (__last1 - __first1 != __last2 - __first2)
      return false;
  }
  // The common prefix needs no counting.
  for (; __first1 != __last1 && __first2 != __last2; (void)++__first1, (void)++__first2)
    if (!__eq(*__first1, *__first2))
      break;
  if (__first1 == __last1 || __first2 == __last2)
    return __first1 == __last1 && __first2 == __last2;
  if (::__ycxx::__detail::__range_length(__first1, __last1) != ::__ycxx::__detail::__range_length(__first2, __last2))
    return false;
  // eq compares an element of range 1 with one of range 2; eq.same1 compares two of range 1.
  for (_I1 i = __first1; i != __last1; ++i) {
    bool __seen = false; // the value was counted already at an earlier position
    for (_I1 k = __first1; k != i; ++k)
      if (__eq.__same1(*k, *i)) {
        __seen = true;
        break;
      }
    if (__seen)
      continue;
    std::iter_difference_t<_I2> __c2 = 0;
    for (_I2 __j = __first2; __j != __last2; ++__j)
      if (__eq(*i, *__j))
        ++__c2;
    if (__c2 == 0)
      return false;
    std::iter_difference_t<_I2> __c1 = 1;
    _I1 k = i;
    for (++k; k != __last1; ++k)
      if (__eq.__same1(*i, *k))
        ++__c1;
    if (__c1 != __c2)
      return false;
  }
  return true;
}

// A two-range predicate that can also compare two elements of the first range.
template <class _Fp>
struct __pred_ref_perm : __pred_ref<_Fp> {
  template <class _Ap, class _Bp>
  constexpr bool __same1(_Ap&& a, _Bp&& b) const {
    return static_cast<bool>(this->__f(static_cast<_Ap&&>(a), static_cast<_Bp&&>(b)));
  }
};
template <class _Comp, class _P1, class _P2>
struct __proj_comp2_perm : __proj_comp2<_Comp, _P1, _P2> {
  template <class _Ap, class _Bp>
  constexpr bool __same1(_Ap&& a, _Bp&& b) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(this->comp, ::__ycxx::__detail::invoke(this->__p1, static_cast<_Ap&&>(a)),
                                                    ::__ycxx::__detail::invoke(this->__p1, static_cast<_Bp&&>(b))));
  }
};

// flipped<F> ([alg.fold]): calls f with its arguments swapped.
template <class _Fp>
class __flipped {
  _Fp __f;

public:
  template <class _Tp, class _Up>
    requires std::invocable<_Fp&, _Up, _Tp>
  std::invoke_result_t<_Fp&, _Up, _Tp> operator()(_Tp&&, _Up&&);
};

template <class _Fp, class _Tp, class _Ip, class _Up>
concept __indirectly_binary_left_foldable_impl =
    std::movable<_Tp> && std::movable<_Up> && std::convertible_to<_Tp, _Up> && std::invocable<_Fp&, _Up, std::iter_reference_t<_Ip>> &&
    std::assignable_from<_Up&, std::invoke_result_t<_Fp&, _Up, std::iter_reference_t<_Ip>>>;
template <class _Fp, class _Tp, class _Ip>
concept __indirectly_binary_left_foldable =
    std::copy_constructible<_Fp> && std::indirectly_readable<_Ip> && std::invocable<_Fp&, _Tp, std::iter_reference_t<_Ip>> &&
    std::convertible_to<std::invoke_result_t<_Fp&, _Tp, std::iter_reference_t<_Ip>>,
                        std::decay_t<std::invoke_result_t<_Fp&, _Tp, std::iter_reference_t<_Ip>>>> &&
    __indirectly_binary_left_foldable_impl<_Fp, _Tp, _Ip, std::decay_t<std::invoke_result_t<_Fp&, _Tp, std::iter_reference_t<_Ip>>>>;
template <class _Fp, class _Tp, class _Ip>
concept __indirectly_binary_right_foldable = __indirectly_binary_left_foldable<__flipped<_Fp>, _Tp, _Ip>;

}} // namespace __ycxx::__detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] std {

// [alg.all.of], [alg.any.of], [alg.none.of]
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr bool all_of(_InputIterator first, _InputIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__negated{::__ycxx::__detail::__ref_pred(pred)}) == last;
}
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr bool any_of(_InputIterator first, _InputIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__ref_pred(pred)) != last;
}
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr bool none_of(_InputIterator first, _InputIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__ref_pred(pred)) == last;
}

// [alg.foreach]
template <class _InputIterator, class _Function>
constexpr _Function for_each(_InputIterator first, _InputIterator last, _Function __f) {
  for (; first != last; ++first)
    __f(*first);
  return __f;
}
template <class _InputIterator, class _Size, class _Function>
constexpr _InputIterator for_each_n(_InputIterator first, _Size n, _Function __f) {
  for (auto count = ::__ycxx::__detail::__integral_count(n); count > 0; --count) {
    __f(*first);
    ++first;
  }
  return first;
}

// [alg.find.end]
template <class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr _ForwardIterator1 find_end(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                  _ForwardIterator2 __first2, _ForwardIterator2 __last2,
                                                  _BinaryPredicate pred) {
  auto r = ::__ycxx::__detail::__find_end_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(pred));
  return r.first;
}
template <class _ForwardIterator1, class _ForwardIterator2>
[[nodiscard]] constexpr _ForwardIterator1 find_end(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                  _ForwardIterator2 __first2, _ForwardIterator2 __last2) {
  return std::find_end(__first1, __last1, __first2, __last2, equal_to<>{});
}

// [alg.find.first.of]
template <class _InputIterator, class _ForwardIterator, class _BinaryPredicate>
[[nodiscard]] constexpr _InputIterator find_first_of(_InputIterator __first1, _InputIterator __last1, _ForwardIterator __first2,
                                                    _ForwardIterator __last2, _BinaryPredicate pred) {
  return ::__ycxx::__detail::__find_first_of_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _InputIterator, class _ForwardIterator>
[[nodiscard]] constexpr _InputIterator find_first_of(_InputIterator __first1, _InputIterator __last1, _ForwardIterator __first2,
                                                    _ForwardIterator __last2) {
  return std::find_first_of(__first1, __last1, __first2, __last2, equal_to<>{});
}

// [alg.adjacent.find]
template <class _ForwardIterator, class _BinaryPredicate>
[[nodiscard]] constexpr _ForwardIterator adjacent_find(_ForwardIterator first, _ForwardIterator last,
                                                      _BinaryPredicate pred) {
  return ::__ycxx::__detail::__adjacent_find_impl(first, last, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _ForwardIterator>
[[nodiscard]] constexpr _ForwardIterator adjacent_find(_ForwardIterator first, _ForwardIterator last) {
  return std::adjacent_find(first, last, equal_to<>{});
}

// [alg.count]
template <class _InputIterator, class _Tp = typename iterator_traits<_InputIterator>::value_type>
[[nodiscard]] constexpr typename iterator_traits<_InputIterator>::difference_type count(_InputIterator first,
                                                                                       _InputIterator last,
                                                                                       const _Tp& value) {
  if constexpr (__ycxx::__detail::__bit_algo_args<_InputIterator, _InputIterator, _Tp>)
    return __ycxx::__detail::__bit_algos<_InputIterator>::count(first, last, value);
  else
    return ::__ycxx::__detail::__count_if_impl(first, last, ::__ycxx::__detail::__equals_value_plain<_Tp>{value});
}
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr typename iterator_traits<_InputIterator>::difference_type count_if(_InputIterator first,
                                                                                          _InputIterator last,
                                                                                          _Predicate pred) {
  return ::__ycxx::__detail::__count_if_impl(first, last, ::__ycxx::__detail::__ref_pred(pred));
}

// [alg.is.permutation]
template <class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr bool is_permutation(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                                            _ForwardIterator2 __last2, _BinaryPredicate pred) {
  static_assert(is_same_v<typename iterator_traits<_ForwardIterator1>::value_type,
                          typename iterator_traits<_ForwardIterator2>::value_type>,
                "std::is_permutation: the two ranges must have the same value type");
  return ::__ycxx::__detail::__is_permutation_impl(__first1, __last1, __first2, __last2,
                                             ::__ycxx::__detail::__pred_ref_perm<_BinaryPredicate>{{pred}});
}
template <class _ForwardIterator1, class _ForwardIterator2>
[[nodiscard]] constexpr bool is_permutation(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                                            _ForwardIterator2 __last2) {
  return std::is_permutation(__first1, __last1, __first2, __last2, equal_to<>{});
}
template <class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr bool is_permutation(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2,
                                            _BinaryPredicate pred) {
  return std::is_permutation(__first1, __last1, __first2,
                             ::__ycxx::__detail::__iter_next(__first2, ::__ycxx::__detail::__range_length(__first1, __last1)), pred);
}
template <class _ForwardIterator1, class _ForwardIterator2>
[[nodiscard]] constexpr bool is_permutation(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2) {
  return std::is_permutation(__first1, __last1, __first2, equal_to<>{});
}

// [alg.search]
template <class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr _ForwardIterator1 search(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                _ForwardIterator2 __first2, _ForwardIterator2 __last2, _BinaryPredicate pred) {
  return ::__ycxx::__detail::__search_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(pred)).first;
}
template <class _ForwardIterator1, class _ForwardIterator2>
[[nodiscard]] constexpr _ForwardIterator1 search(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
                                                _ForwardIterator2 __first2, _ForwardIterator2 __last2) {
  return std::search(__first1, __last1, __first2, __last2, equal_to<>{});
}
template <class _ForwardIterator, class _Searcher>
[[nodiscard]] constexpr _ForwardIterator search(_ForwardIterator first, _ForwardIterator last, const _Searcher& __searcher) {
  return __searcher(first, last).first;
}

template <class _ForwardIterator, class _Size, class _Tp = typename iterator_traits<_ForwardIterator>::value_type,
          class _BinaryPredicate>
[[nodiscard]] constexpr _ForwardIterator search_n(_ForwardIterator first, _ForwardIterator last, _Size count,
                                                 const _Tp& value, _BinaryPredicate pred) {
  auto n = ::__ycxx::__detail::__integral_count(count);
  if (n <= 0)
    return first;
  auto __eq = [&pred, &value](auto&& e) -> bool { return static_cast<bool>(pred(static_cast<decltype(e)&&>(e), value)); };
  return ::__ycxx::__detail::__search_n_impl(first, last, static_cast<iter_difference_t<_ForwardIterator>>(n), __eq).first;
}
template <class _ForwardIterator, class _Size, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
[[nodiscard]] constexpr _ForwardIterator search_n(_ForwardIterator first, _ForwardIterator last, _Size count,
                                                 const _Tp& value) {
  return std::search_n(first, last, count, value, equal_to<>{});
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
template <class _Ip, class _Fp>
using for_each_result = in_fun_result<_Ip, _Fp>;
template <class _Ip, class _Fp>
using for_each_n_result = in_fun_result<_Ip, _Fp>;
template <class _Ip, class _Tp>
using fold_left_with_iter_result = in_value_result<_Ip, _Tp>;
template <class _Ip, class _Tp>
using fold_left_first_with_iter_result = in_value_result<_Ip, _Tp>;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::borrowed_subrange_t;
using std::ranges::iterator_t;

struct __all_of_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last,
                                        ::__ycxx::__detail::__negated{::__ycxx::__detail::__make_pred(pred, proj)}) == last;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct __any_of_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last, ::__ycxx::__detail::__make_pred(pred, proj)) != last;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct __none_of_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last, ::__ycxx::__detail::__make_pred(pred, proj)) == last;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.contains]
struct __contains_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last, ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj}) !=
           last;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, const _Tp& value, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct __contains_subrange_fn {
  template <std::forward_iterator _I1, std::sentinel_for<_I1> _S1, std::forward_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    if (__first2 == __last2)
      return true;
    auto m = ::__ycxx::__detail::__search_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                         ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
    return m.first != m.second;
  }
  template <std::ranges::forward_range _R1, std::ranges::forward_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(pred), std::move(__proj1), std::move(__proj2));
  }
};

// [alg.foreach]
struct __for_each_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<_Ip, _Proj>> _Fun>
  constexpr std::ranges::for_each_result<_Ip, _Fun> operator()(_Ip first, _Sp last, _Fun __f, _Proj proj = {}) const {
    for (; first != last; ++first)
      ::__ycxx::__detail::invoke(__f, ::__ycxx::__detail::invoke(proj, *first));
    return {std::move(first), std::move(__f)};
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<iterator_t<_Rp>, _Proj>> _Fun>
  constexpr std::ranges::for_each_result<borrowed_iterator_t<_Rp>, _Fun> operator()(_Rp&& r, _Fun __f, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(__f), std::move(proj));
  }
};
struct __for_each_n_fn {
  template <std::input_iterator _Ip, class _Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<_Ip, _Proj>> _Fun>
  constexpr std::ranges::for_each_n_result<_Ip, _Fun> operator()(_Ip first, std::iter_difference_t<_Ip> n, _Fun __f,
                                                             _Proj proj = {}) const {
    for (; n > 0; (void)++first, --n)
      ::__ycxx::__detail::invoke(__f, ::__ycxx::__detail::invoke(proj, *first));
    return {std::move(first), std::move(__f)};
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// [alg.find.last]
template <class _Ip, class _Sp, class _Pp>
constexpr std::ranges::subrange<_Ip> __find_last_impl(_Ip first, _Sp last, _Pp pred) {
  if constexpr (std::bidirectional_iterator<_Ip> && (std::same_as<_Ip, _Sp> || std::sized_sentinel_for<_Sp, _Ip>)) {
    _Ip end = ::__ycxx::__detail::__iter_at(first, last);
    for (_Ip __it = end; __it != first;) {
      --__it;
      if (pred(*__it))
        return {__it, end};
    }
    return {end, end};
  } else {
    _Ip found{};
    bool any = false;
    for (; first != last; ++first)
      if (pred(*first)) {
        found = first;
        any = true;
      }
    if (!any)
      return {first, first};
    return {std::move(found), std::move(first)};
  }
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {

struct __find_last_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  [[nodiscard]] constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_last_impl(std::move(first), last, ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj});
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  [[nodiscard]] constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_last_impl(std::ranges::begin(r), std::ranges::end(r),
                                       ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj});
  }
};
struct __find_last_if_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_last_impl(std::move(first), last, ::__ycxx::__detail::__make_pred(pred, proj));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_last_impl(std::ranges::begin(r), std::ranges::end(r), ::__ycxx::__detail::__make_pred(pred, proj));
  }
};
struct __find_last_if_not_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_last_impl(std::move(first), last,
                                       ::__ycxx::__detail::__negated{::__ycxx::__detail::__make_pred(pred, proj)});
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_last_impl(std::ranges::begin(r), std::ranges::end(r),
                                       ::__ycxx::__detail::__negated{::__ycxx::__detail::__make_pred(pred, proj)});
  }
};

// [alg.find.end]
struct __find_end_fn {
  template <std::forward_iterator _I1, std::sentinel_for<_I1> _S1, std::forward_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr std::ranges::subrange<_I1> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2,
                                                              _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    auto r = ::__ycxx::__detail::__find_end_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                           ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range _R1, std::ranges::forward_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr borrowed_subrange_t<_R1> operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {},
                                                            _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(pred), std::move(__proj1), std::move(__proj2));
  }
};

// [alg.find.first.of]
struct __find_first_of_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::forward_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr _I1 operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                                        _Proj2 __proj2 = {}) const {
    return ::__ycxx::__detail::__find_first_of_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                              ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
  }
  template <std::ranges::input_range _R1, std::ranges::forward_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr borrowed_iterator_t<_R1> operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {},
                                                            _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(pred), std::move(__proj1), std::move(__proj2));
  }
};

// [alg.adjacent.find]
struct __adjacent_find_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_binary_predicate<std::projected<_Ip, _Proj>, std::projected<_Ip, _Proj>> _Pred = std::ranges::equal_to>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Pred pred = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__adjacent_find_impl(std::move(first), last, ::__ycxx::__detail::__make_comp(pred, proj));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_binary_predicate<std::projected<iterator_t<_Rp>, _Proj>, std::projected<iterator_t<_Rp>, _Proj>>
                _Pred = std::ranges::equal_to>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Pred pred = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.count]
struct __count_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  [[nodiscard]] constexpr std::iter_difference_t<_Ip> operator()(_Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const {
    if constexpr (__ycxx::__detail::__bit_algo_args<_Ip, _Sp, _Tp, _Proj>)
      return __ycxx::__detail::__bit_algos<_Ip>::count(first, last, value);
    else
      return ::__ycxx::__detail::__count_if_impl(std::move(first), last, ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj});
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  [[nodiscard]] constexpr std::ranges::range_difference_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct __count_if_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr std::iter_difference_t<_Ip> operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__count_if_impl(std::move(first), last, ::__ycxx::__detail::__make_pred(pred, proj));
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr std::ranges::range_difference_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.is.permutation]
struct __is_permutation_fn {
  template <std::forward_iterator _I1, std::sentinel_for<_I1> _S1, std::forward_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_equivalence_relation<std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>> _Pred =
                std::ranges::equal_to>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    if constexpr (std::sized_sentinel_for<_S1, _I1> && std::sized_sentinel_for<_S2, _I2>) {
      if (__last1 - __first1 != __last2 - __first2)
        return false;
    }
    return ::__ycxx::__detail::__is_permutation_impl(
        std::move(__first1), __last1, std::move(__first2), __last2,
        ::__ycxx::__detail::__proj_comp2_perm<_Pred, _Proj1, _Proj2>{{pred, __proj1, __proj2}});
  }
  template <std::ranges::forward_range _R1, std::ranges::forward_range _R2, class _Proj1 = std::identity,
            class _Proj2 = std::identity,
            std::indirect_equivalence_relation<std::projected<iterator_t<_R1>, _Proj1>,
                                               std::projected<iterator_t<_R2>, _Proj2>> _Pred = std::ranges::equal_to>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    if constexpr (std::ranges::sized_range<_R1> && std::ranges::sized_range<_R2>) {
      if (std::ranges::distance(__r1) != std::ranges::distance(__r2))
        return false;
    }
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(pred), std::move(__proj1), std::move(__proj2));
  }
};

// [alg.search]
struct __search_fn {
  template <std::forward_iterator _I1, std::sentinel_for<_I1> _S1, std::forward_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr std::ranges::subrange<_I1> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2,
                                                              _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    auto r = ::__ycxx::__detail::__search_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                         ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range _R1, std::ranges::forward_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr borrowed_subrange_t<_R1> operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {},
                                                            _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(pred), std::move(__proj1), std::move(__proj2));
  }
};
struct __search_n_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Pred = std::ranges::equal_to,
            class _Proj = std::identity, class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirectly_comparable<_Ip, const _Tp*, _Pred, _Proj>
  [[nodiscard]] constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, std::iter_difference_t<_Ip> count,
                                                              const _Tp& value, _Pred pred = {}, _Proj proj = {}) const {
    auto __eq = [&](auto&& e) -> bool {
      return static_cast<bool>(
          ::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, static_cast<decltype(e)&&>(e)), value));
    };
    auto r = ::__ycxx::__detail::__search_n_impl(std::move(first), last, count, __eq);
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range _Rp, class _Pred = std::ranges::equal_to, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::indirectly_comparable<iterator_t<_Rp>, const _Tp*, _Pred, _Proj>
  [[nodiscard]] constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, std::ranges::range_difference_t<_Rp> count,
                                                           const _Tp& value, _Pred pred = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), count, value, std::move(pred), std::move(proj));
  }
};

// [alg.starts.with], [alg.ends.with]
struct __starts_with_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    if constexpr (std::sized_sentinel_for<_S1, _I1> && std::sized_sentinel_for<_S2, _I2>) {
      if (__last1 - __first1 < __last2 - __first2)
        return false;
    }
    return ::__ycxx::__detail::__mismatch_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                         ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2))
               .second == __last2;
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    if constexpr (std::ranges::sized_range<_R1> && std::ranges::sized_range<_R2>) {
      if (std::ranges::distance(__r1) < std::ranges::distance(__r2))
        return false;
    }
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(pred), std::move(__proj1), std::move(__proj2));
  }
};
struct __ends_with_fn {
  template <class _I1, class _S1, class _I2, class _S2, class _Pred, class _Proj1, class _Proj2>
  static constexpr bool __y_impl(_I1 __first1, _S1 __last1, std::iter_difference_t<_I1> __n1, _I2 __first2, _S2 __last2,
                             std::iter_difference_t<_I2> __n2, _Pred& pred, _Proj1& __proj1, _Proj2& __proj2) {
    if (__n1 < __n2)
      return false;
    std::ranges::advance(__first1, __n1 - __n2);
    return ::__ycxx::__detail::__equal_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                      ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
  }
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires(std::forward_iterator<_I1> || std::sized_sentinel_for<_S1, _I1>) &&
            (std::forward_iterator<_I2> || std::sized_sentinel_for<_S2, _I2>) &&
            std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    auto __n1 = std::ranges::distance(__first1, __last1);
    auto __n2 = std::ranges::distance(__first2, __last2);
    return __y_impl(std::move(__first1), __last1, __n1, std::move(__first2), __last2, __n2, pred, __proj1, __proj2);
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires(std::ranges::forward_range<_R1> || std::ranges::sized_range<_R1>) &&
            (std::ranges::forward_range<_R2> || std::ranges::sized_range<_R2>) &&
            std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    auto __n1 = std::ranges::distance(__r1);
    auto __n2 = std::ranges::distance(__r2);
    return __y_impl(std::ranges::begin(__r1), std::ranges::end(__r1), __n1, std::ranges::begin(__r2), std::ranges::end(__r2), __n2,
                pred, __proj1, __proj2);
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// [alg.fold]
template <class _Ip, class _Sp, class _Tp, class _Fp>
constexpr auto __fold_left_impl(_Ip first, _Sp last, _Tp init, _Fp& __f) {
  using _Up = std::decay_t<std::invoke_result_t<_Fp&, _Tp, std::iter_reference_t<_Ip>>>;
  using _Rp = std::ranges::fold_left_with_iter_result<_Ip, _Up>;
  if (first == last)
    return _Rp{std::move(first), _Up(std::move(init))};
  _Up __accum = ::__ycxx::__detail::invoke(__f, std::move(init), *first);
  for (++first; first != last; ++first)
    __accum = ::__ycxx::__detail::invoke(__f, std::move(__accum), *first);
  return _Rp{std::move(first), std::move(__accum)};
}
template <class _Ip, class _Sp, class _Fp>
constexpr auto __fold_left_first_impl(_Ip first, _Sp last, _Fp& __f) {
  using _Up = std::decay_t<std::invoke_result_t<_Fp&, std::iter_value_t<_Ip>, std::iter_reference_t<_Ip>>>;
  using _Rp = std::ranges::fold_left_first_with_iter_result<_Ip, std::optional<_Up>>;
  if (first == last)
    return _Rp{std::move(first), std::optional<_Up>()};
  std::optional<_Up> init(std::in_place, *first);
  for (++first; first != last; ++first)
    *init = ::__ycxx::__detail::invoke(__f, std::move(*init), *first);
  return _Rp{std::move(first), std::move(init)};
}
template <class _Ip, class _Sp, class _Tp, class _Fp>
constexpr auto __fold_right_impl(_Ip first, _Sp last, _Tp init, _Fp& __f) {
  using _Up = std::decay_t<std::invoke_result_t<_Fp&, std::iter_reference_t<_Ip>, _Tp>>;
  if (first == last)
    return _Up(std::move(init));
  _Ip __tail = ::__ycxx::__detail::__iter_at(first, std::move(last));
  _Up __accum = ::__ycxx::__detail::invoke(__f, *--__tail, std::move(init));
  while (first != __tail)
    __accum = ::__ycxx::__detail::invoke(__f, *--__tail, std::move(__accum));
  return __accum;
}
template <class _Ip, class _Sp, class _Fp>
constexpr auto __fold_right_last_impl(_Ip first, _Sp last, _Fp& __f) {
  using _Up = decltype(::__ycxx::__detail::__fold_right_impl(first, last, std::iter_value_t<_Ip>(*first), __f));
  if (first == last)
    return std::optional<_Up>();
  _Ip __tail = std::ranges::prev(::__ycxx::__detail::__iter_at(first, std::move(last)));
  return std::optional<_Up>(std::in_place,
                          ::__ycxx::__detail::__fold_right_impl(std::move(first), __tail, std::iter_value_t<_Ip>(*__tail), __f));
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {

struct __fold_left_with_iter_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Tp = std::iter_value_t<_Ip>,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<_Tp, _Ip> _Fp>
  constexpr auto operator()(_Ip first, _Sp last, _Tp init, _Fp __f) const {
    return ::__ycxx::__detail::__fold_left_impl(std::move(first), std::move(last), std::move(init), __f);
  }
  template <std::ranges::input_range _Rp, class _Tp = std::ranges::range_value_t<_Rp>,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<_Tp, iterator_t<_Rp>> _Fp>
  constexpr auto operator()(_Rp&& r, _Tp init, _Fp __f) const {
    auto __res = ::__ycxx::__detail::__fold_left_impl(std::ranges::begin(r), std::ranges::end(r), std::move(init), __f);
    return std::ranges::fold_left_with_iter_result<borrowed_iterator_t<_Rp>, decltype(__res.value)>{std::move(__res.in),
                                                                                                std::move(__res.value)};
  }
};
struct __fold_left_first_with_iter_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<std::iter_value_t<_Ip>, _Ip> _Fp>
    requires std::constructible_from<std::iter_value_t<_Ip>, std::iter_reference_t<_Ip>>
  constexpr auto operator()(_Ip first, _Sp last, _Fp __f) const {
    return ::__ycxx::__detail::__fold_left_first_impl(std::move(first), std::move(last), __f);
  }
  template <std::ranges::input_range _Rp,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<std::ranges::range_value_t<_Rp>, iterator_t<_Rp>> _Fp>
    requires std::constructible_from<std::ranges::range_value_t<_Rp>, std::ranges::range_reference_t<_Rp>>
  constexpr auto operator()(_Rp&& r, _Fp __f) const {
    auto __res = ::__ycxx::__detail::__fold_left_first_impl(std::ranges::begin(r), std::ranges::end(r), __f);
    return std::ranges::fold_left_first_with_iter_result<borrowed_iterator_t<_Rp>, decltype(__res.value)>{
        std::move(__res.in), std::move(__res.value)};
  }
};
struct __fold_left_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Tp = std::iter_value_t<_Ip>,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<_Tp, _Ip> _Fp>
  constexpr auto operator()(_Ip first, _Sp last, _Tp init, _Fp __f) const {
    return ::__ycxx::__detail::__fold_left_impl(std::move(first), std::move(last), std::move(init), __f).value;
  }
  template <std::ranges::input_range _Rp, class _Tp = std::ranges::range_value_t<_Rp>,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<_Tp, iterator_t<_Rp>> _Fp>
  constexpr auto operator()(_Rp&& r, _Tp init, _Fp __f) const {
    return ::__ycxx::__detail::__fold_left_impl(std::ranges::begin(r), std::ranges::end(r), std::move(init), __f).value;
  }
};
struct __fold_left_first_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<std::iter_value_t<_Ip>, _Ip> _Fp>
    requires std::constructible_from<std::iter_value_t<_Ip>, std::iter_reference_t<_Ip>>
  constexpr auto operator()(_Ip first, _Sp last, _Fp __f) const {
    return ::__ycxx::__detail::__fold_left_first_impl(std::move(first), std::move(last), __f).value;
  }
  template <std::ranges::input_range _Rp,
            ::__ycxx::__detail::__indirectly_binary_left_foldable<std::ranges::range_value_t<_Rp>, iterator_t<_Rp>> _Fp>
    requires std::constructible_from<std::ranges::range_value_t<_Rp>, std::ranges::range_reference_t<_Rp>>
  constexpr auto operator()(_Rp&& r, _Fp __f) const {
    return ::__ycxx::__detail::__fold_left_first_impl(std::ranges::begin(r), std::ranges::end(r), __f).value;
  }
};
struct __fold_right_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Tp = std::iter_value_t<_Ip>,
            ::__ycxx::__detail::__indirectly_binary_right_foldable<_Tp, _Ip> _Fp>
  constexpr auto operator()(_Ip first, _Sp last, _Tp init, _Fp __f) const {
    return ::__ycxx::__detail::__fold_right_impl(std::move(first), std::move(last), std::move(init), __f);
  }
  template <std::ranges::bidirectional_range _Rp, class _Tp = std::ranges::range_value_t<_Rp>,
            ::__ycxx::__detail::__indirectly_binary_right_foldable<_Tp, iterator_t<_Rp>> _Fp>
  constexpr auto operator()(_Rp&& r, _Tp init, _Fp __f) const {
    return ::__ycxx::__detail::__fold_right_impl(std::ranges::begin(r), std::ranges::end(r), std::move(init), __f);
  }
};
struct __fold_right_last_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp,
            ::__ycxx::__detail::__indirectly_binary_right_foldable<std::iter_value_t<_Ip>, _Ip> _Fp>
    requires std::constructible_from<std::iter_value_t<_Ip>, std::iter_reference_t<_Ip>>
  constexpr auto operator()(_Ip first, _Sp last, _Fp __f) const {
    return ::__ycxx::__detail::__fold_right_last_impl(std::move(first), std::move(last), __f);
  }
  template <std::ranges::bidirectional_range _Rp,
            ::__ycxx::__detail::__indirectly_binary_right_foldable<std::ranges::range_value_t<_Rp>, iterator_t<_Rp>> _Fp>
    requires std::constructible_from<std::ranges::range_value_t<_Rp>, std::ranges::range_reference_t<_Rp>>
  constexpr auto operator()(_Rp&& r, _Fp __f) const {
    return ::__ycxx::__detail::__fold_right_last_impl(std::ranges::begin(r), std::ranges::end(r), __f);
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__all_of_fn, __ycxx::__detail::par::kind::all_of> all_of{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__any_of_fn, __ycxx::__detail::par::kind::any_of> any_of{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__none_of_fn, __ycxx::__detail::par::kind::none_of> none_of{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__contains_fn, __ycxx::__detail::par::kind::contains> contains{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__contains_subrange_fn, __ycxx::__detail::par::kind::contains_subrange> contains_subrange{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__for_each_fn, __ycxx::__detail::par::kind::for_each> for_each{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__for_each_n_fn, __ycxx::__detail::par::kind::for_each_n> for_each_n{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_last_fn, __ycxx::__detail::par::kind::find_last> find_last{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_last_if_fn, __ycxx::__detail::par::kind::find_last_if> find_last_if{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_last_if_not_fn, __ycxx::__detail::par::kind::find_last_if_not> find_last_if_not{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_end_fn, __ycxx::__detail::par::kind::find_end> find_end{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_first_of_fn, __ycxx::__detail::par::kind::find_first_of> find_first_of{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__adjacent_find_fn, __ycxx::__detail::par::kind::adjacent_find> adjacent_find{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__count_fn, __ycxx::__detail::par::kind::count> count{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__count_if_fn, __ycxx::__detail::par::kind::count_if> count_if{};
inline constexpr __ycxx::__detail::__ranges_algo::__is_permutation_fn is_permutation{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__search_fn, __ycxx::__detail::par::kind::search> search{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__search_n_fn, __ycxx::__detail::par::kind::search_n> search_n{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__starts_with_fn, __ycxx::__detail::par::kind::starts_with> starts_with{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__ends_with_fn, __ycxx::__detail::par::kind::ends_with> ends_with{};
inline constexpr __ycxx::__detail::__ranges_algo::__fold_left_fn fold_left{};
inline constexpr __ycxx::__detail::__ranges_algo::__fold_left_first_fn fold_left_first{};
inline constexpr __ycxx::__detail::__ranges_algo::__fold_right_fn fold_right{};
inline constexpr __ycxx::__detail::__ranges_algo::__fold_right_last_fn fold_right_last{};
inline constexpr __ycxx::__detail::__ranges_algo::__fold_left_with_iter_fn fold_left_with_iter{};
inline constexpr __ycxx::__detail::__ranges_algo::__fold_left_first_with_iter_fn fold_left_first_with_iter{};
}} // namespace std::ranges
