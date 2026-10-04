// libycxx core: non-modifying sequence operations ([alg.nonmodifying]) other than those in
// algo_base.hpp: all_of / any_of / none_of, contains, for_each, find_last, find_end,
// find_first_of, adjacent_find, count, is_permutation, search, search_n, starts_with,
// ends_with and the folds.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/optional.hpp>

namespace ycxx::detail {

template <class I, class S, class P>
constexpr std::iter_difference_t<I> count_if_impl(I first, S last, P pred) {
  std::iter_difference_t<I> n = 0;
  for (; first != last; ++first)
    if (pred(*first))
      ++n;
  return n;
}

template <class I1, class S1, class I2, class S2, class P>
constexpr I1 find_first_of_impl(I1 first1, S1 last1, I2 first2, S2 last2, P eq) {
  for (; first1 != last1; ++first1)
    for (I2 j = first2; j != last2; ++j)
      if (eq(*first1, *j))
        return first1;
  return first1;
}

// The first occurrence of [first2, last2) in [first1, last1): {match, match end}, or
// {last1, last1}. An empty pattern matches at first1.
template <class I1, class S1, class I2, class S2, class P>
constexpr std::pair<I1, I1> search_impl(I1 first1, S1 last1, I2 first2, S2 last2, P eq) {
  if constexpr (std::sized_sentinel_for<S1, I1> && std::sized_sentinel_for<S2, I2>) {
    // Lengths known: never start a match that cannot fit.
    auto n1 = last1 - first1;
    const auto n2 = last2 - first2;
    for (; n1 >= n2; (void)++first1, --n1) {
      I1 i = first1;
      I2 j = first2;
      for (;; (void)++i, (void)++j) {
        if (j == last2)
          return {first1, i};
        if (!eq(*i, *j))
          break;
      }
    }
    I1 end = ::ycxx::detail::iter_at(first1, last1);
    return {end, end};
  } else {
    for (;; ++first1) {
      I1 i = first1;
      I2 j = first2;
      for (;; (void)++i, (void)++j) {
        if (j == last2)
          return {first1, i};
        if (i == last1)
          return {i, i};
        if (!eq(*i, *j))
          break;
      }
    }
  }
}

// count consecutive elements e with eq(e) ({match, match end} or {last, last}).
template <class I, class S, class P>
constexpr std::pair<I, I> search_n_impl(I first, S last, std::iter_difference_t<I> count, P eq) {
  if (count <= 0)
    return {first, first};
  for (; first != last; ++first) {
    if (!eq(*first))
      continue;
    I start = first;
    std::iter_difference_t<I> n = 1;
    for (;;) {
      if (n == count)
        return {start, ++first};
      if (++first == last)
        return {first, first};
      if (!eq(*first))
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
template <class I1, class S1, class I2, class S2, class P>
constexpr std::pair<I1, I1> find_end_impl(I1 first1, S1 last1, I2 first2, S2 last2, P eq) {
  // Whether the pattern occurs at start; on success stop is the end of the match.
  auto match_at = [&](I1 start, I1& stop) {
    for (I2 j = first2; j != last2; (void)++start, (void)++j)
      if (!eq(*start, *j))
        return false;
    stop = start;
    return true;
  };
  if (first2 == last2) {
    I1 end = ::ycxx::detail::iter_at(std::move(first1), std::move(last1));
    return {end, end};
  }
  if constexpr (bidi_iter<I1>) {
    const I1 end = ::ycxx::detail::iter_at(first1, std::move(last1));
    // the last candidate: M elements before the end
    I1 start = end;
    for (I2 j = first2; j != last2; ++j) {
      if (start == first1)
        return {end, end}; // the pattern is longer than the sequence
      --start;
    }
    for (;;) {
      I1 stop = end;
      if (match_at(start, stop))
        return {std::move(start), std::move(stop)};
      if (start == first1)
        return {end, end};
      --start;
    }
  } else {
    // [start, probe) is the window of M elements under test
    I1 probe = first1;
    for (I2 j = first2; j != last2; (void)++j, (void)++probe)
      if (probe == last1)
        return {probe, probe}; // the pattern is longer than the sequence
    I1 start = std::move(first1);
    I1 found = probe, found_end = probe;
    bool any = false;
    for (;;) {
      if (match_at(start, found_end)) {
        found = start;
        any = true;
      }
      if (probe == last1)
        break;
      ++start;
      ++probe;
    }
    if (!any)
      return {probe, probe};
    return {std::move(found), std::move(found_end)};
  }
}

template <class I1, class S1, class I2, class S2, class P>
constexpr bool is_permutation_impl(I1 first1, S1 last1, I2 first2, S2 last2, P eq) {
  if constexpr ((std::sized_sentinel_for<S1, I1> || (std::same_as<I1, S1> && ra_iter<I1>)) &&
                (std::sized_sentinel_for<S2, I2> || (std::same_as<I2, S2> && ra_iter<I2>))) {
    if (last1 - first1 != last2 - first2)
      return false;
  }
  // The common prefix needs no counting.
  for (; first1 != last1 && first2 != last2; (void)++first1, (void)++first2)
    if (!eq(*first1, *first2))
      break;
  if (first1 == last1 || first2 == last2)
    return first1 == last1 && first2 == last2;
  if (::ycxx::detail::range_length(first1, last1) != ::ycxx::detail::range_length(first2, last2))
    return false;
  // eq compares an element of range 1 with one of range 2; eq.same1 compares two of range 1.
  for (I1 i = first1; i != last1; ++i) {
    bool seen = false; // the value was counted already at an earlier position
    for (I1 k = first1; k != i; ++k)
      if (eq.same1(*k, *i)) {
        seen = true;
        break;
      }
    if (seen)
      continue;
    std::iter_difference_t<I2> c2 = 0;
    for (I2 j = first2; j != last2; ++j)
      if (eq(*i, *j))
        ++c2;
    if (c2 == 0)
      return false;
    std::iter_difference_t<I2> c1 = 1;
    I1 k = i;
    for (++k; k != last1; ++k)
      if (eq.same1(*i, *k))
        ++c1;
    if (c1 != c2)
      return false;
  }
  return true;
}

// A two-range predicate that can also compare two elements of the first range.
template <class F>
struct pred_ref_perm : pred_ref<F> {
  template <class A, class B>
  constexpr bool same1(A&& a, B&& b) const {
    return static_cast<bool>(this->f(static_cast<A&&>(a), static_cast<B&&>(b)));
  }
};
template <class Comp, class P1, class P2>
struct proj_comp2_perm : proj_comp2<Comp, P1, P2> {
  template <class A, class B>
  constexpr bool same1(A&& a, B&& b) const {
    return static_cast<bool>(::ycxx::detail::invoke(this->comp, ::ycxx::detail::invoke(this->p1, static_cast<A&&>(a)),
                                                    ::ycxx::detail::invoke(this->p1, static_cast<B&&>(b))));
  }
};

// flipped<F> ([alg.fold]): calls f with its arguments swapped.
template <class F>
class flipped {
  F f;

public:
  template <class T, class U>
    requires std::invocable<F&, U, T>
  std::invoke_result_t<F&, U, T> operator()(T&&, U&&);
};

template <class F, class T, class I, class U>
concept indirectly_binary_left_foldable_impl =
    std::movable<T> && std::movable<U> && std::convertible_to<T, U> && std::invocable<F&, U, std::iter_reference_t<I>> &&
    std::assignable_from<U&, std::invoke_result_t<F&, U, std::iter_reference_t<I>>>;
template <class F, class T, class I>
concept indirectly_binary_left_foldable =
    std::copy_constructible<F> && std::indirectly_readable<I> && std::invocable<F&, T, std::iter_reference_t<I>> &&
    std::convertible_to<std::invoke_result_t<F&, T, std::iter_reference_t<I>>,
                        std::decay_t<std::invoke_result_t<F&, T, std::iter_reference_t<I>>>> &&
    indirectly_binary_left_foldable_impl<F, T, I, std::decay_t<std::invoke_result_t<F&, T, std::iter_reference_t<I>>>>;
template <class F, class T, class I>
concept indirectly_binary_right_foldable = indirectly_binary_left_foldable<flipped<F>, T, I>;

} // namespace ycxx::detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace std {

// [alg.all.of], [alg.any.of], [alg.none.of]
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr bool all_of(InputIterator first, InputIterator last, Predicate pred) {
  return ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::negated{::ycxx::detail::ref_pred(pred)}) == last;
}
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr bool any_of(InputIterator first, InputIterator last, Predicate pred) {
  return ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::ref_pred(pred)) != last;
}
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr bool none_of(InputIterator first, InputIterator last, Predicate pred) {
  return ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::ref_pred(pred)) == last;
}

