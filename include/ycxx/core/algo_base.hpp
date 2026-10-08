// libycxx core: the algorithm building blocks other headers need without all of <algorithm>
// ([alg.min.max] min/max/minmax/clamp and the element forms, [alg.copy], [alg.move], [alg.swap],
// [alg.fill], [alg.find], [alg.mismatch], [alg.equal], [alg.lex.comparison], [alg.three.way]),
// in both the std:: and the std::ranges:: forms, plus the shared machinery of all algorithms.
//
// Every algorithm is written once, as a __ycxx::__detail template, and serves both forms:
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
#include <ycxx/core/algo_ranges_parallel.hpp>
#include <ycxx/core/bit_iter_algos.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/mem_builtins.hpp>
#include <initializer_list>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- iterator strength of the std:: forms --------------------------------------------------
// [algorithms.requirements]/4 (P2408): an iterator that models the C++20 concept may be used
// where the Cpp17 requirement is stated, so dispatch accepts either.
template <class _Ip, class _Tag>
concept __cpp17_category_from = requires { typename std::iterator_traits<_Ip>::iterator_category; } &&
                              std::derived_from<typename std::iterator_traits<_Ip>::iterator_category, _Tag>;
template <class _Ip>
concept __ra_iter = std::random_access_iterator<_Ip> || __cpp17_category_from<_Ip, std::random_access_iterator_tag>;
template <class _Ip>
concept __bidi_iter = std::bidirectional_iterator<_Ip> || __cpp17_category_from<_Ip, std::bidirectional_iterator_tag>;
template <class _Ip>
concept __fwd_iter = std::forward_iterator<_Ip> || __cpp17_category_from<_Ip, std::forward_iterator_tag>;

// last - first when that is O(1), otherwise counted.
template <class _Ip, class _Sp>
constexpr std::iter_difference_t<_Ip> __range_length(_Ip first, _Sp last) {
  if constexpr (std::sized_sentinel_for<_Sp, _Ip> || (std::same_as<_Ip, _Sp> && __ra_iter<_Ip>)) {
    return static_cast<std::iter_difference_t<_Ip>>(last - first);
  } else {
    std::iter_difference_t<_Ip> n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

// The iterator at the sentinel: first itself when the sentinel is an iterator.
template <class _Ip, class _Sp>
constexpr _Ip __iter_at(_Ip first, _Sp last) {
  if constexpr (std::same_as<_Ip, _Sp>)
    return last;
  else
    return std::ranges::next(static_cast<_Ip&&>(first), last);
}

// it + n for the std:: forms (Cpp17 random access or C++20 concept), else stepwise.
template <class _Ip>
constexpr void __iter_advance(_Ip& __it, std::iter_difference_t<_Ip> n) {
  if constexpr (__ra_iter<_Ip>) {
    __it += n;
  } else if constexpr (__bidi_iter<_Ip>) {
    for (; n > 0; --n)
      ++__it;
    for (; n < 0; ++n)
      --__it;
  } else {
    for (; n > 0; --n)
      ++__it;
  }
}
template <class _Ip>
constexpr _Ip __iter_next(_Ip __it, std::iter_difference_t<_Ip> n) {
  ::__ycxx::__detail::__iter_advance(__it, n);
  return __it;
}

// ---- element moves and swaps ---------------------------------------------------------------
struct __classic_ops {
  template <class _Ip>
  static constexpr decltype(auto) iter_move(_Ip& __it) {
    if constexpr (std::is_lvalue_reference_v<decltype(*__it)>)
      return static_cast<std::remove_reference_t<decltype(*__it)>&&>(*__it);
    else
      return *__it;
  }
  template <class _I1, class _I2>
  static constexpr void iter_swap(_I1& a, _I2& b) {
    ::__ycxx::__detail::__swap_adl::__do_swap(*a, *b);
  }
};
struct __ranges_ops {
  template <class _Ip>
  static constexpr decltype(auto) iter_move(_Ip& __it) {
    return std::ranges::iter_move(__it);
  }
  template <class _I1, class _I2>
  static constexpr void iter_swap(_I1& a, _I2& b) {
    std::ranges::iter_swap(a, b);
  }
};

// ---- predicate wrappers ---------------------------------------------------------------------
// A std:: predicate or comparator, called directly. rev(b, a) calls it with the operands of
// the second range first (merge and the set operations).
template <class _Fp>
struct __pred_ref {
  _Fp& __f;
  template <class... _Ap>
  constexpr bool operator()(_Ap&&... a) const {
    return static_cast<bool>(__f(static_cast<_Ap&&>(a)...));
  }
  template <class _Ap, class _Bp>
  constexpr bool __rev(_Ap&& a, _Bp&& b) const {
    return static_cast<bool>(__f(static_cast<_Ap&&>(a), static_cast<_Bp&&>(b)));
  }
};
template <class _Fp>
constexpr __pred_ref<_Fp> __ref_pred(_Fp& __f) noexcept {
  return {__f};
}

// invoke(pred, invoke(proj, x)).
template <class _Pred, class _Proj>
struct __proj_pred {
  _Pred& pred;
  _Proj& proj;
  template <class _Ap>
  constexpr bool operator()(_Ap&& a) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, static_cast<_Ap&&>(a))));
  }
};
template <class _Pred, class _Proj>
constexpr __proj_pred<_Pred, _Proj> __make_pred(_Pred& pred, _Proj& proj) noexcept {
  return {pred, proj};
}

// invoke(comp, invoke(proj, a), invoke(proj, b)): one range, one projection.
template <class _Comp, class _Proj>
struct __proj_comp {
  _Comp& comp;
  _Proj& proj;
  template <class _Ap, class _Bp>
  constexpr bool operator()(_Ap&& a, _Bp&& b) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, static_cast<_Ap&&>(a)),
                                                    ::__ycxx::__detail::invoke(proj, static_cast<_Bp&&>(b))));
  }
  template <class _Ap, class _Bp>
  constexpr bool __rev(_Ap&& a, _Bp&& b) const {
    return (*this)(static_cast<_Ap&&>(a), static_cast<_Bp&&>(b));
  }
};
template <class _Comp, class _Proj>
constexpr __proj_comp<_Comp, _Proj> __make_comp(_Comp& comp, _Proj& proj) noexcept {
  return {comp, proj};
}

// Two ranges: (*this)(x1, x2) projects x1 with proj1 and x2 with proj2; rev(x2, x1) calls
// comp(proj2(x2), proj1(x1)).
template <class _Comp, class _P1, class _P2>
struct __proj_comp2 {
  _Comp& comp;
  _P1& __p1;
  _P2& __p2;
  template <class _Ap, class _Bp>
  constexpr bool operator()(_Ap&& a, _Bp&& b) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(__p1, static_cast<_Ap&&>(a)),
                                                    ::__ycxx::__detail::invoke(__p2, static_cast<_Bp&&>(b))));
  }
  template <class _Bp, class _Ap>
  constexpr bool __rev(_Bp&& b, _Ap&& a) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(__p2, static_cast<_Bp&&>(b)),
                                                    ::__ycxx::__detail::invoke(__p1, static_cast<_Ap&&>(a))));
  }
};
template <class _Comp, class _P1, class _P2>
constexpr __proj_comp2<_Comp, _P1, _P2> __make_comp2(_Comp& comp, _P1& __p1, _P2& __p2) noexcept {
  return {comp, __p1, __p2};
}

// fill, find and count work a word at a time on vector<bool>'s iterators (bit_iter_algos.hpp),
// for a bool value without a projection.
template <class _Ip, class _Sp, class _Tp, class _Proj = std::identity>
concept __bit_algo_args = __bit_algos<_Ip>::__enabled && std::same_as<_Ip, _Sp> && std::same_as<_Tp, bool> &&
                        std::same_as<_Proj, std::identity>;

