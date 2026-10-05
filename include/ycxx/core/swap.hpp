// libycxx core: std::swap (needed by <type_traits> for is_swappable).
#pragma once

#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/move.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class T>
  requires(is_constructible_v<T, T &&> && is_assignable_v<T&, T &&>)
constexpr void swap(T& a, T& b) noexcept(is_nothrow_constructible_v<T, T &&> && is_nothrow_assignable_v<T&, T &&>) {
  T tmp(static_cast<T&&>(a));
  a = static_cast<T&&>(b);
  b = static_cast<T&&>(tmp);
}

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// Defined below, once both swap overloads are visible (needed for multidimensional arrays).
template <class T>
struct swappable_elem;
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {
template <class T, size_t N>
  requires ycxx::detail::swappable_elem<T>::value
constexpr void swap(T (&a)[N], T (&b)[N]) noexcept(ycxx::detail::swappable_elem<T>::nothrow);
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::swap_adl {
using std::swap;

template <class T, class U>
concept swappable_with_ = requires(T&& t, U&& u) {
  swap(static_cast<T&&>(t), static_cast<U&&>(u));
  swap(static_cast<U&&>(u), static_cast<T&&>(t));
};

template <class T, class U>
concept nothrow_swappable_with_ = swappable_with_<T, U> && requires(T&& t, U&& u) {
  { swap(static_cast<T&&>(t), static_cast<U&&>(u)) } noexcept;
  { swap(static_cast<U&&>(u), static_cast<T&&>(t)) } noexcept;
};

// Calls swap with std::swap visible.
template <class T, class U>
constexpr void do_swap(T&& t, U&& u) noexcept(noexcept(swap(static_cast<T&&>(t), static_cast<U&&>(u)))) {
  swap(static_cast<T&&>(t), static_cast<U&&>(u));
}
}} // namespace ycxx::detail::swap_adl

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class T>
struct swappable_elem {
  static constexpr bool value = swap_adl::swappable_with_<T&, T&>;
  static constexpr bool nothrow = swap_adl::nothrow_swappable_with_<T&, T&>;
};
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T, class U>
struct is_swappable_with : bool_constant<ycxx::detail::swap_adl::swappable_with_<T, U>> {};
template <class T, class U>
inline constexpr bool is_swappable_with_v = ycxx::detail::swap_adl::swappable_with_<T, U>;
template <class T>
struct is_swappable : bool_constant<ycxx::detail::swap_adl::swappable_with_<__add_lvalue_reference(T),
                                                                           __add_lvalue_reference(T)>> {};
template <class T>
inline constexpr bool is_swappable_v =
    ycxx::detail::swap_adl::swappable_with_<__add_lvalue_reference(T), __add_lvalue_reference(T)>;

template <class T, class U>
struct is_nothrow_swappable_with : bool_constant<ycxx::detail::swap_adl::nothrow_swappable_with_<T, U>> {};
template <class T, class U>
inline constexpr bool is_nothrow_swappable_with_v = ycxx::detail::swap_adl::nothrow_swappable_with_<T, U>;
template <class T>
struct is_nothrow_swappable
    : bool_constant<ycxx::detail::swap_adl::nothrow_swappable_with_<__add_lvalue_reference(T),
                                                                    __add_lvalue_reference(T)>> {};
template <class T>
inline constexpr bool is_nothrow_swappable_v =
    ycxx::detail::swap_adl::nothrow_swappable_with_<__add_lvalue_reference(T), __add_lvalue_reference(T)>;

template <class T, size_t N>
  requires ycxx::detail::swappable_elem<T>::value
constexpr void swap(T (&a)[N], T (&b)[N]) noexcept(ycxx::detail::swappable_elem<T>::nothrow) {
  for (size_t i = 0; i != N; ++i)
    ycxx::detail::swap_adl::do_swap(a[i], b[i]);
}

} // namespace std

