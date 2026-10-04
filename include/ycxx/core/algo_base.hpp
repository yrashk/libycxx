// libycxx core: the algorithm building blocks other headers need without all of <algorithm>
// ([alg.min.max] min/max/minmax/clamp and the element forms, [alg.copy], [alg.move], [alg.swap],
// [alg.fill], [alg.find], [alg.mismatch], [alg.equal], [alg.lex.comparison], [alg.three.way]),
// in both the std:: and the std::ranges:: forms, plus the shared machinery of all algorithms.
//
// Every algorithm is written once, as a ycxx::detail template, and serves both forms:
// - Ops selects how elements are moved and swapped. classic_ops uses std::move(*i) and
//   std::iter_swap (swap(*a, *b) with std::swap visible), as [algorithms] specifies for the
//   std:: forms; ranges_ops uses ranges::iter_move and ranges::iter_swap.
// - Predicates arrive wrapped: pred_ref calls a std:: predicate directly, proj_pred / proj_comp /
//   proj_comp2 apply the projections and INVOKE. Both convert the result to bool once, so a
//   boolean-testable result type is never asked for anything else (no user operator&&, !, ...).
// All internal calls are qualified, so user ADL on iterator or value types is never consulted.
#pragma once

#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/algo_results.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/swap.hpp>
#include <initializer_list>

namespace ycxx::detail {

// ---- iterator strength of the std:: forms --------------------------------------------------
// [algorithms.requirements]/4 (P2408): an iterator that models the C++20 concept may be used
// where the Cpp17 requirement is stated, so dispatch accepts either.
template <class I, class Tag>
concept cpp17_category_from = requires { typename std::iterator_traits<I>::iterator_category; } &&
                              std::derived_from<typename std::iterator_traits<I>::iterator_category, Tag>;
template <class I>
concept ra_iter = std::random_access_iterator<I> || cpp17_category_from<I, std::random_access_iterator_tag>;
template <class I>
concept bidi_iter = std::bidirectional_iterator<I> || cpp17_category_from<I, std::bidirectional_iterator_tag>;
template <class I>
concept fwd_iter = std::forward_iterator<I> || cpp17_category_from<I, std::forward_iterator_tag>;

// last - first when that is O(1), otherwise counted.
template <class I, class S>
constexpr std::iter_difference_t<I> range_length(I first, S last) {
  if constexpr (std::sized_sentinel_for<S, I> || (std::same_as<I, S> && ra_iter<I>)) {
    return static_cast<std::iter_difference_t<I>>(last - first);
  } else {
    std::iter_difference_t<I> n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

// The iterator at the sentinel: first itself when the sentinel is an iterator.
template <class I, class S>
constexpr I iter_at(I first, S last) {
  if constexpr (std::same_as<I, S>)
    return last;
  else
    return std::ranges::next(static_cast<I&&>(first), last);
}

// it + n for the std:: forms (Cpp17 random access or C++20 concept), else stepwise.
template <class I>
constexpr void iter_advance(I& it, std::iter_difference_t<I> n) {
  if constexpr (ra_iter<I>) {
    it += n;
  } else if constexpr (bidi_iter<I>) {
    for (; n > 0; --n)
      ++it;
    for (; n < 0; ++n)
      --it;
  } else {
    for (; n > 0; --n)
      ++it;
  }
}
template <class I>
constexpr I iter_next(I it, std::iter_difference_t<I> n) {
  ::ycxx::detail::iter_advance(it, n);
  return it;
}

// ---- element moves and swaps ---------------------------------------------------------------
struct classic_ops {
  template <class I>
  static constexpr decltype(auto) iter_move(I& it) {
    if constexpr (std::is_lvalue_reference_v<decltype(*it)>)
      return static_cast<std::remove_reference_t<decltype(*it)>&&>(*it);
    else
      return *it;
  }
  template <class I1, class I2>
  static constexpr void iter_swap(I1& a, I2& b) {
    ::ycxx::detail::swap_adl::do_swap(*a, *b);
  }
};
struct ranges_ops {
  template <class I>
  static constexpr decltype(auto) iter_move(I& it) {
    return std::ranges::iter_move(it);
  }
  template <class I1, class I2>
  static constexpr void iter_swap(I1& a, I2& b) {
    std::ranges::iter_swap(a, b);
  }
};

// ---- predicate wrappers ---------------------------------------------------------------------
// A std:: predicate or comparator, called directly. rev(b, a) calls it with the operands of
// the second range first (merge and the set operations).
template <class F>
struct pred_ref {
  F& f;
  template <class... A>
  constexpr bool operator()(A&&... a) const {
    return static_cast<bool>(f(static_cast<A&&>(a)...));
  }
  template <class A, class B>
  constexpr bool rev(A&& a, B&& b) const {
    return static_cast<bool>(f(static_cast<A&&>(a), static_cast<B&&>(b)));
  }
};
template <class F>
constexpr pred_ref<F> ref_pred(F& f) noexcept {
  return {f};
}

// invoke(pred, invoke(proj, x)).
template <class Pred, class Proj>
struct proj_pred {
  Pred& pred;
  Proj& proj;
  template <class A>
  constexpr bool operator()(A&& a) const {
    return static_cast<bool>(::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, static_cast<A&&>(a))));
  }
};
template <class Pred, class Proj>
constexpr proj_pred<Pred, Proj> make_pred(Pred& pred, Proj& proj) noexcept {
  return {pred, proj};
}

// invoke(comp, invoke(proj, a), invoke(proj, b)): one range, one projection.
template <class Comp, class Proj>
struct proj_comp {
  Comp& comp;
  Proj& proj;
  template <class A, class B>
  constexpr bool operator()(A&& a, B&& b) const {
    return static_cast<bool>(::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, static_cast<A&&>(a)),
                                                    ::ycxx::detail::invoke(proj, static_cast<B&&>(b))));
  }
  template <class A, class B>
  constexpr bool rev(A&& a, B&& b) const {
    return (*this)(static_cast<A&&>(a), static_cast<B&&>(b));
  }
};
template <class Comp, class Proj>
constexpr proj_comp<Comp, Proj> make_comp(Comp& comp, Proj& proj) noexcept {
  return {comp, proj};
}

