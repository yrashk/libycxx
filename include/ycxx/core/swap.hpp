// libycxx core: std::swap (needed by <type_traits> for is_swappable).
#pragma once

#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/move.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
  requires(is_constructible_v<_Tp, _Tp &&> && is_assignable_v<_Tp&, _Tp &&>)
constexpr void swap(_Tp& a, _Tp& b) noexcept(is_nothrow_constructible_v<_Tp, _Tp &&> && is_nothrow_assignable_v<_Tp&, _Tp &&>) {
  _Tp __tmp(static_cast<_Tp&&>(a));
  a = static_cast<_Tp&&>(b);
  b = static_cast<_Tp&&>(__tmp);
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// Defined below, once both swap overloads are visible (needed for multidimensional arrays).
template <class _Tp>
struct __swappable_elem;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Tp, size_t _Np>
  requires __ycxx::__detail::__swappable_elem<_Tp>::value
constexpr void swap(_Tp (&a)[_Np], _Tp (&b)[_Np]) noexcept(__ycxx::__detail::__swappable_elem<_Tp>::nothrow);
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__swap_adl {
using std::swap;

template <class _Tp, class _Up>
concept __swappable_with_ = requires(_Tp&& t, _Up&& __u) {
  swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
  swap(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t));
};

template <class _Tp, class _Up>
concept __nothrow_swappable_with_ = __swappable_with_<_Tp, _Up> && requires(_Tp&& t, _Up&& __u) {
  { swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)) } noexcept;
  { swap(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t)) } noexcept;
};

// Calls swap with std::swap visible.
template <class _Tp, class _Up>
constexpr void __do_swap(_Tp&& t, _Up&& __u) noexcept(noexcept(swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)))) {
  swap(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
}
}} // namespace __ycxx::__detail::__swap_adl

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
struct __swappable_elem {
  static constexpr bool value = __swap_adl::__swappable_with_<_Tp&, _Tp&>;
  static constexpr bool nothrow = __swap_adl::__nothrow_swappable_with_<_Tp&, _Tp&>;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class _Up>
struct is_swappable_with : bool_constant<__ycxx::__detail::__swap_adl::__swappable_with_<_Tp, _Up>> {};
template <class _Tp, class _Up>
inline constexpr bool is_swappable_with_v = __ycxx::__detail::__swap_adl::__swappable_with_<_Tp, _Up>;
template <class _Tp>
struct is_swappable : bool_constant<__ycxx::__detail::__swap_adl::__swappable_with_<__add_lvalue_reference(_Tp),
                                                                           __add_lvalue_reference(_Tp)>> {};
template <class _Tp>
inline constexpr bool is_swappable_v =
    __ycxx::__detail::__swap_adl::__swappable_with_<__add_lvalue_reference(_Tp), __add_lvalue_reference(_Tp)>;

template <class _Tp, class _Up>
struct is_nothrow_swappable_with : bool_constant<__ycxx::__detail::__swap_adl::__nothrow_swappable_with_<_Tp, _Up>> {};
template <class _Tp, class _Up>
inline constexpr bool is_nothrow_swappable_with_v = __ycxx::__detail::__swap_adl::__nothrow_swappable_with_<_Tp, _Up>;
template <class _Tp>
struct is_nothrow_swappable
    : bool_constant<__ycxx::__detail::__swap_adl::__nothrow_swappable_with_<__add_lvalue_reference(_Tp),
                                                                    __add_lvalue_reference(_Tp)>> {};
template <class _Tp>
inline constexpr bool is_nothrow_swappable_v =
    __ycxx::__detail::__swap_adl::__nothrow_swappable_with_<__add_lvalue_reference(_Tp), __add_lvalue_reference(_Tp)>;

template <class _Tp, size_t _Np>
  requires __ycxx::__detail::__swappable_elem<_Tp>::value
constexpr void swap(_Tp (&a)[_Np], _Tp (&b)[_Np]) noexcept(__ycxx::__detail::__swappable_elem<_Tp>::nothrow) {
  for (size_t i = 0; i != _Np; ++i)
    __ycxx::__detail::__swap_adl::__do_swap(a[i], b[i]);
}

}} // namespace std

