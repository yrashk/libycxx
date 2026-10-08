// libycxx core: the specialized <memory> algorithms ([specialized.algorithms]):
// uninitialized_{default,value}_construct[_n], uninitialized_{copy,move,fill}[_n] and their
// std::ranges forms, and ranges::destroy / destroy_n. construct_at and destroy_at live in
// memory_base.hpp.
//
// If constructing an element throws, the elements already constructed are destroyed before
// the exception propagates ([specialized.algorithms.general]/2). A guard object does this from
// its destructor, so the same code serves -fno-exceptions builds.
// The ranges:: ExecutionPolicy overloads (P3179) forward to the sequential ones and are noexcept,
// as in algo_ranges_parallel.hpp; the std:: ones are in uninitialized_parallel.hpp.
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/ranges_subrange.hpp>
#include <ycxx/core/algo_results.hpp>
#include <ycxx/core/execution_policy.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// [special.mem.concepts]
template <class _Ip>
concept __nothrow_input_iterator = std::input_iterator<_Ip> && std::is_lvalue_reference_v<std::iter_reference_t<_Ip>> &&
                                 std::same_as<std::remove_cvref_t<std::iter_reference_t<_Ip>>, std::iter_value_t<_Ip>>;
template <class _Sp, class _Ip>
concept __nothrow_sentinel_for = std::sentinel_for<_Sp, _Ip>;
template <class _Rp>
concept __nothrow_input_range = std::ranges::range<_Rp> && __nothrow_input_iterator<std::ranges::iterator_t<_Rp>> &&
                              __nothrow_sentinel_for<std::ranges::sentinel_t<_Rp>, std::ranges::iterator_t<_Rp>>;
template <class _Ip>
concept __nothrow_forward_iterator = __nothrow_input_iterator<_Ip> && std::forward_iterator<_Ip> && __nothrow_sentinel_for<_Ip, _Ip>;
template <class _Rp>
concept __nothrow_forward_range = __nothrow_input_range<_Rp> && __nothrow_forward_iterator<std::ranges::iterator_t<_Rp>>;
template <class _Sp, class _Ip>
concept __nothrow_sized_sentinel_for = __nothrow_sentinel_for<_Sp, _Ip> && std::sized_sentinel_for<_Sp, _Ip>;
template <class _Ip>
concept __nothrow_random_access_iterator = __nothrow_forward_iterator<_Ip> && std::random_access_iterator<_Ip> &&
                                         __nothrow_sized_sentinel_for<_Ip, _Ip>;
template <class _Rp>
concept __nothrow_sized_random_access_range = __nothrow_forward_range<_Rp> &&
                                            __nothrow_random_access_iterator<std::ranges::iterator_t<_Rp>> &&
                                            std::ranges::sized_range<_Rp>;
template <class _Rp>
concept __sized_random_access_range = std::ranges::random_access_range<_Rp> && std::ranges::sized_range<_Rp>;

// voidify ([specialized.algorithms.general]/4).
template <class _Tp>
constexpr void* __voidify(_Tp& __obj) noexcept {
  return __builtin_addressof(__obj);
}

// deref-move ([specialized.algorithms.general]/4).
template <class _Ip>
constexpr decltype(auto) __deref_move(_Ip& __it) {
  if constexpr (std::is_lvalue_reference_v<decltype(*__it)>)
    return static_cast<std::remove_reference_t<decltype(*__it)>&&>(*__it);
  else
    return *__it;
}

// Destroys [first, *cur) on scope exit unless released: the rollback of a partially
// constructed range.
template <class _Ip>
struct __uninit_guard {
  _Ip first;
  _Ip* cur;
  constexpr ~__uninit_guard() {
    if (cur)
      for (; first != *cur; ++first)
        std::destroy_at(__builtin_addressof(*first));
  }
  constexpr void release() noexcept { cur = nullptr; }
};
template <class _Ip>
__uninit_guard(_Ip, _Ip*) -> __uninit_guard<_Ip>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [uninitialized.construct.default]
template <class _NoThrowForwardIterator>
constexpr void uninitialized_default_construct(_NoThrowForwardIterator first, _NoThrowForwardIterator last) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = first;
  __ycxx::__detail::__uninit_guard __g{first, __builtin_addressof(cur)};
  for (; cur != last; ++cur)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp;
  __g.release();
}
template <class _NoThrowForwardIterator, class _Size>
constexpr _NoThrowForwardIterator uninitialized_default_construct_n(_NoThrowForwardIterator first, _Size n) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = first;
  __ycxx::__detail::__uninit_guard __g{first, __builtin_addressof(cur)};
  for (; n > 0; (void)++cur, --n)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp;
  __g.release();
  return cur;
}

