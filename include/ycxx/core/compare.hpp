// libycxx core: <compare>
#pragma once

#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// The "unspecified type" that the comparison category operators accept: only the literal 0.
struct __literal_zero {
  consteval __literal_zero(int __v) noexcept {
    if (__v != 0)
      __only_literal_zero_may_be_compared_with_an_ordering();
  }
  // Not constexpr: calling it makes the consteval constructor fail.
  static void __only_literal_zero_may_be_compared_with_an_ordering() noexcept;
};

enum class __ord_value : signed char { less = -1, equivalent = 0, greater = 1, unordered = -128 };

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

class partial_ordering {
  using _Vp = __ycxx::__detail::__ord_value;
  using _Zp = __ycxx::__detail::__literal_zero;
  signed char __v_;
  constexpr explicit partial_ordering(_Vp __v) noexcept : __v_(static_cast<signed char>(__v)) {}

  friend class weak_ordering;
  friend class strong_ordering;

public:
  static const partial_ordering less;
  static const partial_ordering equivalent;
  static const partial_ordering greater;
  static const partial_ordering unordered;

  friend constexpr bool operator==(partial_ordering __v, _Zp) noexcept { return __v.__v_ == 0; }
  friend constexpr bool operator==(partial_ordering, partial_ordering) noexcept = default;
  friend constexpr bool operator<(partial_ordering __v, _Zp) noexcept { return __v.__v_ == -1; }
  friend constexpr bool operator>(partial_ordering __v, _Zp) noexcept { return __v.__v_ == 1; }
  friend constexpr bool operator<=(partial_ordering __v, _Zp) noexcept { return __v.__v_ == 0 || __v.__v_ == -1; }
  friend constexpr bool operator>=(partial_ordering __v, _Zp) noexcept { return __v.__v_ == 0 || __v.__v_ == 1; }
  friend constexpr bool operator<(_Zp, partial_ordering __v) noexcept { return __v.__v_ == 1; }
  friend constexpr bool operator>(_Zp, partial_ordering __v) noexcept { return __v.__v_ == -1; }
  friend constexpr bool operator<=(_Zp, partial_ordering __v) noexcept { return __v.__v_ == 0 || __v.__v_ == 1; }
  friend constexpr bool operator>=(_Zp, partial_ordering __v) noexcept { return __v.__v_ == 0 || __v.__v_ == -1; }
  friend constexpr partial_ordering operator<=>(partial_ordering __v, _Zp) noexcept { return __v; }
  friend constexpr partial_ordering operator<=>(_Zp, partial_ordering __v) noexcept {
    return __v.__v_ == -128 ? __v : partial_ordering(static_cast<_Vp>(-__v.__v_));
  }
};
inline constexpr partial_ordering partial_ordering::less(__ycxx::__detail::__ord_value::less);
inline constexpr partial_ordering partial_ordering::equivalent(__ycxx::__detail::__ord_value::equivalent);
inline constexpr partial_ordering partial_ordering::greater(__ycxx::__detail::__ord_value::greater);
inline constexpr partial_ordering partial_ordering::unordered(__ycxx::__detail::__ord_value::unordered);

class weak_ordering {
  using _Vp = __ycxx::__detail::__ord_value;
  using _Zp = __ycxx::__detail::__literal_zero;
  signed char __v_;
  constexpr explicit weak_ordering(_Vp __v) noexcept : __v_(static_cast<signed char>(__v)) {}

  friend class strong_ordering;

public:
  static const weak_ordering less;
  static const weak_ordering equivalent;
  static const weak_ordering greater;

  constexpr operator partial_ordering() const noexcept { return partial_ordering(static_cast<_Vp>(__v_)); }

  friend constexpr bool operator==(weak_ordering __v, _Zp) noexcept { return __v.__v_ == 0; }
  friend constexpr bool operator==(weak_ordering, weak_ordering) noexcept = default;
  friend constexpr bool operator<(weak_ordering __v, _Zp) noexcept { return __v.__v_ < 0; }
  friend constexpr bool operator>(weak_ordering __v, _Zp) noexcept { return __v.__v_ > 0; }
  friend constexpr bool operator<=(weak_ordering __v, _Zp) noexcept { return __v.__v_ <= 0; }
  friend constexpr bool operator>=(weak_ordering __v, _Zp) noexcept { return __v.__v_ >= 0; }
  friend constexpr bool operator<(_Zp, weak_ordering __v) noexcept { return 0 < __v.__v_; }
  friend constexpr bool operator>(_Zp, weak_ordering __v) noexcept { return 0 > __v.__v_; }
  friend constexpr bool operator<=(_Zp, weak_ordering __v) noexcept { return 0 <= __v.__v_; }
  friend constexpr bool operator>=(_Zp, weak_ordering __v) noexcept { return 0 >= __v.__v_; }
  friend constexpr weak_ordering operator<=>(weak_ordering __v, _Zp) noexcept { return __v; }
  friend constexpr weak_ordering operator<=>(_Zp, weak_ordering __v) noexcept {
    return weak_ordering(static_cast<_Vp>(-__v.__v_));
  }
};
inline constexpr weak_ordering weak_ordering::less(__ycxx::__detail::__ord_value::less);
inline constexpr weak_ordering weak_ordering::equivalent(__ycxx::__detail::__ord_value::equivalent);
inline constexpr weak_ordering weak_ordering::greater(__ycxx::__detail::__ord_value::greater);

