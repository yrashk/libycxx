// libycxx core: function objects ([arithmetic.operations], [comparisons], [logical.operations],
// [bitwise.operations], [func.identity], [range.cmp]) and reference_wrapper ([refwrap]).
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// Converts any pointer-ish operand to an integer so that pointer comparisons form a strict
// total order even across unrelated objects ([comparisons.general]/2).
template <class _Tp>
constexpr __UINTPTR_TYPE__ __ptr_value(const _Tp& p) noexcept {
  if constexpr (std::is_pointer_v<_Tp>)
    return reinterpret_cast<__UINTPTR_TYPE__>(p); // object and function pointers alike
  else
    return reinterpret_cast<__UINTPTR_TYPE__>(static_cast<const volatile void*>(p));
}

// BUILTIN-PTR-CMP(T, op, U): `t op __u` resolves to a built-in operator comparing pointers. Both
// operands convert to pointers and no user-declared operator can be selected instead: neither an
// `op` taking (t, u) nor, for the relational operators, an operator<=> in either operand order
// (a rewritten candidate, [over.match.oper]/3.4), nor, for ==, a reversed operator==. The test
// for class operands comes second, so pointers to incomplete classes are never completed.
template <class _Tp, class _Up>
concept __ptr_operands = std::is_convertible_v<_Tp, const volatile void*> && std::is_convertible_v<_Up, const volatile void*>;
template <class _Tp, class _Up>
concept __user_less = requires(_Tp&& t, _Up&& __u) { operator<(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); } ||
                    requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t).operator<(static_cast<_Up&&>(__u)); };
template <class _Tp, class _Up>
concept __user_greater = requires(_Tp&& t, _Up&& __u) { operator>(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); } ||
                       requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t).operator>(static_cast<_Up&&>(__u)); };
template <class _Tp, class _Up>
concept __user_less_equal = requires(_Tp&& t, _Up&& __u) { operator<=(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); } ||
                          requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t).operator<=(static_cast<_Up&&>(__u)); };
template <class _Tp, class _Up>
concept __user_greater_equal = requires(_Tp&& t, _Up&& __u) { operator>=(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); } ||
                             requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t).operator>=(static_cast<_Up&&>(__u)); };
template <class _Tp, class _Up>
concept __user_equal = requires(_Tp&& t, _Up&& __u) { operator==(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); } ||
                     requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t).operator==(static_cast<_Up&&>(__u)); } ||
                     requires(_Tp&& t, _Up&& __u) { operator==(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t)); } ||
                     requires(_Tp&& t, _Up&& __u) { static_cast<_Up&&>(__u).operator==(static_cast<_Tp&&>(t)); };

template <class _Tp, class _Up>
concept __y_builtin_ptr_less = requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u); } && __ptr_operands<_Tp, _Up> &&
                           (__no_class_operand<_Tp, _Up> || !(__user_less<_Tp, _Up> || __user_three_way_candidate<_Tp, _Up>));
template <class _Tp, class _Up>
concept __y_builtin_ptr_greater = requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t) > static_cast<_Up&&>(__u); } && __ptr_operands<_Tp, _Up> &&
                              (__no_class_operand<_Tp, _Up> || !(__user_greater<_Tp, _Up> || __user_three_way_candidate<_Tp, _Up>));
template <class _Tp, class _Up>
concept __y_builtin_ptr_less_equal = requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t) <= static_cast<_Up&&>(__u); } &&
                                 __ptr_operands<_Tp, _Up> &&
                                 (__no_class_operand<_Tp, _Up> || !(__user_less_equal<_Tp, _Up> || __user_three_way_candidate<_Tp, _Up>));
template <class _Tp, class _Up>
concept __y_builtin_ptr_greater_equal = requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t) >= static_cast<_Up&&>(__u); } &&
                                    __ptr_operands<_Tp, _Up> &&
                                    (__no_class_operand<_Tp, _Up> || !(__user_greater_equal<_Tp, _Up> || __user_three_way_candidate<_Tp, _Up>));
template <class _Tp, class _Up>
concept __y_builtin_ptr_eq = requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u); } && __ptr_operands<_Tp, _Up> &&
                         (__no_class_operand<_Tp, _Up> || !__user_equal<_Tp, _Up>);

