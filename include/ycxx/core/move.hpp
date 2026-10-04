// libycxx core: move, forward, forward_like, move_if_noexcept, as_const, addressof.
#pragma once

#include <ycxx/core/meta_base.hpp>

namespace std {

template <class T>
[[nodiscard]] [[gnu::always_inline]] constexpr T&& forward(remove_reference_t<T>& t) noexcept {
  return static_cast<T&&>(t);
}
template <class T>
[[nodiscard]] [[gnu::always_inline]] constexpr T&& forward(remove_reference_t<T>&& t) noexcept {
  static_assert(!::ycxx::detail::is_lref_v<T>, "std::forward: cannot forward an rvalue as an lvalue");
  return static_cast<T&&>(t);
}

template <class T>
[[nodiscard]] [[gnu::always_inline]] constexpr remove_reference_t<T>&& move(T&& t) noexcept {
  return static_cast<remove_reference_t<T>&&>(t);
}

} // namespace std

namespace ycxx::detail {
// [forward]/6: V = OVERRIDE_REF(T&&, COPY_CONST(remove_reference_t<T>, remove_reference_t<U>)).
template <class T, class U>
using forward_like_base = std::conditional_t<std::is_const_v<std::remove_reference_t<T>>,
                                             const std::remove_reference_t<U>, std::remove_reference_t<U>>;
template <class T, class U>
using forward_like_t =
    std::conditional_t<is_lref_v<T&&>, forward_like_base<T, U>&, forward_like_base<T, U>&&>;
} // namespace ycxx::detail

namespace std {

template <class T, class U>
[[nodiscard]] [[gnu::always_inline]] constexpr auto forward_like(U&& x) noexcept
    -> ::ycxx::detail::forward_like_t<T, U> {
  return static_cast<::ycxx::detail::forward_like_t<T, U>>(x);
}

template <class T>
[[nodiscard]] [[gnu::always_inline]] constexpr conditional_t<
    !is_nothrow_constructible_v<T, T&&> && is_constructible_v<T, const T&>, const T&, T&&>
move_if_noexcept(T& x) noexcept {
  using R = conditional_t<!is_nothrow_constructible_v<T, T&&> && is_constructible_v<T, const T&>, const T&, T&&>;
  return static_cast<R>(x);
}

template <class T>
[[nodiscard]] [[gnu::always_inline]] constexpr add_const_t<T>& as_const(T& t) noexcept {
  return t;
}
template <class T>
void as_const(const T&&) = delete;

template <class T>
[[nodiscard]] [[gnu::always_inline]] constexpr T* addressof(T& r) noexcept {
  return __builtin_addressof(r);
}
template <class T>
const T* addressof(const T&&) = delete;

} // namespace std