// invoke(proj, x) == value, for find / count / remove / replace with a value.
template <class _Tp, class _Proj>
struct __equals_value {
  const _Tp& value;
  _Proj& proj;
  template <class _Ap>
  constexpr bool operator()(_Ap&& a) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(proj, static_cast<_Ap&&>(a)) == value);
  }
};
// *i == value for the std:: forms.
template <class _Tp>
struct __equals_value_plain {
  const _Tp& value;
  template <class _Ap>
  constexpr bool operator()(_Ap&& a) const {
    return static_cast<bool>(static_cast<_Ap&&>(a) == value);
  }
};
// !pred(x)
template <class _Pp>
struct __negated {
  _Pp p;
  template <class _Ap>
  constexpr bool operator()(_Ap&& a) const {
    return !p(static_cast<_Ap&&>(a));
  }
};

// ---- bulk copies of trivially copyable elements --------------------------------------------
// memmove is used only outside constant evaluation, for more than one element (a single
// element may be a potentially-overlapping subobject whose tail padding holds other data),
// when the element assignment it replaces is trivial. Ref is the source expression's type.
template <class _Ip, class _Op, class _Ref>
concept __memmovable_pair =
    std::contiguous_iterator<_Ip> && std::contiguous_iterator<_Op> &&
    (std::same_as<std::remove_reference_t<std::iter_reference_t<_Ip>>, std::iter_value_t<_Op>> ||
     std::same_as<std::remove_reference_t<std::iter_reference_t<_Ip>>, const std::iter_value_t<_Op>>) &&
    std::same_as<std::iter_reference_t<_Op>, std::iter_value_t<_Op>&> && std::is_trivially_copyable_v<std::iter_value_t<_Op>> &&
    std::is_trivially_assignable_v<std::iter_value_t<_Op>&, _Ref>;

template <class _Ip>
constexpr auto __raw_address(const _Ip& __it) noexcept {
  if constexpr (std::is_pointer_v<_Ip>)
    return __it;
  else
    return std::to_address(__it);
}
// Copies [in_first, in_last) to [out_first, out_last) through pointers. As P3349 requires of
// such lowering, the ends are reached by advancing the iterators (in_first + n, ...) and all
// four go through to_address, so a checked contiguous iterator still sees them.
template <class _Ip, class _Op>
constexpr void __bulk_move(const _Ip& __in_first, const _Ip& __in_last, const _Op& __out_first, const _Op& __out_last) noexcept {
  auto __src = ::__ycxx::__detail::__raw_address(__in_first);
  auto __src_end = ::__ycxx::__detail::__raw_address(__in_last);
  auto __dst = ::__ycxx::__detail::__raw_address(__out_first);
  (void)::__ycxx::__detail::__raw_address(__out_last);
  // void* arguments: the builtin is found by unqualified lookup, so typed pointers would make
  // ADL complete their pointee classes.
  __builtin_memmove(const_cast<void*>(static_cast<const volatile void*>(__dst)),
                    const_cast<const void*>(static_cast<const volatile void*>(__src)),
                    static_cast<std::size_t>(__src_end - __src) * sizeof(std::iter_value_t<_Op>));
}

// ---- byte search ------------------------------------------------------------------------------
// find over a contiguous range of narrow character elements for an integral value is memchr
// (outside constant evaluation). With e the value converted to the element type E: an element x
// equals the value exactly when x == e and e itself equals the value (both comparisons as the
// language performs them, after promotion). x and the value agree modulo 2^CHAR_BIT whenever
// they compare equal, so x == e then; and promotion of E to the common type is injective. So
// memchr for e when e == value, and no element can match otherwise.
template <class _Ep>
concept __narrow_char_elem = std::same_as<_Ep, char> || std::same_as<_Ep, signed char> || std::same_as<_Ep, unsigned char> ||
                           std::same_as<_Ep, char8_t>;
template <class _Ip, class _Sp, class _Tp>
concept __memchr_find_args =
    std::contiguous_iterator<_Ip> && std::sized_sentinel_for<_Sp, _Ip> &&
    __narrow_char_elem<std::remove_cvref_t<std::iter_reference_t<_Ip>>> &&
    std::is_lvalue_reference_v<std::iter_reference_t<_Ip>> &&
    !std::is_volatile_v<std::remove_reference_t<std::iter_reference_t<_Ip>>> && std::is_integral_v<_Tp> &&
    !std::is_same_v<_Tp, bool>;

// Advances first to the first element equal to value, or to last.
template <class _Ip, class _Sp, class _Tp>
constexpr void __find_byte(_Ip& first, const _Sp& last, const _Tp& value) noexcept {
  using _Ep = std::remove_cvref_t<std::iter_reference_t<_Ip>>;
  const auto n = last - first;
  if (n <= 0)
    return;
  const _Ep e = static_cast<_Ep>(value);
  if (!(e == value)) {
    first += n;
    return;
  }
  const _Ep* p = ::__ycxx::__detail::__raw_address(first);
  const void* r = ::__ycxx::__detail::__rt_memchr(static_cast<const void*>(p), static_cast<unsigned char>(e), static_cast<std::size_t>(n));
  first += r ? static_cast<const _Ep*>(r) - p : n;
}

// find over a contiguous range of wider integers (2, 4 or 8 bytes) for an integral value: blocks
// of 256 bytes are tested with a loop without an early exit, which the compilers vectorize, and
// only a block that holds a match is scanned element by element. The value is converted as in
// find_byte: an element equals the value exactly when it equals e = E(value) and e == value.
template <class _Ip, class _Sp, class _Tp>
concept __wide_find_args =
    std::contiguous_iterator<_Ip> && std::sized_sentinel_for<_Sp, _Ip> &&
    std::is_integral_v<std::remove_cvref_t<std::iter_reference_t<_Ip>>> &&
    !std::is_same_v<std::remove_cvref_t<std::iter_reference_t<_Ip>>, bool> &&
    (sizeof(std::remove_cvref_t<std::iter_reference_t<_Ip>>) == 2 || sizeof(std::remove_cvref_t<std::iter_reference_t<_Ip>>) == 4 ||
     sizeof(std::remove_cvref_t<std::iter_reference_t<_Ip>>) == 8) &&
    std::is_lvalue_reference_v<std::iter_reference_t<_Ip>> &&
    !std::is_volatile_v<std::remove_reference_t<std::iter_reference_t<_Ip>>> && std::is_integral_v<_Tp> &&
    !std::is_same_v<_Tp, bool>;

template <class _Ip, class _Sp, class _Tp>
constexpr void __find_wide(_Ip& first, const _Sp& last, const _Tp& value) noexcept {
  using _Ep = std::remove_cvref_t<std::iter_reference_t<_Ip>>;
  const auto n = last - first;
  if (n <= 0)
    return;
  const _Ep e = static_cast<_Ep>(value);
  if (!(e == value)) {
    first += n;
    return;
  }
  const _Ep* const p = ::__ycxx::__detail::__raw_address(first);
  constexpr std::ptrdiff_t _Bk = 256 / sizeof(_Ep);
  std::ptrdiff_t i = 0;
  for (; n - i >= _Bk; i += _Bk) {
    bool __any = false;
    for (std::ptrdiff_t __j = 0; __j < _Bk; ++__j)
      __any |= p[i + __j] == e;
    if (__any)
      break;
  }
  for (; i < n && !(p[i] == e); ++i) {
  }
  first += i;
}