template <class _Tp, class _Up>
constexpr bool __total_less(const _Tp& a, const _Up& b) {
  if consteval {
    // A null pointer orders before every other pointer; the core language leaves comparing
    // it with a pointer to an object unspecified, which constant evaluation rejects.
    const volatile void* __pa = a;
    const volatile void* __pb = b;
    if (__pb == nullptr)
      return false;
    if (__pa == nullptr)
      return true;
    return a < b;
  } else {
    return ::__ycxx::__detail::__ptr_value(a) < ::__ycxx::__detail::__ptr_value(b);
  }
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// ---- arithmetic --------------------------------------------------------------------------------
template <class _Tp = void>
struct plus {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x + y; }
};
template <class _Tp = void>
struct minus {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x - y; }
};
template <class _Tp = void>
struct multiplies {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x * y; }
};
template <class _Tp = void>
struct divides {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x / y; }
};
template <class _Tp = void>
struct modulus {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x % y; }
};
template <class _Tp = void>
struct negate {
  constexpr _Tp operator()(const _Tp& __x) const { return -__x; }
};

template <>
struct plus<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) + static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) + static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) + static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct minus<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) - static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) - static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) - static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct multiplies<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) * static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) * static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) * static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct divides<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) / static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) / static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) / static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct modulus<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) % static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) % static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) % static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct negate<void> {
  template <class _Tp>
  constexpr auto operator()(_Tp&& t) const noexcept(noexcept(-static_cast<_Tp&&>(t))) -> decltype(-static_cast<_Tp&&>(t)) {
    return -static_cast<_Tp&&>(t);
  }
  using is_transparent = void;
};

// ---- comparisons -------------------------------------------------------------------------------
template <class _Tp = void>
struct equal_to {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x == y; }
};
template <class _Tp = void>
struct not_equal_to {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x != y; }
};
template <class _Tp = void>
struct less {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x < y; }
};
template <class _Tp = void>
struct greater {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x > y; }
};
template <class _Tp = void>
struct less_equal {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x <= y; }
};
template <class _Tp = void>
struct greater_equal {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x >= y; }
};

// Pointer specializations: strict total order.
template <class _Tp>
struct less<_Tp*> {
  constexpr bool operator()(_Tp* __x, _Tp* y) const noexcept { return __ycxx::__detail::__total_less(__x, y); }
};
template <class _Tp>
struct greater<_Tp*> {
  constexpr bool operator()(_Tp* __x, _Tp* y) const noexcept { return __ycxx::__detail::__total_less(y, __x); }
};
template <class _Tp>
struct less_equal<_Tp*> {
  constexpr bool operator()(_Tp* __x, _Tp* y) const noexcept { return !__ycxx::__detail::__total_less(y, __x); }
};
template <class _Tp>
struct greater_equal<_Tp*> {
  constexpr bool operator()(_Tp* __x, _Tp* y) const noexcept { return !__ycxx::__detail::__total_less(__x, y); }
};

template <>
struct equal_to<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct not_equal_to<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) != static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) != static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) != static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct less<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u)) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_less<_Tp, _Up>)
      return __ycxx::__detail::__total_less(t, __u);
    else
      return static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct greater<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) > static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) > static_cast<_Up&&>(__u)) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_greater<_Tp, _Up>)
      return __ycxx::__detail::__total_less(__u, t);
    else
      return static_cast<_Tp&&>(t) > static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct less_equal<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) <= static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) <= static_cast<_Up&&>(__u)) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_less_equal<_Tp, _Up>)
      return !__ycxx::__detail::__total_less(__u, t);
    else
      return static_cast<_Tp&&>(t) <= static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct greater_equal<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) >= static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) >= static_cast<_Up&&>(__u)) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_greater_equal<_Tp, _Up>)
      return !__ycxx::__detail::__total_less(t, __u);
    else
      return static_cast<_Tp&&>(t) >= static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};

// ---- logical / bitwise -------------------------------------------------------------------------
template <class _Tp = void>
struct logical_and {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x && y; }
};
template <class _Tp = void>
struct logical_or {
  constexpr bool operator()(const _Tp& __x, const _Tp& y) const { return __x || y; }
};
template <class _Tp = void>
struct logical_not {
  constexpr bool operator()(const _Tp& __x) const { return !__x; }
};
template <class _Tp = void>
struct bit_and {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x & y; }
};
template <class _Tp = void>
struct bit_or {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x | y; }
};
template <class _Tp = void>
struct bit_xor {
  constexpr _Tp operator()(const _Tp& __x, const _Tp& y) const { return __x ^ y; }
};
template <class _Tp = void>
struct bit_not {
  constexpr _Tp operator()(const _Tp& __x) const { return ~__x; }
};

