// libycxx core: the most basic metaprogramming vocabulary, used by nearly every header.
// Everything here is declared in namespace std because it is part of <type_traits>.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/prim_traits.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

template <bool B>
using bool_constant = integral_constant<bool, B>;
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

template <class T, class U>
struct is_same : bool_constant<__is_same(T, U)> {};
template <class T, class U>
inline constexpr bool is_same_v = __is_same(T, U);

template <bool B, class T = void>
struct enable_if {};
template <class T>
struct enable_if<true, T> {
  using type = T;
};
template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

template <bool B, class T, class F>
struct conditional {
  using type = T;
};
template <class T, class F>
struct conditional<false, T, F> {
  using type = F;
};
template <bool B, class T, class F>
using conditional_t = typename conditional<B, T, F>::type;

template <class T>
struct type_identity {
  using type = T;
};
template <class T>
using type_identity_t = typename type_identity<T>::type;

template <class...>
using void_t = void;

template <class T>
struct remove_const {
  using type = T;
};
template <class T>
struct remove_const<const T> {
  using type = T;
};
template <class T>
using remove_const_t = typename remove_const<T>::type;

template <class T>
struct remove_volatile {
  using type = T;
};
template <class T>
struct remove_volatile<volatile T> {
  using type = T;
};
template <class T>
using remove_volatile_t = typename remove_volatile<T>::type;

template <class T>
struct remove_cv {
  using type = __remove_cv(T);
};
template <class T>
using remove_cv_t = typename remove_cv<T>::type;

template <class T>
struct remove_reference {
  using type = ::ycxx::detail::remove_ref_t<T>;
};
template <class T>
using remove_reference_t = typename remove_reference<T>::type;

template <class T>
struct remove_cvref {
  using type = __remove_cvref(T);
};
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <class T>
struct add_const {
  using type = const T;
};
template <class T>
using add_const_t = const T;
template <class T>
struct add_volatile {
  using type = volatile T;
};
template <class T>
using add_volatile_t = volatile T;
template <class T>
struct add_cv {
  using type = const volatile T;
};
template <class T>
using add_cv_t = const volatile T;

template <class T>
struct add_lvalue_reference {
  using type = __add_lvalue_reference(T);
};
template <class T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
template <class T>
struct add_rvalue_reference {
  using type = __add_rvalue_reference(T);
};
template <class T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template <class T>
struct decay {
  using type = __decay(T);
};
template <class T>
using decay_t = typename decay<T>::type;

template <class T>
add_rvalue_reference_t<T> declval() noexcept {
  static_assert(false, "std::declval can only be used in unevaluated contexts");
}

template <class T>
struct is_lvalue_reference : bool_constant<::ycxx::detail::is_lref_v<T>> {};
template <class T>
inline constexpr bool is_lvalue_reference_v = ::ycxx::detail::is_lref_v<T>;
template <class T>
struct is_rvalue_reference : bool_constant<::ycxx::detail::is_rref_v<T>> {};
template <class T>
inline constexpr bool is_rvalue_reference_v = ::ycxx::detail::is_rref_v<T>;
template <class T>
struct is_reference : bool_constant<__is_reference(T)> {};
template <class T>
inline constexpr bool is_reference_v = __is_reference(T);

template <class T>
struct is_const : bool_constant<__is_const(T)> {};
template <class T>
inline constexpr bool is_const_v = __is_const(T);
template <class T>
struct is_volatile : bool_constant<__is_volatile(T)> {};
template <class T>
inline constexpr bool is_volatile_v = __is_volatile(T);

template <class T>
struct is_void : bool_constant<::ycxx::detail::is_void_v<T>> {};
template <class T>
inline constexpr bool is_void_v = ::ycxx::detail::is_void_v<T>;

template <class T>
struct is_null_pointer : bool_constant<::ycxx::detail::is_null_pointer_v<T>> {};
template <class T>
inline constexpr bool is_null_pointer_v = ::ycxx::detail::is_null_pointer_v<T>;