// equal over two contiguous ranges of the same integral (or pointer) type, compared with ==:
// equal values are equal object representations for these types ([basic.fundamental]: no padding
// bits in the integral types the library supports; pointers compare equal when they represent the
// same address), so the ranges are equal exactly when their bytes are: memcmp.
template <class _Pp, class _Vp>
concept __plain_equal_pred = std::same_as<_Pp, std::equal_to<void>> || std::same_as<_Pp, std::equal_to<_Vp>> ||
                             std::same_as<_Pp, std::ranges::equal_to>;
template <class _I1, class _I2, class _Pp>
concept __memcmp_equal_args =
    std::contiguous_iterator<_I1> && std::contiguous_iterator<_I2> &&
    std::same_as<std::iter_value_t<_I1>, std::iter_value_t<_I2>> &&
    (std::is_integral_v<std::iter_value_t<_I1>> || std::is_pointer_v<std::iter_value_t<_I1>>) &&
    std::is_lvalue_reference_v<std::iter_reference_t<_I1>> && std::is_lvalue_reference_v<std::iter_reference_t<_I2>> &&
    !std::is_volatile_v<std::remove_reference_t<std::iter_reference_t<_I1>>> &&
    !std::is_volatile_v<std::remove_reference_t<std::iter_reference_t<_I2>>> &&
    __plain_equal_pred<_Pp, std::iter_value_t<_I1>>;

// The first index i < n where the two contiguous ranges differ (n if none): blocks of 256 bytes
// are tested without an early exit (vectorized), then the block that differs is scanned.
template <class _I1, class _I2>
std::iter_difference_t<_I1> __block_mismatch(_I1 __first1, std::iter_difference_t<_I1> n, _I2 __first2) noexcept {
  using _Vp = std::iter_value_t<_I1>;
  const _Vp* const a = ::__ycxx::__detail::__raw_address(__first1);
  const _Vp* const b = ::__ycxx::__detail::__raw_address(__first2);
  constexpr std::ptrdiff_t _Bk = 256 / sizeof(_Vp) > 0 ? 256 / sizeof(_Vp) : 1;
  std::ptrdiff_t i = 0;
  for (; n - i >= _Bk; i += _Bk) {
    bool __diff = false;
    for (std::ptrdiff_t __j = 0; __j < _Bk; ++__j)
      __diff |= a[i + __j] != b[i + __j];
    if (__diff)
      break;
  }
  for (; i < n && a[i] == b[i]; ++i) {
  }
  return i;
}

template <class _I1, class _I2>
bool __memcmp_equal(_I1 __first1, std::iter_difference_t<_I1> n, _I2 __first2) noexcept {
  if (n <= 0)
    return true;
  // void* operands: an element type's associated classes stay out of the call (ADL would
  // instantiate them, and Holder<Incomplete>* elements must work).
  const void* const a = ::__ycxx::__detail::__raw_address(__first1);
  const void* const b = ::__ycxx::__detail::__raw_address(__first2);
  return __builtin_memcmp(a, b, static_cast<std::size_t>(n) * sizeof(std::iter_value_t<_I1>)) == 0;
}

// ---- min / max -------------------------------------------------------------------------------
template <class _Ip, class _Sp, class _Cp>
constexpr _Ip __min_element_impl(_Ip first, _Sp last, _Cp less) {
  if (first == last)
    return first;
  _Ip __best = first;
  while (++first != last)
    if (less(*first, *__best))
      __best = first;
  return __best;
}
template <class _Ip, class _Sp, class _Cp>
constexpr _Ip __max_element_impl(_Ip first, _Sp last, _Cp less) {
  if (first == last)
    return first;
  _Ip __best = first;
  while (++first != last)
    if (less(*__best, *first))
      __best = first;
  return __best;
}
// The leftmost smallest and the rightmost largest, in at most 3/2 (N - 1) comparisons:
// elements are taken in pairs, ordered against each other, then the smaller of the pair is
// compared with the minimum and the larger with the maximum.
template <class _Ip, class _Sp, class _Cp>
constexpr std::pair<_Ip, _Ip> __minmax_element_impl(_Ip first, _Sp last, _Cp less) {
  _Ip __lo = first, __hi = first;
  if (first == last || ++first == last)
    return {__lo, __hi};
  if (less(*first, *__lo))
    __lo = first;
  else
    __hi = first;
  while (++first != last) {
    _Ip a = first;
    if (++first == last) {
      if (less(*a, *__lo))
        __lo = a;
      else if (!less(*a, *__hi))
        __hi = a;
      break;
    }
    if (less(*first, *a)) { // first < a: first is the smaller, a the larger
      if (less(*first, *__lo))
        __lo = first;
      if (!less(*a, *__hi))
        __hi = a;
    } else {
      if (less(*a, *__lo))
        __lo = a;
      if (!less(*first, *__hi))
        __hi = first;
    }
  }
  return {__lo, __hi};
}

// ---- copy / move -----------------------------------------------------------------------------
// A count argument of the std:: forms ("Size is convertible to an integral type").
// Taken by non-const reference: a class type may convert only through a non-const function.
template <class _Size>
constexpr auto __integral_count(_Size& n) {
  if constexpr (std::is_integral_v<_Size> && !std::is_same_v<std::remove_cv_t<_Size>, bool>)
    return n;
  else
    return static_cast<long long>(n);
}

template <class _Ip, class _Sp, class _Op>
constexpr std::pair<_Ip, _Op> __copy_dispatch(_Ip first, _Sp last, _Op result) {
  if constexpr (__memmovable_pair<_Ip, _Op, std::iter_reference_t<_Ip>> && std::sized_sentinel_for<_Sp, _Ip>) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        _Ip __in_last = first + n;
        _Op __out_last = result + n;
        ::__ycxx::__detail::__bulk_move(first, __in_last, result, __out_last);
        return {static_cast<_Ip&&>(__in_last), static_cast<_Op&&>(__out_last)};
      }
    }
  }
  for (; first != last; (void)++first, (void)++result)
    *result = *first;
  return {static_cast<_Ip&&>(first), static_cast<_Op&&>(result)};
}

// Moves elements; with Ops = ranges_ops through ranges::iter_move (customizable), so the bulk
// path is taken only for pointers there.
template <class _Ops, class _Ip, class _Sp, class _Op>
constexpr std::pair<_Ip, _Op> __move_dispatch(_Ip first, _Sp last, _Op result) {
  if constexpr (__memmovable_pair<_Ip, _Op, std::iter_rvalue_reference_t<_Ip>> && std::sized_sentinel_for<_Sp, _Ip> &&
                (std::same_as<_Ops, __classic_ops> || (std::is_pointer_v<_Ip> && std::is_pointer_v<_Op>))) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        _Ip __in_last = first + n;
        _Op __out_last = result + n;
        ::__ycxx::__detail::__bulk_move(first, __in_last, result, __out_last);
        return {static_cast<_Ip&&>(__in_last), static_cast<_Op&&>(__out_last)};
      }
    }
  }
  for (; first != last; (void)++first, (void)++result)
    *result = _Ops::iter_move(first);
  return {static_cast<_Ip&&>(first), static_cast<_Op&&>(result)};
}