template <>
struct logical_and<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) && static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) && static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) && static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct logical_or<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) || static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) || static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) || static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct logical_not<void> {
  template <class _Tp>
  constexpr auto operator()(_Tp&& t) const noexcept(noexcept(!static_cast<_Tp&&>(t))) -> decltype(!static_cast<_Tp&&>(t)) {
    return !static_cast<_Tp&&>(t);
  }
  using is_transparent = void;
};
template <>
struct bit_and<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) & static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) & static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) & static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct bit_or<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) | static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) | static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) | static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct bit_xor<void> {
  template <class _Tp, class _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) ^ static_cast<_Up&&>(__u)))
      -> decltype(static_cast<_Tp&&>(t) ^ static_cast<_Up&&>(__u)) {
    return static_cast<_Tp&&>(t) ^ static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
template <>
struct bit_not<void> {
  template <class _Tp>
  constexpr auto operator()(_Tp&& t) const noexcept(noexcept(~static_cast<_Tp&&>(t))) -> decltype(~static_cast<_Tp&&>(t)) {
    return ~static_cast<_Tp&&>(t);
  }
  using is_transparent = void;
};

// ---- identity ------------------------------------------------------------------------------
struct identity {
  template <class _Tp>
  [[nodiscard]] constexpr _Tp&& operator()(_Tp&& t) const noexcept {
    return static_cast<_Tp&&>(t);
  }
  using is_transparent = void;
};

// ---- [range.cmp] ------------------------------------------------------------------------------
namespace ranges {

struct equal_to {
  template <class _Tp, class _Up>
    requires equality_comparable_with<_Tp, _Up>
  constexpr bool operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(bool(static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u)))) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_eq<_Tp, _Up>)
      return static_cast<const volatile void*>(t) == static_cast<const volatile void*>(__u);
    else
      return static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
struct not_equal_to {
  template <class _Tp, class _Up>
    requires equality_comparable_with<_Tp, _Up>
  constexpr bool operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(bool(static_cast<_Tp&&>(t) == static_cast<_Up&&>(__u)))) {
    return !equal_to{}(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
  }
  using is_transparent = void;
};
struct less {
  template <class _Tp, class _Up>
    requires totally_ordered_with<_Tp, _Up>
  constexpr bool operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(bool(static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u)))) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_less<_Tp, _Up>)
      return __ycxx::__detail::__total_less(t, __u);
    else
      return static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u);
  }
  using is_transparent = void;
};
struct greater {
  template <class _Tp, class _Up>
    requires totally_ordered_with<_Tp, _Up>
  constexpr bool operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(bool(static_cast<_Up&&>(__u) < static_cast<_Tp&&>(t)))) {
    return less{}(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t));
  }
  using is_transparent = void;
};
struct greater_equal {
  template <class _Tp, class _Up>
    requires totally_ordered_with<_Tp, _Up>
  constexpr bool operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(bool(static_cast<_Tp&&>(t) < static_cast<_Up&&>(__u)))) {
    return !less{}(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u));
  }
  using is_transparent = void;
};
struct less_equal {
  template <class _Tp, class _Up>
    requires totally_ordered_with<_Tp, _Up>
  constexpr bool operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(bool(static_cast<_Up&&>(__u) < static_cast<_Tp&&>(t)))) {
    return !less{}(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t));
  }
  using is_transparent = void;
};

} // namespace ranges

// ---- [refwrap] -------------------------------------------------------------------------------
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
void __refwrap_fun(_Tp&) noexcept;
template <class _Tp>
void __refwrap_fun(_Tp&&) = delete;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
class reference_wrapper {
  _Tp* __ptr_;

public:
  using type = _Tp;

  template <class _Up>
    requires(!is_same_v<remove_cvref_t<_Up>, reference_wrapper>) &&
            requires(_Up&& __u) { __ycxx::__detail::__refwrap_fun<_Tp>(static_cast<_Up&&>(__u)); }
  constexpr reference_wrapper(_Up&& __u) noexcept(noexcept(__ycxx::__detail::__refwrap_fun<_Tp>(static_cast<_Up&&>(__u)))) {
    _Tp& r = static_cast<_Up&&>(__u);
    __ptr_ = __builtin_addressof(r);
  }
  constexpr reference_wrapper(const reference_wrapper&) noexcept = default;
  constexpr reference_wrapper& operator=(const reference_wrapper&) noexcept = default;

  constexpr operator _Tp&() const noexcept { return *__ptr_; }
  constexpr _Tp& get() const noexcept { return *__ptr_; }

