// libycxx core: <concepts>
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/compare.hpp>

namespace std {

template <class T, class U>
concept same_as = ycxx::detail::same_as_<T, U>;

template <class Derived, class Base>
concept derived_from =
    __is_base_of(Base, Derived) && __is_convertible(const volatile Derived*, const volatile Base*);

template <class From, class To>
concept convertible_to = ycxx::detail::convertible_to_<From, To>;

template <class T, class U>
concept common_reference_with = ycxx::detail::common_reference_with_<T, U>;

template <class T, class U>
concept common_with =
    same_as<common_type_t<T, U>, common_type_t<U, T>> && requires {
      static_cast<common_type_t<T, U>>(std::declval<T>());
      static_cast<common_type_t<T, U>>(std::declval<U>());
    } && common_reference_with<add_lvalue_reference_t<const T>, add_lvalue_reference_t<const U>> &&
    common_reference_with<add_lvalue_reference_t<common_type_t<T, U>>,
                          common_reference_t<add_lvalue_reference_t<const T>, add_lvalue_reference_t<const U>>>;

template <class T>
concept integral = ::ycxx::detail::is_integral_v<T>;
template <class T>
concept signed_integral = integral<T> && ::ycxx::detail::is_signed_v<T>;
template <class T>
concept unsigned_integral = integral<T> && !signed_integral<T>;
template <class T>
concept floating_point = ::ycxx::detail::is_floating_v<T>;

template <class LHS, class RHS>
concept assignable_from =
    ::ycxx::detail::is_lref_v<LHS> &&
    common_reference_with<const ::ycxx::detail::remove_ref_t<LHS>&, const ::ycxx::detail::remove_ref_t<RHS>&> &&
    requires(LHS lhs, RHS&& rhs) {
      { lhs = static_cast<RHS&&>(rhs) } -> same_as<LHS>;
    };

template <class T>
concept destructible = __is_nothrow_destructible(T);

template <class T, class... Args>
concept constructible_from = destructible<T> && __is_constructible(T, Args...);

template <class T>
concept default_initializable = constructible_from<T> && requires { T{}; } && requires { ::new T; };

template <class T>
concept move_constructible = constructible_from<T, T> && convertible_to<T, T>;

template <class T>
concept copy_constructible = move_constructible<T> && constructible_from<T, T&> && convertible_to<T&, T> &&
                             constructible_from<T, const T&> && convertible_to<const T&, T> &&
                             constructible_from<T, const T> && convertible_to<const T, T>;

} // namespace std

// ranges::swap customization point object
namespace ycxx::detail::swap_cpo {

template <class T>
void swap(T&, T&) = delete;

template <class T, class U>
concept adl_swappable = (__is_class(__remove_cvref(T)) || __is_union(__remove_cvref(T)) || __is_enum(__remove_cvref(T)) ||
                         __is_class(__remove_cvref(U)) || __is_union(__remove_cvref(U)) || __is_enum(__remove_cvref(U))) &&
                        requires(T&& t, U&& u) { swap(static_cast<T&&>(t), static_cast<U&&>(u)); };

template <class T>
concept move_and_assign = std::move_constructible<T> && std::assignable_from<T&, T>;

struct fn {
  template <class T, class U>
    requires adl_swappable<T, U>
  constexpr void operator()(T&& t, U&& u) const noexcept(noexcept(swap(static_cast<T&&>(t), static_cast<U&&>(u)))) {
    swap(static_cast<T&&>(t), static_cast<U&&>(u));
  }

  template <class T, class U, std::size_t N>
    requires(!adl_swappable<T (&)[N], U (&)[N]>) && requires(const fn& f, T& a, U& b) { f(a, b); }
  constexpr void operator()(T (&t)[N], U (&u)[N]) const noexcept(noexcept((*this)(*t, *u))) {
    for (std::size_t i = 0; i < N; ++i)
      (*this)(t[i], u[i]);
  }

  template <class T>
    requires(!adl_swappable<T&, T&>) && move_and_assign<T>
  constexpr void operator()(T& a, T& b) const
      noexcept(__is_nothrow_constructible(T, T &&) && __is_nothrow_assignable(T&, T &&)) {
    T tmp(static_cast<T&&>(a));
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(tmp);
  }
};
} // namespace ycxx::detail::swap_cpo

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::swap_cpo::fn swap{};
}
} // namespace std::ranges

namespace std {

template <class T>
concept swappable = requires(T& a, T& b) { ranges::swap(a, b); };

template <class T, class U>
concept swappable_with = common_reference_with<T, U> && requires(T&& t, U&& u) {
  ranges::swap(static_cast<T&&>(t), static_cast<T&&>(t));
  ranges::swap(static_cast<U&&>(u), static_cast<U&&>(u));
  ranges::swap(static_cast<T&&>(t), static_cast<U&&>(u));
  ranges::swap(static_cast<U&&>(u), static_cast<T&&>(t));
};

template <class T>
concept equality_comparable = ycxx::detail::weakly_equality_comparable_with<T, T>;

template <class T, class U>
concept equality_comparable_with =
    equality_comparable<T> && equality_comparable<U> && ycxx::detail::comparison_common_type_with<T, U> &&
    equality_comparable<common_reference_t<const ::ycxx::detail::remove_ref_t<T>&, const ::ycxx::detail::remove_ref_t<U>&>> &&
    ycxx::detail::weakly_equality_comparable_with<T, U>;

template <class T>
concept totally_ordered = equality_comparable<T> && ycxx::detail::partially_ordered_with<T, T>;

template <class T, class U>
concept totally_ordered_with =
    totally_ordered<T> && totally_ordered<U> && equality_comparable_with<T, U> &&
    totally_ordered<common_reference_t<const ::ycxx::detail::remove_ref_t<T>&, const ::ycxx::detail::remove_ref_t<U>&>> &&
    ycxx::detail::partially_ordered_with<T, U>;

template <class T>
concept movable = is_object_v<T> && move_constructible<T> && assignable_from<T&, T> && swappable<T>;
template <class T>
concept copyable = copy_constructible<T> && movable<T> && assignable_from<T&, T&> &&
                   assignable_from<T&, const T&> && assignable_from<T&, const T>;
template <class T>
concept semiregular = copyable<T> && default_initializable<T>;
template <class T>
concept regular = semiregular<T> && equality_comparable<T>;

template <class F, class... Args>
concept invocable = ycxx::detail::invocable_<F, Args...>;
template <class F, class... Args>
concept regular_invocable = invocable<F, Args...>;

template <class F, class... Args>
concept predicate =
    regular_invocable<F, Args...> && ycxx::detail::boolean_testable<invoke_result_t<F, Args...>>;
template <class R, class T, class U>
concept relation = predicate<R, T, T> && predicate<R, U, U> && predicate<R, T, U> && predicate<R, U, T>;
template <class R, class T, class U>
concept equivalence_relation = relation<R, T, U>;
template <class R, class T, class U>
concept strict_weak_order = relation<R, T, U>;

} // namespace std