template <class _Ip, class _Op>
constexpr _Op __copy_backward_dispatch(_Ip first, _Ip last, _Op result) {
  if constexpr (__memmovable_pair<_Ip, _Op, std::iter_reference_t<_Ip>>) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        _Op __out_first = result - n;
        ::__ycxx::__detail::__bulk_move(first, first + n, __out_first, result);
        return __out_first;
      }
    }
  }
  while (first != last)
    *--result = *--last;
  return result;
}
template <class _Ops, class _Ip, class _Op>
constexpr _Op __move_backward_dispatch(_Ip first, _Ip last, _Op result) {
  if constexpr (__memmovable_pair<_Ip, _Op, std::iter_rvalue_reference_t<_Ip>> &&
                (std::same_as<_Ops, __classic_ops> || (std::is_pointer_v<_Ip> && std::is_pointer_v<_Op>))) {
    if !consteval {
      auto n = last - first;
      if (n > 1) {
        _Op __out_first = result - n;
        ::__ycxx::__detail::__bulk_move(first, first + n, __out_first, result);
        return __out_first;
      }
    }
  }
  while (first != last)
    *--result = _Ops::iter_move(--last);
  return result;
}

// ---- find / mismatch / equal / lexicographical compare --------------------------------------
template <class _Ip, class _Sp, class _Pp>
constexpr _Ip __find_if_impl(_Ip first, _Sp last, _Pp pred) {
  if constexpr (std::random_access_iterator<_Ip> && std::sized_sentinel_for<_Sp, _Ip>) {
    // Four tests per loop-count check (the counted loop also lets the compiler drop the
    // iterator comparisons).
    for (auto n = last - first; n >= 4; n -= 4) {
      if (pred(*first))
        return first;
      ++first;
      if (pred(*first))
        return first;
      ++first;
      if (pred(*first))
        return first;
      ++first;
      if (pred(*first))
        return first;
      ++first;
    }
  }
  for (; first != last; ++first)
    if (pred(*first))
      break;
  return first;
}

template <class _Ip, class _Sp, class _Pp>
constexpr _Ip __adjacent_find_impl(_Ip first, _Sp last, _Pp pred) {
  if (first == last)
    return first;
  _Ip next = first;
  while (++next != last) {
    if (pred(*first, *next))
      return first;
    first = next;
  }
  return next;
}

template <class _I1, class _S1, class _I2, class _S2, class _Pp>
constexpr std::pair<_I1, _I2> __mismatch_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pp eq) {
  while (__first1 != __last1 && __first2 != __last2 && eq(*__first1, *__first2)) {
    ++__first1;
    ++__first2;
  }
  return {static_cast<_I1&&>(__first1), static_cast<_I2&&>(__first2)};
}
// The three-iterator form: the second range is as long as the first.
template <class _I1, class _S1, class _I2, class _Pp>
constexpr std::pair<_I1, _I2> __mismatch3_impl(_I1 __first1, _S1 __last1, _I2 __first2, _Pp eq) {
  while (__first1 != __last1 && eq(*__first1, *__first2)) {
    ++__first1;
    ++__first2;
  }
  return {static_cast<_I1&&>(__first1), static_cast<_I2&&>(__first2)};
}

template <class _I1, class _S1, class _I2, class _S2, class _Pp>
constexpr bool __equal_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pp eq) {
  // [alg.equal]/5: no comparisons when the lengths are known to differ.
  if constexpr ((std::sized_sentinel_for<_S1, _I1> || (std::same_as<_I1, _S1> && __ra_iter<_I1>)) &&
                (std::sized_sentinel_for<_S2, _I2> || (std::same_as<_I2, _S2> && __ra_iter<_I2>))) {
    if (__last1 - __first1 != __last2 - __first2)
      return false;
    for (; __first1 != __last1; (void)++__first1, (void)++__first2)
      if (!eq(*__first1, *__first2))
        return false;
    return true;
  } else {
    for (; __first1 != __last1 && __first2 != __last2; (void)++__first1, (void)++__first2)
      if (!eq(*__first1, *__first2))
        return false;
    return __first1 == __last1 && __first2 == __last2;
  }
}

template <class _I1, class _S1, class _I2, class _S2, class _Cp>
constexpr bool __lex_compare_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Cp less) {
  for (; __first2 != __last2; (void)++__first1, (void)++__first2) {
    if (__first1 == __last1 || less(*__first1, *__first2))
      return true;
    if (less.__rev(*__first2, *__first1))
      return false;
  }
  return false;
}

template <class _Tp>
concept comparison_category = !std::is_void_v<std::common_comparison_category_t<_Tp>>;

}} // namespace __ycxx::__detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] std {

// [alg.min.max]
template <class _Tp>
[[nodiscard]] constexpr const _Tp& min(const _Tp& a, const _Tp& b) {
  return b < a ? b : a;
}
template <class _Tp, class _Compare>
[[nodiscard]] constexpr const _Tp& min(const _Tp& a, const _Tp& b, _Compare comp) {
  return comp(b, a) ? b : a;
}
template <class _Tp>
[[nodiscard]] constexpr const _Tp& max(const _Tp& a, const _Tp& b) {
  return a < b ? b : a;
}
template <class _Tp, class _Compare>
[[nodiscard]] constexpr const _Tp& max(const _Tp& a, const _Tp& b, _Compare comp) {
  return comp(a, b) ? b : a;
}
template <class _Tp, class _Compare>
[[nodiscard]] constexpr _Tp min(initializer_list<_Tp> r, _Compare comp) {
  __ycxx::__detail::__precondition(r.size() != 0, "std::min: empty initializer_list");
  return *::__ycxx::__detail::__min_element_impl(r.begin(), r.end(), ::__ycxx::__detail::__ref_pred(comp));
}
template <class _Tp>
[[nodiscard]] constexpr _Tp min(initializer_list<_Tp> r) {
  return std::min(r, less<>{});
}
template <class _Tp, class _Compare>
[[nodiscard]] constexpr _Tp max(initializer_list<_Tp> r, _Compare comp) {
  __ycxx::__detail::__precondition(r.size() != 0, "std::max: empty initializer_list");
  return *::__ycxx::__detail::__max_element_impl(r.begin(), r.end(), ::__ycxx::__detail::__ref_pred(comp));
}
template <class _Tp>
[[nodiscard]] constexpr _Tp max(initializer_list<_Tp> r) {
  return std::max(r, less<>{});
}
template <class _Tp>
[[nodiscard]] constexpr pair<const _Tp&, const _Tp&> minmax(const _Tp& a, const _Tp& b) {
  if (b < a)
    return pair<const _Tp&, const _Tp&>(b, a);
  return pair<const _Tp&, const _Tp&>(a, b);
}
template <class _Tp, class _Compare>
[[nodiscard]] constexpr pair<const _Tp&, const _Tp&> minmax(const _Tp& a, const _Tp& b, _Compare comp) {
  if (comp(b, a))
    return pair<const _Tp&, const _Tp&>(b, a);
  return pair<const _Tp&, const _Tp&>(a, b);
}
template <class _Tp, class _Compare>
[[nodiscard]] constexpr pair<_Tp, _Tp> minmax(initializer_list<_Tp> r, _Compare comp) {
  __ycxx::__detail::__precondition(r.size() != 0, "std::minmax: empty initializer_list");
  auto p = ::__ycxx::__detail::__minmax_element_impl(r.begin(), r.end(), ::__ycxx::__detail::__ref_pred(comp));
  return pair<_Tp, _Tp>(*p.first, *p.second);
}
template <class _Tp>
[[nodiscard]] constexpr pair<_Tp, _Tp> minmax(initializer_list<_Tp> r) {
  return std::minmax(r, less<>{});
}

template <class _Tp, class _Compare>
[[nodiscard]] constexpr const _Tp& clamp(const _Tp& __v, const _Tp& __lo, const _Tp& __hi, _Compare comp) {
  return comp(__v, __lo) ? __lo : comp(__hi, __v) ? __hi : __v;
}
template <class _Tp>
[[nodiscard]] constexpr const _Tp& clamp(const _Tp& __v, const _Tp& __lo, const _Tp& __hi) {
  return std::clamp(__v, __lo, __hi, less<>{});
}