// [uninitialized.construct.value]
template <class _NoThrowForwardIterator>
constexpr void uninitialized_value_construct(_NoThrowForwardIterator first, _NoThrowForwardIterator last) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = first;
  __ycxx::__detail::__uninit_guard __g{first, __builtin_addressof(cur)};
  for (; cur != last; ++cur)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp();
  __g.release();
}
template <class _NoThrowForwardIterator, class _Size>
constexpr _NoThrowForwardIterator uninitialized_value_construct_n(_NoThrowForwardIterator first, _Size n) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = first;
  __ycxx::__detail::__uninit_guard __g{first, __builtin_addressof(cur)};
  for (; n > 0; (void)++cur, --n)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp();
  __g.release();
  return cur;
}

// [uninitialized.copy]
template <class _InputIterator, class _NoThrowForwardIterator>
constexpr _NoThrowForwardIterator uninitialized_copy(_InputIterator first, _InputIterator last,
                                                    _NoThrowForwardIterator result) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = result;
  __ycxx::__detail::__uninit_guard __g{result, __builtin_addressof(cur)};
  for (; first != last; ++cur, (void)++first)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp(*first);
  __g.release();
  return cur;
}
template <class _InputIterator, class _Size, class _NoThrowForwardIterator>
constexpr _NoThrowForwardIterator uninitialized_copy_n(_InputIterator first, _Size n, _NoThrowForwardIterator result) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = result;
  __ycxx::__detail::__uninit_guard __g{result, __builtin_addressof(cur)};
  for (; n > 0; ++cur, (void)++first, --n)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp(*first);
  __g.release();
  return cur;
}

// [uninitialized.move]
template <class _InputIterator, class _NoThrowForwardIterator>
constexpr _NoThrowForwardIterator uninitialized_move(_InputIterator first, _InputIterator last,
                                                    _NoThrowForwardIterator result) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = result;
  __ycxx::__detail::__uninit_guard __g{result, __builtin_addressof(cur)};
  for (; first != last; (void)++cur, ++first)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp(::__ycxx::__detail::__deref_move(first));
  __g.release();
  return cur;
}
template <class _InputIterator, class _Size, class _NoThrowForwardIterator>
constexpr pair<_InputIterator, _NoThrowForwardIterator> uninitialized_move_n(_InputIterator first, _Size n,
                                                                           _NoThrowForwardIterator result) {
  using _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = result;
  __ycxx::__detail::__uninit_guard __g{result, __builtin_addressof(cur)};
  for (; n > 0; ++cur, (void)++first, --n)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Tp(::__ycxx::__detail::__deref_move(first));
  __g.release();
  return {first, cur};
}

// [uninitialized.fill]
template <class _NoThrowForwardIterator, class _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type>
constexpr void uninitialized_fill(_NoThrowForwardIterator first, _NoThrowForwardIterator last, const _Tp& __x) {
  using _Vp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = first;
  __ycxx::__detail::__uninit_guard __g{first, __builtin_addressof(cur)};
  for (; cur != last; ++cur)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Vp(__x);
  __g.release();
}
template <class _NoThrowForwardIterator, class _Size,
          class _Tp = typename iterator_traits<_NoThrowForwardIterator>::value_type>
constexpr _NoThrowForwardIterator uninitialized_fill_n(_NoThrowForwardIterator first, _Size n, const _Tp& __x) {
  using _Vp = typename iterator_traits<_NoThrowForwardIterator>::value_type;
  _NoThrowForwardIterator cur = first;
  __ycxx::__detail::__uninit_guard __g{first, __builtin_addressof(cur)};
  for (; n--; ++cur)
    ::new (::__ycxx::__detail::__voidify(*cur)) _Vp(__x);
  __g.release();
  return cur;
}

}} // namespace std

