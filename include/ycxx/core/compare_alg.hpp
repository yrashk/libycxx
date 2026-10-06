// libycxx core: comparison algorithms ([cmp.alg]) and type ordering ([compare.type]).
#pragma once

#include <ycxx/core/compare.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- IEEE totalOrder key for floating-point values -------------------------------------------
// Maps a floating-point value to (sign, magnitude-key) such that comparing keys implements
// IEEE 754 totalOrder (-NaN < -inf < ... < -0 < +0 < ... < +inf < +NaN).
struct __fp_key {
  bool __negative;
  __uint128 __magnitude; // exponent and significand bits, as an unsigned integer
};

struct __x87_layout {
  unsigned long long __significand;
  unsigned short __sign_exponent;
};

template <class _Tp>
constexpr __fp_key __fp_total_key(_Tp __x) noexcept {
  constexpr __fp_format_info __f = __fp_format<_Tp>;
  if constexpr (__is_same(_Tp, long double) && __f.digits == 64) {
    auto r = __builtin_bit_cast(__x87_layout, __x);
    return {static_cast<bool>(r.__sign_exponent >> 15),
            (static_cast<__uint128>(r.__sign_exponent & 0x7fff) << 64) | r.__significand};
  } else {
    using _Up = std::conditional_t<sizeof(_Tp) == 2, unsigned short,
                                 std::conditional_t<sizeof(_Tp) == 4, unsigned int,
                                                    std::conditional_t<sizeof(_Tp) == 8, unsigned long long, __uint128>>>;
    constexpr int __bits = static_cast<int>(sizeof(_Tp) * __CHAR_BIT__);
    _Up __u = __builtin_bit_cast(_Up, __x);
    _Up sign = _Up(_Up(1) << (__bits - 1));
    return {(__u & sign) != 0, static_cast<__uint128>(__u & _Up(~sign))};
  }
}

template <class _Tp>
constexpr std::strong_ordering __fp_strong_order(_Tp a, _Tp b) noexcept {
  __fp_key __ka = __fp_total_key(a), __kb = __fp_total_key(b);
  if (__ka.__negative != __kb.__negative)
    return __ka.__negative ? std::strong_ordering::less : std::strong_ordering::greater;
  if (__ka.__magnitude == __kb.__magnitude)
    return std::strong_ordering::equal;
  bool less = __ka.__magnitude < __kb.__magnitude;
  return (less != __ka.__negative) ? std::strong_ordering::less : std::strong_ordering::greater;
}

template <class _Tp>
constexpr std::weak_ordering __fp_weak_order(_Tp a, _Tp b) noexcept {
  bool __na = a != a, __nb = b != b;
  if (!__na && !__nb) { // ordinary comparison; -0 == +0
    if (a < b)
      return std::weak_ordering::less;
    if (b < a)
      return std::weak_ordering::greater;
    return std::weak_ordering::equivalent;
  }
  // NaNs: all negative NaNs are least, all positive NaNs greatest.
  auto rank = [](_Tp __v, bool nan) { return nan ? (__fp_total_key(__v).__negative ? -1 : 1) : 0; };
  int __ra = rank(a, __na), __rb = rank(b, __nb);
  if (__ra == __rb)
    return std::weak_ordering::equivalent;
  return __ra < __rb ? std::weak_ordering::less : std::weak_ordering::greater;
}

// ---- CPOs ----------------------------------------------------------------------------------
// Each call is expression-equivalent to the expression [cmp.alg] selects, so it is noexcept
// exactly when that expression is. `choose` names the selected branch (none: the call is
// ill-formed) and that expression's exception specification; the call operators are constrained
// on the first and carry the second.
namespace __cmp_cpo {

void strong_order() = delete;
void weak_order() = delete;
void partial_order() = delete;

template <class _Ep, class _Fp>
concept __same_decayed = __is_same(std::decay_t<_Ep>, std::decay_t<_Fp>);

template <class _Ep, class _Fp>
concept __adl_strong = requires(_Ep&& e, _Fp&& __f) { std::strong_ordering(strong_order(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f))); };
template <class _Ep, class _Fp>
concept __adl_weak = requires(_Ep&& e, _Fp&& __f) { std::weak_ordering(weak_order(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f))); };
template <class _Ep, class _Fp>
concept __adl_partial = requires(_Ep&& e, _Fp&& __f) { std::partial_ordering(partial_order(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f))); };