template <class T>
struct is_reflection : bool_constant<::ycxx::detail::is_reflection_v<T>> {};
template <class T>
inline constexpr bool is_reflection_v = ::ycxx::detail::is_reflection_v<T>;

template <class T>
struct is_integral : bool_constant<::ycxx::detail::is_integral_v<T>> {};
template <class T>
inline constexpr bool is_integral_v = ::ycxx::detail::is_integral_v<T>;
template <class T>
struct is_floating_point : bool_constant<::ycxx::detail::is_floating_v<T>> {};
template <class T>
inline constexpr bool is_floating_point_v = ::ycxx::detail::is_floating_v<T>;
template <class T>
struct is_arithmetic : bool_constant<::ycxx::detail::is_arithmetic_v<T>> {};
template <class T>
inline constexpr bool is_arithmetic_v = ::ycxx::detail::is_arithmetic_v<T>;

template <class T>
struct is_array : bool_constant<__is_array(T)> {};
template <class T>
inline constexpr bool is_array_v = __is_array(T);
template <class T>
struct is_pointer : bool_constant<__is_pointer(T)> {};
template <class T>
inline constexpr bool is_pointer_v = __is_pointer(T);
template <class T>
struct is_function : bool_constant<__is_function(T)> {};
template <class T>
inline constexpr bool is_function_v = __is_function(T);
template <class T>
struct is_enum : bool_constant<__is_enum(T)> {};
template <class T>
inline constexpr bool is_enum_v = __is_enum(T);
template <class T>
struct is_union : bool_constant<__is_union(T)> {};
template <class T>
inline constexpr bool is_union_v = __is_union(T);
template <class T>
struct is_class : bool_constant<__is_class(T)> {};
template <class T>
inline constexpr bool is_class_v = __is_class(T);
template <class T>
struct is_member_pointer : bool_constant<__is_member_pointer(T)> {};
template <class T>
inline constexpr bool is_member_pointer_v = __is_member_pointer(T);
template <class T>
struct is_member_object_pointer : bool_constant<__is_member_object_pointer(T)> {};
template <class T>
inline constexpr bool is_member_object_pointer_v = __is_member_object_pointer(T);
template <class T>
struct is_member_function_pointer : bool_constant<__is_member_function_pointer(T)> {};
template <class T>
inline constexpr bool is_member_function_pointer_v = __is_member_function_pointer(T);
template <class T>
struct is_object : bool_constant<__is_object(T)> {};
template <class T>
inline constexpr bool is_object_v = __is_object(T);
template <class T>
struct is_scalar : bool_constant<::ycxx::detail::is_scalar_v<T>> {};
template <class T>
inline constexpr bool is_scalar_v = ::ycxx::detail::is_scalar_v<T>;
template <class T>
struct is_fundamental : bool_constant<::ycxx::detail::is_fundamental_v<T>> {};
template <class T>
inline constexpr bool is_fundamental_v = ::ycxx::detail::is_fundamental_v<T>;
template <class T>
struct is_compound : bool_constant<!::ycxx::detail::is_fundamental_v<T>> {};
template <class T>
inline constexpr bool is_compound_v = !::ycxx::detail::is_fundamental_v<T>;

template <class T>
struct is_trivially_copyable : bool_constant<__is_trivially_copyable(T)> {};
template <class T>
inline constexpr bool is_trivially_copyable_v = __is_trivially_copyable(T);

template <class T, class... Args>
struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};
template <class T, class... Args>
inline constexpr bool is_constructible_v = __is_constructible(T, Args...);
template <class T, class... Args>
struct is_nothrow_constructible : bool_constant<__is_nothrow_constructible(T, Args...)> {};
template <class T, class... Args>
inline constexpr bool is_nothrow_constructible_v = __is_nothrow_constructible(T, Args...);
template <class T, class... Args>
struct is_trivially_constructible : bool_constant<__is_trivially_constructible(T, Args...)> {};
template <class T, class... Args>
inline constexpr bool is_trivially_constructible_v = __is_trivially_constructible(T, Args...);

