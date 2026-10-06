// libycxx core: constant_wrapper and cw ([const.wrap.class]).
//
// The operator set is the draft's cw-operators, transcribed: hidden friends (and explicit-object
// members for the pseudo-mutators) of a base class in __ycxx::__adl_free, a namespace with no
// functions, so ADL on a constant_wrapper finds exactly these operators plus whatever the
// second template parameter's namespaces contribute.
#pragma once

#include <ycxx/core/invoke.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {
// COMPILER-BUG(gcc): when X is substituted by a dependent expression (as in the cw-operators'
// return types), GCC 16 computes the default `decltype(_Xp)` from that expression, so
// `_Lp::value ->* _Rp::value` yields constant_wrapper<9, const int>. An auto non-type parameter never
// has a cv-qualified or reference type, so remove_cvref_t changes nothing on a correct compiler.
template <auto _Xp, class = remove_cvref_t<decltype(_Xp)>>
struct constant_wrapper;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// constexpr-param
template <class _Tp>
concept __constexpr_param = requires { typename std::constant_wrapper<_Tp::value>; };

// "constant_wrapper<INVOKE(value, remove_cvref_t<Args>::value...)> is a valid type" with every
// argument a constexpr-param ([const.wrap.class]/4; also [func.wrap.ref.ctor]/11.2).
template <class _CW, class... _Args>
concept __cw_constant_call = (__constexpr_param<std::remove_cvref_t<_Args>> && ...) && requires {
  typename std::constant_wrapper<::__ycxx::__detail::invoke(_CW::value, std::remove_cvref_t<_Args>::value...)>;
};
template <class _CW, class... _Args>
concept __cw_constant_subscript = (__constexpr_param<std::remove_cvref_t<_Args>> && ...) &&
                                requires { typename std::constant_wrapper<_CW::value[std::remove_cvref_t<_Args>::value...]>; };

// The declared type of constant_wrapper::value: decltype((X)), i.e. const V& for a class-type
// template parameter object and V otherwise. COMPILER-BUG(gcc): GCC 16 deduces
// `static constexpr decltype(auto) value = (_Xp);` as const V for a class type, so spell it out.
template <class _Vp>
using __cw_value_type = std::conditional_t<std::is_class_v<_Vp> || std::is_union_v<_Vp>, const _Vp&, _Vp>;

