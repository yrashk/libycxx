// libycxx core: <compare>
#pragma once

#include <ycxx/core/type_traits.hpp>

namespace ycxx::detail {

// The "unspecified type" that the comparison category operators accept: only the literal 0.
struct literal_zero {
  consteval literal_zero(int v) noexcept {
    if (v != 0)
      only_literal_zero_may_be_compared_with_an_ordering();
  }
  // Not constexpr: calling it makes the consteval constructor fail.
  static void only_literal_zero_may_be_compared_with_an_ordering() noexcept;
};

enum class ord_value : signed char { less = -1, equivalent = 0, greater = 1, unordered = -128 };

} // namespace ycxx::detail

namespace std {

class partial_ordering {
  using V = ycxx::detail::ord_value;
  using Z = ycxx::detail::literal_zero;
  signed char v_;
  constexpr explicit partial_ordering(V v) noexcept : v_(static_cast<signed char>(v)) {}

  friend class weak_ordering;
  friend class strong_ordering;

public:
  static const partial_ordering less;
  static const partial_ordering equivalent;
  static const partial_ordering greater;
  static const partial_ordering unordered;

  friend constexpr bool operator==(partial_ordering v, Z) noexcept { return v.v_ == 0; }
  friend constexpr bool operator==(partial_ordering, partial_ordering) noexcept = default;
  friend constexpr bool operator<(partial_ordering v, Z) noexcept { return v.v_ == -1; }
  friend constexpr bool operator>(partial_ordering v, Z) noexcept { return v.v_ == 1; }
  friend constexpr bool operator<=(partial_ordering v, Z) noexcept { return v.v_ == 0 || v.v_ == -1; }
  friend constexpr bool operator>=(partial_ordering v, Z) noexcept { return v.v_ == 0 || v.v_ == 1; }
  friend constexpr bool operator<(Z, partial_ordering v) noexcept { return v.v_ == 1; }
  friend constexpr bool operator>(Z, partial_ordering v) noexcept { return v.v_ == -1; }
  friend constexpr bool operator<=(Z, partial_ordering v) noexcept { return v.v_ == 0 || v.v_ == 1; }
  friend constexpr bool operator>=(Z, partial_ordering v) noexcept { return v.v_ == 0 || v.v_ == -1; }
  friend constexpr partial_ordering operator<=>(partial_ordering v, Z) noexcept { return v; }
  friend constexpr partial_ordering operator<=>(Z, partial_ordering v) noexcept {
    return v.v_ == -128 ? v : partial_ordering(static_cast<V>(-v.v_));
  }
};
inline constexpr partial_ordering partial_ordering::less(ycxx::detail::ord_value::less);
inline constexpr partial_ordering partial_ordering::equivalent(ycxx::detail::ord_value::equivalent);
inline constexpr partial_ordering partial_ordering::greater(ycxx::detail::ord_value::greater);
inline constexpr partial_ordering partial_ordering::unordered(ycxx::detail::ord_value::unordered);

class weak_ordering {
  using V = ycxx::detail::ord_value;
  using Z = ycxx::detail::literal_zero;
  signed char v_;
  constexpr explicit weak_ordering(V v) noexcept : v_(static_cast<signed char>(v)) {}

  friend class strong_ordering;

public:
  static const weak_ordering less;
  static const weak_ordering equivalent;
  static const weak_ordering greater;

  constexpr operator partial_ordering() const noexcept { return partial_ordering(static_cast<V>(v_)); }