// [alg.foreach]
template <class InputIterator, class Function>
constexpr Function for_each(InputIterator first, InputIterator last, Function f) {
  for (; first != last; ++first)
    f(*first);
  return f;
}
template <class InputIterator, class Size, class Function>
constexpr InputIterator for_each_n(InputIterator first, Size n, Function f) {
  for (auto count = ::ycxx::detail::integral_count(n); count > 0; --count) {
    f(*first);
    ++first;
  }
  return first;
}

// [alg.find.end]
template <class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
[[nodiscard]] constexpr ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
                                                  ForwardIterator2 first2, ForwardIterator2 last2,
                                                  BinaryPredicate pred) {
  auto r = ::ycxx::detail::find_end_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(pred));
  return r.first;
}
template <class ForwardIterator1, class ForwardIterator2>
[[nodiscard]] constexpr ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
                                                  ForwardIterator2 first2, ForwardIterator2 last2) {
  return std::find_end(first1, last1, first2, last2, equal_to<>{});
}

// [alg.find.first.of]
template <class InputIterator, class ForwardIterator, class BinaryPredicate>
[[nodiscard]] constexpr InputIterator find_first_of(InputIterator first1, InputIterator last1, ForwardIterator first2,
                                                    ForwardIterator last2, BinaryPredicate pred) {
  return ::ycxx::detail::find_first_of_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(pred));
}
template <class InputIterator, class ForwardIterator>
[[nodiscard]] constexpr InputIterator find_first_of(InputIterator first1, InputIterator last1, ForwardIterator first2,
                                                    ForwardIterator last2) {
  return std::find_first_of(first1, last1, first2, last2, equal_to<>{});
}

