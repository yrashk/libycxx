// libycxx core: the tuple protocol (tuple_size / tuple_element) and the tuple-like concept.
#ifndef YCXX_CORE_TUPLE_LIKE_HPP
#define YCXX_CORE_TUPLE_LIKE_HPP

#include <ycxx/core/type_traits.hpp>

namespace std {

template <class T>
struct tuple_size;

template <class T>
  requires requires { tuple_size<T>::value; }
struct tuple_size<const T> : integral_constant<size_t, tuple_size<T>::value> {};
template <class T>
  requires requires { tuple_size<T>::value; }
struct tuple_size<volatile T> : integral_constant<size_t, tuple_size<T>::value> {};
template <class T>
  requires requires { tuple_size<T>::value; }
struct tuple_size<const volatile T> : integral_constant<size_t, tuple_size<T>::value> {};

template <class T>
constexpr size_t tuple_size_v = tuple_size<T>::value;

template <size_t I, class T>
struct tuple_element;

template <size_t I, class T>
struct tuple_element<I, const T> {
  using type = const typename tuple_element<I, T>::type;
};
template <size_t I, class T>
struct tuple_element<I, volatile T> {
  using type = volatile typename tuple_element<I, T>::type;
};
template <size_t I, class T>
struct tuple_element<I, const volatile T> {
  using type = const volatile typename tuple_element<I, T>::type;
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

#endif // YCXX_CORE_TUPLE_LIKE_HPP
