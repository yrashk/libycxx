// libycxx core: function objects ([arithmetic.operations], [comparisons], [logical.operations],
// [bitwise.operations], [func.identity], [range.cmp]) and reference_wrapper ([refwrap]).
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/utility_base.hpp>

namespace ycxx::detail {
// Converts any pointer-ish operand to an integer so that pointer comparisons form a strict
// total order even across unrelated objects ([comparisons.general]/2).
template <class T>
constexpr __UINTPTR_TYPE__ ptr_value(const T& p) noexcept {
  if constexpr (std::is_pointer_v<T>)
    return reinterpret_cast<__UINTPTR_TYPE__>(p); // object and function pointers alike
  else
    return reinterpret_cast<__UINTPTR_TYPE__>(static_cast<const volatile void*>(p));
}

// BUILTIN-PTR-CMP(T, op, U): `t op u` resolves to a built-in operator comparing pointers.
template <class T, class U>
concept builtin_ptr_less = requires(T&& t, U&& u) { static_cast<T&&>(t) < static_cast<U&&>(u); } &&
                           std::is_convertible_v<T, const volatile void*> && std::is_convertible_v<U, const volatile void*> &&
                           (no_class_operand<T, U> ||
                            (!requires(T&& t, U&& u) { operator<(static_cast<T&&>(t), static_cast<U&&>(u)); } &&
                             !requires(T&& t, U&& u) { static_cast<T&&>(t).operator<(static_cast<U&&>(u)); }));
template <class T, class U>
concept builtin_ptr_eq = requires(T&& t, U&& u) { static_cast<T&&>(t) == static_cast<U&&>(u); } &&
                         std::is_convertible_v<T, const volatile void*> && std::is_convertible_v<U, const volatile void*> &&
                         (no_class_operand<T, U> ||
                          (!requires(T&& t, U&& u) { operator==(static_cast<T&&>(t), static_cast<U&&>(u)); } &&
                           !requires(T&& t, U&& u) { static_cast<T&&>(t).operator==(static_cast<U&&>(u)); }));

template <class T, class U>
constexpr bool total_less(const T& a, const U& b) {
  if consteval {
    // A null pointer orders before every other pointer; the core language leaves comparing
    // it with a pointer to an object unspecified, which constant evaluation rejects.
    const volatile void* pa = a;
    const volatile void* pb = b;
    if (pb == nullptr)
      return false;
    if (pa == nullptr)
      return true;
    return a < b;
  } else {
    return ::ycxx::detail::ptr_value(a) < ::ycxx::detail::ptr_value(b);
  }
}
} // namespace ycxx::detail