template <class _ForwardIterator, class _Compare>
[[nodiscard]] constexpr _ForwardIterator min_element(_ForwardIterator first, _ForwardIterator last, _Compare comp) {
  return ::__ycxx::__detail::__min_element_impl(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _ForwardIterator>
[[nodiscard]] constexpr _ForwardIterator min_element(_ForwardIterator first, _ForwardIterator last) {
  return std::min_element(first, last, less<>{});
}
template <class _ForwardIterator, class _Compare>
[[nodiscard]] constexpr _ForwardIterator max_element(_ForwardIterator first, _ForwardIterator last, _Compare comp) {
  return ::__ycxx::__detail::__max_element_impl(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _ForwardIterator>
[[nodiscard]] constexpr _ForwardIterator max_element(_ForwardIterator first, _ForwardIterator last) {
  return std::max_element(first, last, less<>{});
}
template <class _ForwardIterator, class _Compare>
[[nodiscard]] constexpr pair<_ForwardIterator, _ForwardIterator> minmax_element(_ForwardIterator first, _ForwardIterator last,
                                                                              _Compare comp) {
  return ::__ycxx::__detail::__minmax_element_impl(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _ForwardIterator>
[[nodiscard]] constexpr pair<_ForwardIterator, _ForwardIterator> minmax_element(_ForwardIterator first, _ForwardIterator last) {
  return std::minmax_element(first, last, less<>{});
}

// [alg.swap]
template <class _ForwardIterator1, class _ForwardIterator2>
constexpr void iter_swap(_ForwardIterator1 a, _ForwardIterator2 b) {
  ::__ycxx::__detail::__swap_adl::__do_swap(*a, *b);
}
template <class _ForwardIterator1, class _ForwardIterator2>
constexpr _ForwardIterator2 swap_ranges(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2) {
  for (; __first1 != __last1; (void)++__first1, (void)++__first2)
    ::__ycxx::__detail::__swap_adl::__do_swap(*__first1, *__first2);
  return __first2;
}

// [alg.copy]
template <class _InputIterator, class _OutputIterator>
constexpr _OutputIterator copy(_InputIterator first, _InputIterator last, _OutputIterator result) {
  return ::__ycxx::__detail::__copy_dispatch(first, last, result).second;
}
template <class _InputIterator, class _Size, class _OutputIterator>
constexpr _OutputIterator copy_n(_InputIterator first, _Size n, _OutputIterator result) {
  auto count = ::__ycxx::__detail::__integral_count(n);
  if (count <= 0)
    return result;
  if constexpr (::__ycxx::__detail::__ra_iter<_InputIterator>) {
    return ::__ycxx::__detail::__copy_dispatch(first, first + static_cast<std::iter_difference_t<_InputIterator>>(count),
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
template <class _InputIterator, class _OutputIterator, class _Predicate>
constexpr _OutputIterator copy_if(_InputIterator first, _InputIterator last, _OutputIterator result, _Predicate pred) {
  for (; first != last; ++first)
    if (pred(*first)) {
      *result = *first;
      ++result;
    }
  return result;
}
template <class _BidirectionalIterator1, class _BidirectionalIterator2>
constexpr _BidirectionalIterator2 copy_backward(_BidirectionalIterator1 first, _BidirectionalIterator1 last,
                                               _BidirectionalIterator2 result) {
  return ::__ycxx::__detail::__copy_backward_dispatch(first, last, result);
}

// [alg.move]
template <class _InputIterator, class _OutputIterator>
constexpr _OutputIterator move(_InputIterator first, _InputIterator last, _OutputIterator result) {
  return ::__ycxx::__detail::__move_dispatch<__ycxx::__detail::__classic_ops>(first, last, result).second;
}
template <class _BidirectionalIterator1, class _BidirectionalIterator2>
constexpr _BidirectionalIterator2 move_backward(_BidirectionalIterator1 first, _BidirectionalIterator1 last,
                                               _BidirectionalIterator2 result) {
  return ::__ycxx::__detail::__move_backward_dispatch<__ycxx::__detail::__classic_ops>(first, last, result);
}

// [alg.fill]
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
constexpr void fill(_ForwardIterator first, _ForwardIterator last, const _Tp& value) {
  if constexpr (__ycxx::__detail::__bit_algo_args<_ForwardIterator, _ForwardIterator, _Tp>) {
    __ycxx::__detail::__bit_algos<_ForwardIterator>::fill(first, last, value);
  } else {
    for (; first != last; ++first)
      *first = value;
  }
}
template <class _OutputIterator, class _Size, class _Tp = typename iterator_traits<_OutputIterator>::value_type>
constexpr _OutputIterator fill_n(_OutputIterator first, _Size n, const _Tp& value) {
  for (auto count = ::__ycxx::__detail::__integral_count(n); count > 0; --count) {
    *first = value;
    ++first;
  }
  return first;
}

// [alg.find]
template <class _InputIterator, class _Tp = typename iterator_traits<_InputIterator>::value_type>
[[nodiscard]] constexpr _InputIterator find(_InputIterator first, _InputIterator last, const _Tp& value) {
  if constexpr (__ycxx::__detail::__bit_algo_args<_InputIterator, _InputIterator, _Tp>) {
    return __ycxx::__detail::__bit_algos<_InputIterator>::find(first, last, value);
  } else {
    if constexpr (__ycxx::__detail::__memchr_find_args<_InputIterator, _InputIterator, _Tp>) {
      if !consteval {
        ::__ycxx::__detail::__find_byte(first, last, value);
        return first;
      }
    } else if constexpr (__ycxx::__detail::__wide_find_args<_InputIterator, _InputIterator, _Tp>) {
      if !consteval {
        ::__ycxx::__detail::__find_wide(first, last, value);
        return first;
      }
    }
    return ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__equals_value_plain<_Tp>{value});
  }
}
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr _InputIterator find_if(_InputIterator first, _InputIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr _InputIterator find_if_not(_InputIterator first, _InputIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__negated{::__ycxx::__detail::__ref_pred(pred)});
}

// [alg.mismatch]
template <class _InputIterator1, class _InputIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr pair<_InputIterator1, _InputIterator2> mismatch(_InputIterator1 __first1, _InputIterator1 __last1,
                                                                      _InputIterator2 __first2, _BinaryPredicate pred) {
  if constexpr (__ycxx::__detail::__memcmp_equal_args<_InputIterator1, _InputIterator2, _BinaryPredicate>)
    if !consteval {
      const auto i = ::__ycxx::__detail::__block_mismatch(__first1, __last1 - __first1, __first2);
      return {__first1 + i, __first2 + static_cast<iter_difference_t<_InputIterator2>>(i)};
    }
  return ::__ycxx::__detail::__mismatch3_impl(__first1, __last1, __first2, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr pair<_InputIterator1, _InputIterator2> mismatch(_InputIterator1 __first1, _InputIterator1 __last1,
                                                                      _InputIterator2 __first2) {
  return std::mismatch(__first1, __last1, __first2, equal_to<>{});
}
template <class _InputIterator1, class _InputIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr pair<_InputIterator1, _InputIterator2> mismatch(_InputIterator1 __first1, _InputIterator1 __last1,
                                                                      _InputIterator2 __first2, _InputIterator2 __last2,
                                                                      _BinaryPredicate pred) {
  if constexpr (__ycxx::__detail::__memcmp_equal_args<_InputIterator1, _InputIterator2, _BinaryPredicate>)
    if !consteval {
      const auto __n1 = __last1 - __first1;
      const auto __n2 = static_cast<iter_difference_t<_InputIterator1>>(__last2 - __first2);
      const auto i = ::__ycxx::__detail::__block_mismatch(__first1, __n1 < __n2 ? __n1 : __n2, __first2);
      return {__first1 + i, __first2 + static_cast<iter_difference_t<_InputIterator2>>(i)};
    }
  return ::__ycxx::__detail::__mismatch_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr pair<_InputIterator1, _InputIterator2> mismatch(_InputIterator1 __first1, _InputIterator1 __last1,
                                                                      _InputIterator2 __first2, _InputIterator2 __last2) {
  return std::mismatch(__first1, __last1, __first2, __last2, equal_to<>{});
}

// [alg.equal]
template <class _InputIterator1, class _InputIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr bool equal(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                   _BinaryPredicate pred) {
  if constexpr (__ycxx::__detail::__memcmp_equal_args<_InputIterator1, _InputIterator2, _BinaryPredicate>)
    if !consteval {
      return ::__ycxx::__detail::__memcmp_equal(__first1, __last1 - __first1, __first2);
    }
  return ::__ycxx::__detail::__mismatch3_impl(__first1, __last1, __first2, ::__ycxx::__detail::__ref_pred(pred)).first == __last1;
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr bool equal(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2) {
  return std::equal(__first1, __last1, __first2, equal_to<>{});
}
template <class _InputIterator1, class _InputIterator2, class _BinaryPredicate>
[[nodiscard]] constexpr bool equal(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                   _InputIterator2 __last2, _BinaryPredicate pred) {
  if constexpr (__ycxx::__detail::__memcmp_equal_args<_InputIterator1, _InputIterator2, _BinaryPredicate>)
    if !consteval {
      return __last1 - __first1 == __last2 - __first2 &&
             ::__ycxx::__detail::__memcmp_equal(__first1, __last1 - __first1, __first2);
    }
  return ::__ycxx::__detail::__equal_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(pred));
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr bool equal(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                   _InputIterator2 __last2) {
  return std::equal(__first1, __last1, __first2, __last2, equal_to<>{});
}

// [alg.lex.comparison]
template <class _InputIterator1, class _InputIterator2, class _Compare>
[[nodiscard]] constexpr bool lexicographical_compare(_InputIterator1 __first1, _InputIterator1 __last1,
                                                     _InputIterator2 __first2, _InputIterator2 __last2, _Compare comp) {
  return ::__ycxx::__detail::__lex_compare_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr bool lexicographical_compare(_InputIterator1 __first1, _InputIterator1 __last1,
                                                     _InputIterator2 __first2, _InputIterator2 __last2) {
  return std::lexicographical_compare(__first1, __last1, __first2, __last2, less<>{});
}

// [alg.three.way]
template <class _InputIterator1, class _InputIterator2, class _Cmp>
[[nodiscard]] constexpr auto lexicographical_compare_three_way(_InputIterator1 __b1, _InputIterator1 __e1,
                                                               _InputIterator2 __b2, _InputIterator2 __e2, _Cmp comp)
    -> decltype(comp(*__b1, *__b2)) {
  using _Rp = decltype(comp(*__b1, *__b2));
  static_assert(__ycxx::__detail::comparison_category<_Rp>,
                "std::lexicographical_compare_three_way: comp must return a comparison category type");
  for (; __b1 != __e1 && __b2 != __e2; (void)++__b1, (void)++__b2)
    if (auto c = comp(*__b1, *__b2); c != 0)
      return c;
  return __b1 != __e1 ? _Rp(strong_ordering::greater) : __b2 != __e2 ? _Rp(strong_ordering::less) : _Rp(strong_ordering::equal);
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr auto lexicographical_compare_three_way(_InputIterator1 __b1, _InputIterator1 __e1,
                                                               _InputIterator2 __b2, _InputIterator2 __e2) {
  return std::lexicographical_compare_three_way(__b1, __e1, __b2, __e2, compare_three_way());
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {

template <class _Ip, class _Op>
using copy_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using copy_n_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using copy_if_result = in_out_result<_Ip, _Op>;
template <class _I1, class _I2>
using copy_backward_result = in_out_result<_I1, _I2>;
template <class _Ip, class _Op>
using move_result = in_out_result<_Ip, _Op>;
template <class _I1, class _I2>
using move_backward_result = in_out_result<_I1, _I2>;
template <class _I1, class _I2>
using swap_ranges_result = in_in_result<_I1, _I2>;
template <class _I1, class _I2>
using mismatch_result = in_in_result<_I1, _I2>;
template <class _Tp>
using minmax_result = min_max_result<_Tp>;
template <class _Ip>
using minmax_element_result = min_max_result<_Ip>;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::iterator_t;

// [alg.min.max]
struct __min_fn {
  template <class _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr const _Tp& operator()(const _Tp& a, const _Tp& b, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, b), ::__ycxx::__detail::invoke(proj, a)) ? b : a;
  }
  template <std::copyable _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Tp operator()(std::initializer_list<_Tp> r, _Comp comp = {}, _Proj proj = {}) const {
    ::__ycxx::__detail::__precondition(r.size() != 0, "ranges::min: empty range");
    return *::__ycxx::__detail::__min_element_impl(r.begin(), r.end(), ::__ycxx::__detail::__make_comp(comp, proj));
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires std::indirectly_copyable_storable<iterator_t<_Rp>, std::ranges::range_value_t<_Rp>*>
  [[nodiscard]] constexpr std::ranges::range_value_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    ::__ycxx::__detail::__precondition(first != last, "ranges::min: empty range");
    auto less = ::__ycxx::__detail::__make_comp(comp, proj);
    if constexpr (std::ranges::forward_range<_Rp>) {
      return static_cast<std::ranges::range_value_t<_Rp>>(*::__ycxx::__detail::__min_element_impl(first, last, less));
    } else {
      std::ranges::range_value_t<_Rp> __best(*first);
      while (++first != last) {
        decltype(auto) __x = *first;
        if (less(__x, __best))
          __best = static_cast<decltype(__x)&&>(__x);
      }
      return __best;
    }
  }
};
struct __max_fn {
  template <class _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr const _Tp& operator()(const _Tp& a, const _Tp& b, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, a), ::__ycxx::__detail::invoke(proj, b)) ? b : a;
  }
  template <std::copyable _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Tp operator()(std::initializer_list<_Tp> r, _Comp comp = {}, _Proj proj = {}) const {
    ::__ycxx::__detail::__precondition(r.size() != 0, "ranges::max: empty range");
    return *::__ycxx::__detail::__max_element_impl(r.begin(), r.end(), ::__ycxx::__detail::__make_comp(comp, proj));
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires std::indirectly_copyable_storable<iterator_t<_Rp>, std::ranges::range_value_t<_Rp>*>
  [[nodiscard]] constexpr std::ranges::range_value_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    ::__ycxx::__detail::__precondition(first != last, "ranges::max: empty range");
    auto less = ::__ycxx::__detail::__make_comp(comp, proj);
    if constexpr (std::ranges::forward_range<_Rp>) {
      return static_cast<std::ranges::range_value_t<_Rp>>(*::__ycxx::__detail::__max_element_impl(first, last, less));
    } else {
      std::ranges::range_value_t<_Rp> __best(*first);
      while (++first != last) {
        decltype(auto) __x = *first;
        if (less(__best, __x))
          __best = static_cast<decltype(__x)&&>(__x);
      }
      return __best;
    }
  }
};
struct __minmax_fn {
  template <class _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_result<const _Tp&> operator()(const _Tp& a, const _Tp& b, _Comp comp = {},
                                                                         _Proj proj = {}) const {
    if (::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, b), ::__ycxx::__detail::invoke(proj, a)))
      return {b, a};
    return {a, b};
  }
  template <std::copyable _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_result<_Tp> operator()(std::initializer_list<_Tp> r, _Comp comp = {},
                                                                  _Proj proj = {}) const {
    ::__ycxx::__detail::__precondition(r.size() != 0, "ranges::minmax: empty range");
    auto p = ::__ycxx::__detail::__minmax_element_impl(r.begin(), r.end(), ::__ycxx::__detail::__make_comp(comp, proj));
    return {*p.first, *p.second};
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
    requires std::indirectly_copyable_storable<iterator_t<_Rp>, std::ranges::range_value_t<_Rp>*>
  [[nodiscard]] constexpr std::ranges::minmax_result<std::ranges::range_value_t<_Rp>> operator()(_Rp&& r, _Comp comp = {},
                                                                                              _Proj proj = {}) const {
    using _Vp = std::ranges::range_value_t<_Rp>;
    auto first = std::ranges::begin(r);
    auto last = std::ranges::end(r);
    ::__ycxx::__detail::__precondition(first != last, "ranges::minmax: empty range");
    auto less = ::__ycxx::__detail::__make_comp(comp, proj);
    if constexpr (std::ranges::forward_range<_Rp>) {
      auto p = ::__ycxx::__detail::__minmax_element_impl(first, last, less);
      // Each element is read once: *it may move from it (move_iterator).
      _Vp __lo(*p.first);
      if (p.first == p.second)
        return {__lo, __lo};
      return {std::move(__lo), static_cast<_Vp>(*p.second)};
    } else {
      // Single pass over copies: the leftmost smallest and the rightmost largest.
      _Vp __lo(*first);
      _Vp __hi(__lo);
      while (++first != last) {
        _Vp a(*first);
        if (++first == last) {
          if (less(a, __lo))
            __lo = std::move(a);
          else if (!less(a, __hi))
            __hi = std::move(a);
          break;
        }
        _Vp b(*first);
        if (less(b, a)) {
          if (less(b, __lo))
            __lo = std::move(b);
          if (!less(a, __hi))
            __hi = std::move(a);
        } else {
          if (less(a, __lo))
            __lo = std::move(a);
          if (!less(b, __hi))
            __hi = std::move(b);
        }
      }
      return {std::move(__lo), std::move(__hi)};
    }
  }
};
struct __clamp_fn {
  template <class _Tp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<const _Tp*, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr const _Tp& operator()(const _Tp& __v, const _Tp& __lo, const _Tp& __hi, _Comp comp = {}, _Proj proj = {}) const {
    // proj(v) is computed once ("at most three applications of the projection") and passed
    // on with its value category; a prvalue result is passed as an lvalue, so that a
    // comparator taking its parameters by value cannot move from it twice.
    using _PV = decltype(::__ycxx::__detail::invoke(proj, __v));
    using _Arg = std::conditional_t<std::is_reference_v<_PV>, _PV, _PV&>;
    auto&& __pv = ::__ycxx::__detail::invoke(proj, __v);
    if (::__ycxx::__detail::invoke(comp, static_cast<_Arg>(__pv), ::__ycxx::__detail::invoke(proj, __lo)))
      return __lo;
    if (::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, __hi), static_cast<_Arg>(__pv)))
      return __hi;
    return __v;
  }
};

struct __min_element_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__min_element_impl(std::move(first), last, ::__ycxx::__detail::__make_comp(comp, proj));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__min_element_impl(std::ranges::begin(r), std::ranges::end(r),
                                            ::__ycxx::__detail::__make_comp(comp, proj));
  }
};
struct __max_element_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__max_element_impl(std::move(first), last, ::__ycxx::__detail::__make_comp(comp, proj));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__max_element_impl(std::ranges::begin(r), std::ranges::end(r),
                                            ::__ycxx::__detail::__make_comp(comp, proj));
  }
};
struct __minmax_element_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_element_result<_Ip> operator()(_Ip first, _Sp last, _Comp comp = {},
                                                                          _Proj proj = {}) const {
    auto p = ::__ycxx::__detail::__minmax_element_impl(std::move(first), last, ::__ycxx::__detail::__make_comp(comp, proj));
    return {std::move(p.first), std::move(p.second)};
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::minmax_element_result<borrowed_iterator_t<_Rp>> operator()(_Rp&& r, _Comp comp = {},
                                                                                               _Proj proj = {}) const {
    auto p = ::__ycxx::__detail::__minmax_element_impl(std::ranges::begin(r), std::ranges::end(r),
                                                 ::__ycxx::__detail::__make_comp(comp, proj));
    return {std::move(p.first), std::move(p.second)};
  }
};

// [alg.swap]
struct __swap_ranges_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2>
    requires std::indirectly_swappable<_I1, _I2>
  constexpr std::ranges::swap_ranges_result<_I1, _I2> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2) const {
    for (; __first1 != __last1 && __first2 != __last2; (void)++__first1, (void)++__first2)
      std::ranges::iter_swap(__first1, __first2);
    return {std::move(__first1), std::move(__first2)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2>
    requires std::indirectly_swappable<iterator_t<_R1>, iterator_t<_R2>>
  constexpr std::ranges::swap_ranges_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>> operator()(_R1&& __r1,
                                                                                                        _R2&& __r2) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2));
  }
};

// [alg.copy]
struct __copy_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<_Ip, _Op>
  constexpr std::ranges::copy_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result) const {
    auto r = ::__ycxx::__detail::__copy_dispatch(std::move(first), std::move(last), std::move(result));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op>
  constexpr std::ranges::copy_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};
struct __copy_n_fn {
  template <std::input_iterator _Ip, std::weakly_incrementable _Op>
    requires std::indirectly_copyable<_Ip, _Op>
  constexpr std::ranges::copy_n_result<_Ip, _Op> operator()(_Ip first, std::iter_difference_t<_Ip> n, _Op result) const {
    if constexpr (std::random_access_iterator<_Ip>) {
      if (n <= 0)
        return {std::move(first), std::move(result)};
      auto r = ::__ycxx::__detail::__copy_dispatch(first, first + n, std::move(result));
      return {std::move(r.first), std::move(r.second)};
    } else {
      for (; n > 0; (void)++first, (void)++result, --n)
        *result = *first;
      return {std::move(first), std::move(result)};
    }
  }
};
struct __copy_if_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires std::indirectly_copyable<_Ip, _Op>
  constexpr std::ranges::copy_if_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result, _Pred pred, _Proj proj = {}) const {
    auto p = ::__ycxx::__detail::__make_pred(pred, proj);
    for (; first != last; ++first)
      if (p(*first)) {
        *result = *first;
        ++result;
      }
    return {std::move(first), std::move(result)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Op>
  constexpr std::ranges::copy_if_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result, _Pred pred,
                                                                             _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result), std::move(pred), std::move(proj));
  }
};
struct __copy_backward_fn {
  template <std::bidirectional_iterator _I1, std::sentinel_for<_I1> _S1, std::bidirectional_iterator _I2>
    requires std::indirectly_copyable<_I1, _I2>
  constexpr std::ranges::copy_backward_result<_I1, _I2> operator()(_I1 first, _S1 last, _I2 result) const {
    _I1 end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    return {end, ::__ycxx::__detail::__copy_backward_dispatch(std::move(first), end, std::move(result))};
  }
  template <std::ranges::bidirectional_range _Rp, std::bidirectional_iterator _Ip>
    requires std::indirectly_copyable<iterator_t<_Rp>, _Ip>
  constexpr std::ranges::copy_backward_result<borrowed_iterator_t<_Rp>, _Ip> operator()(_Rp&& r, _Ip result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};

// [alg.move]
struct __move_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _Op>
    requires std::indirectly_movable<_Ip, _Op>
  constexpr std::ranges::move_result<_Ip, _Op> operator()(_Ip first, _Sp last, _Op result) const {
    auto r = ::__ycxx::__detail::__move_dispatch<__ranges_ops>(std::move(first), std::move(last), std::move(result));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _Op>
    requires std::indirectly_movable<iterator_t<_Rp>, _Op>
  constexpr std::ranges::move_result<borrowed_iterator_t<_Rp>, _Op> operator()(_Rp&& r, _Op result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};
struct __move_backward_fn {
  template <std::bidirectional_iterator _I1, std::sentinel_for<_I1> _S1, std::bidirectional_iterator _I2>
    requires std::indirectly_movable<_I1, _I2>
  constexpr std::ranges::move_backward_result<_I1, _I2> operator()(_I1 first, _S1 last, _I2 result) const {
    _I1 end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    return {end, ::__ycxx::__detail::__move_backward_dispatch<__ranges_ops>(std::move(first), end, std::move(result))};
  }
  template <std::ranges::bidirectional_range _Rp, std::bidirectional_iterator _Ip>
    requires std::indirectly_movable<iterator_t<_Rp>, _Ip>
  constexpr std::ranges::move_backward_result<borrowed_iterator_t<_Rp>, _Ip> operator()(_Rp&& r, _Ip result) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(result));
  }
};

