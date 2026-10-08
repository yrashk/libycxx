// libycxx core: the tuple protocol (tuple_size / tuple_element) and the tuple-like concept.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/integer_sequence.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
struct tuple_size;

template <class _Tp>
  requires requires { tuple_size<_Tp>::value; }
struct tuple_size<const _Tp> : integral_constant<size_t, tuple_size<_Tp>::value> {};
// [depr.tuple]: the volatile and const volatile forms (Annex D). This header is reached from
// <tuple>, <array>, <ranges> and <utility>, as /4 requires. GCC 16 ignores [[deprecated]] on a
// partial specialization, so the member (value, type) carries it too: a direct use then warns on
// both compilers.
template <class _Tp>
  requires requires { tuple_size<_Tp>::value; }
struct [[deprecated("tuple_size<volatile T> is deprecated ([depr.tuple])")]] tuple_size<volatile _Tp>
    : integral_constant<size_t, tuple_size<_Tp>::value> {
  [[deprecated("tuple_size<volatile T> is deprecated ([depr.tuple])")]]
  static constexpr size_t value = tuple_size<_Tp>::value;
};
template <class _Tp>
  requires requires { tuple_size<_Tp>::value; }
struct [[deprecated("tuple_size<const volatile T> is deprecated ([depr.tuple])")]] tuple_size<const volatile _Tp>
    : integral_constant<size_t, tuple_size<_Tp>::value> {
  [[deprecated("tuple_size<const volatile T> is deprecated ([depr.tuple])")]]
  static constexpr size_t value = tuple_size<_Tp>::value;
};

template <class _Tp>
constexpr size_t tuple_size_v = tuple_size<_Tp>::value;

template <size_t _Ip, class _Tp>
struct tuple_element;

template <size_t _Ip, class _Tp>
struct tuple_element<_Ip, const _Tp> {
  using type = const typename tuple_element<_Ip, _Tp>::type;
};
template <size_t _Ip, class _Tp>
struct [[deprecated("tuple_element<I, volatile T> is deprecated ([depr.tuple])")]] tuple_element<_Ip, volatile _Tp> {
  using type [[deprecated("tuple_element<I, volatile T> is deprecated ([depr.tuple])")]] =
      volatile typename tuple_element<_Ip, _Tp>::type;
};
template <size_t _Ip, class _Tp>
struct [[deprecated("tuple_element<I, const volatile T> is deprecated ([depr.tuple])")]] tuple_element<_Ip, const volatile _Tp> {
  using type [[deprecated("tuple_element<I, const volatile T> is deprecated ([depr.tuple])")]] =
      const volatile typename tuple_element<_Ip, _Tp>::type;
};

template <size_t _Ip, class _Tp>
using tuple_element_t = typename tuple_element<_Ip, _Tp>::type;

template <class... _Types>
class tuple;
template <class _T1, class _T2>
struct pair;
template <class _Tp, size_t _Np>
struct array;
template <class _Tp>
class complex;

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Makes `get<_Ip>(__x)` parse as a template-id inside __ycxx::__detail so that argument-dependent
// lookup finds the std::get overloads of every tuple-like type. Never selected.
struct __get_poison {};
template <std::size_t>
void get(__get_poison) = delete;

// [tuple.like]: specializations of array, complex, pair, tuple, ranges::subrange.
template <class _Tp>
inline constexpr bool __is_tuple_like_impl = false;
template <class... _Ts>
inline constexpr bool __is_tuple_like_impl<std::tuple<_Ts...>> = true;
template <class _T1, class _T2>
inline constexpr bool __is_tuple_like_impl<std::pair<_T1, _T2>> = true;
template <class _Tp, std::size_t _Np>
inline constexpr bool __is_tuple_like_impl<std::array<_Tp, _Np>> = true;
template <class _Tp>
inline constexpr bool __is_tuple_like_impl<std::complex<_Tp>> = true;
// ranges::subrange adds its own specialization in <ranges>.

template <class _Tp>
concept __tuple_like = __is_tuple_like_impl<__remove_cvref(_Tp)>;

template <class _Tp>
concept __pair_like = __tuple_like<_Tp> && std::tuple_size_v<__remove_cvref(_Tp)> == 2;

}} // namespace __ycxx::__detail

// ---------------------------------------------------------------------------------------------
// [meta.rel] is_applicable / is_nothrow_applicable, [meta.trans.other] apply_result
// ELEMS-OF(Tuple) is get<I>(declval<Tuple>())... (found by ADL).
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Fn, class _Tuple, std::size_t... _Ip>
consteval bool __applicable_impl(std::index_sequence<_Ip...>*) {
  return requires { ::__ycxx::__detail::invoke(std::declval<_Fn>(), get<_Ip>(std::declval<_Tuple>())...); };
}
template <class _Fn, class _Tuple, std::size_t... _Ip>
consteval bool __nothrow_applicable_impl(std::index_sequence<_Ip...>*) {
  return requires {
    { ::__ycxx::__detail::invoke(std::declval<_Fn>(), get<_Ip>(std::declval<_Tuple>())...) } noexcept;
  };
}
template <class _Fn, class _Tuple, std::size_t... _Ip>
auto __apply_result_impl(std::index_sequence<_Ip...>*)
    -> decltype(::__ycxx::__detail::invoke(std::declval<_Fn>(), get<_Ip>(std::declval<_Tuple>())...));

template <class _Tuple>
using __tuple_indices = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>;

template <class _Fn, class _Tuple>
consteval bool is_applicable_v() {
  if constexpr (__tuple_like<_Tuple>)
    return __applicable_impl<_Fn, _Tuple>(static_cast<__tuple_indices<_Tuple>*>(nullptr));
  else
    return false;
}
template <class _Fn, class _Tuple>
consteval bool is_nothrow_applicable_v() {
  if constexpr (__tuple_like<_Tuple>)
    return __nothrow_applicable_impl<_Fn, _Tuple>(static_cast<__tuple_indices<_Tuple>*>(nullptr));
  else
    return false;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Fn, class _Tuple>
struct is_applicable : bool_constant<__ycxx::__detail::is_applicable_v<_Fn, _Tuple>()> {};
template <class _Fn, class _Tuple>
constexpr bool is_applicable_v = __ycxx::__detail::is_applicable_v<_Fn, _Tuple>();
template <class _Fn, class _Tuple>
struct is_nothrow_applicable : bool_constant<__ycxx::__detail::is_nothrow_applicable_v<_Fn, _Tuple>()> {};
template <class _Fn, class _Tuple>
constexpr bool is_nothrow_applicable_v = __ycxx::__detail::is_nothrow_applicable_v<_Fn, _Tuple>();

template <class _Fn, class _Tuple>
struct apply_result {};
template <class _Fn, class _Tuple>
  requires(__ycxx::__detail::is_applicable_v<_Fn, _Tuple>())
struct apply_result<_Fn, _Tuple> {
  using type = decltype(__ycxx::__detail::__apply_result_impl<_Fn, _Tuple>(
      static_cast<__ycxx::__detail::__tuple_indices<_Tuple>*>(nullptr)));
};
template <class _Fn, class _Tuple>
using apply_result_t = typename apply_result<_Fn, _Tuple>::type;
}} // namespace std