// Two ranges: (*this)(x1, x2) projects x1 with proj1 and x2 with proj2; rev(x2, x1) calls
// comp(proj2(x2), proj1(x1)).
template <class Comp, class P1, class P2>
struct proj_comp2 {
  Comp& comp;
  P1& p1;
  P2& p2;
  template <class A, class B>
  constexpr bool operator()(A&& a, B&& b) const {
    return static_cast<bool>(::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(p1, static_cast<A&&>(a)),
                                                    ::ycxx::detail::invoke(p2, static_cast<B&&>(b))));
  }
  template <class B, class A>
  constexpr bool rev(B&& b, A&& a) const {
    return static_cast<bool>(::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(p2, static_cast<B&&>(b)),
                                                    ::ycxx::detail::invoke(p1, static_cast<A&&>(a))));
  }
};
template <class Comp, class P1, class P2>
constexpr proj_comp2<Comp, P1, P2> make_comp2(Comp& comp, P1& p1, P2& p2) noexcept {
  return {comp, p1, p2};
}

// invoke(proj, x) == value, for find / count / remove / replace with a value.
template <class T, class Proj>
struct equals_value {
  const T& value;
  Proj& proj;
  template <class A>
  constexpr bool operator()(A&& a) const {
    return static_cast<bool>(::ycxx::detail::invoke(proj, static_cast<A&&>(a)) == value);
  }
};
// *i == value for the std:: forms.
template <class T>
struct equals_value_plain {
  const T& value;
  template <class A>
  constexpr bool operator()(A&& a) const {
    return static_cast<bool>(static_cast<A&&>(a) == value);
  }
};
// !pred(x)
template <class P>
struct negated {
  P p;
  template <class A>
  constexpr bool operator()(A&& a) const {
    return !p(static_cast<A&&>(a));
  }
};

// ---- bulk copies of trivially copyable elements --------------------------------------------
// memmove is used only outside constant evaluation, for more than one element (a single
// element may be a potentially-overlapping subobject whose tail padding holds other data),
// when the element assignment it replaces is trivial. Ref is the source expression's type.
template <class I, class O, class Ref>
concept memmovable_pair =
    std::contiguous_iterator<I> && std::contiguous_iterator<O> &&
    (std::same_as<std::remove_reference_t<std::iter_reference_t<I>>, std::iter_value_t<O>> ||
     std::same_as<std::remove_reference_t<std::iter_reference_t<I>>, const std::iter_value_t<O>>) &&
    std::same_as<std::iter_reference_t<O>, std::iter_value_t<O>&> && std::is_trivially_copyable_v<std::iter_value_t<O>> &&
    std::is_trivially_assignable_v<std::iter_value_t<O>&, Ref>;

template <class I>
constexpr auto raw_address(const I& it) noexcept {
  if constexpr (std::is_pointer_v<I>)
    return it;
  else
    return std::to_address(it);
}
// Copies [in_first, in_last) to [out_first, out_last) through pointers. As P3349 requires of
// such lowering, the ends are reached by advancing the iterators (in_first + n, ...) and all
// four go through to_address, so a checked contiguous iterator still sees them.
template <class I, class O>
constexpr void bulk_move(const I& in_first, const I& in_last, const O& out_first, const O& out_last) noexcept {
  auto src = ::ycxx::detail::raw_address(in_first);
  auto src_end = ::ycxx::detail::raw_address(in_last);
  auto dst = ::ycxx::detail::raw_address(out_first);
  (void)::ycxx::detail::raw_address(out_last);
  // void* arguments: the builtin is found by unqualified lookup, so typed pointers would make
  // ADL complete their pointee classes.
  __builtin_memmove(const_cast<void*>(static_cast<const volatile void*>(dst)),
                    const_cast<const void*>(static_cast<const volatile void*>(src)),
                    static_cast<std::size_t>(src_end - src) * sizeof(std::iter_value_t<O>));
}

// ---- min / max -------------------------------------------------------------------------------
template <class I, class S, class C>
constexpr I min_element_impl(I first, S last, C less) {
  if (first == last)
    return first;
  I best = first;
  while (++first != last)
    if (less(*first, *best))
      best = first;
  return best;
}
template <class I, class S, class C>
constexpr I max_element_impl(I first, S last, C less) {
  if (first == last)
    return first;
  I best = first;
  while (++first != last)
    if (less(*best, *first))
      best = first;
  return best;
}
// The leftmost smallest and the rightmost largest, in at most 3/2 (N - 1) comparisons:
// elements are taken in pairs, ordered against each other, then the smaller of the pair is
// compared with the minimum and the larger with the maximum.
template <class I, class S, class C>
constexpr std::pair<I, I> minmax_element_impl(I first, S last, C less) {
  I lo = first, hi = first;
  if (first == last || ++first == last)
    return {lo, hi};
  if (less(*first, *lo))
    lo = first;
  else
    hi = first;
  while (++first != last) {
    I a = first;
    if (++first == last) {
      if (less(*a, *lo))
        lo = a;
      else if (!less(*a, *hi))
        hi = a;
      break;
    }
    if (less(*first, *a)) { // first < a: first is the smaller, a the larger
      if (less(*first, *lo))
        lo = first;
      if (!less(*a, *hi))
        hi = a;
    } else {
      if (less(*a, *lo))
        lo = a;
      if (!less(*first, *hi))
        hi = first;
    }
  }
  return {lo, hi};
}

