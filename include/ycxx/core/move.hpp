// libycxx core: move, forward, forward_like, move_if_noexcept, as_const, addressof.
#ifndef YCXX_CORE_MOVE_HPP
#define YCXX_CORE_MOVE_HPP

#include <ycxx/core/meta_base.hpp>

namespace std {

template <class T>
[[nodiscard]] YCXX_INTRINSIC constexpr T&& forward(remove_reference_t<T>& t) noexcept {
  return static_cast<T&&>(t);
}
template <class T>
[[nodiscard]] YCXX_INTRINSIC constexpr T&& forward(remove_reference_t<T>&& t) noexcept {
  static_assert(!__is_lvalue_reference(T), "std::forward: cannot forward an rvalue as an lvalue");
  return static_cast<T&&>(t);
}

template <class T>
[[nodiscard]] YCXX_INTRINSIC constexpr remove_reference_t<T>&& move(T&& t) noexcept {
  return static_cast<remove_reference_t<T>&&>(t);
}

template <class T, class U>
[[nodiscard]] YCXX_INTRINSIC constexpr auto&& forward_like(U&& x) noexcept {
  constexpr bool is_adding_const = __is_const(__remove_reference_t(T));
  if constexpr (__is_lvalue_reference(T&&)) {
    if constexpr (is_adding_const)
      return static_cast<const __remove_reference_t(U)&>(x);
    else
      return static_cast<__remove_reference_t(U)&>(x);
  } else {
    if constexpr (is_adding_const)
      return static_cast<const __remove_reference_t(U)&&>(x);
    else
      return static_cast<__remove_reference_t(U)&&>(x);
  }
}

template <class T>
[[nodiscard]] YCXX_INTRINSIC constexpr conditional_t<
    !__is_nothrow_constructible(T, T&&) && __is_constructible(T, const T&), const T&, T&&>
move_if_noexcept(T& x) noexcept {
  return static_cast<conditional_t<!__is_nothrow_constructible(T, T&&) && __is_constructible(T, const T&),
                                   const T&, T&&>>(x);
}

template <class T>
[[nodiscard]] YCXX_INTRINSIC constexpr add_const_t<T>& as_const(T& t) noexcept {
  return t;
}
template <class T>
void as_const(const T&&) = delete;

template <class T>
[[nodiscard]] YCXX_INTRINSIC constexpr T* addressof(T& r) noexcept {
  return __builtin_addressof(r);
}
template <class T>
const T* addressof(const T&&) = delete;

} // namespace std

// Shorthands used inside the library (avoid template instantiation of std::move/forward).
#define YCXX_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

#endif // YCXX_CORE_MOVE_HPP
