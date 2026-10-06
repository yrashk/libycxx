// libycxx core: small <utility> components (everything except pair).
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [utility.exchange]
template <class _Tp, class _Up = _Tp>
constexpr _Tp exchange(_Tp& __obj, _Up&& __new_val) noexcept(std::is_nothrow_constructible_v<_Tp, _Tp&&> &&
                                                    std::is_nothrow_assignable_v<_Tp&, _Up &&>) {
  _Tp __old = static_cast<_Tp&&>(__obj);
  __obj = static_cast<_Up&&>(__new_val);
  return __old;
}

// [utility.intcmp]
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
concept __cmp_integer = __is_signed_or_unsigned_integer<_Tp>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <__ycxx::__detail::__cmp_integer _Tp, __ycxx::__detail::__cmp_integer _Up>
constexpr bool cmp_equal(_Tp t, _Up __u) noexcept {
  if constexpr (__ycxx::__detail::is_signed_v<_Tp> == __ycxx::__detail::is_signed_v<_Up>)
    return t == __u;
  else if constexpr (__ycxx::__detail::is_signed_v<_Tp>)
    return t >= 0 && make_unsigned_t<_Tp>(t) == __u;
  else
    return __u >= 0 && t == make_unsigned_t<_Up>(__u);
}
template <__ycxx::__detail::__cmp_integer _Tp, __ycxx::__detail::__cmp_integer _Up>
constexpr bool cmp_not_equal(_Tp t, _Up __u) noexcept {
  return !cmp_equal(t, __u);
}
template <__ycxx::__detail::__cmp_integer _Tp, __ycxx::__detail::__cmp_integer _Up>
constexpr bool cmp_less(_Tp t, _Up __u) noexcept {
  if constexpr (__ycxx::__detail::is_signed_v<_Tp> == __ycxx::__detail::is_signed_v<_Up>)
    return t < __u;
  else if constexpr (__ycxx::__detail::is_signed_v<_Tp>)
    return t < 0 || make_unsigned_t<_Tp>(t) < __u;
  else
    return __u >= 0 && t < make_unsigned_t<_Up>(__u);
}
template <__ycxx::__detail::__cmp_integer _Tp, __ycxx::__detail::__cmp_integer _Up>
constexpr bool cmp_greater(_Tp t, _Up __u) noexcept {
  return cmp_less(__u, t);
}
template <__ycxx::__detail::__cmp_integer _Tp, __ycxx::__detail::__cmp_integer _Up>
constexpr bool cmp_less_equal(_Tp t, _Up __u) noexcept {
  return !cmp_less(__u, t);
}
template <__ycxx::__detail::__cmp_integer _Tp, __ycxx::__detail::__cmp_integer _Up>
constexpr bool cmp_greater_equal(_Tp t, _Up __u) noexcept {
  return !cmp_less(t, __u);
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
consteval _Tp __int_min() {
  if constexpr (is_signed_v<_Tp>)
    return _Tp(_Tp(1) << (sizeof(_Tp) * __CHAR_BIT__ - 1));
  else
    return _Tp(0);
}
template <class _Tp>
consteval _Tp __int_max() {
  if constexpr (is_signed_v<_Tp>)
    return _Tp(~__int_min<_Tp>());
  else
    return _Tp(~_Tp(0));
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Rp, class _Tp>
  requires __ycxx::__detail::__cmp_integer<_Rp> && __ycxx::__detail::__cmp_integer<_Tp>
constexpr bool in_range(_Tp t) noexcept {
  return cmp_greater_equal(t, __ycxx::__detail::__int_min<_Rp>()) && cmp_less_equal(t, __ycxx::__detail::__int_max<_Rp>());
}

// [utility.underlying]
template <class _Tp>
  requires is_enum_v<_Tp>
[[nodiscard]] constexpr underlying_type_t<_Tp> to_underlying(_Tp value) noexcept {
  return static_cast<underlying_type_t<_Tp>>(value);
}

// [utility.undefined]
[[noreturn]] [[__gnu__::__always_inline__]] inline void unreachable() {
  if constexpr (__ycxx::__detail::__cfg::__hardened)
    __ycxx::__detail::__assertion_failed("std::unreachable() reached");
  __builtin_unreachable();
}

// Prevents the compiler from moving observable behaviour across this point ([utility.undefined]).
inline void observable_checkpoint() noexcept { asm volatile("" ::: "memory"); }

// [intseq.binding]
template <class _Tp, _Tp... _Values>
struct tuple_size<integer_sequence<_Tp, _Values...>> : integral_constant<size_t, sizeof...(_Values)> {};
template <size_t _Ip, class _Tp, _Tp... _Values>
struct tuple_element<_Ip, integer_sequence<_Tp, _Values...>> {
  static_assert(_Ip < sizeof...(_Values), "index out of range");
  using type = _Tp;
};
template <size_t _Ip, class _Tp, _Tp... _Values>
struct tuple_element<_Ip, const integer_sequence<_Tp, _Values...>> {
  static_assert(_Ip < sizeof...(_Values), "index out of range");
  using type = _Tp;
};
template <size_t _Ip, class _Tp, _Tp... _Values>
constexpr _Tp get(integer_sequence<_Tp, _Values...>) noexcept {
  static_assert(_Ip < sizeof...(_Values), "index out of range");
  return _Values...[_Ip];
}

// [pair.piecewise], in-place tags
struct piecewise_construct_t {
  explicit piecewise_construct_t() = default;
};
inline constexpr piecewise_construct_t piecewise_construct{};

struct in_place_t {
  explicit in_place_t() = default;
};
inline constexpr in_place_t in_place{};
template <class _Tp>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};
template <class _Tp>
constexpr in_place_type_t<_Tp> in_place_type{};
template <size_t _Ip>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};
template <size_t _Ip>
constexpr in_place_index_t<_Ip> in_place_index{};