class strong_ordering {
  using _Vp = __ycxx::__detail::__ord_value;
  using _Zp = __ycxx::__detail::__literal_zero;
  signed char __v_;
  constexpr explicit strong_ordering(_Vp __v) noexcept : __v_(static_cast<signed char>(__v)) {}

public:
  static const strong_ordering less;
  static const strong_ordering equal;
  static const strong_ordering equivalent;
  static const strong_ordering greater;

  constexpr operator partial_ordering() const noexcept { return partial_ordering(static_cast<_Vp>(__v_)); }
  constexpr operator weak_ordering() const noexcept { return weak_ordering(static_cast<_Vp>(__v_)); }

  friend constexpr bool operator==(strong_ordering __v, _Zp) noexcept { return __v.__v_ == 0; }
  friend constexpr bool operator==(strong_ordering, strong_ordering) noexcept = default;
  friend constexpr bool operator<(strong_ordering __v, _Zp) noexcept { return __v.__v_ < 0; }
  friend constexpr bool operator>(strong_ordering __v, _Zp) noexcept { return __v.__v_ > 0; }
  friend constexpr bool operator<=(strong_ordering __v, _Zp) noexcept { return __v.__v_ <= 0; }
  friend constexpr bool operator>=(strong_ordering __v, _Zp) noexcept { return __v.__v_ >= 0; }
  friend constexpr bool operator<(_Zp, strong_ordering __v) noexcept { return 0 < __v.__v_; }
  friend constexpr bool operator>(_Zp, strong_ordering __v) noexcept { return 0 > __v.__v_; }
  friend constexpr bool operator<=(_Zp, strong_ordering __v) noexcept { return 0 <= __v.__v_; }
  friend constexpr bool operator>=(_Zp, strong_ordering __v) noexcept { return 0 >= __v.__v_; }
  friend constexpr strong_ordering operator<=>(strong_ordering __v, _Zp) noexcept { return __v; }
  friend constexpr strong_ordering operator<=>(_Zp, strong_ordering __v) noexcept {
    return strong_ordering(static_cast<_Vp>(-__v.__v_));
  }
};
inline constexpr strong_ordering strong_ordering::less(__ycxx::__detail::__ord_value::less);
inline constexpr strong_ordering strong_ordering::equal(__ycxx::__detail::__ord_value::equivalent);
inline constexpr strong_ordering strong_ordering::equivalent(__ycxx::__detail::__ord_value::equivalent);
inline constexpr strong_ordering strong_ordering::greater(__ycxx::__detail::__ord_value::greater);

constexpr bool is_eq(partial_ordering __cmp) noexcept { return __cmp == 0; }
constexpr bool is_neq(partial_ordering __cmp) noexcept { return __cmp != 0; }
constexpr bool is_lt(partial_ordering __cmp) noexcept { return __cmp < 0; }
constexpr bool is_lteq(partial_ordering __cmp) noexcept { return __cmp <= 0; }
constexpr bool is_gt(partial_ordering __cmp) noexcept { return __cmp > 0; }
constexpr bool is_gteq(partial_ordering __cmp) noexcept { return __cmp >= 0; }

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// 0 = partial, 1 = weak, 2 = strong, -1 = not a comparison category
template <class _Tp>
inline constexpr int __cmp_cat_rank = -1;
template <>
inline constexpr int __cmp_cat_rank<std::partial_ordering> = 0;
template <>
inline constexpr int __cmp_cat_rank<std::weak_ordering> = 1;
template <>
inline constexpr int __cmp_cat_rank<std::strong_ordering> = 2;