template <class T, class U>
struct is_assignable : bool_constant<__is_assignable(T, U)> {};
template <class T, class U>
inline constexpr bool is_assignable_v = __is_assignable(T, U);
template <class T, class U>
struct is_nothrow_assignable : bool_constant<__is_nothrow_assignable(T, U)> {};
template <class T, class U>
inline constexpr bool is_nothrow_assignable_v = __is_nothrow_assignable(T, U);
template <class T, class U>
struct is_trivially_assignable : bool_constant<__is_trivially_assignable(T, U)> {};
template <class T, class U>
inline constexpr bool is_trivially_assignable_v = __is_trivially_assignable(T, U);

template <class From, class To>
struct is_convertible : bool_constant<__is_convertible(From, To)> {};
template <class From, class To>
inline constexpr bool is_convertible_v = __is_convertible(From, To);
template <class From, class To>
struct is_nothrow_convertible : bool_constant<__is_nothrow_convertible(From, To)> {};
template <class From, class To>
inline constexpr bool is_nothrow_convertible_v = __is_nothrow_convertible(From, To);

template <class Base, class Derived>
struct is_base_of : bool_constant<__is_base_of(Base, Derived)> {};
template <class Base, class Derived>
inline constexpr bool is_base_of_v = __is_base_of(Base, Derived);

template <class T>
struct is_empty : bool_constant<__is_empty(T)> {};
template <class T>
inline constexpr bool is_empty_v = __is_empty(T);

template <class T>
struct is_destructible : bool_constant<__is_destructible(T)> {};
template <class T>
inline constexpr bool is_destructible_v = __is_destructible(T);
template <class T>
struct is_nothrow_destructible : bool_constant<__is_nothrow_destructible(T)> {};
template <class T>
inline constexpr bool is_nothrow_destructible_v = __is_nothrow_destructible(T);
template <class T>
struct is_trivially_destructible : bool_constant<__is_trivially_destructible(T)> {};
template <class T>
inline constexpr bool is_trivially_destructible_v = __is_trivially_destructible(T);

constexpr bool is_constant_evaluated() noexcept {
  if consteval {
    return true;
  } else {
    return false;
  }
}

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
inline constexpr bool always_false = false;

template <class T>
using remove_cvref = __remove_cvref(T);

// Copies cv-qualifiers of From onto To.
template <class From, class To>
struct copy_cv_impl {
  using type = To;
};
template <class From, class To>
struct copy_cv_impl<const From, To> {
  using type = const To;
};
template <class From, class To>
struct copy_cv_impl<volatile From, To> {
  using type = volatile To;
};
template <class From, class To>
struct copy_cv_impl<const volatile From, To> {
  using type = const volatile To;
};
template <class From, class To>
using copy_cv = typename copy_cv_impl<From, To>::type;

// Copies cv and reference qualifiers of From onto To.
template <class From, class To>
struct copy_cvref_impl {
  using type = copy_cv<From, To>;
};
template <class From, class To>
struct copy_cvref_impl<From&, To> {
  using type = copy_cv<From, To>&;
};
template <class From, class To>
struct copy_cvref_impl<From&&, To> {
  using type = copy_cv<From, To>&&;
};
template <class From, class To>
using copy_cvref = typename copy_cvref_impl<From, To>::type;

template <class T>
inline constexpr bool is_char_like =
    is_any_of<__remove_cv(T), char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>;

// "Standard" integers: signed/unsigned integer types (excludes bool and character types).
template <class T>
inline constexpr bool is_standard_signed_integer =
    is_any_of<__remove_cv(T), signed char, short, int, long, long long, int128>;
template <class T>
inline constexpr bool is_standard_unsigned_integer =
    is_any_of<__remove_cv(T), unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long, uint128>;
// [basic.fundamental] "signed or unsigned integer type" (not bool / char types).
template <class T>
inline constexpr bool is_signed_or_unsigned_integer =
    is_standard_signed_integer<T> || is_standard_unsigned_integer<T>;

}} // namespace ycxx::detail