// ---- copy / move -----------------------------------------------------------------------------
// A count argument of the std:: forms ("Size is convertible to an integral type").
// Taken by non-const reference: a class type may convert only through a non-const function.
template <class Size>
constexpr auto integral_count(Size& n) {
  if constexpr (std::is_integral_v<Size> && !std::is_same_v<std::remove_cv_t<Size>, bool>)
    return n;
  else
    return static_cast<long long>(n);
}

template <class I, class S, class O>
constexpr std::pair<I, O> copy_dispatch(I first, S last, O result) {
  if constexpr (memmovable_pair<I, O, std::iter_reference_t<I>> && std::sized_sentinel_for<S, I>) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        I in_last = first + n;
        O out_last = result + n;
        ::ycxx::detail::bulk_move(first, in_last, result, out_last);
        return {static_cast<I&&>(in_last), static_cast<O&&>(out_last)};
      }
    }
  }
  for (; first != last; (void)++first, (void)++result)
    *result = *first;
  return {static_cast<I&&>(first), static_cast<O&&>(result)};
}

// Moves elements; with Ops = ranges_ops through ranges::iter_move (customizable), so the bulk
// path is taken only for pointers there.
template <class Ops, class I, class S, class O>
constexpr std::pair<I, O> move_dispatch(I first, S last, O result) {
  if constexpr (memmovable_pair<I, O, std::iter_rvalue_reference_t<I>> && std::sized_sentinel_for<S, I> &&
                (std::same_as<Ops, classic_ops> || (std::is_pointer_v<I> && std::is_pointer_v<O>))) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        I in_last = first + n;
        O out_last = result + n;
        ::ycxx::detail::bulk_move(first, in_last, result, out_last);
        return {static_cast<I&&>(in_last), static_cast<O&&>(out_last)};
      }
    }
  }
  for (; first != last; (void)++first, (void)++result)
    *result = Ops::iter_move(first);
  return {static_cast<I&&>(first), static_cast<O&&>(result)};
}

template <class I, class O>
constexpr O copy_backward_dispatch(I first, I last, O result) {
  if constexpr (memmovable_pair<I, O, std::iter_reference_t<I>>) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        O out_first = result - n;
        ::ycxx::detail::bulk_move(first, first + n, out_first, result);
        return out_first;
      }
    }
  }
  while (first != last)
    *--result = *--last;
  return result;
}
template <class Ops, class I, class O>
constexpr O move_backward_dispatch(I first, I last, O result) {
  if constexpr (memmovable_pair<I, O, std::iter_rvalue_reference_t<I>> &&
                (std::same_as<Ops, classic_ops> || (std::is_pointer_v<I> && std::is_pointer_v<O>))) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        O out_first = result - n;
        ::ycxx::detail::bulk_move(first, first + n, out_first, result);
        return out_first;
      }
    }
  }
  while (first != last)
    *--result = Ops::iter_move(--last);
  return result;
}

// ---- find / mismatch / equal / lexicographical compare --------------------------------------
template <class I, class S, class P>
constexpr I find_if_impl(I first, S last, P pred) {
  for (; first != last; ++first)
    if (pred(*first))
      break;
  return first;
}

template <class I, class S, class P>
constexpr I adjacent_find_impl(I first, S last, P pred) {
  if (first == last)
    return first;
  I next = first;
  while (++next != last) {
    if (pred(*first, *next))
      return first;
    first = next;
  }
  return next;
}

template <class I1, class S1, class I2, class S2, class P>
constexpr std::pair<I1, I2> mismatch_impl(I1 first1, S1 last1, I2 first2, S2 last2, P eq) {
  while (first1 != last1 && first2 != last2 && eq(*first1, *first2)) {
    ++first1;
    ++first2;
  }
  return {static_cast<I1&&>(first1), static_cast<I2&&>(first2)};
}
// The three-iterator form: the second range is as long as the first.
template <class I1, class S1, class I2, class P>
constexpr std::pair<I1, I2> mismatch3_impl(I1 first1, S1 last1, I2 first2, P eq) {
  while (first1 != last1 && eq(*first1, *first2)) {
    ++first1;
    ++first2;
  }
  return {static_cast<I1&&>(first1), static_cast<I2&&>(first2)};
}

template <class I1, class S1, class I2, class S2, class P>
constexpr bool equal_impl(I1 first1, S1 last1, I2 first2, S2 last2, P eq) {
  // [alg.equal]/5: no comparisons when the lengths are known to differ.
  if constexpr ((std::sized_sentinel_for<S1, I1> || (std::same_as<I1, S1> && ra_iter<I1>)) &&
                (std::sized_sentinel_for<S2, I2> || (std::same_as<I2, S2> && ra_iter<I2>))) {
    if (last1 - first1 != last2 - first2)
      return false;
    for (; first1 != last1; (void)++first1, (void)++first2)
      if (!eq(*first1, *first2))
        return false;
    return true;
  } else {
    for (; first1 != last1 && first2 != last2; (void)++first1, (void)++first2)
      if (!eq(*first1, *first2))
        return false;
    return first1 == last1 && first2 == last2;
  }
}

template <class I1, class S1, class I2, class S2, class C>
constexpr bool lex_compare_impl(I1 first1, S1 last1, I2 first2, S2 last2, C less) {
  for (; first2 != last2; (void)++first1, (void)++first2) {
    if (first1 == last1 || less(*first1, *first2))
      return true;
    if (less.rev(*first2, *first1))
      return false;
  }
  return false;
}