// ---------------------------------------------------------------------------------------------
// std::ranges forms
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {
template <class _Ip, class _Op>
using uninitialized_copy_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using uninitialized_copy_n_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using uninitialized_move_result = in_out_result<_Ip, _Op>;
template <class _Ip, class _Op>
using uninitialized_move_n_result = in_out_result<_Ip, _Op>;
}}} // namespace std::ranges

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__uninit_fn {

using std::iter_difference_t;
using std::iter_value_t;
using std::ranges::borrowed_iterator_t;
using std::ranges::range_value_t;

// The element type the range forms construct: remove_reference_t<iter_reference_t<I>>.
template <class _Ip>
using __elem_t = std::remove_reference_t<std::iter_reference_t<_Ip>>;

struct __default_construct {
  template <__nothrow_forward_iterator _Ip, __nothrow_sentinel_for<_Ip> _Sp>
    requires std::default_initializable<iter_value_t<_Ip>>
  static constexpr _Ip operator()(_Ip first, _Sp last) {
    _Ip cur = first;
    __uninit_guard __g{first, __builtin_addressof(cur)};
    for (; cur != last; ++cur)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Ip>;
    __g.release();
    return cur;
  }
  template <__nothrow_forward_range _Rp>
    requires std::default_initializable<range_value_t<_Rp>>
  static constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r) {
    return operator()(std::ranges::begin(r), std::ranges::end(r));
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip, __nothrow_sized_sentinel_for<_Ip> _Sp>
    requires std::default_initializable<iter_value_t<_Ip>>
  static _Ip operator()(_Ep_&&, _Ip first, _Sp last) noexcept {
    return operator()(static_cast<_Ip&&>(first), static_cast<_Sp&&>(last));
  }
  template <__execution_policy _Ep_, __nothrow_sized_random_access_range _Rp>
    requires std::default_initializable<range_value_t<_Rp>>
  static borrowed_iterator_t<_Rp> operator()(_Ep_&&, _Rp&& r) noexcept {
    return operator()(static_cast<_Rp&&>(r));
  }
};
struct __default_construct_n {
  template <__nothrow_forward_iterator _Ip>
    requires std::default_initializable<iter_value_t<_Ip>>
  static constexpr _Ip operator()(_Ip first, iter_difference_t<_Ip> n) {
    _Ip cur = first;
    __uninit_guard __g{first, __builtin_addressof(cur)};
    for (; n > 0; (void)++cur, --n)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Ip>;
    __g.release();
    return cur;
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip>
    requires std::default_initializable<iter_value_t<_Ip>>
  static _Ip operator()(_Ep_&&, _Ip first, iter_difference_t<_Ip> n) noexcept {
    return operator()(static_cast<_Ip&&>(first), n);
  }
};

struct __value_construct {
  template <__nothrow_forward_iterator _Ip, __nothrow_sentinel_for<_Ip> _Sp>
    requires std::default_initializable<iter_value_t<_Ip>>
  static constexpr _Ip operator()(_Ip first, _Sp last) {
    _Ip cur = first;
    __uninit_guard __g{first, __builtin_addressof(cur)};
    for (; cur != last; ++cur)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Ip>();
    __g.release();
    return cur;
  }
  template <__nothrow_forward_range _Rp>
    requires std::default_initializable<range_value_t<_Rp>>
  static constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r) {
    return operator()(std::ranges::begin(r), std::ranges::end(r));
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip, __nothrow_sized_sentinel_for<_Ip> _Sp>
    requires std::default_initializable<iter_value_t<_Ip>>
  static _Ip operator()(_Ep_&&, _Ip first, _Sp last) noexcept {
    return operator()(static_cast<_Ip&&>(first), static_cast<_Sp&&>(last));
  }
  template <__execution_policy _Ep_, __nothrow_sized_random_access_range _Rp>
    requires std::default_initializable<range_value_t<_Rp>>
  static borrowed_iterator_t<_Rp> operator()(_Ep_&&, _Rp&& r) noexcept {
    return operator()(static_cast<_Rp&&>(r));
  }
};
struct __value_construct_n {
  template <__nothrow_forward_iterator _Ip>
    requires std::default_initializable<iter_value_t<_Ip>>
  static constexpr _Ip operator()(_Ip first, iter_difference_t<_Ip> n) {
    _Ip cur = first;
    __uninit_guard __g{first, __builtin_addressof(cur)};
    for (; n > 0; (void)++cur, --n)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Ip>();
    __g.release();
    return cur;
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip>
    requires std::default_initializable<iter_value_t<_Ip>>
  static _Ip operator()(_Ep_&&, _Ip first, iter_difference_t<_Ip> n) noexcept {
    return operator()(static_cast<_Ip&&>(first), n);
  }
};

struct copy {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _S1, __nothrow_forward_iterator _Op, __nothrow_sentinel_for<_Op> _S2>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_reference_t<_Ip>>
  static constexpr std::ranges::uninitialized_copy_result<_Ip, _Op> operator()(_Ip __ifirst, _S1 __ilast, _Op __ofirst, _S2 __olast) {
    _Op cur = __ofirst;
    __uninit_guard __g{__ofirst, __builtin_addressof(cur)};
    for (; __ifirst != __ilast && cur != __olast; ++cur, (void)++__ifirst)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Op>(*__ifirst);
    __g.release();
    return {static_cast<_Ip&&>(__ifirst), cur};
  }
  template <std::ranges::input_range _IR, __nothrow_forward_range _OR>
    requires std::constructible_from<range_value_t<_OR>, std::ranges::range_reference_t<_IR>>
  static constexpr std::ranges::uninitialized_copy_result<borrowed_iterator_t<_IR>, borrowed_iterator_t<_OR>>
  operator()(_IR&& in_range, _OR&& __out_range) {
    auto r = operator()(std::ranges::begin(in_range), std::ranges::end(in_range), std::ranges::begin(__out_range),
                        std::ranges::end(__out_range));
    return {static_cast<decltype(r.in)&&>(r.in), r.out};
  }
  template <__execution_policy _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _S1,
            __nothrow_random_access_iterator _Op, __nothrow_sized_sentinel_for<_Op> _S2>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_reference_t<_Ip>>
  static std::ranges::uninitialized_copy_result<_Ip, _Op> operator()(_Ep_&&, _Ip __ifirst, _S1 __ilast, _Op __ofirst, _S2 __olast) noexcept {
    return operator()(static_cast<_Ip&&>(__ifirst), static_cast<_S1&&>(__ilast), static_cast<_Op&&>(__ofirst),
                      static_cast<_S2&&>(__olast));
  }
  template <__execution_policy _Ep_, __sized_random_access_range _IR, __nothrow_sized_random_access_range _OR>
    requires std::constructible_from<range_value_t<_OR>, std::ranges::range_reference_t<_IR>>
  static std::ranges::uninitialized_copy_result<borrowed_iterator_t<_IR>, borrowed_iterator_t<_OR>>
  operator()(_Ep_&&, _IR&& in_range, _OR&& __out_range) noexcept {
    return operator()(static_cast<_IR&&>(in_range), static_cast<_OR&&>(__out_range));
  }
};
struct copy_n {
  template <std::input_iterator _Ip, __nothrow_forward_iterator _Op, __nothrow_sentinel_for<_Op> _Sp>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_reference_t<_Ip>>
  static constexpr std::ranges::uninitialized_copy_n_result<_Ip, _Op> operator()(_Ip __ifirst, iter_difference_t<_Ip> n,
                                                                             _Op __ofirst, _Sp __olast) {
    _Op cur = __ofirst;
    __uninit_guard __g{__ofirst, __builtin_addressof(cur)};
    for (; n > 0 && cur != __olast; ++cur, (void)++__ifirst, --n)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Op>(*__ifirst);
    __g.release();
    return {static_cast<_Ip&&>(__ifirst), cur};
  }
  template <__execution_policy _Ep_, std::random_access_iterator _Ip, __nothrow_random_access_iterator _Op,
            __nothrow_sized_sentinel_for<_Op> _Sp>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_reference_t<_Ip>>
  static std::ranges::uninitialized_copy_n_result<_Ip, _Op> operator()(_Ep_&&, _Ip __ifirst, iter_difference_t<_Ip> n, _Op __ofirst,
                                                                   _Sp __olast) noexcept {
    return operator()(static_cast<_Ip&&>(__ifirst), n, static_cast<_Op&&>(__ofirst), static_cast<_Sp&&>(__olast));
  }
};

struct move {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _S1, __nothrow_forward_iterator _Op, __nothrow_sentinel_for<_Op> _S2>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_rvalue_reference_t<_Ip>>
  static constexpr std::ranges::uninitialized_move_result<_Ip, _Op> operator()(_Ip __ifirst, _S1 __ilast, _Op __ofirst, _S2 __olast) {
    _Op cur = __ofirst;
    __uninit_guard __g{__ofirst, __builtin_addressof(cur)};
    for (; __ifirst != __ilast && cur != __olast; ++cur, (void)++__ifirst)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Op>(std::ranges::iter_move(__ifirst));
    __g.release();
    return {static_cast<_Ip&&>(__ifirst), cur};
  }
  template <std::ranges::input_range _IR, __nothrow_forward_range _OR>
    requires std::constructible_from<range_value_t<_OR>, std::ranges::range_rvalue_reference_t<_IR>>
  static constexpr std::ranges::uninitialized_move_result<borrowed_iterator_t<_IR>, borrowed_iterator_t<_OR>>
  operator()(_IR&& in_range, _OR&& __out_range) {
    auto r = operator()(std::ranges::begin(in_range), std::ranges::end(in_range), std::ranges::begin(__out_range),
                        std::ranges::end(__out_range));
    return {static_cast<decltype(r.in)&&>(r.in), r.out};
  }
  template <__execution_policy _Ep_, std::random_access_iterator _Ip, std::sized_sentinel_for<_Ip> _S1,
            __nothrow_random_access_iterator _Op, __nothrow_sized_sentinel_for<_Op> _S2>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_rvalue_reference_t<_Ip>>
  static std::ranges::uninitialized_move_result<_Ip, _Op> operator()(_Ep_&&, _Ip __ifirst, _S1 __ilast, _Op __ofirst, _S2 __olast) noexcept {
    return operator()(static_cast<_Ip&&>(__ifirst), static_cast<_S1&&>(__ilast), static_cast<_Op&&>(__ofirst),
                      static_cast<_S2&&>(__olast));
  }
  template <__execution_policy _Ep_, __sized_random_access_range _IR, __nothrow_sized_random_access_range _OR>
    requires std::constructible_from<range_value_t<_OR>, std::ranges::range_rvalue_reference_t<_IR>>
  static std::ranges::uninitialized_move_result<borrowed_iterator_t<_IR>, borrowed_iterator_t<_OR>>
  operator()(_Ep_&&, _IR&& in_range, _OR&& __out_range) noexcept {
    return operator()(static_cast<_IR&&>(in_range), static_cast<_OR&&>(__out_range));
  }
};
struct __move_n {
  template <std::input_iterator _Ip, __nothrow_forward_iterator _Op, __nothrow_sentinel_for<_Op> _Sp>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_rvalue_reference_t<_Ip>>
  static constexpr std::ranges::uninitialized_move_n_result<_Ip, _Op> operator()(_Ip __ifirst, iter_difference_t<_Ip> n,
                                                                             _Op __ofirst, _Sp __olast) {
    _Op cur = __ofirst;
    __uninit_guard __g{__ofirst, __builtin_addressof(cur)};
    for (; n > 0 && cur != __olast; ++cur, (void)++__ifirst, --n)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Op>(std::ranges::iter_move(__ifirst));
    __g.release();
    return {static_cast<_Ip&&>(__ifirst), cur};
  }
  template <__execution_policy _Ep_, std::random_access_iterator _Ip, __nothrow_random_access_iterator _Op,
            __nothrow_sized_sentinel_for<_Op> _Sp>
    requires std::constructible_from<iter_value_t<_Op>, std::iter_rvalue_reference_t<_Ip>>
  static std::ranges::uninitialized_move_n_result<_Ip, _Op> operator()(_Ep_&&, _Ip __ifirst, iter_difference_t<_Ip> n, _Op __ofirst,
                                                                   _Sp __olast) noexcept {
    return operator()(static_cast<_Ip&&>(__ifirst), n, static_cast<_Op&&>(__ofirst), static_cast<_Sp&&>(__olast));
  }
};

struct fill {
  template <__nothrow_forward_iterator _Ip, __nothrow_sentinel_for<_Ip> _Sp, class _Tp = iter_value_t<_Ip>>
    requires std::constructible_from<iter_value_t<_Ip>, const _Tp&>
  static constexpr _Ip operator()(_Ip first, _Sp last, const _Tp& __x) {
    _Ip cur = first;
    __uninit_guard __g{first, __builtin_addressof(cur)};
    for (; cur != last; ++cur)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Ip>(__x);
    __g.release();
    return cur;
  }
  template <__nothrow_forward_range _Rp, class _Tp = range_value_t<_Rp>>
    requires std::constructible_from<range_value_t<_Rp>, const _Tp&>
  static constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, const _Tp& __x) {
    return operator()(std::ranges::begin(r), std::ranges::end(r), __x);
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip, __nothrow_sized_sentinel_for<_Ip> _Sp,
            class _Tp = iter_value_t<_Ip>>
    requires std::constructible_from<iter_value_t<_Ip>, const _Tp&>
  static _Ip operator()(_Ep_&&, _Ip first, _Sp last, const _Tp& __x) noexcept {
    return operator()(static_cast<_Ip&&>(first), static_cast<_Sp&&>(last), __x);
  }
  template <__execution_policy _Ep_, __nothrow_sized_random_access_range _Rp, class _Tp = range_value_t<_Rp>>
    requires std::constructible_from<range_value_t<_Rp>, const _Tp&>
  static borrowed_iterator_t<_Rp> operator()(_Ep_&&, _Rp&& r, const _Tp& __x) noexcept {
    return operator()(static_cast<_Rp&&>(r), __x);
  }
};
struct fill_n {
  template <__nothrow_forward_iterator _Ip, class _Tp = iter_value_t<_Ip>>
    requires std::constructible_from<iter_value_t<_Ip>, const _Tp&>
  static constexpr _Ip operator()(_Ip first, iter_difference_t<_Ip> n, const _Tp& __x) {
    _Ip cur = first;
    __uninit_guard __g{first, __builtin_addressof(cur)};
    for (; n > 0; (void)++cur, --n)
      ::new (::__ycxx::__detail::__voidify(*cur)) __elem_t<_Ip>(__x);
    __g.release();
    return cur;
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip, class _Tp = iter_value_t<_Ip>>
    requires std::constructible_from<iter_value_t<_Ip>, const _Tp&>
  static _Ip operator()(_Ep_&&, _Ip first, iter_difference_t<_Ip> n, const _Tp& __x) noexcept {
    return operator()(static_cast<_Ip&&>(first), n, __x);
  }
};

// [specialized.destroy]
struct destroy {
  template <__nothrow_input_iterator _Ip, __nothrow_sentinel_for<_Ip> _Sp>
    requires std::destructible<iter_value_t<_Ip>>
  static constexpr _Ip operator()(_Ip first, _Sp last) noexcept {
    for (; first != last; ++first)
      std::destroy_at(__builtin_addressof(*first));
    return first;
  }
  template <__nothrow_input_range _Rp>
    requires std::destructible<range_value_t<_Rp>>
  static constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r) noexcept {
    return operator()(std::ranges::begin(r), std::ranges::end(r));
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip, __nothrow_sized_sentinel_for<_Ip> _Sp>
    requires std::destructible<iter_value_t<_Ip>>
  static _Ip operator()(_Ep_&&, _Ip first, _Sp last) noexcept {
    return operator()(static_cast<_Ip&&>(first), static_cast<_Sp&&>(last));
  }
  template <__execution_policy _Ep_, __nothrow_sized_random_access_range _Rp>
    requires std::destructible<range_value_t<_Rp>>
  static borrowed_iterator_t<_Rp> operator()(_Ep_&&, _Rp&& r) noexcept {
    return operator()(static_cast<_Rp&&>(r));
  }
};
struct destroy_n {
  template <__nothrow_input_iterator _Ip>
    requires std::destructible<iter_value_t<_Ip>>
  static constexpr _Ip operator()(_Ip first, iter_difference_t<_Ip> n) noexcept {
    for (; n > 0; (void)++first, --n)
      std::destroy_at(__builtin_addressof(*first));
    return first;
  }
  template <__execution_policy _Ep_, __nothrow_random_access_iterator _Ip>
    requires std::destructible<iter_value_t<_Ip>>
  static _Ip operator()(_Ep_&&, _Ip first, iter_difference_t<_Ip> n) noexcept {
    return operator()(static_cast<_Ip&&>(first), n);
  }
};

}} // namespace __ycxx::__detail::__uninit_fn

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace ranges {
inline constexpr __ycxx::__detail::__uninit_fn::__default_construct uninitialized_default_construct{};
inline constexpr __ycxx::__detail::__uninit_fn::__default_construct_n uninitialized_default_construct_n{};
inline constexpr __ycxx::__detail::__uninit_fn::__value_construct uninitialized_value_construct{};
inline constexpr __ycxx::__detail::__uninit_fn::__value_construct_n uninitialized_value_construct_n{};
inline constexpr __ycxx::__detail::__uninit_fn::copy uninitialized_copy{};
inline constexpr __ycxx::__detail::__uninit_fn::copy_n uninitialized_copy_n{};
inline constexpr __ycxx::__detail::__uninit_fn::move uninitialized_move{};
inline constexpr __ycxx::__detail::__uninit_fn::__move_n uninitialized_move_n{};
inline constexpr __ycxx::__detail::__uninit_fn::fill uninitialized_fill{};
inline constexpr __ycxx::__detail::__uninit_fn::fill_n uninitialized_fill_n{};
inline constexpr __ycxx::__detail::__uninit_fn::destroy destroy{};
inline constexpr __ycxx::__detail::__uninit_fn::destroy_n destroy_n{};
}}} // namespace std::ranges