  friend constexpr bool operator==(weak_ordering v, Z) noexcept { return v.v_ == 0; }
  friend constexpr bool operator==(weak_ordering, weak_ordering) noexcept = default;
  friend constexpr bool operator<(weak_ordering v, Z) noexcept { return v.v_ < 0; }
  friend constexpr bool operator>(weak_ordering v, Z) noexcept { return v.v_ > 0; }
  friend constexpr bool operator<=(weak_ordering v, Z) noexcept { return v.v_ <= 0; }
  friend constexpr bool operator>=(weak_ordering v, Z) noexcept { return v.v_ >= 0; }
  friend constexpr bool operator<(Z, weak_ordering v) noexcept { return 0 < v.v_; }
  friend constexpr bool operator>(Z, weak_ordering v) noexcept { return 0 > v.v_; }
  friend constexpr bool operator<=(Z, weak_ordering v) noexcept { return 0 <= v.v_; }
  friend constexpr bool operator>=(Z, weak_ordering v) noexcept { return 0 >= v.v_; }
  friend constexpr weak_ordering operator<=>(weak_ordering v, Z) noexcept { return v; }
  friend constexpr weak_ordering operator<=>(Z, weak_ordering v) noexcept {
    return weak_ordering(static_cast<V>(-v.v_));
  }
};
inline constexpr weak_ordering weak_ordering::less(ycxx::detail::ord_value::less);
inline constexpr weak_ordering weak_ordering::equivalent(ycxx::detail::ord_value::equivalent);
inline constexpr weak_ordering weak_ordering::greater(ycxx::detail::ord_value::greater);

class strong_ordering {
  using V = ycxx::detail::ord_value;
  using Z = ycxx::detail::literal_zero;
  signed char v_;
  constexpr explicit strong_ordering(V v) noexcept : v_(static_cast<signed char>(v)) {}

public:
  static const strong_ordering less;
  static const strong_ordering equal;
  static const strong_ordering equivalent;
  static const strong_ordering greater;

  constexpr operator partial_ordering() const noexcept { return partial_ordering(static_cast<V>(v_)); }
  constexpr operator weak_ordering() const noexcept { return weak_ordering(static_cast<V>(v_)); }

  friend constexpr bool operator==(strong_ordering v, Z) noexcept { return v.v_ == 0; }
  friend constexpr bool operator==(strong_ordering, strong_ordering) noexcept = default;
  friend constexpr bool operator<(strong_ordering v, Z) noexcept { return v.v_ < 0; }
  friend constexpr bool operator>(strong_ordering v, Z) noexcept { return v.v_ > 0; }
  friend constexpr bool operator<=(strong_ordering v, Z) noexcept { return v.v_ <= 0; }
  friend constexpr bool operator>=(strong_ordering v, Z) noexcept { return v.v_ >= 0; }
  friend constexpr bool operator<(Z, strong_ordering v) noexcept { return 0 < v.v_; }
  friend constexpr bool operator>(Z, strong_ordering v) noexcept { return 0 > v.v_; }
  friend constexpr bool operator<=(Z, strong_ordering v) noexcept { return 0 <= v.v_; }
  friend constexpr bool operator>=(Z, strong_ordering v) noexcept { return 0 >= v.v_; }
  friend constexpr strong_ordering operator<=>(strong_ordering v, Z) noexcept { return v; }
  friend constexpr strong_ordering operator<=>(Z, strong_ordering v) noexcept {
    return strong_ordering(static_cast<V>(-v.v_));
  }
};
inline constexpr strong_ordering strong_ordering::less(ycxx::detail::ord_value::less);
inline constexpr strong_ordering strong_ordering::equal(ycxx::detail::ord_value::equivalent);
inline constexpr strong_ordering strong_ordering::equivalent(ycxx::detail::ord_value::equivalent);
inline constexpr strong_ordering strong_ordering::greater(ycxx::detail::ord_value::greater);

constexpr bool is_eq(partial_ordering cmp) noexcept { return cmp == 0; }
constexpr bool is_neq(partial_ordering cmp) noexcept { return cmp != 0; }
constexpr bool is_lt(partial_ordering cmp) noexcept { return cmp < 0; }
constexpr bool is_lteq(partial_ordering cmp) noexcept { return cmp <= 0; }
constexpr bool is_gt(partial_ordering cmp) noexcept { return cmp > 0; }
constexpr bool is_gteq(partial_ordering cmp) noexcept { return cmp >= 0; }

} // namespace std