template <class T>
concept comparison_category = !std::is_void_v<std::common_comparison_category_t<T>>;

} // namespace ycxx::detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace std {

// [alg.min.max]
template <class T>
[[nodiscard]] constexpr const T& min(const T& a, const T& b) {
  return b < a ? b : a;
}
template <class T, class Compare>
[[nodiscard]] constexpr const T& min(const T& a, const T& b, Compare comp) {
  return comp(b, a) ? b : a;
}
template <class T>
[[nodiscard]] constexpr const T& max(const T& a, const T& b) {
  return a < b ? b : a;
}
template <class T, class Compare>
[[nodiscard]] constexpr const T& max(const T& a, const T& b, Compare comp) {
  return comp(a, b) ? b : a;
}
template <class T, class Compare>
[[nodiscard]] constexpr T min(initializer_list<T> r, Compare comp) {
  ycxx::detail::precondition(r.size() != 0, "std::min: empty initializer_list");
  return *::ycxx::detail::min_element_impl(r.begin(), r.end(), ::ycxx::detail::ref_pred(comp));
}
template <class T>
[[nodiscard]] constexpr T min(initializer_list<T> r) {
  return std::min(r, less<>{});
}
template <class T, class Compare>
[[nodiscard]] constexpr T max(initializer_list<T> r, Compare comp) {
  ycxx::detail::precondition(r.size() != 0, "std::max: empty initializer_list");
  return *::ycxx::detail::max_element_impl(r.begin(), r.end(), ::ycxx::detail::ref_pred(comp));
}
template <class T>
[[nodiscard]] constexpr T max(initializer_list<T> r) {
  return std::max(r, less<>{});
}
template <class T>
[[nodiscard]] constexpr pair<const T&, const T&> minmax(const T& a, const T& b) {
  if (b < a)
    return pair<const T&, const T&>(b, a);
  return pair<const T&, const T&>(a, b);
}
template <class T, class Compare>
[[nodiscard]] constexpr pair<const T&, const T&> minmax(const T& a, const T& b, Compare comp) {
  if (comp(b, a))
    return pair<const T&, const T&>(b, a);
  return pair<const T&, const T&>(a, b);
}
template <class T, class Compare>
[[nodiscard]] constexpr pair<T, T> minmax(initializer_list<T> r, Compare comp) {
  ycxx::detail::precondition(r.size() != 0, "std::minmax: empty initializer_list");
  auto p = ::ycxx::detail::minmax_element_impl(r.begin(), r.end(), ::ycxx::detail::ref_pred(comp));
  return pair<T, T>(*p.first, *p.second);
}
template <class T>
[[nodiscard]] constexpr pair<T, T> minmax(initializer_list<T> r) {
  return std::minmax(r, less<>{});
}

template <class T, class Compare>
[[nodiscard]] constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp) {
  return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}
template <class T>
[[nodiscard]] constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
  return std::clamp(v, lo, hi, less<>{});
}

template <class ForwardIterator, class Compare>
[[nodiscard]] constexpr ForwardIterator min_element(ForwardIterator first, ForwardIterator last, Compare comp) {
  return ::ycxx::detail::min_element_impl(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class ForwardIterator>
[[nodiscard]] constexpr ForwardIterator min_element(ForwardIterator first, ForwardIterator last) {
  return std::min_element(first, last, less<>{});
}
template <class ForwardIterator, class Compare>
[[nodiscard]] constexpr ForwardIterator max_element(ForwardIterator first, ForwardIterator last, Compare comp) {
  return ::ycxx::detail::max_element_impl(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class ForwardIterator>
[[nodiscard]] constexpr ForwardIterator max_element(ForwardIterator first, ForwardIterator last) {
  return std::max_element(first, last, less<>{});
}
template <class ForwardIterator, class Compare>
[[nodiscard]] constexpr pair<ForwardIterator, ForwardIterator> minmax_element(ForwardIterator first, ForwardIterator last,
                                                                              Compare comp) {
  return ::ycxx::detail::minmax_element_impl(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class ForwardIterator>
[[nodiscard]] constexpr pair<ForwardIterator, ForwardIterator> minmax_element(ForwardIterator first, ForwardIterator last) {
  return std::minmax_element(first, last, less<>{});
}

// [alg.swap]
template <class ForwardIterator1, class ForwardIterator2>
constexpr void iter_swap(ForwardIterator1 a, ForwardIterator2 b) {
  ::ycxx::detail::swap_adl::do_swap(*a, *b);
}
template <class ForwardIterator1, class ForwardIterator2>
constexpr ForwardIterator2 swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2) {
  for (; first1 != last1; (void)++first1, (void)++first2)
    ::ycxx::detail::swap_adl::do_swap(*first1, *first2);
  return first2;
}

// [alg.copy]
template <class InputIterator, class OutputIterator>
constexpr OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result) {
  return ::ycxx::detail::copy_dispatch(first, last, result).second;
}
template <class InputIterator, class Size, class OutputIterator>
constexpr OutputIterator copy_n(InputIterator first, Size n, OutputIterator result) {
  auto count = ::ycxx::detail::integral_count(n);
  if (count <= 0)
    return result;
  if constexpr (::ycxx::detail::ra_iter<InputIterator>) {
    return ::ycxx::detail::copy_dispatch(first, first + static_cast<std::iter_difference_t<InputIterator>>(count),
                                         result)
        .second;
  } else {
    // [alg.copy]/13: exactly N assignments; do not step the input past the last element read.
    *result = *first;
    ++result;
    while (--count > 0) {
      ++first;
      *result = *first;
      ++result;
    }
    return result;
  }
}
template <class InputIterator, class OutputIterator, class Predicate>
constexpr OutputIterator copy_if(InputIterator first, InputIterator last, OutputIterator result, Predicate pred) {
  for (; first != last; ++first)
    if (pred(*first)) {
      *result = *first;
      ++result;
    }
  return result;
}
template <class BidirectionalIterator1, class BidirectionalIterator2>
constexpr BidirectionalIterator2 copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last,
                                               BidirectionalIterator2 result) {
  return ::ycxx::detail::copy_backward_dispatch(first, last, result);
}

// [alg.move]
template <class InputIterator, class OutputIterator>
constexpr OutputIterator move(InputIterator first, InputIterator last, OutputIterator result) {
  return ::ycxx::detail::move_dispatch<ycxx::detail::classic_ops>(first, last, result).second;
}
template <class BidirectionalIterator1, class BidirectionalIterator2>
constexpr BidirectionalIterator2 move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last,
                                               BidirectionalIterator2 result) {
  return ::ycxx::detail::move_backward_dispatch<ycxx::detail::classic_ops>(first, last, result);
}

// [alg.fill]
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
constexpr void fill(ForwardIterator first, ForwardIterator last, const T& value) {
  for (; first != last; ++first)
    *first = value;
}
template <class OutputIterator, class Size, class T = typename iterator_traits<OutputIterator>::value_type>
constexpr OutputIterator fill_n(OutputIterator first, Size n, const T& value) {
  for (auto count = ::ycxx::detail::integral_count(n); count > 0; --count) {
    *first = value;
    ++first;
  }
  return first;
}

// [alg.find]
template <class InputIterator, class T = typename iterator_traits<InputIterator>::value_type>
[[nodiscard]] constexpr InputIterator find(InputIterator first, InputIterator last, const T& value) {
  return ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::equals_value_plain<T>{value});
}
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr InputIterator find_if(InputIterator first, InputIterator last, Predicate pred) {
  return ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::ref_pred(pred));
}
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr InputIterator find_if_not(InputIterator first, InputIterator last, Predicate pred) {
  return ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::negated{::ycxx::detail::ref_pred(pred)});
}