template <class... _Ts>
consteval int __common_cmp_cat() {
  int r = 2;
  ((r = __cmp_cat_rank<_Ts> < r ? __cmp_cat_rank<_Ts> : r), ...);
  return r;
}
template <int _Rp>
struct __cmp_cat_of {
  using type = void;
};
template <>
struct __cmp_cat_of<0> {
  using type = std::partial_ordering;
};
template <>
struct __cmp_cat_of<1> {
  using type = std::weak_ordering;
};
template <>
struct __cmp_cat_of<2> {
  using type = std::strong_ordering;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class... _Ts>
struct common_comparison_category {
  using type = typename __ycxx::__detail::__cmp_cat_of<__ycxx::__detail::__common_cmp_cat<_Ts...>()>::type;
};
template <class... _Ts>
using common_comparison_category_t = typename common_comparison_category<_Ts...>::type;

} // namespace std

// ---------------------------------------------------------------------------------------------
// Concepts needed by three_way_comparable (subset of <concepts>, defined here to avoid a cycle).
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
concept __boolean_testable_impl = __is_convertible(_Tp, bool);
template <class _Tp>
concept __boolean_testable = __boolean_testable_impl<_Tp> && requires(_Tp&& t) {
  { !static_cast<_Tp&&>(t) } -> __boolean_testable_impl;
};

template <class _Tp, class _Up>
concept __weakly_equality_comparable_with =
    requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& t, const ::__ycxx::__detail::__remove_ref_t<_Up>& __u) {
      { t == __u } -> __boolean_testable;
      { t != __u } -> __boolean_testable;
      { __u == t } -> __boolean_testable;
      { __u != t } -> __boolean_testable;
    };

template <class _Tp, class _Up>
concept __partially_ordered_with = requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& t, const ::__ycxx::__detail::__remove_ref_t<_Up>& __u) {
  { t < __u } -> __boolean_testable;
  { t > __u } -> __boolean_testable;
  { t <= __u } -> __boolean_testable;
  { t >= __u } -> __boolean_testable;
  { __u < t } -> __boolean_testable;
  { __u > t } -> __boolean_testable;
  { __u <= t } -> __boolean_testable;
  { __u >= t } -> __boolean_testable;
};

template <class _Tp, class _Cat>
concept __compares_as = __is_same(std::common_comparison_category_t<_Tp, _Cat>, _Cat);

// [concept.same]: same-as-impl applied in both orders. One atomic constraint used twice is what
// makes same_as<T, U> and same_as<U, T> subsume each other; two different atoms
// (__is_same(T, U) && __is_same(U, T)) do not.
template <class _Tp, class _Up>
concept __same_as_impl = __is_same(_Tp, _Up);
template <class _Tp, class _Up>
concept __same_as_ = __same_as_impl<_Tp, _Up> && __same_as_impl<_Up, _Tp>;

template <class _From, class _To>
concept __convertible_to_ = __is_convertible(_From, _To) && requires { static_cast<_To>(std::declval<_From>()); };

template <class _Tp, class _Up>
concept __common_reference_with_ =
    __same_as_<std::common_reference_t<_Tp, _Up>, std::common_reference_t<_Up, _Tp>> &&
    __convertible_to_<_Tp, std::common_reference_t<_Tp, _Up>> && __convertible_to_<_Up, std::common_reference_t<_Tp, _Up>>;

template <class _Tp, class _Up, class _Cp = std::common_reference_t<const _Tp&, const _Up&>>
concept __comparison_common_type_with_impl =
    __same_as_<std::common_reference_t<const _Tp&, const _Up&>, std::common_reference_t<const _Up&, const _Tp&>> &&
    requires {
      requires __convertible_to_<const _Tp&, const _Cp&> || __convertible_to_<_Tp, const _Cp&>;
      requires __convertible_to_<const _Up&, const _Cp&> || __convertible_to_<_Up, const _Cp&>;
    };
template <class _Tp, class _Up>
concept __comparison_common_type_with = __comparison_common_type_with_impl<__remove_cvref(_Tp), __remove_cvref(_Up)>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Cat = partial_ordering>
concept three_way_comparable =
    __ycxx::__detail::__weakly_equality_comparable_with<_Tp, _Tp> && __ycxx::__detail::__partially_ordered_with<_Tp, _Tp> &&
    requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& a, const ::__ycxx::__detail::__remove_ref_t<_Tp>& b) {
      { a <=> b } -> __ycxx::__detail::__compares_as<_Cat>;
    };