// [alg.adjacent.find]
template <class ForwardIterator, class BinaryPredicate>
[[nodiscard]] constexpr ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last,
                                                      BinaryPredicate pred) {
  return ::ycxx::detail::adjacent_find_impl(first, last, ::ycxx::detail::ref_pred(pred));
}
template <class ForwardIterator>
[[nodiscard]] constexpr ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last) {
  return std::adjacent_find(first, last, equal_to<>{});
}

// [alg.count]
template <class InputIterator, class T = typename iterator_traits<InputIterator>::value_type>
[[nodiscard]] constexpr typename iterator_traits<InputIterator>::difference_type count(InputIterator first,
                                                                                       InputIterator last,
                                                                                       const T& value) {
  return ::ycxx::detail::count_if_impl(first, last, ::ycxx::detail::equals_value_plain<T>{value});
}
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr typename iterator_traits<InputIterator>::difference_type count_if(InputIterator first,
                                                                                          InputIterator last,
                                                                                          Predicate pred) {
  return ::ycxx::detail::count_if_impl(first, last, ::ycxx::detail::ref_pred(pred));
}

// [alg.is.permutation]
template <class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
[[nodiscard]] constexpr bool is_permutation(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                                            ForwardIterator2 last2, BinaryPredicate pred) {
  static_assert(is_same_v<typename iterator_traits<ForwardIterator1>::value_type,
                          typename iterator_traits<ForwardIterator2>::value_type>,
                "std::is_permutation: the two ranges must have the same value type");
  return ::ycxx::detail::is_permutation_impl(first1, last1, first2, last2,
                                             ::ycxx::detail::pred_ref_perm<BinaryPredicate>{{pred}});
}
template <class ForwardIterator1, class ForwardIterator2>
[[nodiscard]] constexpr bool is_permutation(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                                            ForwardIterator2 last2) {
  return std::is_permutation(first1, last1, first2, last2, equal_to<>{});
}
template <class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
[[nodiscard]] constexpr bool is_permutation(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2,
                                            BinaryPredicate pred) {
  return std::is_permutation(first1, last1, first2,
                             ::ycxx::detail::iter_next(first2, ::ycxx::detail::range_length(first1, last1)), pred);
}
template <class ForwardIterator1, class ForwardIterator2>
[[nodiscard]] constexpr bool is_permutation(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2) {
  return std::is_permutation(first1, last1, first2, equal_to<>{});
}

// [alg.search]
template <class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
[[nodiscard]] constexpr ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                                                ForwardIterator2 first2, ForwardIterator2 last2, BinaryPredicate pred) {
  return ::ycxx::detail::search_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(pred)).first;
}
template <class ForwardIterator1, class ForwardIterator2>
[[nodiscard]] constexpr ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                                                ForwardIterator2 first2, ForwardIterator2 last2) {
  return std::search(first1, last1, first2, last2, equal_to<>{});
}
template <class ForwardIterator, class Searcher>
[[nodiscard]] constexpr ForwardIterator search(ForwardIterator first, ForwardIterator last, const Searcher& searcher) {
  return searcher(first, last).first;
}