// [alg.fill]
struct __fill_fn {
  template <class _Op, std::sentinel_for<_Op> _Sp, class _Tp = std::iter_value_t<_Op>>
    requires std::output_iterator<_Op, const _Tp&>
  constexpr _Op operator()(_Op first, _Sp last, const _Tp& value) const {
    if constexpr (__ycxx::__detail::__bit_algo_args<_Op, _Sp, _Tp>) {
      __ycxx::__detail::__bit_algos<_Op>::fill(first, last, value);
      return last;
    } else {
      for (; first != last; ++first)
        *first = value;
      return first;
    }
  }
  template <class _Rp, class _Tp = std::ranges::range_value_t<_Rp>>
    requires std::ranges::output_range<_Rp, const _Tp&>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, const _Tp& value) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value);
  }
};
struct __fill_n_fn {
  template <class _Op, class _Tp = std::iter_value_t<_Op>>
    requires std::output_iterator<_Op, const _Tp&>
  constexpr _Op operator()(_Op first, std::iter_difference_t<_Op> n, const _Tp& value) const {
    for (; n > 0; --n) {
      *first = value;
      ++first;
    }
    return first;
  }
};

// [alg.find]
struct __find_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<_Ip, _Proj>, const _Tp*>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, const _Tp& value, _Proj proj = {}) const {
    if constexpr (__ycxx::__detail::__bit_algo_args<_Ip, _Sp, _Tp, _Proj>) {
      return __ycxx::__detail::__bit_algos<_Ip>::find(first, last, value);
    } else {
      if constexpr (std::same_as<_Proj, std::identity> && __ycxx::__detail::__memchr_find_args<_Ip, _Sp, _Tp>)
        if !consteval {
          ::__ycxx::__detail::__find_byte(first, last, value);
          return first;
        }
      return ::__ycxx::__detail::__find_if_impl(std::move(first), last, ::__ycxx::__detail::__equals_value<_Tp, _Proj>{value, proj});
    }
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<iterator_t<_Rp>, _Proj>, const _Tp*>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
  }
};
struct __find_if_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last, ::__ycxx::__detail::__make_pred(pred, proj));
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::ranges::begin(r), std::ranges::end(r), ::__ycxx::__detail::__make_pred(pred, proj));
  }
};
struct __find_if_not_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last,
                                        ::__ycxx::__detail::__negated{::__ycxx::__detail::__make_pred(pred, proj)});
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return ::__ycxx::__detail::__find_if_impl(std::ranges::begin(r), std::ranges::end(r),
                                        ::__ycxx::__detail::__negated{::__ycxx::__detail::__make_pred(pred, proj)});
  }
};