namespace std {

// ---- arithmetic --------------------------------------------------------------------------------
template <class T = void>
struct plus {
  constexpr T operator()(const T& x, const T& y) const { return x + y; }
};
template <class T = void>
struct minus {
  constexpr T operator()(const T& x, const T& y) const { return x - y; }
};
template <class T = void>
struct multiplies {
  constexpr T operator()(const T& x, const T& y) const { return x * y; }
};
template <class T = void>
struct divides {
  constexpr T operator()(const T& x, const T& y) const { return x / y; }
};
template <class T = void>
struct modulus {
  constexpr T operator()(const T& x, const T& y) const { return x % y; }
};
template <class T = void>
struct negate {
  constexpr T operator()(const T& x) const { return -x; }
};

template <>
struct plus<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) + static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) + static_cast<U&&>(u)) {
    return static_cast<T&&>(t) + static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct minus<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) - static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) - static_cast<U&&>(u)) {
    return static_cast<T&&>(t) - static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct multiplies<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) * static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) * static_cast<U&&>(u)) {
    return static_cast<T&&>(t) * static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct divides<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) / static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) / static_cast<U&&>(u)) {
    return static_cast<T&&>(t) / static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct modulus<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) % static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) % static_cast<U&&>(u)) {
    return static_cast<T&&>(t) % static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct negate<void> {
  template <class T>
  constexpr auto operator()(T&& t) const noexcept(noexcept(-static_cast<T&&>(t))) -> decltype(-static_cast<T&&>(t)) {
    return -static_cast<T&&>(t);
  }
  using is_transparent = void;
};

// ---- comparisons -------------------------------------------------------------------------------
template <class T = void>
struct equal_to {
  constexpr bool operator()(const T& x, const T& y) const { return x == y; }
};
template <class T = void>
struct not_equal_to {
  constexpr bool operator()(const T& x, const T& y) const { return x != y; }
};
template <class T = void>
struct less {
  constexpr bool operator()(const T& x, const T& y) const { return x < y; }
};
template <class T = void>
struct greater {
  constexpr bool operator()(const T& x, const T& y) const { return x > y; }
};
template <class T = void>
struct less_equal {
  constexpr bool operator()(const T& x, const T& y) const { return x <= y; }
};
template <class T = void>
struct greater_equal {
  constexpr bool operator()(const T& x, const T& y) const { return x >= y; }
};

// Pointer specializations: strict total order.
template <class T>
struct less<T*> {
  constexpr bool operator()(T* x, T* y) const noexcept { return ycxx::detail::total_less(x, y); }
};
template <class T>
struct greater<T*> {
  constexpr bool operator()(T* x, T* y) const noexcept { return ycxx::detail::total_less(y, x); }
};
template <class T>
struct less_equal<T*> {
  constexpr bool operator()(T* x, T* y) const noexcept { return !ycxx::detail::total_less(y, x); }
};
template <class T>
struct greater_equal<T*> {
  constexpr bool operator()(T* x, T* y) const noexcept { return !ycxx::detail::total_less(x, y); }
};

template <>
struct equal_to<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) == static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) == static_cast<U&&>(u)) {
    return static_cast<T&&>(t) == static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct not_equal_to<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) != static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) != static_cast<U&&>(u)) {
    return static_cast<T&&>(t) != static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct less<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) < static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) < static_cast<U&&>(u)) {
    if constexpr (ycxx::detail::builtin_ptr_less<T, U>)
      return ycxx::detail::total_less(t, u);
    else
      return static_cast<T&&>(t) < static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct greater<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) > static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) > static_cast<U&&>(u)) {
    if constexpr (ycxx::detail::builtin_ptr_less<U, T>)
      return ycxx::detail::total_less(u, t);
    else
      return static_cast<T&&>(t) > static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct less_equal<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) <= static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) <= static_cast<U&&>(u)) {
    if constexpr (ycxx::detail::builtin_ptr_less<U, T>)
      return !ycxx::detail::total_less(u, t);
    else
      return static_cast<T&&>(t) <= static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct greater_equal<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) >= static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) >= static_cast<U&&>(u)) {
    if constexpr (ycxx::detail::builtin_ptr_less<T, U>)
      return !ycxx::detail::total_less(t, u);
    else
      return static_cast<T&&>(t) >= static_cast<U&&>(u);
  }
  using is_transparent = void;
};

// ---- logical / bitwise -------------------------------------------------------------------------
template <class T = void>
struct logical_and {
  constexpr bool operator()(const T& x, const T& y) const { return x && y; }
};
template <class T = void>
struct logical_or {
  constexpr bool operator()(const T& x, const T& y) const { return x || y; }
};
template <class T = void>
struct logical_not {
  constexpr bool operator()(const T& x) const { return !x; }
};
template <class T = void>
struct bit_and {
  constexpr T operator()(const T& x, const T& y) const { return x & y; }
};
template <class T = void>
struct bit_or {
  constexpr T operator()(const T& x, const T& y) const { return x | y; }
};
template <class T = void>
struct bit_xor {
  constexpr T operator()(const T& x, const T& y) const { return x ^ y; }
};
template <class T = void>
struct bit_not {
  constexpr T operator()(const T& x) const { return ~x; }
};

template <>
struct logical_and<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) && static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) && static_cast<U&&>(u)) {
    return static_cast<T&&>(t) && static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct logical_or<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) || static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) || static_cast<U&&>(u)) {
    return static_cast<T&&>(t) || static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct logical_not<void> {
  template <class T>
  constexpr auto operator()(T&& t) const noexcept(noexcept(!static_cast<T&&>(t))) -> decltype(!static_cast<T&&>(t)) {
    return !static_cast<T&&>(t);
  }
  using is_transparent = void;
};
template <>
struct bit_and<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) & static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) & static_cast<U&&>(u)) {
    return static_cast<T&&>(t) & static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct bit_or<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) | static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) | static_cast<U&&>(u)) {
    return static_cast<T&&>(t) | static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct bit_xor<void> {
  template <class T, class U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) ^ static_cast<U&&>(u)))
      -> decltype(static_cast<T&&>(t) ^ static_cast<U&&>(u)) {
    return static_cast<T&&>(t) ^ static_cast<U&&>(u);
  }
  using is_transparent = void;
};
template <>
struct bit_not<void> {
  template <class T>
  constexpr auto operator()(T&& t) const noexcept(noexcept(~static_cast<T&&>(t))) -> decltype(~static_cast<T&&>(t)) {
    return ~static_cast<T&&>(t);
  }
  using is_transparent = void;
};