// [alg.mismatch]
template <class InputIterator1, class InputIterator2, class BinaryPredicate>
[[nodiscard]] constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                                      InputIterator2 first2, BinaryPredicate pred) {
  return ::ycxx::detail::mismatch3_impl(first1, last1, first2, ::ycxx::detail::ref_pred(pred));
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                                      InputIterator2 first2) {
  return std::mismatch(first1, last1, first2, equal_to<>{});
}
template <class InputIterator1, class InputIterator2, class BinaryPredicate>
[[nodiscard]] constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                                      InputIterator2 first2, InputIterator2 last2,
                                                                      BinaryPredicate pred) {
  return ::ycxx::detail::mismatch_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(pred));
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1,
                                                                      InputIterator2 first2, InputIterator2 last2) {
  return std::mismatch(first1, last1, first2, last2, equal_to<>{});
}

// [alg.equal]
template <class InputIterator1, class InputIterator2, class BinaryPredicate>
[[nodiscard]] constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                   BinaryPredicate pred) {
  return ::ycxx::detail::mismatch3_impl(first1, last1, first2, ::ycxx::detail::ref_pred(pred)).first == last1;
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2) {
  return std::equal(first1, last1, first2, equal_to<>{});
}
template <class InputIterator1, class InputIterator2, class BinaryPredicate>
[[nodiscard]] constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                   InputIterator2 last2, BinaryPredicate pred) {
  return ::ycxx::detail::equal_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(pred));
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                   InputIterator2 last2) {
  return std::equal(first1, last1, first2, last2, equal_to<>{});
}

// [alg.lex.comparison]
template <class InputIterator1, class InputIterator2, class Compare>
[[nodiscard]] constexpr bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                                                     InputIterator2 first2, InputIterator2 last2, Compare comp) {
  return ::ycxx::detail::lex_compare_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(comp));
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                                                     InputIterator2 first2, InputIterator2 last2) {
  return std::lexicographical_compare(first1, last1, first2, last2, less<>{});
}