template <class _Cat, class _Ep, class _Fp>
concept __three_way_as = requires(_Ep&& e, _Fp&& __f) { _Cat(std::compare_three_way()(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f))); };

template <class _Ep>
concept __floating = __is_floating_v<std::decay_t<_Ep>>;

enum class __cmp_branch : unsigned char { none, __adl, __floating, __three_way, __stronger, operators };
struct __cmp_choice {
  __cmp_branch __branch;
  bool nothrow;
};

struct __strong_order_fn {
  template <class _Ep, class _Fp>
  static consteval __cmp_choice choose() {
    if constexpr (!__same_decayed<_Ep, _Fp>)
      return {__cmp_branch::none, false};
    else if constexpr (__adl_strong<_Ep, _Fp>)
      return {__cmp_branch::__adl, noexcept(std::strong_ordering(strong_order(std::declval<_Ep>(), std::declval<_Fp>())))};
    else if constexpr (__floating<_Ep>)
      return {__cmp_branch::__floating, true};
    else if constexpr (__three_way_as<std::strong_ordering, _Ep, _Fp>)
      return {__cmp_branch::__three_way,
              noexcept(std::strong_ordering(std::compare_three_way()(std::declval<_Ep>(), std::declval<_Fp>())))};
    else
      return {__cmp_branch::none, false};
  }

