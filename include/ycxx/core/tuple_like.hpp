// libycxx core: the tuple protocol (tuple_size / tuple_element) and the tuple-like concept.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/integer_sequence.hpp>

namespace std {

template <class T>
struct tuple_size;

template <class T>
  requires requires { tuple_size<T>::value; }
struct tuple_size<const T> : integral_constant<size_t, tuple_size<T>::value> {};
// tuple_size<volatile T> / <const volatile T> were deprecated (C++20) and are not provided.

template <class T>
constexpr size_t tuple_size_v = tuple_size<T>::value;

template <size_t I, class T>
struct tuple_element;

template <size_t I, class T>
struct tuple_element<I, const T> {
  using type = const typename tuple_element<I, T>::type;
};

template <size_t I, class T>
using tuple_element_t = typename tuple_element<I, T>::type;

template <class... Types>
class tuple;
template <class T1, class T2>
struct pair;
template <class T, size_t N>
struct array;
template <class T>
class complex;

} // namespace std

namespace ycxx::detail {

// Makes `get<I>(x)` parse as a template-id inside ycxx::detail so that argument-dependent
// lookup finds the std::get overloads of every tuple-like type. Never selected.
struct get_poison {};
template <std::size_t>
void get(get_poison) = delete;

// [tuple.like]: specializations of array, complex, pair, tuple, ranges::subrange.
template <class T>
inline constexpr bool is_tuple_like_impl = false;
template <class... Ts>
inline constexpr bool is_tuple_like_impl<std::tuple<Ts...>> = true;
template <class T1, class T2>
inline constexpr bool is_tuple_like_impl<std::pair<T1, T2>> = true;
template <class T, std::size_t N>
inline constexpr bool is_tuple_like_impl<std::array<T, N>> = true;
template <class T>
inline constexpr bool is_tuple_like_impl<std::complex<T>> = true;
// ranges::subrange adds its own specialization in <ranges>.

template <class T>
concept tuple_like = is_tuple_like_impl<__remove_cvref(T)>;

template <class T>
concept pair_like = tuple_like<T> && std::tuple_size_v<__remove_cvref(T)> == 2;

} // namespace ycxx::detail

// ---------------------------------------------------------------------------------------------
// [meta.rel] is_applicable / is_nothrow_applicable, [meta.trans.other] apply_result
// ELEMS-OF(Tuple) is get<I>(declval<Tuple>())... (found by ADL).
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail {
template <class Fn, class Tuple, std::size_t... I>
consteval bool applicable_impl(std::index_sequence<I...>*) {
  return requires { ::ycxx::detail::invoke(std::declval<Fn>(), get<I>(std::declval<Tuple>())...); };
}
template <class Fn, class Tuple, std::size_t... I>
consteval bool nothrow_applicable_impl(std::index_sequence<I...>*) {
  return requires {
    { ::ycxx::detail::invoke(std::declval<Fn>(), get<I>(std::declval<Tuple>())...) } noexcept;
  };
}
template <class Fn, class Tuple, std::size_t... I>
auto apply_result_impl(std::index_sequence<I...>*)
    -> decltype(::ycxx::detail::invoke(std::declval<Fn>(), get<I>(std::declval<Tuple>())...));

template <class Tuple>
using tuple_indices = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>;

template <class Fn, class Tuple>
consteval bool is_applicable_v() {
  if constexpr (tuple_like<Tuple>)
    return applicable_impl<Fn, Tuple>(static_cast<tuple_indices<Tuple>*>(nullptr));
  else
    return false;
}
template <class Fn, class Tuple>
consteval bool is_nothrow_applicable_v() {
  if constexpr (tuple_like<Tuple>)
    return nothrow_applicable_impl<Fn, Tuple>(static_cast<tuple_indices<Tuple>*>(nullptr));
  else
    return false;
}
} // namespace ycxx::detail

namespace std {
template <class Fn, class Tuple>
struct is_applicable : bool_constant<ycxx::detail::is_applicable_v<Fn, Tuple>()> {};
template <class Fn, class Tuple>
constexpr bool is_applicable_v = ycxx::detail::is_applicable_v<Fn, Tuple>();
template <class Fn, class Tuple>
struct is_nothrow_applicable : bool_constant<ycxx::detail::is_nothrow_applicable_v<Fn, Tuple>()> {};
template <class Fn, class Tuple>
constexpr bool is_nothrow_applicable_v = ycxx::detail::is_nothrow_applicable_v<Fn, Tuple>();

template <class Fn, class Tuple>
struct apply_result {};
template <class Fn, class Tuple>
  requires(ycxx::detail::is_applicable_v<Fn, Tuple>())
struct apply_result<Fn, Tuple> {
  using type = decltype(ycxx::detail::apply_result_impl<Fn, Tuple>(
      static_cast<ycxx::detail::tuple_indices<Tuple>*>(nullptr)));
};
template <class Fn, class Tuple>
using apply_result_t = typename apply_result<Fn, Tuple>::type;
} // namespace std