// [alg.three.way]
template <class InputIterator1, class InputIterator2, class Cmp>
[[nodiscard]] constexpr auto lexicographical_compare_three_way(InputIterator1 b1, InputIterator1 e1,
                                                               InputIterator2 b2, InputIterator2 e2, Cmp comp)
    -> decltype(comp(*b1, *b2)) {
  using R = decltype(comp(*b1, *b2));
  static_assert(ycxx::detail::comparison_category<R>,
                "std::lexicographical_compare_three_way: comp must return a comparison category type");
  for (; b1 != e1 && b2 != e2; (void)++b1, (void)++b2)
    if (auto c = comp(*b1, *b2); c != 0)
      return c;
  return b1 != e1 ? R(strong_ordering::greater) : b2 != e2 ? R(strong_ordering::less) : R(strong_ordering::equal);
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr auto lexicographical_compare_three_way(InputIterator1 b1, InputIterator1 e1,
                                                               InputIterator2 b2, InputIterator2 e2) {
  return std::lexicographical_compare_three_way(b1, e1, b2, e2, compare_three_way());
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace std::ranges {

template <class I, class O>
using copy_result = in_out_result<I, O>;
template <class I, class O>
using copy_n_result = in_out_result<I, O>;
template <class I, class O>
using copy_if_result = in_out_result<I, O>;
template <class I1, class I2>
using copy_backward_result = in_out_result<I1, I2>;
template <class I, class O>
using move_result = in_out_result<I, O>;
template <class I1, class I2>
using move_backward_result = in_out_result<I1, I2>;
template <class I1, class I2>
using swap_ranges_result = in_in_result<I1, I2>;
template <class I1, class I2>
using mismatch_result = in_in_result<I1, I2>;
template <class T>
using minmax_result = min_max_result<T>;
template <class I>
using minmax_element_result = min_max_result<I>;

} // namespace std::ranges

namespace ycxx::detail::ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::iterator_t;

// [alg.min.max]
struct min_fn {
  template <class T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr const T& operator()(const T& a, const T& b, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, b), ::ycxx::detail::invoke(proj, a)) ? b : a;
  }
  template <std::copyable T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr T operator()(std::initializer_list<T> r, Comp comp = {}, Proj proj = {}) const {
    ::ycxx::detail::precondition(r.size() != 0, "ranges::min: empty range");
    return *::ycxx::detail::min_element_impl(r.begin(), r.end(), ::ycxx::detail::make_comp(comp, proj));
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires std::indirectly_copyable_storable<iterator_t<R>, std::ranges::range_value_t<R>*>
  [[nodiscard]] constexpr std::ranges::range_value_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    ::ycxx::detail::precondition(first != last, "ranges::min: empty range");
    auto less = ::ycxx::detail::make_comp(comp, proj);
    if constexpr (std::ranges::forward_range<R>) {
      return static_cast<std::ranges::range_value_t<R>>(*::ycxx::detail::min_element_impl(first, last, less));
    } else {
      std::ranges::range_value_t<R> best(*first);
      while (++first != last) {
        decltype(auto) x = *first;
        if (less(x, best))
          best = static_cast<decltype(x)&&>(x);
      }
      return best;
    }
  }
};
struct max_fn {
  template <class T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr const T& operator()(const T& a, const T& b, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, a), ::ycxx::detail::invoke(proj, b)) ? b : a;
  }
  template <std::copyable T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr T operator()(std::initializer_list<T> r, Comp comp = {}, Proj proj = {}) const {
    ::ycxx::detail::precondition(r.size() != 0, "ranges::max: empty range");
    return *::ycxx::detail::max_element_impl(r.begin(), r.end(), ::ycxx::detail::make_comp(comp, proj));
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires std::indirectly_copyable_storable<iterator_t<R>, std::ranges::range_value_t<R>*>
  [[nodiscard]] constexpr std::ranges::range_value_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    ::ycxx::detail::precondition(first != last, "ranges::max: empty range");
    auto less = ::ycxx::detail::make_comp(comp, proj);
    if constexpr (std::ranges::forward_range<R>) {
      return static_cast<std::ranges::range_value_t<R>>(*::ycxx::detail::max_element_impl(first, last, less));
    } else {
      std::ranges::range_value_t<R> best(*first);
      while (++first != last) {
        decltype(auto) x = *first;
        if (less(best, x))
          best = static_cast<decltype(x)&&>(x);
      }
      return best;
    }
  }
};
struct minmax_fn {
  template <class T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_result<const T&> operator()(const T& a, const T& b, Comp comp = {},
                                                                         Proj proj = {}) const {
    if (::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, b), ::ycxx::detail::invoke(proj, a)))
      return {b, a};
    return {a, b};
  }
  template <std::copyable T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_result<T> operator()(std::initializer_list<T> r, Comp comp = {},
                                                                  Proj proj = {}) const {
    ::ycxx::detail::precondition(r.size() != 0, "ranges::minmax: empty range");
    auto p = ::ycxx::detail::minmax_element_impl(r.begin(), r.end(), ::ycxx::detail::make_comp(comp, proj));
    return {*p.first, *p.second};
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
    requires std::indirectly_copyable_storable<iterator_t<R>, std::ranges::range_value_t<R>*>
  [[nodiscard]] constexpr std::ranges::minmax_result<std::ranges::range_value_t<R>> operator()(R&& r, Comp comp = {},
                                                                                              Proj proj = {}) const {
    using V = std::ranges::range_value_t<R>;
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    ::ycxx::detail::precondition(first != last, "ranges::minmax: empty range");
    auto less = ::ycxx::detail::make_comp(comp, proj);
    if constexpr (std::ranges::forward_range<R>) {
      auto p = ::ycxx::detail::minmax_element_impl(first, last, less);
      // Each element is read once: *it may move from it (move_iterator).
      V lo(*p.first);
      if (p.first == p.second)
        return {lo, lo};
      return {std::move(lo), static_cast<V>(*p.second)};
    } else {
      // Single pass over copies: the leftmost smallest and the rightmost largest.
      V lo(*first);
      V hi(lo);
      while (++first != last) {
        V a(*first);
        if (++first == last) {
          if (less(a, lo))
            lo = std::move(a);
          else if (!less(a, hi))
            hi = std::move(a);
          break;
        }
        V b(*first);
        if (less(b, a)) {
          if (less(b, lo))
            lo = std::move(b);
          if (!less(a, hi))
            hi = std::move(a);
        } else {
          if (less(a, lo))
            lo = std::move(a);
          if (!less(b, hi))
            hi = std::move(b);
        }
      }
      return {std::move(lo), std::move(hi)};
    }
  }
};
struct clamp_fn {
  template <class T, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const T*, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr const T& operator()(const T& v, const T& lo, const T& hi, Comp comp = {}, Proj proj = {}) const {
    // proj(v) is computed once ("at most three applications of the projection") and passed
    // on with its value category; a prvalue result is passed as an lvalue, so that a
    // comparator taking its parameters by value cannot move from it twice.
    using PV = decltype(::ycxx::detail::invoke(proj, v));
    using Arg = std::conditional_t<std::is_reference_v<PV>, PV, PV&>;
    auto&& pv = ::ycxx::detail::invoke(proj, v);
    if (::ycxx::detail::invoke(comp, static_cast<Arg>(pv), ::ycxx::detail::invoke(proj, lo)))
      return lo;
    if (::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, hi), static_cast<Arg>(pv)))
      return hi;
    return v;
  }
};