// [alg.mismatch]
struct __mismatch_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr std::ranges::mismatch_result<_I1, _I2> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2,
                                                                         _Pred pred = {}, _Proj1 __proj1 = {},
                                                                         _Proj2 __proj2 = {}) const {
    auto r = ::__ycxx::__detail::__mismatch_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                           ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr std::ranges::mismatch_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>>
  operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    auto r = ::__ycxx::__detail::__mismatch_impl(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2),
                                           std::ranges::end(__r2), ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
    return {std::move(r.first), std::move(r.second)};
  }
};

// [alg.equal]
struct __equal_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Pred = std::ranges::equal_to, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<_I1, _I2, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Pred pred = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    return ::__ycxx::__detail::__equal_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                      ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, class _Pred = std::ranges::equal_to,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_comparable<iterator_t<_R1>, iterator_t<_R2>, _Pred, _Proj1, _Proj2>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Pred pred = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    if constexpr (std::ranges::sized_range<_R1> && std::ranges::sized_range<_R2>) {
      if (std::ranges::distance(__r1) != std::ranges::distance(__r2))
        return false;
    }
    return ::__ycxx::__detail::__equal_impl(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2),
                                      std::ranges::end(__r2), ::__ycxx::__detail::__make_comp2(pred, __proj1, __proj2));
  }
};

