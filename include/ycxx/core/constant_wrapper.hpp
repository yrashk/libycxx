// libycxx core: constant_wrapper and cw ([const.wrap.class]).
//
// The operator set is the draft's cw-operators, transcribed: hidden friends (and explicit-object
// members for the pseudo-mutators) of a base class in ycxx::adl_free, a namespace with no
// functions, so ADL on a constant_wrapper finds exactly these operators plus whatever the
// second template parameter's namespaces contribute.
#pragma once

#include <ycxx/core/invoke.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[gnu::visibility("hidden")]] std {
// COMPILER-BUG(gcc): when X is substituted by a dependent expression (as in the cw-operators'
// return types), GCC 16 computes the default `decltype(X)` from that expression, so
// `L::value ->* R::value` yields constant_wrapper<9, const int>. An auto non-type parameter never
// has a cv-qualified or reference type, so remove_cvref_t changes nothing on a correct compiler.
template <auto X, class = remove_cvref_t<decltype(X)>>
struct constant_wrapper;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// constexpr-param
template <class T>
concept constexpr_param = requires { typename std::constant_wrapper<T::value>; };

// "constant_wrapper<INVOKE(value, remove_cvref_t<Args>::value...)> is a valid type" with every
// argument a constexpr-param ([const.wrap.class]/4; also [func.wrap.ref.ctor]/11.2).
template <class CW, class... Args>
concept cw_constant_call = (constexpr_param<std::remove_cvref_t<Args>> && ...) && requires {
  typename std::constant_wrapper<::ycxx::detail::invoke(CW::value, std::remove_cvref_t<Args>::value...)>;
};
template <class CW, class... Args>
concept cw_constant_subscript = (constexpr_param<std::remove_cvref_t<Args>> && ...) &&
                                requires { typename std::constant_wrapper<CW::value[std::remove_cvref_t<Args>::value...]>; };

// The declared type of constant_wrapper::value: decltype((X)), i.e. const V& for a class-type
// template parameter object and V otherwise. COMPILER-BUG(gcc): GCC 16 deduces
// `static constexpr decltype(auto) value = (X);` as const V for a class type, so spell it out.
template <class V>
using cw_value_type = std::conditional_t<std::is_class_v<V> || std::is_union_v<V>, const V&, V>;