template <class ForwardIterator, class Size, class T = typename iterator_traits<ForwardIterator>::value_type,
          class BinaryPredicate>
[[nodiscard]] constexpr ForwardIterator search_n(ForwardIterator first, ForwardIterator last, Size count,
                                                 const T& value, BinaryPredicate pred) {
  auto n = ::ycxx::detail::integral_count(count);
  if (n <= 0)
    return first;
  auto eq = [&pred, &value](auto&& e) -> bool { return static_cast<bool>(pred(static_cast<decltype(e)&&>(e), value)); };
  return ::ycxx::detail::search_n_impl(first, last, static_cast<iter_difference_t<ForwardIterator>>(n), eq).first;
}
template <class ForwardIterator, class Size, class T = typename iterator_traits<ForwardIterator>::value_type>
[[nodiscard]] constexpr ForwardIterator search_n(ForwardIterator first, ForwardIterator last, Size count,
                                                 const T& value) {
  return std::search_n(first, last, count, value, equal_to<>{});
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace std::ranges {
template <class I, class F>
using for_each_result = in_fun_result<I, F>;
template <class I, class F>
using for_each_n_result = in_fun_result<I, F>;
template <class I, class T>
using fold_left_with_iter_result = in_value_result<I, T>;
template <class I, class T>
using fold_left_first_with_iter_result = in_value_result<I, T>;
} // namespace std::ranges

namespace ycxx::detail::ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::borrowed_subrange_t;
using std::ranges::iterator_t;

struct all_of_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last,
                                        ::ycxx::detail::negated{::ycxx::detail::make_pred(pred, proj)}) == last;
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct any_of_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last, ::ycxx::detail::make_pred(pred, proj)) != last;
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct none_of_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last, ::ycxx::detail::make_pred(pred, proj)) == last;
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.contains]
struct contains_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  [[nodiscard]] constexpr bool operator()(I first, S last, const T& value, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last, ::ycxx::detail::equals_value<T, Proj>{value, proj}) !=
           last;
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T*>
  [[nodiscard]] constexpr bool operator()(R&& r, const T& value, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct contains_subrange_fn {
  template <std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    if (first2 == last2)
      return true;
    auto m = ::ycxx::detail::search_impl(std::move(first1), last1, std::move(first2), last2,
                                         ::ycxx::detail::make_comp2(pred, proj1, proj2));
    return m.first != m.second;
  }
  template <std::ranges::forward_range R1, std::ranges::forward_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};

// [alg.foreach]
struct for_each_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>
  constexpr std::ranges::for_each_result<I, Fun> operator()(I first, S last, Fun f, Proj proj = {}) const {
    for (; first != last; ++first)
      ::ycxx::detail::invoke(f, ::ycxx::detail::invoke(proj, *first));
    return {std::move(first), std::move(f)};
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<iterator_t<R>, Proj>> Fun>
  constexpr std::ranges::for_each_result<borrowed_iterator_t<R>, Fun> operator()(R&& r, Fun f, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(f), std::move(proj));
  }
};
struct for_each_n_fn {
  template <std::input_iterator I, class Proj = std::identity,
            std::indirectly_unary_invocable<std::projected<I, Proj>> Fun>
  constexpr std::ranges::for_each_n_result<I, Fun> operator()(I first, std::iter_difference_t<I> n, Fun f,
                                                             Proj proj = {}) const {
    for (; n > 0; (void)++first, --n)
      ::ycxx::detail::invoke(f, ::ycxx::detail::invoke(proj, *first));
    return {std::move(first), std::move(f)};
  }
};

} // namespace ycxx::detail::ranges_algo