struct min_element_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::min_element_impl(std::move(first), last, ::ycxx::detail::make_comp(comp, proj));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::min_element_impl(std::ranges::begin(r), std::ranges::end(r),
                                            ::ycxx::detail::make_comp(comp, proj));
  }
};
struct max_element_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::max_element_impl(std::move(first), last, ::ycxx::detail::make_comp(comp, proj));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::max_element_impl(std::ranges::begin(r), std::ranges::end(r),
                                            ::ycxx::detail::make_comp(comp, proj));
  }
};
struct minmax_element_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_element_result<I> operator()(I first, S last, Comp comp = {},
                                                                          Proj proj = {}) const {
    auto p = ::ycxx::detail::minmax_element_impl(std::move(first), last, ::ycxx::detail::make_comp(comp, proj));
    return {std::move(p.first), std::move(p.second)};
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_element_result<borrowed_iterator_t<R>> operator()(R&& r, Comp comp = {},
                                                                                               Proj proj = {}) const {
    auto p = ::ycxx::detail::minmax_element_impl(std::ranges::begin(r), std::ranges::end(r),
                                                 ::ycxx::detail::make_comp(comp, proj));
    return {std::move(p.first), std::move(p.second)};
  }
};

// [alg.swap]
struct swap_ranges_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2>
    requires std::indirectly_swappable<I1, I2>
  constexpr std::ranges::swap_ranges_result<I1, I2> operator()(I1 first1, S1 last1, I2 first2, S2 last2) const {
    for (; first1 != last1 && first2 != last2; (void)++first1, (void)++first2)
      std::ranges::iter_swap(first1, first2);
    return {std::move(first1), std::move(first2)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2>
    requires std::indirectly_swappable<iterator_t<R1>, iterator_t<R2>>
  constexpr std::ranges::swap_ranges_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>> operator()(R1&& r1,
                                                                                                        R2&& r2) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2));
  }
};

// [alg.copy]
struct copy_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O>
    requires std::indirectly_copyable<I, O>
  constexpr std::ranges::copy_result<I, O> operator()(I first, S last, O result) const {
    auto r = ::ycxx::detail::copy_dispatch(std::move(first), std::move(last), std::move(result));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O>
    requires std::indirectly_copyable<iterator_t<R>, O>
  constexpr std::ranges::copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};
struct copy_n_fn {
  template <std::input_iterator I, std::weakly_incrementable O>
    requires std::indirectly_copyable<I, O>
  constexpr std::ranges::copy_n_result<I, O> operator()(I first, std::iter_difference_t<I> n, O result) const {
    if constexpr (std::random_access_iterator<I>) {
      if (n <= 0)
        return {std::move(first), std::move(result)};
      auto r = ::ycxx::detail::copy_dispatch(first, first + n, std::move(result));
      return {std::move(r.first), std::move(r.second)};
    } else {
      for (; n > 0; (void)++first, (void)++result, --n)
        *result = *first;
      return {std::move(first), std::move(result)};
    }
  }
};
struct copy_if_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires std::indirectly_copyable<I, O>
  constexpr std::ranges::copy_if_result<I, O> operator()(I first, S last, O result, Pred pred, Proj proj = {}) const {
    auto p = ::ycxx::detail::make_pred(pred, proj);
    for (; first != last; ++first)
      if (p(*first)) {
        *result = *first;
        ++result;
      }
    return {std::move(first), std::move(result)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::indirectly_copyable<iterator_t<R>, O>
  constexpr std::ranges::copy_if_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result, Pred pred,
                                                                             Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(pred), std::move(proj));
  }
};
struct copy_backward_fn {
  template <std::bidirectional_iterator I1, std::sentinel_for<I1> S1, std::bidirectional_iterator I2>
    requires std::indirectly_copyable<I1, I2>
  constexpr std::ranges::copy_backward_result<I1, I2> operator()(I1 first, S1 last, I2 result) const {
    I1 end = ::ycxx::detail::iter_at(first, std::move(last));
    return {end, ::ycxx::detail::copy_backward_dispatch(std::move(first), end, std::move(result))};
  }
  template <std::ranges::bidirectional_range R, std::bidirectional_iterator I>
    requires std::indirectly_copyable<iterator_t<R>, I>
  constexpr std::ranges::copy_backward_result<borrowed_iterator_t<R>, I> operator()(R&& r, I result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};

// [alg.move]
struct move_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O>
    requires std::indirectly_movable<I, O>
  constexpr std::ranges::move_result<I, O> operator()(I first, S last, O result) const {
    auto r = ::ycxx::detail::move_dispatch<ranges_ops>(std::move(first), std::move(last), std::move(result));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O>
    requires std::indirectly_movable<iterator_t<R>, O>
  constexpr std::ranges::move_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};
struct move_backward_fn {
  template <std::bidirectional_iterator I1, std::sentinel_for<I1> S1, std::bidirectional_iterator I2>
    requires std::indirectly_movable<I1, I2>
  constexpr std::ranges::move_backward_result<I1, I2> operator()(I1 first, S1 last, I2 result) const {
    I1 end = ::ycxx::detail::iter_at(first, std::move(last));
    return {end, ::ycxx::detail::move_backward_dispatch<ranges_ops>(std::move(first), end, std::move(result))};
  }
  template <std::ranges::bidirectional_range R, std::bidirectional_iterator I>
    requires std::indirectly_movable<iterator_t<R>, I>
  constexpr std::ranges::move_backward_result<borrowed_iterator_t<R>, I> operator()(R&& r, I result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};

// [alg.fill]
struct fill_fn {
  template <class O, std::sentinel_for<O> S, class T = std::iter_value_t<O>>
    requires std::output_iterator<O, const T&>
  constexpr O operator()(O first, S last, const T& value) const {
    for (; first != last; ++first)
      *first = value;
    return first;
  }
  template <class R, class T = std::ranges::range_value_t<R>>
    requires std::ranges::output_range<R, const T&>
  constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value);
  }
};
struct fill_n_fn {
  template <class O, class T = std::iter_value_t<O>>
    requires std::output_iterator<O, const T&>
  constexpr O operator()(O first, std::iter_difference_t<O> n, const T& value) const {
    for (; n > 0; --n) {
      *first = value;
      ++first;
    }
    return first;
  }
};

