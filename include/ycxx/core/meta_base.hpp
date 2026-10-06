// libycxx core: the most basic metaprogramming vocabulary, used by nearly every header.
// Everything here is declared in namespace std because it is part of <type_traits>.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/prim_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, _Tp __v>
struct integral_constant {
  static constexpr _Tp value = __v;
  using value_type = _Tp;
  using type = integral_constant;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

template <bool _Bp>
using bool_constant = integral_constant<bool, _Bp>;
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

template <class _Tp, class _Up>
struct is_same : bool_constant<__is_same(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool is_same_v = __is_same(_Tp, _Up);

template <bool _Bp, class _Tp = void>
struct enable_if {};
template <class _Tp>
struct enable_if<true, _Tp> {
  using type = _Tp;
};
template <bool _Bp, class _Tp = void>
using enable_if_t = typename enable_if<_Bp, _Tp>::type;

template <bool _Bp, class _Tp, class _Fp>
struct conditional {
  using type = _Tp;
};
template <class _Tp, class _Fp>
struct conditional<false, _Tp, _Fp> {
  using type = _Fp;
};
template <bool _Bp, class _Tp, class _Fp>
using conditional_t = typename conditional<_Bp, _Tp, _Fp>::type;

template <class _Tp>
struct type_identity {
  using type = _Tp;
};
template <class _Tp>
using type_identity_t = typename type_identity<_Tp>::type;

template <class...>
using void_t = void;

template <class _Tp>
struct remove_const {
  using type = _Tp;
};
template <class _Tp>
struct remove_const<const _Tp> {
  using type = _Tp;
};
template <class _Tp>
using remove_const_t = typename remove_const<_Tp>::type;

template <class _Tp>
struct remove_volatile {
  using type = _Tp;
};
template <class _Tp>
struct remove_volatile<volatile _Tp> {
  using type = _Tp;
};
template <class _Tp>
using remove_volatile_t = typename remove_volatile<_Tp>::type;

template <class _Tp>
struct remove_cv {
  using type = __remove_cv(_Tp);
};
template <class _Tp>
using remove_cv_t = typename remove_cv<_Tp>::type;

template <class _Tp>
struct remove_reference {
  using type = ::__ycxx::__detail::__remove_ref_t<_Tp>;
};
template <class _Tp>
using remove_reference_t = typename remove_reference<_Tp>::type;

template <class _Tp>
struct remove_cvref {
  using type = __remove_cvref(_Tp);
};
template <class _Tp>
using remove_cvref_t = typename remove_cvref<_Tp>::type;

template <class _Tp>
struct add_const {
  using type = const _Tp;
};
template <class _Tp>
using add_const_t = const _Tp;
template <class _Tp>
struct add_volatile {
  using type = volatile _Tp;
};
template <class _Tp>
using add_volatile_t = volatile _Tp;
template <class _Tp>
struct add_cv {
  using type = const volatile _Tp;
};
template <class _Tp>
using add_cv_t = const volatile _Tp;

template <class _Tp>
struct add_lvalue_reference {
  using type = __add_lvalue_reference(_Tp);
};
template <class _Tp>
using add_lvalue_reference_t = typename add_lvalue_reference<_Tp>::type;
template <class _Tp>
struct add_rvalue_reference {
  using type = __add_rvalue_reference(_Tp);
};
template <class _Tp>
using add_rvalue_reference_t = typename add_rvalue_reference<_Tp>::type;

template <class _Tp>
struct decay {
  using type = __decay(_Tp);
};
template <class _Tp>
using decay_t = typename decay<_Tp>::type;

template <class _Tp>
add_rvalue_reference_t<_Tp> declval() noexcept {
  static_assert(false, "std::declval can only be used in unevaluated contexts");
}

template <class _Tp>
struct is_lvalue_reference : bool_constant<::__ycxx::__detail::__is_lref_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_lvalue_reference_v = ::__ycxx::__detail::__is_lref_v<_Tp>;
template <class _Tp>
struct is_rvalue_reference : bool_constant<::__ycxx::__detail::__is_rref_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_rvalue_reference_v = ::__ycxx::__detail::__is_rref_v<_Tp>;
template <class _Tp>
struct is_reference : bool_constant<__is_reference(_Tp)> {};
template <class _Tp>
inline constexpr bool is_reference_v = __is_reference(_Tp);

template <class _Tp>
struct is_const : bool_constant<__is_const(_Tp)> {};
template <class _Tp>
inline constexpr bool is_const_v = __is_const(_Tp);
template <class _Tp>
struct is_volatile : bool_constant<__is_volatile(_Tp)> {};
template <class _Tp>
inline constexpr bool is_volatile_v = __is_volatile(_Tp);

template <class _Tp>
struct is_void : bool_constant<::__ycxx::__detail::is_void_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_void_v = ::__ycxx::__detail::is_void_v<_Tp>;

template <class _Tp>
struct is_null_pointer : bool_constant<::__ycxx::__detail::is_null_pointer_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_null_pointer_v = ::__ycxx::__detail::is_null_pointer_v<_Tp>;

template <class _Tp>
struct is_reflection : bool_constant<::__ycxx::__detail::is_reflection_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_reflection_v = ::__ycxx::__detail::is_reflection_v<_Tp>;

template <class _Tp>
struct is_integral : bool_constant<::__ycxx::__detail::is_integral_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_integral_v = ::__ycxx::__detail::is_integral_v<_Tp>;
template <class _Tp>
struct is_floating_point : bool_constant<::__ycxx::__detail::__is_floating_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_floating_point_v = ::__ycxx::__detail::__is_floating_v<_Tp>;
template <class _Tp>
struct is_arithmetic : bool_constant<::__ycxx::__detail::is_arithmetic_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_arithmetic_v = ::__ycxx::__detail::is_arithmetic_v<_Tp>;

template <class _Tp>
struct is_array : bool_constant<__is_array(_Tp)> {};
template <class _Tp>
inline constexpr bool is_array_v = __is_array(_Tp);
template <class _Tp>
struct is_pointer : bool_constant<__is_pointer(_Tp)> {};
template <class _Tp>
inline constexpr bool is_pointer_v = __is_pointer(_Tp);
template <class _Tp>
struct is_function : bool_constant<__is_function(_Tp)> {};
template <class _Tp>
inline constexpr bool is_function_v = __is_function(_Tp);
template <class _Tp>
struct is_enum : bool_constant<__is_enum(_Tp)> {};
template <class _Tp>
inline constexpr bool is_enum_v = __is_enum(_Tp);
template <class _Tp>
struct is_union : bool_constant<__is_union(_Tp)> {};
template <class _Tp>
inline constexpr bool is_union_v = __is_union(_Tp);
template <class _Tp>
struct is_class : bool_constant<__is_class(_Tp)> {};
template <class _Tp>
inline constexpr bool is_class_v = __is_class(_Tp);
template <class _Tp>
struct is_member_pointer : bool_constant<__is_member_pointer(_Tp)> {};
template <class _Tp>
inline constexpr bool is_member_pointer_v = __is_member_pointer(_Tp);
template <class _Tp>
struct is_member_object_pointer : bool_constant<__is_member_object_pointer(_Tp)> {};
template <class _Tp>
inline constexpr bool is_member_object_pointer_v = __is_member_object_pointer(_Tp);
template <class _Tp>
struct is_member_function_pointer : bool_constant<__is_member_function_pointer(_Tp)> {};
template <class _Tp>
inline constexpr bool is_member_function_pointer_v = __is_member_function_pointer(_Tp);
template <class _Tp>
struct is_object : bool_constant<__is_object(_Tp)> {};
template <class _Tp>
inline constexpr bool is_object_v = __is_object(_Tp);
template <class _Tp>
struct is_scalar : bool_constant<::__ycxx::__detail::is_scalar_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_scalar_v = ::__ycxx::__detail::is_scalar_v<_Tp>;
template <class _Tp>
struct is_fundamental : bool_constant<::__ycxx::__detail::is_fundamental_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_fundamental_v = ::__ycxx::__detail::is_fundamental_v<_Tp>;
template <class _Tp>
struct is_compound : bool_constant<!::__ycxx::__detail::is_fundamental_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_compound_v = !::__ycxx::__detail::is_fundamental_v<_Tp>;

template <class _Tp>
struct is_trivially_copyable : bool_constant<__is_trivially_copyable(_Tp)> {};
template <class _Tp>
inline constexpr bool is_trivially_copyable_v = __is_trivially_copyable(_Tp);

template <class _Tp, class... _Args>
struct is_constructible : bool_constant<__is_constructible(_Tp, _Args...)> {};
template <class _Tp, class... _Args>
inline constexpr bool is_constructible_v = __is_constructible(_Tp, _Args...);
template <class _Tp, class... _Args>
struct is_nothrow_constructible : bool_constant<__is_nothrow_constructible(_Tp, _Args...)> {};
template <class _Tp, class... _Args>
inline constexpr bool is_nothrow_constructible_v = __is_nothrow_constructible(_Tp, _Args...);
template <class _Tp, class... _Args>
struct is_trivially_constructible : bool_constant<__is_trivially_constructible(_Tp, _Args...)> {};
template <class _Tp, class... _Args>
inline constexpr bool is_trivially_constructible_v = __is_trivially_constructible(_Tp, _Args...);

template <class _Tp, class _Up>
struct is_assignable : bool_constant<__is_assignable(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool is_assignable_v = __is_assignable(_Tp, _Up);
template <class _Tp, class _Up>
struct is_nothrow_assignable : bool_constant<__is_nothrow_assignable(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool is_nothrow_assignable_v = __is_nothrow_assignable(_Tp, _Up);
template <class _Tp, class _Up>
struct is_trivially_assignable : bool_constant<__is_trivially_assignable(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool is_trivially_assignable_v = __is_trivially_assignable(_Tp, _Up);

template <class _From, class _To>
struct is_convertible : bool_constant<__is_convertible(_From, _To)> {};
template <class _From, class _To>
inline constexpr bool is_convertible_v = __is_convertible(_From, _To);
template <class _From, class _To>
struct is_nothrow_convertible : bool_constant<__is_nothrow_convertible(_From, _To)> {};
template <class _From, class _To>
inline constexpr bool is_nothrow_convertible_v = __is_nothrow_convertible(_From, _To);

template <class _Base, class _Derived>
struct is_base_of : bool_constant<__is_base_of(_Base, _Derived)> {};
template <class _Base, class _Derived>
inline constexpr bool is_base_of_v = __is_base_of(_Base, _Derived);

template <class _Tp>
struct is_empty : bool_constant<__is_empty(_Tp)> {};
template <class _Tp>
inline constexpr bool is_empty_v = __is_empty(_Tp);

template <class _Tp>
struct is_destructible : bool_constant<__is_destructible(_Tp)> {};
template <class _Tp>
inline constexpr bool is_destructible_v = __is_destructible(_Tp);
template <class _Tp>
struct is_nothrow_destructible : bool_constant<__is_nothrow_destructible(_Tp)> {};
template <class _Tp>
inline constexpr bool is_nothrow_destructible_v = __is_nothrow_destructible(_Tp);
template <class _Tp>
struct is_trivially_destructible : bool_constant<__is_trivially_destructible(_Tp)> {};
template <class _Tp>
inline constexpr bool is_trivially_destructible_v = __is_trivially_destructible(_Tp);

constexpr bool is_constant_evaluated() noexcept {
  if consteval {
    return true;
  } else {
    return false;
  }
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __always_false = false;

template <class _Tp>
using remove_cvref = __remove_cvref(_Tp);

// Copies cv-qualifiers of From onto To.
template <class _From, class _To>
struct __copy_cv_impl {
  using type = _To;
};
template <class _From, class _To>
struct __copy_cv_impl<const _From, _To> {
  using type = const _To;
};
template <class _From, class _To>
struct __copy_cv_impl<volatile _From, _To> {
  using type = volatile _To;
};
template <class _From, class _To>
struct __copy_cv_impl<const volatile _From, _To> {
  using type = const volatile _To;
};
template <class _From, class _To>
using __copy_cv = typename __copy_cv_impl<_From, _To>::type;

// Copies cv and reference qualifiers of From onto To.
template <class _From, class _To>
struct __copy_cvref_impl {
  using type = __copy_cv<_From, _To>;
};
template <class _From, class _To>
struct __copy_cvref_impl<_From&, _To> {
  using type = __copy_cv<_From, _To>&;
};
template <class _From, class _To>
struct __copy_cvref_impl<_From&&, _To> {
  using type = __copy_cv<_From, _To>&&;
};
template <class _From, class _To>
using __copy_cvref = typename __copy_cvref_impl<_From, _To>::type;

template <class _Tp>
inline constexpr bool __is_char_like =
    __is_any_of<__remove_cv(_Tp), char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>;

// "Standard" integers: signed/unsigned integer types (excludes bool and character types).
template <class _Tp>
inline constexpr bool __is_standard_signed_integer =
    __is_any_of<__remove_cv(_Tp), signed char, short, int, long, long long, __y_int128>;
template <class _Tp>
inline constexpr bool __is_standard_unsigned_integer =
    __is_any_of<__remove_cv(_Tp), unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long, __uint128>;
// [basic.fundamental] "signed or unsigned integer type" (not bool / char types).
template <class _Tp>
inline constexpr bool __is_signed_or_unsigned_integer =
    __is_standard_signed_integer<_Tp> || __is_standard_unsigned_integer<_Tp>;

}} // namespace __ycxx::__detail