namespace ycxx::detail {
// [alg.find.last]
template <class I, class S, class P>
constexpr std::ranges::subrange<I> find_last_impl(I first, S last, P pred) {
  if constexpr (std::bidirectional_iterator<I> && (std::same_as<I, S> || std::sized_sentinel_for<S, I>)) {
    I end = ::ycxx::detail::iter_at(first, last);
    for (I it = end; it != first;) {
      --it;
      if (pred(*it))
        return {it, end};
    }
    return {end, end};
  } else {
    I found{};
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
} // namespace ycxx::detail

namespace ycxx::detail::ranges_algo {

struct find_last_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  [[nodiscard]] constexpr std::ranges::subrange<I> operator()(I first, S last, const T& value, Proj proj = {}) const {
    return ::ycxx::detail::find_last_impl(std::move(first), last, ::ycxx::detail::equals_value<T, Proj>{value, proj});
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T*>
  [[nodiscard]] constexpr borrowed_subrange_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
    return ::ycxx::detail::find_last_impl(std::ranges::begin(r), std::ranges::end(r),
                                       ::ycxx::detail::equals_value<T, Proj>{value, proj});
  }
};
struct find_last_if_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr std::ranges::subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_last_impl(std::move(first), last, ::ycxx::detail::make_pred(pred, proj));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_last_impl(std::ranges::begin(r), std::ranges::end(r), ::ycxx::detail::make_pred(pred, proj));
  }
};
struct find_last_if_not_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr std::ranges::subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_last_impl(std::move(first), last,
                                       ::ycxx::detail::negated{::ycxx::detail::make_pred(pred, proj)});
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_last_impl(std::ranges::begin(r), std::ranges::end(r),
                                       ::ycxx::detail::negated{::ycxx::detail::make_pred(pred, proj)});
  }
};

// [alg.find.end]
struct find_end_fn {
  template <std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr std::ranges::subrange<I1> operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                                                              Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    auto r = ::ycxx::detail::find_end_impl(std::move(first1), last1, std::move(first2), last2,
                                           ::ycxx::detail::make_comp2(pred, proj1, proj2));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range R1, std::ranges::forward_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr borrowed_subrange_t<R1> operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {},
                                                            Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};

// [alg.find.first.of]
struct find_first_of_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr I1 operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                                        Proj2 proj2 = {}) const {
    return ::ycxx::detail::find_first_of_impl(std::move(first1), last1, std::move(first2), last2,
                                              ::ycxx::detail::make_comp2(pred, proj1, proj2));
  }
  template <std::ranges::input_range R1, std::ranges::forward_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr borrowed_iterator_t<R1> operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {},
                                                            Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};

// [alg.adjacent.find]
struct adjacent_find_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_binary_predicate<std::projected<I, Proj>, std::projected<I, Proj>> Pred = std::ranges::equal_to>
  [[nodiscard]] constexpr I operator()(I first, S last, Pred pred = {}, Proj proj = {}) const {
    return ::ycxx::detail::adjacent_find_impl(std::move(first), last, ::ycxx::detail::make_comp(pred, proj));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_binary_predicate<std::projected<iterator_t<R>, Proj>, std::projected<iterator_t<R>, Proj>>
                Pred = std::ranges::equal_to>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.count]
struct count_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  [[nodiscard]] constexpr std::iter_difference_t<I> operator()(I first, S last, const T& value, Proj proj = {}) const {
    return ::ycxx::detail::count_if_impl(std::move(first), last, ::ycxx::detail::equals_value<T, Proj>{value, proj});
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T*>
  [[nodiscard]] constexpr std::ranges::range_difference_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct count_if_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr std::iter_difference_t<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::count_if_impl(std::move(first), last, ::ycxx::detail::make_pred(pred, proj));
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr std::ranges::range_difference_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.is.permutation]
struct is_permutation_fn {
  template <std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2, std::sentinel_for<I2> S2,
            class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_equivalence_relation<std::projected<I1, Proj1>, std::projected<I2, Proj2>> Pred =
                std::ranges::equal_to>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    if constexpr (std::sized_sentinel_for<S1, I1> && std::sized_sentinel_for<S2, I2>) {
      if (last1 - first1 != last2 - first2)
        return false;
    }
    return ::ycxx::detail::is_permutation_impl(
        std::move(first1), last1, std::move(first2), last2,
        ::ycxx::detail::proj_comp2_perm<Pred, Proj1, Proj2>{{pred, proj1, proj2}});
  }
  template <std::ranges::forward_range R1, std::ranges::forward_range R2, class Proj1 = std::identity,
            class Proj2 = std::identity,
            std::indirect_equivalence_relation<std::projected<iterator_t<R1>, Proj1>,
                                               std::projected<iterator_t<R2>, Proj2>> Pred = std::ranges::equal_to>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    if constexpr (std::ranges::sized_range<R1> && std::ranges::sized_range<R2>) {
      if (std::ranges::distance(r1) != std::ranges::distance(r2))
        return false;
    }
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};

// [alg.search]
struct search_fn {
  template <std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr std::ranges::subrange<I1> operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                                                              Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    auto r = ::ycxx::detail::search_impl(std::move(first1), last1, std::move(first2), last2,
                                         ::ycxx::detail::make_comp2(pred, proj1, proj2));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range R1, std::ranges::forward_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr borrowed_subrange_t<R1> operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {},
                                                            Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};