namespace ycxx::detail {
// 0 = partial, 1 = weak, 2 = strong, -1 = not a comparison category
template <class T>
inline constexpr int cmp_cat_rank = -1;
template <>
inline constexpr int cmp_cat_rank<std::partial_ordering> = 0;
template <>
inline constexpr int cmp_cat_rank<std::weak_ordering> = 1;
template <>
inline constexpr int cmp_cat_rank<std::strong_ordering> = 2;

template <class... Ts>
consteval int common_cmp_cat() {
  int r = 2;
  ((r = cmp_cat_rank<Ts> < r ? cmp_cat_rank<Ts> : r), ...);
  return r;
}
template <int R>
struct cmp_cat_of {
  using type = void;
};
template <>
struct cmp_cat_of<0> {
  using type = std::partial_ordering;
};
template <>
struct cmp_cat_of<1> {
  using type = std::weak_ordering;
};
template <>
struct cmp_cat_of<2> {
  using type = std::strong_ordering;
};
} // namespace ycxx::detail

namespace std {

template <class... Ts>
struct common_comparison_category {
  using type = typename ycxx::detail::cmp_cat_of<ycxx::detail::common_cmp_cat<Ts...>()>::type;
};
template <class... Ts>
using common_comparison_category_t = typename common_comparison_category<Ts...>::type;

} // namespace std

// ---------------------------------------------------------------------------------------------
// Concepts needed by three_way_comparable (subset of <concepts>, defined here to avoid a cycle).
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail {

template <class T>
concept boolean_testable_impl = __is_convertible(T, bool);
template <class T>
concept boolean_testable = boolean_testable_impl<T> && requires(T&& t) {
  { !static_cast<T&&>(t) } -> boolean_testable_impl;
};

template <class T, class U>
concept weakly_equality_comparable_with =
    requires(const ::ycxx::detail::remove_ref_t<T>& t, const ::ycxx::detail::remove_ref_t<U>& u) {
      { t == u } -> boolean_testable;
      { t != u } -> boolean_testable;
      { u == t } -> boolean_testable;
      { u != t } -> boolean_testable;
    };

template <class T, class U>
concept partially_ordered_with = requires(const ::ycxx::detail::remove_ref_t<T>& t, const ::ycxx::detail::remove_ref_t<U>& u) {
  { t < u } -> boolean_testable;
  { t > u } -> boolean_testable;
  { t <= u } -> boolean_testable;
  { t >= u } -> boolean_testable;
  { u < t } -> boolean_testable;
  { u > t } -> boolean_testable;
  { u <= t } -> boolean_testable;
  { u >= t } -> boolean_testable;
};

template <class T, class Cat>
concept compares_as = __is_same(std::common_comparison_category_t<T, Cat>, Cat);

// [concept.same]: same-as-impl applied in both orders. One atomic constraint used twice is what
// makes same_as<T, U> and same_as<U, T> subsume each other; two different atoms
// (__is_same(T, U) && __is_same(U, T)) do not.
template <class T, class U>
concept same_as_impl = __is_same(T, U);
template <class T, class U>
concept same_as_ = same_as_impl<T, U> && same_as_impl<U, T>;

template <class From, class To>
concept convertible_to_ = __is_convertible(From, To) && requires { static_cast<To>(std::declval<From>()); };

template <class T, class U>
concept common_reference_with_ =
    same_as_<std::common_reference_t<T, U>, std::common_reference_t<U, T>> &&
    convertible_to_<T, std::common_reference_t<T, U>> && convertible_to_<U, std::common_reference_t<T, U>>;

template <class T, class U, class C = std::common_reference_t<const T&, const U&>>
concept comparison_common_type_with_impl =
    same_as_<std::common_reference_t<const T&, const U&>, std::common_reference_t<const U&, const T&>> &&
    requires {
      requires convertible_to_<const T&, const C&> || convertible_to_<T, const C&>;
      requires convertible_to_<const U&, const C&> || convertible_to_<U, const C&>;
    };
template <class T, class U>
concept comparison_common_type_with = comparison_common_type_with_impl<__remove_cvref(T), __remove_cvref(U)>;

} // namespace ycxx::detail