template <class _CW, class... _Args>
consteval bool __cw_call_noexcept() {
  if constexpr (__cw_constant_call<_CW, _Args...>)
    return true;
  else
    return noexcept(::__ycxx::__detail::invoke(_CW::value, std::declval<_Args>()...));
}
template <class _CW, class... _Args>
consteval bool __cw_subscript_noexcept() {
  if constexpr (__cw_constant_subscript<_CW, _Args...>)
    return true;
  else
    return noexcept(_CW::value[std::declval<_Args>()...]);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

struct __cw_operators {
  // unary operators
  template <::__ycxx::__detail::__constexpr_param _Tp>
  friend constexpr auto operator+(_Tp) noexcept -> std::constant_wrapper<(+_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  friend constexpr auto operator-(_Tp) noexcept -> std::constant_wrapper<(-_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  friend constexpr auto operator~(_Tp) noexcept -> std::constant_wrapper<(~_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  friend constexpr auto operator!(_Tp) noexcept -> std::constant_wrapper<(!_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  friend constexpr auto operator&(_Tp) noexcept -> std::constant_wrapper<(&_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  friend constexpr auto operator*(_Tp) noexcept -> std::constant_wrapper<(*_Tp::value)> {
    return {};
  }

  // binary operators
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator+(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value + _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator-(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value - _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator*(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value * _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator/(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value / _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator%(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value % _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator<<(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value << _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator>>(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value >> _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator&(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value & _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator|(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value | _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator^(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value ^ _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
    requires(!std::is_constructible_v<bool, decltype(_Lp::value)> || !std::is_constructible_v<bool, decltype(_Rp::value)>)
  friend constexpr auto operator&&(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value && _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
    requires(!std::is_constructible_v<bool, decltype(_Lp::value)> || !std::is_constructible_v<bool, decltype(_Rp::value)>)
  friend constexpr auto operator||(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value || _Rp::value)> {
    return {};
  }

  // comparisons
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator<=>(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value <=> _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator<(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value < _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator<=(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value <= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator==(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value == _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator!=(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value != _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator>(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value > _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator>=(_Lp, _Rp) noexcept -> std::constant_wrapper<(_Lp::value >= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator,(_Lp, _Rp) noexcept = delete;
  template <::__ycxx::__detail::__constexpr_param _Lp, ::__ycxx::__detail::__constexpr_param _Rp>
  friend constexpr auto operator->*(_Lp, _Rp) noexcept -> std::constant_wrapper<_Lp::value->*(_Rp::value)> {
    return {};
  }

  // pseudo-mutators
  template <::__ycxx::__detail::__constexpr_param _Tp>
  constexpr auto operator++(this _Tp) noexcept -> std::constant_wrapper<(++_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  constexpr auto operator++(this _Tp, int) noexcept -> std::constant_wrapper<(_Tp::value++)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  constexpr auto operator--(this _Tp) noexcept -> std::constant_wrapper<(--_Tp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp>
  constexpr auto operator--(this _Tp, int) noexcept -> std::constant_wrapper<(_Tp::value--)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator+=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value += _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator-=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value -= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator*=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value *= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator/=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value /= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator%=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value %= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator&=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value &= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator|=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value |= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator^=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value ^= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator<<=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value <<= _Rp::value)> {
    return {};
  }
  template <::__ycxx::__detail::__constexpr_param _Tp, ::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator>>=(this _Tp, _Rp) noexcept -> std::constant_wrapper<(_Tp::value >>= _Rp::value)> {
    return {};
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <auto _Xp, class _Tp>
struct constant_wrapper : ::__ycxx::__adl_free::__cw_operators {
  static_assert(std::is_same_v<_Tp, decltype(_Xp)>, "constant_wrapper: the second template argument must be decltype(X)");

  static constexpr ::__ycxx::__detail::__cw_value_type<decltype(_Xp)> value = _Xp;
  using type = constant_wrapper;
  using value_type = decltype(_Xp);

  template <::__ycxx::__detail::__constexpr_param _Rp>
  constexpr auto operator=(_Rp) const noexcept -> constant_wrapper<(value = _Rp::value)> {
    return {};
  }
  constexpr operator decltype(value)() const noexcept { return value; }

  template <class... _Args>
    requires ::__ycxx::__detail::__cw_constant_call<constant_wrapper, _Args...> ||
             requires(_Args&&... __args) { ::__ycxx::__detail::invoke(value, static_cast<_Args&&>(__args)...); }
  static constexpr decltype(auto) operator()(_Args&&... __args) noexcept(
      ::__ycxx::__detail::__cw_call_noexcept<constant_wrapper, _Args...>()) {
    if constexpr (::__ycxx::__detail::__cw_constant_call<constant_wrapper, _Args...>)
      return constant_wrapper<::__ycxx::__detail::invoke(value, remove_cvref_t<_Args>::value...)>{};
    else
      return ::__ycxx::__detail::invoke(value, static_cast<_Args&&>(__args)...);
  }

  template <class... _Args>
    requires ::__ycxx::__detail::__cw_constant_subscript<constant_wrapper, _Args...> ||
             requires(_Args&&... __args) { value[static_cast<_Args&&>(__args)...]; }
  static constexpr decltype(auto) operator[](_Args&&... __args) noexcept(
      ::__ycxx::__detail::__cw_subscript_noexcept<constant_wrapper, _Args...>()) {
    if constexpr (::__ycxx::__detail::__cw_constant_subscript<constant_wrapper, _Args...>)
      return constant_wrapper<value[remove_cvref_t<_Args>::value...]>{};
    else
      return value[static_cast<_Args&&>(__args)...];
  }
};

template <auto _Xp>
constexpr auto cw = constant_wrapper<_Xp>{};

} // namespace std