struct search_n_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Pred = std::ranges::equal_to,
            class Proj = std::identity, class T = std::projected_value_t<I, Proj>>
    requires std::indirectly_comparable<I, const T*, Pred, Proj>
  [[nodiscard]] constexpr std::ranges::subrange<I> operator()(I first, S last, std::iter_difference_t<I> count,
                                                              const T& value, Pred pred = {}, Proj proj = {}) const {
    auto eq = [&](auto&& e) -> bool {
      return static_cast<bool>(
          ::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, static_cast<decltype(e)&&>(e)), value));
    };
    auto r = ::ycxx::detail::search_n_impl(std::move(first), last, count, eq);
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range R, class Pred = std::ranges::equal_to, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirectly_comparable<iterator_t<R>, const T*, Pred, Proj>
  [[nodiscard]] constexpr borrowed_subrange_t<R> operator()(R&& r, std::ranges::range_difference_t<R> count,
                                                           const T& value, Pred pred = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), count, value, std::move(pred), std::move(proj));
  }
};

// [alg.starts.with], [alg.ends.with]
struct starts_with_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    if constexpr (std::sized_sentinel_for<S1, I1> && std::sized_sentinel_for<S2, I2>) {
      if (last1 - first1 < last2 - first2)
        return false;
    }
    return ::ycxx::detail::mismatch_impl(std::move(first1), last1, std::move(first2), last2,
                                         ::ycxx::detail::make_comp2(pred, proj1, proj2))
               .second == last2;
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    if constexpr (std::ranges::sized_range<R1> && std::ranges::sized_range<R2>) {
      if (std::ranges::distance(r1) < std::ranges::distance(r2))
        return false;
    }
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(pred), std::move(proj1), std::move(proj2));
  }
};
struct ends_with_fn {
  template <class I1, class S1, class I2, class S2, class Pred, class Proj1, class Proj2>
  static constexpr bool impl(I1 first1, S1 last1, std::iter_difference_t<I1> n1, I2 first2, S2 last2,
                             std::iter_difference_t<I2> n2, Pred& pred, Proj1& proj1, Proj2& proj2) {
    if (n1 < n2)
      return false;
    std::ranges::advance(first1, n1 - n2);
    return ::ycxx::detail::equal_impl(std::move(first1), last1, std::move(first2), last2,
                                      ::ycxx::detail::make_comp2(pred, proj1, proj2));
  }
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires(std::forward_iterator<I1> || std::sized_sentinel_for<S1, I1>) &&
            (std::forward_iterator<I2> || std::sized_sentinel_for<S2, I2>) &&
            std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    auto n1 = std::ranges::distance(first1, last1);
    auto n2 = std::ranges::distance(first2, last2);
    return impl(std::move(first1), last1, n1, std::move(first2), last2, n2, pred, proj1, proj2);
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires(std::ranges::forward_range<R1> || std::ranges::sized_range<R1>) &&
            (std::ranges::forward_range<R2> || std::ranges::sized_range<R2>) &&
            std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    auto n1 = std::ranges::distance(r1);
    auto n2 = std::ranges::distance(r2);
    return impl(std::ranges::begin(r1), std::ranges::end(r1), n1, std::ranges::begin(r2), std::ranges::end(r2), n2,
                pred, proj1, proj2);
  }
};

} // namespace ycxx::detail::ranges_algo