// ---- identity ------------------------------------------------------------------------------
struct identity {
  template <class T>
  [[nodiscard]] constexpr T&& operator()(T&& t) const noexcept {
    return static_cast<T&&>(t);
  }
  using is_transparent = void;
};

// ---- [range.cmp] ------------------------------------------------------------------------------
namespace ranges {

struct equal_to {
  template <class T, class U>
    requires equality_comparable_with<T, U>
  constexpr bool operator()(T&& t, U&& u) const noexcept(noexcept(bool(static_cast<T&&>(t) == static_cast<U&&>(u)))) {
    if constexpr (ycxx::detail::builtin_ptr_eq<T, U>)
      return static_cast<const volatile void*>(t) == static_cast<const volatile void*>(u);
    else
      return static_cast<T&&>(t) == static_cast<U&&>(u);
  }
  using is_transparent = void;
};
struct not_equal_to {
  template <class T, class U>
    requires equality_comparable_with<T, U>
  constexpr bool operator()(T&& t, U&& u) const noexcept(noexcept(bool(static_cast<T&&>(t) == static_cast<U&&>(u)))) {
    return !equal_to{}(static_cast<T&&>(t), static_cast<U&&>(u));
  }
  using is_transparent = void;
};
struct less {
  template <class T, class U>
    requires totally_ordered_with<T, U>
  constexpr bool operator()(T&& t, U&& u) const noexcept(noexcept(bool(static_cast<T&&>(t) < static_cast<U&&>(u)))) {
    if constexpr (ycxx::detail::builtin_ptr_less<T, U>)
      return ycxx::detail::total_less(t, u);
    else
      return static_cast<T&&>(t) < static_cast<U&&>(u);
  }
  using is_transparent = void;
};
struct greater {
  template <class T, class U>
    requires totally_ordered_with<T, U>
  constexpr bool operator()(T&& t, U&& u) const noexcept(noexcept(bool(static_cast<U&&>(u) < static_cast<T&&>(t)))) {
    return less{}(static_cast<U&&>(u), static_cast<T&&>(t));
  }
  using is_transparent = void;
};
struct greater_equal {
  template <class T, class U>
    requires totally_ordered_with<T, U>
  constexpr bool operator()(T&& t, U&& u) const noexcept(noexcept(bool(static_cast<T&&>(t) < static_cast<U&&>(u)))) {
    return !less{}(static_cast<T&&>(t), static_cast<U&&>(u));
  }
  using is_transparent = void;
};
struct less_equal {
  template <class T, class U>
    requires totally_ordered_with<T, U>
  constexpr bool operator()(T&& t, U&& u) const noexcept(noexcept(bool(static_cast<U&&>(u) < static_cast<T&&>(t)))) {
    return !less{}(static_cast<U&&>(u), static_cast<T&&>(t));
  }
  using is_transparent = void;
};

} // namespace ranges

// ---- [refwrap] -------------------------------------------------------------------------------
} // namespace std

namespace ycxx::detail {
template <class T>
void refwrap_fun(T&) noexcept;
template <class T>
void refwrap_fun(T&&) = delete;
} // namespace ycxx::detail

namespace std {

template <class T>
class reference_wrapper {
  T* ptr_;

public:
  using type = T;

  template <class U>
    requires(!is_same_v<remove_cvref_t<U>, reference_wrapper>) &&
            requires(U&& u) { ycxx::detail::refwrap_fun<T>(static_cast<U&&>(u)); }
  constexpr reference_wrapper(U&& u) noexcept(noexcept(ycxx::detail::refwrap_fun<T>(static_cast<U&&>(u)))) {
    T& r = static_cast<U&&>(u);
    ptr_ = __builtin_addressof(r);
  }
  constexpr reference_wrapper(const reference_wrapper&) noexcept = default;
  constexpr reference_wrapper& operator=(const reference_wrapper&) noexcept = default;