namespace std {

template <class T, class Cat = partial_ordering>
concept three_way_comparable =
    ycxx::detail::weakly_equality_comparable_with<T, T> && ycxx::detail::partially_ordered_with<T, T> &&
    requires(const ::ycxx::detail::remove_ref_t<T>& a, const ::ycxx::detail::remove_ref_t<T>& b) {
      { a <=> b } -> ycxx::detail::compares_as<Cat>;
    };

template <class T, class U, class Cat = partial_ordering>
concept three_way_comparable_with =
    three_way_comparable<T, Cat> && three_way_comparable<U, Cat> &&
    ycxx::detail::comparison_common_type_with<T, U> &&
    three_way_comparable<common_reference_t<const ::ycxx::detail::remove_ref_t<T>&, const ::ycxx::detail::remove_ref_t<U>&>, Cat> &&
    ycxx::detail::weakly_equality_comparable_with<T, U> && ycxx::detail::partially_ordered_with<T, U> &&
    requires(const ::ycxx::detail::remove_ref_t<T>& t, const ::ycxx::detail::remove_ref_t<U>& u) {
      { t <=> u } -> ycxx::detail::compares_as<Cat>;
      { u <=> t } -> ycxx::detail::compares_as<Cat>;
    };

template <class T, class U = T>
struct compare_three_way_result {};
template <class T, class U>
  requires requires(const ::ycxx::detail::remove_ref_t<T>& t, const ::ycxx::detail::remove_ref_t<U>& u) { t <=> u; }
struct compare_three_way_result<T, U> {
  using type = decltype(std::declval<const ::ycxx::detail::remove_ref_t<T>&>() <=>
                        std::declval<const ::ycxx::detail::remove_ref_t<U>&>());
};
template <class T, class U = T>
using compare_three_way_result_t = typename compare_three_way_result<T, U>::type;

} // namespace std

namespace ycxx::detail {
// BUILTIN-PTR-CMP(T, op, U): the comparison resolves to a built-in pointer comparison.
template <class T, class U>
concept builtin_ptr_three_way = requires(T&& t, U&& u) {
  static_cast<T&&>(t) <=> static_cast<U&&>(u);
} && __is_convertible(T, const volatile void*) && __is_convertible(U, const volatile void*) &&
                                !requires(T&& t, U&& u) {
                                  operator<=>(static_cast<T&&>(t), static_cast<U&&>(u));
                                } && !requires(T&& t, U&& u) { static_cast<T&&>(t).operator<=>(static_cast<U&&>(u)); };
} // namespace ycxx::detail

namespace std {

struct compare_three_way {
  template <class T, class U>
    requires three_way_comparable_with<T, U> || ycxx::detail::builtin_ptr_three_way<T, U>
  constexpr auto operator()(T&& t, U&& u) const noexcept(noexcept(static_cast<T&&>(t) <=> static_cast<U&&>(u))) {
    if constexpr (ycxx::detail::builtin_ptr_three_way<T, U>) {
      auto pt = static_cast<const volatile void*>(t);
      auto pu = static_cast<const volatile void*>(u);
      if consteval {
        // Null orders before every other pointer (see total_less in functional_base.hpp).
        if (pt == pu)
          return strong_ordering::equal;
        if (pt == nullptr)
          return strong_ordering::less;
        if (pu == nullptr)
          return strong_ordering::greater;
        return pt <=> pu;
      } else {
        auto a = reinterpret_cast<__UINTPTR_TYPE__>(pt);
        auto b = reinterpret_cast<__UINTPTR_TYPE__>(pu);
        return a <=> b;
      }
    } else {
      return static_cast<T&&>(t) <=> static_cast<U&&>(u);
    }
  }
  using is_transparent = void;
};

} // namespace std