namespace ycxx::detail {
// [alg.fold]
template <class I, class S, class T, class F>
constexpr auto fold_left_impl(I first, S last, T init, F& f) {
  using U = std::decay_t<std::invoke_result_t<F&, T, std::iter_reference_t<I>>>;
  using R = std::ranges::fold_left_with_iter_result<I, U>;
  if (first == last)
    return R{std::move(first), U(std::move(init))};
  U accum = ::ycxx::detail::invoke(f, std::move(init), *first);
  for (++first; first != last; ++first)
    accum = ::ycxx::detail::invoke(f, std::move(accum), *first);
  return R{std::move(first), std::move(accum)};
}
template <class I, class S, class F>
constexpr auto fold_left_first_impl(I first, S last, F& f) {
  using U = std::decay_t<std::invoke_result_t<F&, std::iter_value_t<I>, std::iter_reference_t<I>>>;
  using R = std::ranges::fold_left_first_with_iter_result<I, std::optional<U>>;
  if (first == last)
    return R{std::move(first), std::optional<U>()};
  std::optional<U> init(std::in_place, *first);
  for (++first; first != last; ++first)
    *init = ::ycxx::detail::invoke(f, std::move(*init), *first);
  return R{std::move(first), std::move(init)};
}
template <class I, class S, class T, class F>
constexpr auto fold_right_impl(I first, S last, T init, F& f) {
  using U = std::decay_t<std::invoke_result_t<F&, std::iter_reference_t<I>, T>>;
  if (first == last)
    return U(std::move(init));
  I tail = ::ycxx::detail::iter_at(first, std::move(last));
  U accum = ::ycxx::detail::invoke(f, *--tail, std::move(init));
  while (first != tail)
    accum = ::ycxx::detail::invoke(f, *--tail, std::move(accum));
  return accum;
}
template <class I, class S, class F>
constexpr auto fold_right_last_impl(I first, S last, F& f) {
  using U = decltype(::ycxx::detail::fold_right_impl(first, last, std::iter_value_t<I>(*first), f));
  if (first == last)
    return std::optional<U>();
  I tail = std::ranges::prev(::ycxx::detail::iter_at(first, std::move(last)));
  return std::optional<U>(std::in_place,
                          ::ycxx::detail::fold_right_impl(std::move(first), tail, std::iter_value_t<I>(*tail), f));
}

} // namespace ycxx::detail

namespace ycxx::detail::ranges_algo {

struct fold_left_with_iter_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class T = std::iter_value_t<I>,
            ::ycxx::detail::indirectly_binary_left_foldable<T, I> F>
  constexpr auto operator()(I first, S last, T init, F f) const {
    return ::ycxx::detail::fold_left_impl(std::move(first), std::move(last), std::move(init), f);
  }
  template <std::ranges::input_range R, class T = std::ranges::range_value_t<R>,
            ::ycxx::detail::indirectly_binary_left_foldable<T, iterator_t<R>> F>
  constexpr auto operator()(R&& r, T init, F f) const {
    auto res = ::ycxx::detail::fold_left_impl(std::ranges::begin(r), std::ranges::end(r), std::move(init), f);
    return std::ranges::fold_left_with_iter_result<borrowed_iterator_t<R>, decltype(res.value)>{std::move(res.in),
                                                                                                std::move(res.value)};
  }
};
struct fold_left_first_with_iter_fn {
  template <std::input_iterator I, std::sentinel_for<I> S,
            ::ycxx::detail::indirectly_binary_left_foldable<std::iter_value_t<I>, I> F>
    requires std::constructible_from<std::iter_value_t<I>, std::iter_reference_t<I>>
  constexpr auto operator()(I first, S last, F f) const {
    return ::ycxx::detail::fold_left_first_impl(std::move(first), std::move(last), f);
  }
  template <std::ranges::input_range R,
            ::ycxx::detail::indirectly_binary_left_foldable<std::ranges::range_value_t<R>, iterator_t<R>> F>
    requires std::constructible_from<std::ranges::range_value_t<R>, std::ranges::range_reference_t<R>>
  constexpr auto operator()(R&& r, F f) const {
    auto res = ::ycxx::detail::fold_left_first_impl(std::ranges::begin(r), std::ranges::end(r), f);
    return std::ranges::fold_left_first_with_iter_result<borrowed_iterator_t<R>, decltype(res.value)>{
        std::move(res.in), std::move(res.value)};
  }
};
struct fold_left_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class T = std::iter_value_t<I>,
            ::ycxx::detail::indirectly_binary_left_foldable<T, I> F>
  constexpr auto operator()(I first, S last, T init, F f) const {
    return ::ycxx::detail::fold_left_impl(std::move(first), std::move(last), std::move(init), f).value;
  }
  template <std::ranges::input_range R, class T = std::ranges::range_value_t<R>,
            ::ycxx::detail::indirectly_binary_left_foldable<T, iterator_t<R>> F>
  constexpr auto operator()(R&& r, T init, F f) const {
    return ::ycxx::detail::fold_left_impl(std::ranges::begin(r), std::ranges::end(r), std::move(init), f).value;
  }
};
struct fold_left_first_fn {
  template <std::input_iterator I, std::sentinel_for<I> S,
            ::ycxx::detail::indirectly_binary_left_foldable<std::iter_value_t<I>, I> F>
    requires std::constructible_from<std::iter_value_t<I>, std::iter_reference_t<I>>
  constexpr auto operator()(I first, S last, F f) const {
    return ::ycxx::detail::fold_left_first_impl(std::move(first), std::move(last), f).value;
  }
  template <std::ranges::input_range R,
            ::ycxx::detail::indirectly_binary_left_foldable<std::ranges::range_value_t<R>, iterator_t<R>> F>
    requires std::constructible_from<std::ranges::range_value_t<R>, std::ranges::range_reference_t<R>>
  constexpr auto operator()(R&& r, F f) const {
    return ::ycxx::detail::fold_left_first_impl(std::ranges::begin(r), std::ranges::end(r), f).value;
  }
};
struct fold_right_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S, class T = std::iter_value_t<I>,
            ::ycxx::detail::indirectly_binary_right_foldable<T, I> F>
  constexpr auto operator()(I first, S last, T init, F f) const {
    return ::ycxx::detail::fold_right_impl(std::move(first), std::move(last), std::move(init), f);
  }
  template <std::ranges::bidirectional_range R, class T = std::ranges::range_value_t<R>,
            ::ycxx::detail::indirectly_binary_right_foldable<T, iterator_t<R>> F>
  constexpr auto operator()(R&& r, T init, F f) const {
    return ::ycxx::detail::fold_right_impl(std::ranges::begin(r), std::ranges::end(r), std::move(init), f);
  }
};
struct fold_right_last_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S,
            ::ycxx::detail::indirectly_binary_right_foldable<std::iter_value_t<I>, I> F>
    requires std::constructible_from<std::iter_value_t<I>, std::iter_reference_t<I>>
  constexpr auto operator()(I first, S last, F f) const {
    return ::ycxx::detail::fold_right_last_impl(std::move(first), std::move(last), f);
  }
  template <std::ranges::bidirectional_range R,
            ::ycxx::detail::indirectly_binary_right_foldable<std::ranges::range_value_t<R>, iterator_t<R>> F>
    requires std::constructible_from<std::ranges::range_value_t<R>, std::ranges::range_reference_t<R>>
  constexpr auto operator()(R&& r, F f) const {
    return ::ycxx::detail::fold_right_last_impl(std::ranges::begin(r), std::ranges::end(r), f);
  }
};

} // namespace ycxx::detail::ranges_algo