template <class CW, class... Args>
consteval bool cw_call_noexcept() {
  if constexpr (cw_constant_call<CW, Args...>)
    return true;
  else
    return noexcept(::ycxx::detail::invoke(CW::value, std::declval<Args>()...));
}
template <class CW, class... Args>
consteval bool cw_subscript_noexcept() {
  if constexpr (cw_constant_subscript<CW, Args...>)
    return true;
  else
    return noexcept(CW::value[std::declval<Args>()...]);
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

struct cw_operators {
  // unary operators
  template <::ycxx::detail::constexpr_param T>
  friend constexpr auto operator+(T) noexcept -> std::constant_wrapper<(+T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  friend constexpr auto operator-(T) noexcept -> std::constant_wrapper<(-T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  friend constexpr auto operator~(T) noexcept -> std::constant_wrapper<(~T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  friend constexpr auto operator!(T) noexcept -> std::constant_wrapper<(!T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  friend constexpr auto operator&(T) noexcept -> std::constant_wrapper<(&T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  friend constexpr auto operator*(T) noexcept -> std::constant_wrapper<(*T::value)> {
    return {};
  }

  // binary operators
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator+(L, R) noexcept -> std::constant_wrapper<(L::value + R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator-(L, R) noexcept -> std::constant_wrapper<(L::value - R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator*(L, R) noexcept -> std::constant_wrapper<(L::value * R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator/(L, R) noexcept -> std::constant_wrapper<(L::value / R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator%(L, R) noexcept -> std::constant_wrapper<(L::value % R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator<<(L, R) noexcept -> std::constant_wrapper<(L::value << R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator>>(L, R) noexcept -> std::constant_wrapper<(L::value >> R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator&(L, R) noexcept -> std::constant_wrapper<(L::value & R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator|(L, R) noexcept -> std::constant_wrapper<(L::value | R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator^(L, R) noexcept -> std::constant_wrapper<(L::value ^ R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
    requires(!std::is_constructible_v<bool, decltype(L::value)> || !std::is_constructible_v<bool, decltype(R::value)>)
  friend constexpr auto operator&&(L, R) noexcept -> std::constant_wrapper<(L::value && R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
    requires(!std::is_constructible_v<bool, decltype(L::value)> || !std::is_constructible_v<bool, decltype(R::value)>)
  friend constexpr auto operator||(L, R) noexcept -> std::constant_wrapper<(L::value || R::value)> {
    return {};
  }

  // comparisons
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator<=>(L, R) noexcept -> std::constant_wrapper<(L::value <=> R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator<(L, R) noexcept -> std::constant_wrapper<(L::value < R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator<=(L, R) noexcept -> std::constant_wrapper<(L::value <= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator==(L, R) noexcept -> std::constant_wrapper<(L::value == R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator!=(L, R) noexcept -> std::constant_wrapper<(L::value != R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator>(L, R) noexcept -> std::constant_wrapper<(L::value > R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator>=(L, R) noexcept -> std::constant_wrapper<(L::value >= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator,(L, R) noexcept = delete;
  template <::ycxx::detail::constexpr_param L, ::ycxx::detail::constexpr_param R>
  friend constexpr auto operator->*(L, R) noexcept -> std::constant_wrapper<L::value->*(R::value)> {
    return {};
  }

  // pseudo-mutators
  template <::ycxx::detail::constexpr_param T>
  constexpr auto operator++(this T) noexcept -> std::constant_wrapper<(++T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  constexpr auto operator++(this T, int) noexcept -> std::constant_wrapper<(T::value++)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  constexpr auto operator--(this T) noexcept -> std::constant_wrapper<(--T::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T>
  constexpr auto operator--(this T, int) noexcept -> std::constant_wrapper<(T::value--)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator+=(this T, R) noexcept -> std::constant_wrapper<(T::value += R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator-=(this T, R) noexcept -> std::constant_wrapper<(T::value -= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator*=(this T, R) noexcept -> std::constant_wrapper<(T::value *= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator/=(this T, R) noexcept -> std::constant_wrapper<(T::value /= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator%=(this T, R) noexcept -> std::constant_wrapper<(T::value %= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator&=(this T, R) noexcept -> std::constant_wrapper<(T::value &= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator|=(this T, R) noexcept -> std::constant_wrapper<(T::value |= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator^=(this T, R) noexcept -> std::constant_wrapper<(T::value ^= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator<<=(this T, R) noexcept -> std::constant_wrapper<(T::value <<= R::value)> {
    return {};
  }
  template <::ycxx::detail::constexpr_param T, ::ycxx::detail::constexpr_param R>
  constexpr auto operator>>=(this T, R) noexcept -> std::constant_wrapper<(T::value >>= R::value)> {
    return {};
  }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <auto X, class T>
struct constant_wrapper : ::ycxx::adl_free::cw_operators {
  static_assert(std::is_same_v<T, decltype(X)>, "constant_wrapper: the second template argument must be decltype(X)");

  static constexpr ::ycxx::detail::cw_value_type<decltype(X)> value = X;
  using type = constant_wrapper;
  using value_type = decltype(X);

  template <::ycxx::detail::constexpr_param R>
  constexpr auto operator=(R) const noexcept -> constant_wrapper<(value = R::value)> {
    return {};
  }
  constexpr operator decltype(value)() const noexcept { return value; }

  template <class... Args>
    requires ::ycxx::detail::cw_constant_call<constant_wrapper, Args...> ||
             requires(Args&&... args) { ::ycxx::detail::invoke(value, static_cast<Args&&>(args)...); }
  static constexpr decltype(auto) operator()(Args&&... args) noexcept(
      ::ycxx::detail::cw_call_noexcept<constant_wrapper, Args...>()) {
    if constexpr (::ycxx::detail::cw_constant_call<constant_wrapper, Args...>)
      return constant_wrapper<::ycxx::detail::invoke(value, remove_cvref_t<Args>::value...)>{};
    else
      return ::ycxx::detail::invoke(value, static_cast<Args&&>(args)...);
  }

  template <class... Args>
    requires ::ycxx::detail::cw_constant_subscript<constant_wrapper, Args...> ||
             requires(Args&&... args) { value[static_cast<Args&&>(args)...]; }
  static constexpr decltype(auto) operator[](Args&&... args) noexcept(
      ::ycxx::detail::cw_subscript_noexcept<constant_wrapper, Args...>()) {
    if constexpr (::ycxx::detail::cw_constant_subscript<constant_wrapper, Args...>)
      return constant_wrapper<value[remove_cvref_t<Args>::value...]>{};
    else
      return value[static_cast<Args&&>(args)...];
  }
};

template <auto X>
constexpr auto cw = constant_wrapper<X>{};

} // namespace std