  template <class _Ep, class _Fp>
    requires(choose<_Ep, _Fp>().__branch != __cmp_branch::none)
  constexpr std::strong_ordering operator()(_Ep&& e, _Fp&& __f) const noexcept(choose<_Ep, _Fp>().nothrow) {
    constexpr __cmp_branch b = choose<_Ep, _Fp>().__branch;
    if constexpr (b == __cmp_branch::__adl)
      return std::strong_ordering(strong_order(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
    else if constexpr (b == __cmp_branch::__floating)
      return __fp_strong_order<std::decay_t<_Ep>>(e, __f);
    else
      return std::strong_ordering(std::compare_three_way()(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
  }
};
inline constexpr __strong_order_fn __strong_order_obj{};

struct __weak_order_fn {
  template <class _Ep, class _Fp>
  static consteval __cmp_choice choose() {
    if constexpr (!__same_decayed<_Ep, _Fp>)
      return {__cmp_branch::none, false};
    else if constexpr (__adl_weak<_Ep, _Fp>)
      return {__cmp_branch::__adl, noexcept(std::weak_ordering(weak_order(std::declval<_Ep>(), std::declval<_Fp>())))};
    else if constexpr (__floating<_Ep>)
      return {__cmp_branch::__floating, true};
    else if constexpr (__three_way_as<std::weak_ordering, _Ep, _Fp>)
      return {__cmp_branch::__three_way,
              noexcept(std::weak_ordering(std::compare_three_way()(std::declval<_Ep>(), std::declval<_Fp>())))};
    else if constexpr (requires { std::weak_ordering(__strong_order_obj(std::declval<_Ep>(), std::declval<_Fp>())); })
      return {__cmp_branch::__stronger, noexcept(std::weak_ordering(__strong_order_obj(std::declval<_Ep>(), std::declval<_Fp>())))};
    else
      return {__cmp_branch::none, false};
  }

  template <class _Ep, class _Fp>
    requires(choose<_Ep, _Fp>().__branch != __cmp_branch::none)
  constexpr std::weak_ordering operator()(_Ep&& e, _Fp&& __f) const noexcept(choose<_Ep, _Fp>().nothrow) {
    constexpr __cmp_branch b = choose<_Ep, _Fp>().__branch;
    if constexpr (b == __cmp_branch::__adl)
      return std::weak_ordering(weak_order(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
    else if constexpr (b == __cmp_branch::__floating)
      return __fp_weak_order<std::decay_t<_Ep>>(e, __f);
    else if constexpr (b == __cmp_branch::__three_way)
      return std::weak_ordering(std::compare_three_way()(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
    else
      return std::weak_ordering(__strong_order_obj(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
  }
};
inline constexpr __weak_order_fn __weak_order_obj{};

struct __partial_order_fn {
  template <class _Ep, class _Fp>
  static consteval __cmp_choice choose() {
    if constexpr (!__same_decayed<_Ep, _Fp>)
      return {__cmp_branch::none, false};
    else if constexpr (__adl_partial<_Ep, _Fp>)
      return {__cmp_branch::__adl, noexcept(std::partial_ordering(partial_order(std::declval<_Ep>(), std::declval<_Fp>())))};
    else if constexpr (__three_way_as<std::partial_ordering, _Ep, _Fp>)
      return {__cmp_branch::__three_way,
              noexcept(std::partial_ordering(std::compare_three_way()(std::declval<_Ep>(), std::declval<_Fp>())))};
    else if constexpr (requires { std::partial_ordering(__weak_order_obj(std::declval<_Ep>(), std::declval<_Fp>())); })
      return {__cmp_branch::__stronger, noexcept(std::partial_ordering(__weak_order_obj(std::declval<_Ep>(), std::declval<_Fp>())))};
    else
      return {__cmp_branch::none, false};
  }

  template <class _Ep, class _Fp>
    requires(choose<_Ep, _Fp>().__branch != __cmp_branch::none)
  constexpr std::partial_ordering operator()(_Ep&& e, _Fp&& __f) const noexcept(choose<_Ep, _Fp>().nothrow) {
    constexpr __cmp_branch b = choose<_Ep, _Fp>().__branch;
    if constexpr (b == __cmp_branch::__adl)
      return std::partial_ordering(partial_order(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
    else if constexpr (b == __cmp_branch::__three_way)
      return std::partial_ordering(std::compare_three_way()(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
    else
      return std::partial_ordering(__weak_order_obj(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f)));
  }
};
inline constexpr __partial_order_fn __partial_order_obj{};

// The fallbacks' operator branches evaluate E and F once each, as the lvalues e and f.
template <class _Ep, class _Fp>
concept __eq_lt_testable = requires(_Ep&& e, _Fp&& __f) {
  { e == __f } -> __boolean_testable;
  { e < __f } -> __boolean_testable;
};

struct __strong_fallback_fn {
  template <class _Ep, class _Fp>
  static consteval __cmp_choice choose() {
    if constexpr (!__same_decayed<_Ep, _Fp>)
      return {__cmp_branch::none, false};
    else if constexpr (requires { __strong_order_obj(std::declval<_Ep>(), std::declval<_Fp>()); })
      return {__cmp_branch::__stronger, noexcept(__strong_order_obj(std::declval<_Ep>(), std::declval<_Fp>()))};
    else if constexpr (__eq_lt_testable<_Ep, _Fp>)
      return {__cmp_branch::operators,
              noexcept(std::declval<_Ep&>() == std::declval<_Fp&>()  ? std::strong_ordering::equal
                       : std::declval<_Ep&>() < std::declval<_Fp&>() ? std::strong_ordering::less
                                                                 : std::strong_ordering::greater)};
    else
      return {__cmp_branch::none, false};
  }

  template <class _Ep, class _Fp>
    requires(choose<_Ep, _Fp>().__branch != __cmp_branch::none)
  constexpr std::strong_ordering operator()(_Ep&& e, _Fp&& __f) const noexcept(choose<_Ep, _Fp>().nothrow) {
    if constexpr (choose<_Ep, _Fp>().__branch == __cmp_branch::__stronger)
      return __strong_order_obj(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
    else
      return e == __f ? std::strong_ordering::equal : e < __f ? std::strong_ordering::less : std::strong_ordering::greater;
  }
};
struct __weak_fallback_fn {
  template <class _Ep, class _Fp>
  static consteval __cmp_choice choose() {
    if constexpr (!__same_decayed<_Ep, _Fp>)
      return {__cmp_branch::none, false};
    else if constexpr (requires { __weak_order_obj(std::declval<_Ep>(), std::declval<_Fp>()); })
      return {__cmp_branch::__stronger, noexcept(__weak_order_obj(std::declval<_Ep>(), std::declval<_Fp>()))};
    else if constexpr (__eq_lt_testable<_Ep, _Fp>)
      return {__cmp_branch::operators,
              noexcept(std::declval<_Ep&>() == std::declval<_Fp&>()  ? std::weak_ordering::equivalent
                       : std::declval<_Ep&>() < std::declval<_Fp&>() ? std::weak_ordering::less
                                                                 : std::weak_ordering::greater)};
    else
      return {__cmp_branch::none, false};
  }

  template <class _Ep, class _Fp>
    requires(choose<_Ep, _Fp>().__branch != __cmp_branch::none)
  constexpr std::weak_ordering operator()(_Ep&& e, _Fp&& __f) const noexcept(choose<_Ep, _Fp>().nothrow) {
    if constexpr (choose<_Ep, _Fp>().__branch == __cmp_branch::__stronger)
      return __weak_order_obj(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
    else
      return e == __f ? std::weak_ordering::equivalent : e < __f ? std::weak_ordering::less : std::weak_ordering::greater;
  }
};
struct __partial_fallback_fn {
  template <class _Ep, class _Fp>
  static consteval __cmp_choice choose() {
    if constexpr (!__same_decayed<_Ep, _Fp>)
      return {__cmp_branch::none, false};
    else if constexpr (requires { __partial_order_obj(std::declval<_Ep>(), std::declval<_Fp>()); })
      return {__cmp_branch::__stronger, noexcept(__partial_order_obj(std::declval<_Ep>(), std::declval<_Fp>()))};
    else if constexpr (requires(_Ep&& e, _Fp&& __f) {
                         { e == __f } -> __boolean_testable;
                         { e < __f } -> __boolean_testable;
                         { __f < e } -> __boolean_testable;
                       })
      return {__cmp_branch::operators,
              noexcept(std::declval<_Ep&>() == std::declval<_Fp&>()  ? std::partial_ordering::equivalent
                       : std::declval<_Ep&>() < std::declval<_Fp&>() ? std::partial_ordering::less
                       : std::declval<_Fp&>() < std::declval<_Ep&>() ? std::partial_ordering::greater
                                                                 : std::partial_ordering::unordered)};
    else
      return {__cmp_branch::none, false};
  }

  template <class _Ep, class _Fp>
    requires(choose<_Ep, _Fp>().__branch != __cmp_branch::none)
  constexpr std::partial_ordering operator()(_Ep&& e, _Fp&& __f) const noexcept(choose<_Ep, _Fp>().nothrow) {
    if constexpr (choose<_Ep, _Fp>().__branch == __cmp_branch::__stronger)
      return __partial_order_obj(static_cast<_Ep&&>(e), static_cast<_Fp&&>(__f));
    else
      return e == __f  ? std::partial_ordering::equivalent
             : e < __f ? std::partial_ordering::less
             : __f < e ? std::partial_ordering::greater
                     : std::partial_ordering::unordered;
  }
};

} // namespace cmp_cpo

// ---- type ordering --------------------------------------------------------------------------
// The implementation-defined total order on types: lexicographic order of the compiler's
// spelling of a function signature that mentions the type. Same type => same spelling;
// distinct types visible in one TU => distinct spellings.
template <class _Tp>
consteval const char* __type_spelling() {
  return __PRETTY_FUNCTION__;
}
template <class _Tp, class _Up>
consteval int __type_compare() {
  if constexpr (__is_same(_Tp, _Up)) {
    return 0;
  } else {
    const char* a = __type_spelling<_Tp>();
    const char* b = __type_spelling<_Up>();
    for (;; ++a, ++b) {
      if (*a != *b)
        return static_cast<unsigned char>(*a) < static_cast<unsigned char>(*b) ? -1 : 1;
      if (*a == '\0')
        return 0; // unreachable for distinct types
    }
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

inline namespace __cpo {
inline constexpr __ycxx::__detail::__cmp_cpo::__strong_order_fn strong_order{};
inline constexpr __ycxx::__detail::__cmp_cpo::__weak_order_fn weak_order{};
inline constexpr __ycxx::__detail::__cmp_cpo::__partial_order_fn partial_order{};
inline constexpr __ycxx::__detail::__cmp_cpo::__strong_fallback_fn compare_strong_order_fallback{};
inline constexpr __ycxx::__detail::__cmp_cpo::__weak_fallback_fn compare_weak_order_fallback{};
inline constexpr __ycxx::__detail::__cmp_cpo::__partial_fallback_fn compare_partial_order_fallback{};
} // namespace cpo

template <class _Tp, class _Up>
struct type_order {
  static constexpr strong_ordering value = __ycxx::__detail::__type_compare<_Tp, _Up>() < 0    ? strong_ordering::less
                                           : __ycxx::__detail::__type_compare<_Tp, _Up>() == 0 ? strong_ordering::equal
                                                                                     : strong_ordering::greater;
  using value_type = strong_ordering;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};
template <class _Tp, class _Up>
constexpr strong_ordering type_order_v = type_order<_Tp, _Up>::value;

} // namespace std