// [variant.monostate]
struct monostate {};
constexpr bool operator==(monostate, monostate) noexcept { return true; }
constexpr strong_ordering operator<=>(monostate, monostate) noexcept { return strong_ordering::equal; }

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_in_place_type = false;
template <class _Tp>
inline constexpr bool __is_in_place_type<std::in_place_type_t<_Tp>> = true;
template <class _Tp>
inline constexpr bool __is_in_place_index = false;
template <std::size_t _Ip>
inline constexpr bool __is_in_place_index<std::in_place_index_t<_Ip>> = true;

// synth-three-way ([expos.only.entity])
struct __synth_three_way_fn {
  template <class _Tp, class _Up>
    requires requires(const _Tp& t, const _Up& __u) {
      { t < __u } -> __boolean_testable;
      { __u < t } -> __boolean_testable;
    }
  static constexpr auto operator()(const _Tp& t, const _Up& __u) {
    if constexpr (std::three_way_comparable_with<_Tp, _Up>) {
      return t <=> __u;
    } else {
      if (t < __u)
        return std::weak_ordering::less;
      if (__u < t)
        return std::weak_ordering::greater;
      return std::weak_ordering::equivalent;
    }
  }
};
inline constexpr __synth_three_way_fn __synth_three_way{};
template <class _Tp, class _Up = _Tp>
using __synth_three_way_result = decltype(__synth_three_way(std::declval<_Tp&>(), std::declval<_Up&>()));

// converts-from-any-cvref ([optional.ctor]/1), shared with <expected>
template <class _Tp, class _Wp>
concept __converts_from_any_cvref =
    std::is_constructible_v<_Tp, _Wp&> || std::is_convertible_v<_Wp&, _Tp> || std::is_constructible_v<_Tp, _Wp> ||
    std::is_convertible_v<_Wp, _Tp> || std::is_constructible_v<_Tp, const _Wp&> || std::is_convertible_v<const _Wp&, _Tp> ||
    std::is_constructible_v<_Tp, const _Wp> || std::is_convertible_v<const _Wp, _Tp>;

}} // namespace __ycxx::__detail