namespace std::ranges {
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::all_of_fn, ycxx::detail::par::kind::all_of> all_of{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::any_of_fn, ycxx::detail::par::kind::any_of> any_of{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::none_of_fn, ycxx::detail::par::kind::none_of> none_of{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::contains_fn, ycxx::detail::par::kind::contains> contains{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::contains_subrange_fn, ycxx::detail::par::kind::contains_subrange> contains_subrange{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::for_each_fn, ycxx::detail::par::kind::for_each> for_each{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::for_each_n_fn, ycxx::detail::par::kind::for_each_n> for_each_n{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::find_last_fn, ycxx::detail::par::kind::find_last> find_last{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::find_last_if_fn, ycxx::detail::par::kind::find_last_if> find_last_if{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::find_last_if_not_fn, ycxx::detail::par::kind::find_last_if_not> find_last_if_not{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::find_end_fn, ycxx::detail::par::kind::find_end> find_end{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::find_first_of_fn, ycxx::detail::par::kind::find_first_of> find_first_of{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::adjacent_find_fn, ycxx::detail::par::kind::adjacent_find> adjacent_find{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::count_fn, ycxx::detail::par::kind::count> count{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::count_if_fn, ycxx::detail::par::kind::count_if> count_if{};
inline constexpr ycxx::detail::ranges_algo::is_permutation_fn is_permutation{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::search_fn, ycxx::detail::par::kind::search> search{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::search_n_fn, ycxx::detail::par::kind::search_n> search_n{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::starts_with_fn, ycxx::detail::par::kind::starts_with> starts_with{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::ends_with_fn, ycxx::detail::par::kind::ends_with> ends_with{};
inline constexpr ycxx::detail::ranges_algo::fold_left_fn fold_left{};
inline constexpr ycxx::detail::ranges_algo::fold_left_first_fn fold_left_first{};
inline constexpr ycxx::detail::ranges_algo::fold_right_fn fold_right{};
inline constexpr ycxx::detail::ranges_algo::fold_right_last_fn fold_right_last{};
inline constexpr ycxx::detail::ranges_algo::fold_left_with_iter_fn fold_left_with_iter{};
inline constexpr ycxx::detail::ranges_algo::fold_left_first_with_iter_fn fold_left_first_with_iter{};
} // namespace std::ranges