  constexpr operator T&() const noexcept { return *ptr_; }
  constexpr T& get() const noexcept { return *ptr_; }

  template <class... Args>
  constexpr invoke_result_t<T&, Args...> operator()(Args&&... args) const
      noexcept(is_nothrow_invocable_v<T&, Args...>) {
    if constexpr (!is_function_v<T>)
      static_assert(sizeof(T) != 0, "reference_wrapper: incomplete type");
    return ycxx::detail::invoke(get(), static_cast<Args&&>(args)...);
  }

  // [refwrap.comparisons]
  friend constexpr bool operator==(reference_wrapper x, reference_wrapper y)
    requires requires {
      { x.get() == y.get() } -> ycxx::detail::boolean_testable;
    }
  {
    return x.get() == y.get();
  }
  friend constexpr bool operator==(reference_wrapper x, const T& y)
    requires requires {
      { x.get() == y } -> ycxx::detail::boolean_testable;
    }
  {
    return x.get() == y;
  }
  friend constexpr bool operator==(reference_wrapper x, reference_wrapper<const T> y)
    requires(!is_const_v<T>) && requires {
      { x.get() == y.get() } -> ycxx::detail::boolean_testable;
    }
  {
    return x.get() == y.get();
  }
  friend constexpr auto operator<=>(reference_wrapper x, reference_wrapper y)
    requires requires(const T t) { ycxx::detail::synth_three_way(t, t); }
  {
    return ycxx::detail::synth_three_way(x.get(), y.get());
  }
  friend constexpr auto operator<=>(reference_wrapper x, const T& y)
    requires requires { ycxx::detail::synth_three_way(x.get(), y); }
  {
    return ycxx::detail::synth_three_way(x.get(), y);
  }
  friend constexpr auto operator<=>(reference_wrapper x, reference_wrapper<const T> y)
    requires(!is_const_v<T>) && requires { ycxx::detail::synth_three_way(x.get(), y.get()); }
  {
    return ycxx::detail::synth_three_way(x.get(), y.get());
  }
};

template <class T>
reference_wrapper(T&) -> reference_wrapper<T>;

template <class T>
constexpr reference_wrapper<T> ref(T& t) noexcept {
  return reference_wrapper<T>(t);
}
template <class T>
constexpr reference_wrapper<T> ref(reference_wrapper<T> t) noexcept {
  return t;
}
template <class T>
void ref(const T&&) = delete;
template <class T>
constexpr reference_wrapper<const T> cref(const T& t) noexcept {
  return reference_wrapper<const T>(t);
}
template <class T>
constexpr reference_wrapper<const T> cref(reference_wrapper<T> t) noexcept {
  return reference_wrapper<const T>(t.get());
}
template <class T>
void cref(const T&&) = delete;

// common_reference with reference_wrapper ([refwrap.common.ref])
} // namespace std

namespace ycxx::detail {
template <class T>
inline constexpr bool is_ref_wrapper_v = false;
template <class T>
inline constexpr bool is_ref_wrapper_v<std::reference_wrapper<T>> = true;

template <class R, class T, class RQ, class TQ>
concept ref_wrap_common_reference_exists_with =
    is_ref_wrapper_v<R> && requires { typename std::common_reference_t<typename R::type&, TQ>; } &&
    std::convertible_to<RQ, std::common_reference_t<typename R::type&, TQ>>;
} // namespace ycxx::detail

namespace std {

template <class R, class T, template <class> class RQual, template <class> class TQual>
  requires(ycxx::detail::ref_wrap_common_reference_exists_with<R, T, RQual<R>, TQual<T>> &&
           !ycxx::detail::ref_wrap_common_reference_exists_with<T, R, TQual<T>, RQual<R>>)
struct basic_common_reference<R, T, RQual, TQual> {
  using type = common_reference_t<typename R::type&, TQual<T>>;
};
template <class T, class R, template <class> class TQual, template <class> class RQual>
  requires(ycxx::detail::ref_wrap_common_reference_exists_with<R, T, RQual<R>, TQual<T>> &&
           !ycxx::detail::ref_wrap_common_reference_exists_with<T, R, TQual<T>, RQual<R>>)
struct basic_common_reference<T, R, TQual, RQual> {
  using type = common_reference_t<typename R::type&, TQual<T>>;
};

} // namespace std
