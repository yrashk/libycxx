// libycxx core: small <utility> components (everything except pair).
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>

namespace std {

// [utility.exchange]
template <class T, class U = T>
constexpr T exchange(T& obj, U&& new_val) noexcept(std::is_nothrow_constructible_v<T, T&&> &&
                                                    std::is_nothrow_assignable_v<T&, U &&>) {
  T old = static_cast<T&&>(obj);
  obj = static_cast<U&&>(new_val);
  return old;
}

// [utility.intcmp]
} // namespace std

namespace ycxx::detail {
template <class T>
concept cmp_integer = is_signed_or_unsigned_integer<T>;
} // namespace ycxx::detail

namespace std {

template <ycxx::detail::cmp_integer T, ycxx::detail::cmp_integer U>
constexpr bool cmp_equal(T t, U u) noexcept {
  if constexpr (ycxx::detail::is_signed_v<T> == ycxx::detail::is_signed_v<U>)
    return t == u;
  else if constexpr (ycxx::detail::is_signed_v<T>)
    return t >= 0 && make_unsigned_t<T>(t) == u;
  else
    return u >= 0 && t == make_unsigned_t<U>(u);
}
template <ycxx::detail::cmp_integer T, ycxx::detail::cmp_integer U>
constexpr bool cmp_not_equal(T t, U u) noexcept {
  return !cmp_equal(t, u);
}
template <ycxx::detail::cmp_integer T, ycxx::detail::cmp_integer U>
constexpr bool cmp_less(T t, U u) noexcept {
  if constexpr (ycxx::detail::is_signed_v<T> == ycxx::detail::is_signed_v<U>)
    return t < u;
  else if constexpr (ycxx::detail::is_signed_v<T>)
    return t < 0 || make_unsigned_t<T>(t) < u;
  else
    return u >= 0 && t < make_unsigned_t<U>(u);
}
template <ycxx::detail::cmp_integer T, ycxx::detail::cmp_integer U>
constexpr bool cmp_greater(T t, U u) noexcept {
  return cmp_less(u, t);
}
template <ycxx::detail::cmp_integer T, ycxx::detail::cmp_integer U>
constexpr bool cmp_less_equal(T t, U u) noexcept {
  return !cmp_less(u, t);
}
template <ycxx::detail::cmp_integer T, ycxx::detail::cmp_integer U>
constexpr bool cmp_greater_equal(T t, U u) noexcept {
  return !cmp_less(t, u);
}

} // namespace std

namespace ycxx::detail {
template <class T>
consteval T int_min() {
  if constexpr (is_signed_v<T>)
    return T(T(1) << (sizeof(T) * __CHAR_BIT__ - 1));
  else
    return T(0);
}
template <class T>
consteval T int_max() {
  if constexpr (is_signed_v<T>)
    return T(~int_min<T>());
  else
    return T(~T(0));
}
} // namespace ycxx::detail

namespace std {

template <class R, class T>
  requires ycxx::detail::cmp_integer<R> && ycxx::detail::cmp_integer<T>
constexpr bool in_range(T t) noexcept {
  return cmp_greater_equal(t, ycxx::detail::int_min<R>()) && cmp_less_equal(t, ycxx::detail::int_max<R>());
}

// [utility.underlying]
template <class T>
  requires is_enum_v<T>
[[nodiscard]] constexpr underlying_type_t<T> to_underlying(T value) noexcept {
  return static_cast<underlying_type_t<T>>(value);
}

// [utility.undefined]
[[noreturn]] [[gnu::always_inline]] inline void unreachable() {
  if constexpr (ycxx::detail::cfg::hardened)
    ycxx::detail::assertion_failed("std::unreachable() reached");
  __builtin_unreachable();
}

// Prevents the compiler from moving observable behaviour across this point ([utility.undefined]).
inline void observable_checkpoint() noexcept { asm volatile("" ::: "memory"); }

// [intseq.binding]
template <class T, T... Values>
struct tuple_size<integer_sequence<T, Values...>> : integral_constant<size_t, sizeof...(Values)> {};
template <size_t I, class T, T... Values>
struct tuple_element<I, integer_sequence<T, Values...>> {
  static_assert(I < sizeof...(Values), "index out of range");
  using type = T;
};
template <size_t I, class T, T... Values>
struct tuple_element<I, const integer_sequence<T, Values...>> {
  static_assert(I < sizeof...(Values), "index out of range");
  using type = T;
};
template <size_t I, class T, T... Values>
constexpr T get(integer_sequence<T, Values...>) noexcept {
  static_assert(I < sizeof...(Values), "index out of range");
  return Values...[I];
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
template <class T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};
template <class T>
constexpr in_place_type_t<T> in_place_type{};
template <size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};
template <size_t I>
constexpr in_place_index_t<I> in_place_index{};

// [variant.monostate]
struct monostate {};
constexpr bool operator==(monostate, monostate) noexcept { return true; }
constexpr strong_ordering operator<=>(monostate, monostate) noexcept { return strong_ordering::equal; }

} // namespace std

namespace ycxx::detail {

template <class T>
inline constexpr bool is_in_place_type = false;
template <class T>
inline constexpr bool is_in_place_type<std::in_place_type_t<T>> = true;
template <class T>
inline constexpr bool is_in_place_index = false;
template <std::size_t I>
inline constexpr bool is_in_place_index<std::in_place_index_t<I>> = true;

// synth-three-way ([expos.only.entity])
struct synth_three_way_fn {
  template <class T, class U>
    requires requires(const T& t, const U& u) {
      { t < u } -> boolean_testable;
      { u < t } -> boolean_testable;
    }
  static constexpr auto operator()(const T& t, const U& u) {
    if constexpr (std::three_way_comparable_with<T, U>) {
      return t <=> u;
    } else {
      if (t < u)
        return std::weak_ordering::less;
      if (u < t)
        return std::weak_ordering::greater;
      return std::weak_ordering::equivalent;
    }
  }
};
inline constexpr synth_three_way_fn synth_three_way{};
template <class T, class U = T>
using synth_three_way_result = decltype(synth_three_way(std::declval<T&>(), std::declval<U&>()));

} // namespace ycxx::detail