// [alg.find]
struct find_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
  [[nodiscard]] constexpr I operator()(I first, S last, const T& value, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last, ::ycxx::detail::equals_value<T, Proj>{value, proj});
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<R>, Proj>, const T*>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::ranges::begin(r), std::ranges::end(r),
                                        ::ycxx::detail::equals_value<T, Proj>{value, proj});
  }
};
struct find_if_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last, ::ycxx::detail::make_pred(pred, proj));
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::ranges::begin(r), std::ranges::end(r), ::ycxx::detail::make_pred(pred, proj));
  }
};
struct find_if_not_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::move(first), last,
                                        ::ycxx::detail::negated{::ycxx::detail::make_pred(pred, proj)});
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return ::ycxx::detail::find_if_impl(std::ranges::begin(r), std::ranges::end(r),
                                        ::ycxx::detail::negated{::ycxx::detail::make_pred(pred, proj)});
  }
};

// [alg.mismatch]
struct mismatch_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr std::ranges::mismatch_result<I1, I2> operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                                                                         Pred pred = {}, Proj1 proj1 = {},
                                                                         Proj2 proj2 = {}) const {
    auto r = ::ycxx::detail::mismatch_impl(std::move(first1), last1, std::move(first2), last2,
                                           ::ycxx::detail::make_comp2(pred, proj1, proj2));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr std::ranges::mismatch_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>>
  operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    auto r = ::ycxx::detail::mismatch_impl(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2),
                                           std::ranges::end(r2), ::ycxx::detail::make_comp2(pred, proj1, proj2));
    return {std::move(r.first), std::move(r.second)};
  }
};

// [alg.equal]
struct equal_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            class Pred = std::ranges::equal_to, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    return ::ycxx::detail::equal_impl(std::move(first1), last1, std::move(first2), last2,
                                      ::ycxx::detail::make_comp2(pred, proj1, proj2));
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, class Pred = std::ranges::equal_to,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    if constexpr (std::ranges::sized_range<R1> && std::ranges::sized_range<R2>) {
      if (std::ranges::distance(r1) != std::ranges::distance(r2))
        return false;
    }
    return ::ycxx::detail::equal_impl(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2),
                                      std::ranges::end(r2), ::ycxx::detail::make_comp2(pred, proj1, proj2));
  }
};

// [alg.lex.comparison]
struct lexicographical_compare_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<I1, Proj1>, std::projected<I2, Proj2>> Comp =
                std::ranges::less>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    return ::ycxx::detail::lex_compare_impl(std::move(first1), last1, std::move(first2), last2,
                                            ::ycxx::detail::make_comp2(comp, proj1, proj2));
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, class Proj1 = std::identity,
            class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R1>, Proj1>, std::projected<iterator_t<R2>, Proj2>>
                Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return ::ycxx::detail::lex_compare_impl(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2),
                                            std::ranges::end(r2), ::ycxx::detail::make_comp2(comp, proj1, proj2));
  }
};

} // namespace ycxx::detail::ranges_algo

namespace std::ranges {
inline constexpr ycxx::detail::ranges_algo::min_fn min{};
inline constexpr ycxx::detail::ranges_algo::max_fn max{};
inline constexpr ycxx::detail::ranges_algo::minmax_fn minmax{};
inline constexpr ycxx::detail::ranges_algo::clamp_fn clamp{};
inline constexpr ycxx::detail::ranges_algo::min_element_fn min_element{};
inline constexpr ycxx::detail::ranges_algo::max_element_fn max_element{};
inline constexpr ycxx::detail::ranges_algo::minmax_element_fn minmax_element{};
inline constexpr ycxx::detail::ranges_algo::swap_ranges_fn swap_ranges{};
inline constexpr ycxx::detail::ranges_algo::copy_fn copy{};
inline constexpr ycxx::detail::ranges_algo::copy_n_fn copy_n{};
inline constexpr ycxx::detail::ranges_algo::copy_if_fn copy_if{};
inline constexpr ycxx::detail::ranges_algo::copy_backward_fn copy_backward{};
inline constexpr ycxx::detail::ranges_algo::move_fn move{};
inline constexpr ycxx::detail::ranges_algo::move_backward_fn move_backward{};
inline constexpr ycxx::detail::ranges_algo::fill_fn fill{};
inline constexpr ycxx::detail::ranges_algo::fill_n_fn fill_n{};
inline constexpr ycxx::detail::ranges_algo::find_fn find{};
inline constexpr ycxx::detail::ranges_algo::find_if_fn find_if{};
inline constexpr ycxx::detail::ranges_algo::find_if_not_fn find_if_not{};
inline constexpr ycxx::detail::ranges_algo::mismatch_fn mismatch{};
inline constexpr ycxx::detail::ranges_algo::equal_fn equal{};
inline constexpr ycxx::detail::ranges_algo::lexicographical_compare_fn lexicographical_compare{};
} // namespace std::ranges