  template <class... _Args>
  constexpr invoke_result_t<_Tp&, _Args...> operator()(_Args&&... __args) const
      noexcept(is_nothrow_invocable_v<_Tp&, _Args...>) {
    if constexpr (!is_function_v<_Tp>)
      static_assert(sizeof(_Tp) != 0, "reference_wrapper: incomplete type");
    return __ycxx::__detail::invoke(get(), static_cast<_Args&&>(__args)...);
  }

  // [refwrap.comparisons]
  friend constexpr bool operator==(reference_wrapper __x, reference_wrapper y)
    requires requires {
      { __x.get() == y.get() } -> __ycxx::__detail::__boolean_testable;
    }
  {
    return __x.get() == y.get();
  }
  friend constexpr bool operator==(reference_wrapper __x, const _Tp& y)
    requires requires {
      { __x.get() == y } -> __ycxx::__detail::__boolean_testable;
    }
  {
    return __x.get() == y;
  }
  friend constexpr bool operator==(reference_wrapper __x, reference_wrapper<const _Tp> y)
    requires(!is_const_v<_Tp>) && requires {
      { __x.get() == y.get() } -> __ycxx::__detail::__boolean_testable;
    }
  {
    return __x.get() == y.get();
  }
  friend constexpr auto operator<=>(reference_wrapper __x, reference_wrapper y)
    requires requires(const _Tp t) { __ycxx::__detail::__synth_three_way(t, t); }
  {
    return __ycxx::__detail::__synth_three_way(__x.get(), y.get());
  }
  friend constexpr auto operator<=>(reference_wrapper __x, const _Tp& y)
    requires requires { __ycxx::__detail::__synth_three_way(__x.get(), y); }
  {
    return __ycxx::__detail::__synth_three_way(__x.get(), y);
  }
  friend constexpr auto operator<=>(reference_wrapper __x, reference_wrapper<const _Tp> y)
    requires(!is_const_v<_Tp>) && requires { __ycxx::__detail::__synth_three_way(__x.get(), y.get()); }
  {
    return __ycxx::__detail::__synth_three_way(__x.get(), y.get());
  }
};

template <class _Tp>
reference_wrapper(_Tp&) -> reference_wrapper<_Tp>;

template <class _Tp>
constexpr reference_wrapper<_Tp> ref(_Tp& t) noexcept {
  return reference_wrapper<_Tp>(t);
}
template <class _Tp>
constexpr reference_wrapper<_Tp> ref(reference_wrapper<_Tp> t) noexcept {
  return t;
}
template <class _Tp>
void ref(const _Tp&&) = delete;
template <class _Tp>
constexpr reference_wrapper<const _Tp> cref(const _Tp& t) noexcept {
  return reference_wrapper<const _Tp>(t);
}
template <class _Tp>
constexpr reference_wrapper<const _Tp> cref(reference_wrapper<_Tp> t) noexcept {
  return reference_wrapper<const _Tp>(t.get());
}
template <class _Tp>
void cref(const _Tp&&) = delete;

// common_reference with reference_wrapper ([refwrap.common.ref])
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __is_ref_wrapper_v = false;
template <class _Tp>
inline constexpr bool __is_ref_wrapper_v<std::reference_wrapper<_Tp>> = true;

template <class _Rp, class _Tp, class _RQ, class _TQ>
concept __ref_wrap_common_reference_exists_with =
    __is_ref_wrapper_v<_Rp> && requires { typename std::common_reference_t<typename _Rp::type&, _TQ>; } &&
    std::convertible_to<_RQ, std::common_reference_t<typename _Rp::type&, _TQ>>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Rp, class _Tp, template <class> class _RQual, template <class> class _TQual>
  requires(__ycxx::__detail::__ref_wrap_common_reference_exists_with<_Rp, _Tp, _RQual<_Rp>, _TQual<_Tp>> &&
           !__ycxx::__detail::__ref_wrap_common_reference_exists_with<_Tp, _Rp, _TQual<_Tp>, _RQual<_Rp>>)
struct basic_common_reference<_Rp, _Tp, _RQual, _TQual> {
  using type = common_reference_t<typename _Rp::type&, _TQual<_Tp>>;
};
template <class _Tp, class _Rp, template <class> class _TQual, template <class> class _RQual>
  requires(__ycxx::__detail::__ref_wrap_common_reference_exists_with<_Rp, _Tp, _RQual<_Rp>, _TQual<_Tp>> &&
           !__ycxx::__detail::__ref_wrap_common_reference_exists_with<_Tp, _Rp, _TQual<_Tp>, _RQual<_Rp>>)
struct basic_common_reference<_Tp, _Rp, _TQual, _RQual> {
  using type = common_reference_t<typename _Rp::type&, _TQual<_Tp>>;
};

}} // namespace std