// [alg.lex.comparison]
struct __lexicographical_compare_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>> _Comp =
                std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Comp comp = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    return ::__ycxx::__detail::__lex_compare_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                            ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, class _Proj1 = std::identity,
            class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_R1>, _Proj1>, std::projected<iterator_t<_R2>, _Proj2>>
                _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return ::__ycxx::__detail::__lex_compare_impl(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2),
                                            std::ranges::end(__r2), ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__min_fn, __ycxx::__detail::par::kind::min> min{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__max_fn, __ycxx::__detail::par::kind::max> max{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__minmax_fn, __ycxx::__detail::par::kind::minmax> minmax{};
inline constexpr __ycxx::__detail::__ranges_algo::__clamp_fn clamp{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__min_element_fn, __ycxx::__detail::par::kind::min_element> min_element{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__max_element_fn, __ycxx::__detail::par::kind::max_element> max_element{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__minmax_element_fn, __ycxx::__detail::par::kind::minmax_element> minmax_element{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__swap_ranges_fn, __ycxx::__detail::par::kind::swap_ranges> swap_ranges{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__copy_fn, __ycxx::__detail::par::kind::copy> copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__copy_n_fn, __ycxx::__detail::par::kind::copy_n> copy_n{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__copy_if_fn, __ycxx::__detail::par::kind::copy_if> copy_if{};
inline constexpr __ycxx::__detail::__ranges_algo::__copy_backward_fn copy_backward{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__move_fn, __ycxx::__detail::par::kind::move> move{};
inline constexpr __ycxx::__detail::__ranges_algo::__move_backward_fn move_backward{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__fill_fn, __ycxx::__detail::par::kind::fill> fill{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__fill_n_fn, __ycxx::__detail::par::kind::fill_n> fill_n{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_fn, __ycxx::__detail::par::kind::find> find{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_if_fn, __ycxx::__detail::par::kind::find_if> find_if{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__find_if_not_fn, __ycxx::__detail::par::kind::find_if_not> find_if_not{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__mismatch_fn, __ycxx::__detail::par::kind::mismatch> mismatch{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__equal_fn, __ycxx::__detail::par::kind::equal> equal{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__lexicographical_compare_fn, __ycxx::__detail::par::kind::lexicographical_compare> lexicographical_compare{};
}} // namespace std::ranges