template <class _Tp, class _Up, class _Cat = partial_ordering>
concept three_way_comparable_with =
    three_way_comparable<_Tp, _Cat> && three_way_comparable<_Up, _Cat> &&
    __ycxx::__detail::__comparison_common_type_with<_Tp, _Up> &&
    three_way_comparable<common_reference_t<const ::__ycxx::__detail::__remove_ref_t<_Tp>&, const ::__ycxx::__detail::__remove_ref_t<_Up>&>, _Cat> &&
    __ycxx::__detail::__weakly_equality_comparable_with<_Tp, _Up> && __ycxx::__detail::__partially_ordered_with<_Tp, _Up> &&
    requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& t, const ::__ycxx::__detail::__remove_ref_t<_Up>& __u) {
      { t <=> __u } -> __ycxx::__detail::__compares_as<_Cat>;
      { __u <=> t } -> __ycxx::__detail::__compares_as<_Cat>;
    };

template <class _Tp, class _Up = _Tp>
struct compare_three_way_result {};
template <class _Tp, class _Up>
  requires requires(const ::__ycxx::__detail::__remove_ref_t<_Tp>& t, const ::__ycxx::__detail::__remove_ref_t<_Up>& __u) { t <=> __u; }
struct compare_three_way_result<_Tp, _Up> {
  using type = decltype(std::declval<const ::__ycxx::__detail::__remove_ref_t<_Tp>&>() <=>
                        std::declval<const ::__ycxx::__detail::__remove_ref_t<_Up>&>());
};
template <class _Tp, class _Up = _Tp>
using compare_three_way_result_t = typename compare_three_way_result<_Tp, _Up>::type;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Neither operand has class or enumeration type, so an operator expression on them is always
// the built-in one ([over.match.oper]/1). Checked first: looking for operator functions by
// ADL would complete the classes that pointer operands point to.
template <class _Tp, class _Up>
concept __no_class_operand = !__is_class(__remove_cvref(_Tp)) && !__is_union(__remove_cvref(_Tp)) &&
                           !__is_enum(__remove_cvref(_Tp)) && !__is_class(__remove_cvref(_Up)) &&
                           !__is_union(__remove_cvref(_Up)) && !__is_enum(__remove_cvref(_Up));

// A user-declared operator<=> (member or non-member) accepts the operands in either order, so
// it is a candidate for `t <=> __u` and, as a rewritten or synthesized candidate, for `t < __u`,
// `t > __u`, `t <= __u` and `t >= __u` ([over.match.oper]/3.4).
template <class _Tp, class _Up>
concept __user_three_way_candidate =
    requires(_Tp&& t, _Up&& __u) { operator<=>(static_cast<_Tp&&>(t), static_cast<_Up&&>(__u)); } ||
    requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t).operator<=>(static_cast<_Up&&>(__u)); } ||
    requires(_Tp&& t, _Up&& __u) { operator<=>(static_cast<_Up&&>(__u), static_cast<_Tp&&>(t)); } ||
    requires(_Tp&& t, _Up&& __u) { static_cast<_Up&&>(__u).operator<=>(static_cast<_Tp&&>(t)); };

// BUILTIN-PTR-CMP(T, op, U): the comparison resolves to a built-in pointer comparison.
template <class _Tp, class _Up>
concept __y_builtin_ptr_three_way =
    requires(_Tp&& t, _Up&& __u) { static_cast<_Tp&&>(t) <=> static_cast<_Up&&>(__u); } &&
    __is_convertible(_Tp, const volatile void*) && __is_convertible(_Up, const volatile void*) &&
    (__no_class_operand<_Tp, _Up> || !__user_three_way_candidate<_Tp, _Up>);
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

struct compare_three_way {
  template <class _Tp, class _Up>
    requires three_way_comparable_with<_Tp, _Up>
  constexpr auto operator()(_Tp&& t, _Up&& __u) const noexcept(noexcept(static_cast<_Tp&&>(t) <=> static_cast<_Up&&>(__u))) {
    if constexpr (__ycxx::__detail::__y_builtin_ptr_three_way<_Tp, _Up>) {
      auto __pt = static_cast<const volatile void*>(t);
      auto __pu = static_cast<const volatile void*>(__u);
      if consteval {
        // Null orders before every other pointer (see total_less in functional_base.hpp).
        if (__pt == __pu)
          return strong_ordering::equal;
        if (__pt == nullptr)
          return strong_ordering::less;
        if (__pu == nullptr)
          return strong_ordering::greater;
        return __pt <=> __pu;
      } else {
        auto a = reinterpret_cast<__UINTPTR_TYPE__>(__pt);
        auto b = reinterpret_cast<__UINTPTR_TYPE__>(__pu);
        return a <=> b;
      }
    } else {
      return static_cast<_Tp&&>(t) <=> static_cast<_Up&&>(__u);
    }
  }
  using is_transparent = void;
};

} // namespace std

