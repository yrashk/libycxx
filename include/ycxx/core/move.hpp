// libycxx core: move, forward, forward_like, move_if_noexcept, as_const, addressof.
#pragma once

#include <ycxx/core/meta_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr _Tp&& forward(remove_reference_t<_Tp>& t) noexcept {
  return static_cast<_Tp&&>(t);
}
template <class _Tp>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr _Tp&& forward(remove_reference_t<_Tp>&& t) noexcept {
  static_assert(!::__ycxx::__detail::__is_lref_v<_Tp>, "std::forward: cannot forward an rvalue as an lvalue");
  return static_cast<_Tp&&>(t);
}

template <class _Tp>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr remove_reference_t<_Tp>&& move(_Tp&& t) noexcept {
  return static_cast<remove_reference_t<_Tp>&&>(t);
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// [forward]/6: V = OVERRIDE_REF(T&&, COPY_CONST(remove_reference_t<T>, remove_reference_t<U>)).
template <class _Tp, class _Up>
using __forward_like_base = std::conditional_t<std::is_const_v<std::remove_reference_t<_Tp>>,
                                             const std::remove_reference_t<_Up>, std::remove_reference_t<_Up>>;
template <class _Tp, class _Up>
using __forward_like_t =
    std::conditional_t<__is_lref_v<_Tp&&>, __forward_like_base<_Tp, _Up>&, __forward_like_base<_Tp, _Up>&&>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class _Up>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr auto forward_like(_Up&& __x) noexcept
    -> ::__ycxx::__detail::__forward_like_t<_Tp, _Up> {
  return static_cast<::__ycxx::__detail::__forward_like_t<_Tp, _Up>>(__x);
}

template <class _Tp>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr conditional_t<
    !is_nothrow_constructible_v<_Tp, _Tp&&> && is_constructible_v<_Tp, const _Tp&>, const _Tp&, _Tp&&>
move_if_noexcept(_Tp& __x) noexcept {
  using _Rp = conditional_t<!is_nothrow_constructible_v<_Tp, _Tp&&> && is_constructible_v<_Tp, const _Tp&>, const _Tp&, _Tp&&>;
  return static_cast<_Rp>(__x);
}

template <class _Tp>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr add_const_t<_Tp>& as_const(_Tp& t) noexcept {
  return t;
}
template <class _Tp>
void as_const(const _Tp&&) = delete;

template <class _Tp>
[[nodiscard]] [[__gnu__::__always_inline__]] constexpr _Tp* addressof(_Tp& r) noexcept {
  return __builtin_addressof(r);
}
template <class _Tp>
const _Tp* addressof(const _Tp&&) = delete;

}} // namespace std


