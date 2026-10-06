// libycxx core: the complete <type_traits>.
#pragma once

#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/move.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/swap.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---------------------------------------------------------------------------------------------
// [meta.unary.prop]
// ---------------------------------------------------------------------------------------------
template <class _Tp>
struct is_signed : bool_constant<::__ycxx::__detail::is_signed_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_signed_v = ::__ycxx::__detail::is_signed_v<_Tp>;
template <class _Tp>
struct is_unsigned : bool_constant<::__ycxx::__detail::is_unsigned_v<_Tp>> {};
template <class _Tp>
inline constexpr bool is_unsigned_v = ::__ycxx::__detail::is_unsigned_v<_Tp>;

template <class _Tp>
struct is_bounded_array : bool_constant<__is_bounded_array(_Tp)> {};
template <class _Tp>
inline constexpr bool is_bounded_array_v = __is_bounded_array(_Tp);
template <class _Tp>
struct is_unbounded_array : bool_constant<__is_unbounded_array(_Tp)> {};
template <class _Tp>
inline constexpr bool is_unbounded_array_v = __is_unbounded_array(_Tp);

template <class _Tp>
struct is_scoped_enum : bool_constant<__is_scoped_enum(_Tp)> {};
template <class _Tp>
inline constexpr bool is_scoped_enum_v = __is_scoped_enum(_Tp);

template <class _Tp>
struct is_standard_layout : bool_constant<__is_standard_layout(_Tp)> {};
template <class _Tp>
inline constexpr bool is_standard_layout_v = __is_standard_layout(_Tp);
template <class _Tp>
struct is_polymorphic : bool_constant<__is_polymorphic(_Tp)> {};
template <class _Tp>
inline constexpr bool is_polymorphic_v = __is_polymorphic(_Tp);
template <class _Tp>
struct is_abstract : bool_constant<__is_abstract(_Tp)> {};
template <class _Tp>
inline constexpr bool is_abstract_v = __is_abstract(_Tp);
template <class _Tp>
struct is_final : bool_constant<__is_final(_Tp)> {};
template <class _Tp>
inline constexpr bool is_final_v = __is_final(_Tp);
template <class _Tp>
struct is_aggregate : bool_constant<__is_aggregate(_Tp)> {};
template <class _Tp>
inline constexpr bool is_aggregate_v = __is_aggregate(_Tp);

#if _YCXX_HAS_IS_STRUCTURAL
template <class _Tp>
struct is_structural : bool_constant<__builtin_is_structural(_Tp)> {};
template <class _Tp>
inline constexpr bool is_structural_v = __builtin_is_structural(_Tp);
#endif

// is_trivial / is_trivial_v ([depr.meta.types]) are in type_traits_depr.hpp.

template <class _Tp>
struct is_implicit_lifetime : bool_constant<__builtin_is_implicit_lifetime(_Tp)> {};
template <class _Tp>
inline constexpr bool is_implicit_lifetime_v = __builtin_is_implicit_lifetime(_Tp);

template <class _Tp>
struct is_default_constructible : bool_constant<__is_constructible(_Tp)> {};
template <class _Tp>
inline constexpr bool is_default_constructible_v = __is_constructible(_Tp);
template <class _Tp>
struct is_copy_constructible : bool_constant<__is_constructible(_Tp, __add_lvalue_reference(const _Tp))> {};
template <class _Tp>
inline constexpr bool is_copy_constructible_v = __is_constructible(_Tp, __add_lvalue_reference(const _Tp));
template <class _Tp>
struct is_move_constructible : bool_constant<__is_constructible(_Tp, __add_rvalue_reference(_Tp))> {};
template <class _Tp>
inline constexpr bool is_move_constructible_v = __is_constructible(_Tp, __add_rvalue_reference(_Tp));

template <class _Tp>
struct is_copy_assignable
    : bool_constant<__is_assignable(__add_lvalue_reference(_Tp), __add_lvalue_reference(const _Tp))> {};
template <class _Tp>
inline constexpr bool is_copy_assignable_v =
    __is_assignable(__add_lvalue_reference(_Tp), __add_lvalue_reference(const _Tp));
template <class _Tp>
struct is_move_assignable : bool_constant<__is_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp))> {};
template <class _Tp>
inline constexpr bool is_move_assignable_v = __is_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp));

template <class _Tp>
struct is_trivially_default_constructible : bool_constant<__is_trivially_constructible(_Tp)> {};
template <class _Tp>
inline constexpr bool is_trivially_default_constructible_v = __is_trivially_constructible(_Tp);
template <class _Tp>
struct is_trivially_copy_constructible
    : bool_constant<__is_trivially_constructible(_Tp, __add_lvalue_reference(const _Tp))> {};
template <class _Tp>
inline constexpr bool is_trivially_copy_constructible_v =
    __is_trivially_constructible(_Tp, __add_lvalue_reference(const _Tp));
template <class _Tp>
struct is_trivially_move_constructible : bool_constant<__is_trivially_constructible(_Tp, __add_rvalue_reference(_Tp))> {
};
template <class _Tp>
inline constexpr bool is_trivially_move_constructible_v = __is_trivially_constructible(_Tp, __add_rvalue_reference(_Tp));
template <class _Tp>
struct is_trivially_copy_assignable
    : bool_constant<__is_trivially_assignable(__add_lvalue_reference(_Tp), __add_lvalue_reference(const _Tp))> {};
template <class _Tp>
inline constexpr bool is_trivially_copy_assignable_v =
    __is_trivially_assignable(__add_lvalue_reference(_Tp), __add_lvalue_reference(const _Tp));
template <class _Tp>
struct is_trivially_move_assignable
    : bool_constant<__is_trivially_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp))> {};
template <class _Tp>
inline constexpr bool is_trivially_move_assignable_v =
    __is_trivially_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp));

template <class _Tp>
struct is_nothrow_default_constructible : bool_constant<__is_nothrow_constructible(_Tp)> {};
template <class _Tp>
inline constexpr bool is_nothrow_default_constructible_v = __is_nothrow_constructible(_Tp);
template <class _Tp>
struct is_nothrow_copy_constructible
    : bool_constant<__is_nothrow_constructible(_Tp, __add_lvalue_reference(const _Tp))> {};
template <class _Tp>
inline constexpr bool is_nothrow_copy_constructible_v =
    __is_nothrow_constructible(_Tp, __add_lvalue_reference(const _Tp));
template <class _Tp>
struct is_nothrow_move_constructible : bool_constant<__is_nothrow_constructible(_Tp, __add_rvalue_reference(_Tp))> {};
template <class _Tp>
inline constexpr bool is_nothrow_move_constructible_v = __is_nothrow_constructible(_Tp, __add_rvalue_reference(_Tp));
template <class _Tp>
struct is_nothrow_copy_assignable
    : bool_constant<__is_nothrow_assignable(__add_lvalue_reference(_Tp), __add_lvalue_reference(const _Tp))> {};
template <class _Tp>
inline constexpr bool is_nothrow_copy_assignable_v =
    __is_nothrow_assignable(__add_lvalue_reference(_Tp), __add_lvalue_reference(const _Tp));
template <class _Tp>
struct is_nothrow_move_assignable
    : bool_constant<__is_nothrow_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp))> {};
template <class _Tp>
inline constexpr bool is_nothrow_move_assignable_v =
    __is_nothrow_assignable(__add_lvalue_reference(_Tp), __add_rvalue_reference(_Tp));

template <class _Tp>
struct has_virtual_destructor : bool_constant<__has_virtual_destructor(_Tp)> {};
template <class _Tp>
inline constexpr bool has_virtual_destructor_v = __has_virtual_destructor(_Tp);

template <class _Tp>
struct has_unique_object_representations : bool_constant<__has_unique_object_representations(_Tp)> {};
template <class _Tp>
inline constexpr bool has_unique_object_representations_v = __has_unique_object_representations(_Tp);

template <class _Tp, class _Up>
struct reference_constructs_from_temporary : bool_constant<__reference_constructs_from_temporary(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool reference_constructs_from_temporary_v = __reference_constructs_from_temporary(_Tp, _Up);
template <class _Tp, class _Up>
struct reference_converts_from_temporary : bool_constant<__reference_converts_from_temporary(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool reference_converts_from_temporary_v = __reference_converts_from_temporary(_Tp, _Up);

// ---------------------------------------------------------------------------------------------
// [meta.unary.prop.query]
// ---------------------------------------------------------------------------------------------
template <class _Tp>
struct alignment_of : integral_constant<size_t, alignof(_Tp)> {};
template <class _Tp>
inline constexpr size_t alignment_of_v = alignof(_Tp);

template <class _Tp>
struct rank : integral_constant<size_t, __array_rank(_Tp)> {};
template <class _Tp>
inline constexpr size_t rank_v = __array_rank(_Tp);

template <class _Tp, unsigned _Ip = 0>
inline constexpr size_t extent_v = 0;
template <class _Tp, size_t _Np>
inline constexpr size_t extent_v<_Tp[_Np], 0> = _Np;
template <class _Tp, unsigned _Ip>
inline constexpr size_t extent_v<_Tp[], _Ip> = extent_v<_Tp, _Ip - 1>;
template <class _Tp, size_t _Np, unsigned _Ip>
inline constexpr size_t extent_v<_Tp[_Np], _Ip> = extent_v<_Tp, _Ip - 1>;
template <class _Tp, unsigned _Ip = 0>
struct extent : integral_constant<size_t, extent_v<_Tp, _Ip>> {};

// ---------------------------------------------------------------------------------------------
// [meta.rel]
// ---------------------------------------------------------------------------------------------
template <class _Base, class _Derived>
struct is_virtual_base_of : bool_constant<__builtin_is_virtual_base_of(_Base, _Derived)> {};
template <class _Base, class _Derived>
inline constexpr bool is_virtual_base_of_v = __builtin_is_virtual_base_of(_Base, _Derived);

template <class _Tp, class _Up>
struct is_layout_compatible : bool_constant<__is_layout_compatible(_Tp, _Up)> {};
template <class _Tp, class _Up>
inline constexpr bool is_layout_compatible_v = __is_layout_compatible(_Tp, _Up);

template <class _Base, class _Derived>
struct is_pointer_interconvertible_base_of : bool_constant<__is_pointer_interconvertible_base_of(_Base, _Derived)> {};
template <class _Base, class _Derived>
inline constexpr bool is_pointer_interconvertible_base_of_v = __is_pointer_interconvertible_base_of(_Base, _Derived);

// Available where the compiler provides the builtin (GCC 16); a constraint failure otherwise.
template <class _Sp, class _Mp>
  requires __ycxx::__detail::__y_builtin::__has_is_pointer_interconvertible_with_class<_Sp, _Mp>
constexpr bool is_pointer_interconvertible_with_class(_Mp _Sp::* m) noexcept {
  return __builtin_is_pointer_interconvertible_with_class(m);
}
template <class _S1, class _S2, class _M1, class _M2>
  requires __ycxx::__detail::__y_builtin::__has_is_corresponding_member<_S1, _S2, _M1, _M2>
constexpr bool is_corresponding_member(_M1 _S1::* __m1, _M2 _S2::* __m2) noexcept {
  return __builtin_is_corresponding_member(__m1, __m2);
}

// ---------------------------------------------------------------------------------------------
// [meta.trans.sign]
// ---------------------------------------------------------------------------------------------
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
struct __sign_pair; // {signed, unsigned} for each standard integer type
template <class _Sp, class _Up>
struct __sign_pair_def {
  using s = _Sp;
  using __u = _Up;
};
template <> struct __sign_pair<signed char> : __sign_pair_def<signed char, unsigned char> {};
template <> struct __sign_pair<unsigned char> : __sign_pair_def<signed char, unsigned char> {};
template <> struct __sign_pair<short> : __sign_pair_def<short, unsigned short> {};
template <> struct __sign_pair<unsigned short> : __sign_pair_def<short, unsigned short> {};
template <> struct __sign_pair<int> : __sign_pair_def<int, unsigned int> {};
template <> struct __sign_pair<unsigned int> : __sign_pair_def<int, unsigned int> {};
template <> struct __sign_pair<long> : __sign_pair_def<long, unsigned long> {};
template <> struct __sign_pair<unsigned long> : __sign_pair_def<long, unsigned long> {};
template <> struct __sign_pair<long long> : __sign_pair_def<long long, unsigned long long> {};
template <> struct __sign_pair<unsigned long long> : __sign_pair_def<long long, unsigned long long> {};
template <> struct __sign_pair<__y_int128> : __sign_pair_def<__y_int128, __uint128> {};
template <> struct __sign_pair<__uint128> : __sign_pair_def<__y_int128, __uint128> {};

// Smallest-rank standard integer type with the same size (for character types and enums).
template <class _Tp>
consteval auto __same_size_signed() {
  if constexpr (sizeof(_Tp) == sizeof(signed char))
    return static_cast<signed char*>(nullptr);
  else if constexpr (sizeof(_Tp) == sizeof(short))
    return static_cast<short*>(nullptr);
  else if constexpr (sizeof(_Tp) == sizeof(int))
    return static_cast<int*>(nullptr);
  else if constexpr (sizeof(_Tp) == sizeof(long))
    return static_cast<long*>(nullptr);
  else if constexpr (sizeof(_Tp) == sizeof(long long))
    return static_cast<long long*>(nullptr);
  else
    return static_cast<__y_int128*>(nullptr);
}

template <class _Tp>
struct __sign_base {
  using type = __sign_pair<__remove_pointer(decltype(__same_size_signed<_Tp>()))>;
};
template <class _Tp>
  requires requires { typename __sign_pair<_Tp>::s; }
struct __sign_base<_Tp> {
  using type = __sign_pair<_Tp>;
};

template <class _Tp>
concept __sign_changeable = (is_integral_v<_Tp> && !__is_same(__remove_cv(_Tp), bool)) || __is_enum(_Tp);
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
struct make_signed {};
template <class _Tp>
  requires __ycxx::__detail::__sign_changeable<_Tp>
struct make_signed<_Tp> {
  using type = __ycxx::__detail::__copy_cv<_Tp, typename __ycxx::__detail::__sign_base<__remove_cv(_Tp)>::type::s>;
};
template <class _Tp>
using make_signed_t = typename make_signed<_Tp>::type;
template <class _Tp>
struct make_unsigned {};
template <class _Tp>
  requires __ycxx::__detail::__sign_changeable<_Tp>
struct make_unsigned<_Tp> {
  using type = __ycxx::__detail::__copy_cv<_Tp, typename __ycxx::__detail::__sign_base<__remove_cv(_Tp)>::type::__u>;
};
template <class _Tp>
using make_unsigned_t = typename make_unsigned<_Tp>::type;


// ---------------------------------------------------------------------------------------------
// [meta.trans.arr], [meta.trans.ptr]
// ---------------------------------------------------------------------------------------------
template <class _Tp>
struct remove_extent {
  using type = __remove_extent(_Tp);
};
template <class _Tp>
using remove_extent_t = typename remove_extent<_Tp>::type;
template <class _Tp>
struct remove_all_extents {
  using type = __remove_all_extents(_Tp);
};
template <class _Tp>
using remove_all_extents_t = typename remove_all_extents<_Tp>::type;

template <class _Tp>
struct remove_pointer {
  using type = __remove_pointer(_Tp);
};
template <class _Tp>
using remove_pointer_t = typename remove_pointer<_Tp>::type;
template <class _Tp>
struct add_pointer {
  using type = __add_pointer(_Tp);
};
template <class _Tp>
using add_pointer_t = typename add_pointer<_Tp>::type;

// ---------------------------------------------------------------------------------------------
// [meta.trans.other]
// ---------------------------------------------------------------------------------------------
template <class _Tp>
struct underlying_type {};
template <class _Tp>
  requires __is_enum(_Tp)
struct underlying_type<_Tp> {
  using type = __underlying_type(_Tp);
};
template <class _Tp>
using underlying_type_t = typename underlying_type<_Tp>::type;

template <class _Tp>
struct unwrap_reference {
  using type = _Tp;
};
template <class _Tp>
struct unwrap_reference<reference_wrapper<_Tp>> {
  using type = _Tp&;
};
template <class _Tp>
using unwrap_reference_t = typename unwrap_reference<_Tp>::type;
template <class _Tp>
struct unwrap_ref_decay : unwrap_reference<__decay(_Tp)> {};
template <class _Tp>
using unwrap_ref_decay_t = typename unwrap_ref_decay<_Tp>::type;

// conjunction / disjunction / negation: short-circuiting, and derive from the deciding trait.
template <class... _Bp>
struct conjunction : true_type {};
template <class _B1, class... _Bn>
struct conjunction<_B1, _Bn...> : conditional_t<!bool(_B1::value) || sizeof...(_Bn) == 0, _B1, conjunction<_Bn...>> {};
template <class... _Bp>
inline constexpr bool conjunction_v = conjunction<_Bp...>::value;

template <class... _Bp>
struct disjunction : false_type {};
template <class _B1, class... _Bn>
struct disjunction<_B1, _Bn...> : conditional_t<bool(_B1::value) || sizeof...(_Bn) == 0, _B1, disjunction<_Bn...>> {};
template <class... _Bp>
inline constexpr bool disjunction_v = disjunction<_Bp...>::value;

template <class _Bp>
struct negation : bool_constant<!bool(_Bp::value)> {};
template <class _Bp>
inline constexpr bool negation_v = !bool(_Bp::value);

} // namespace std

// ---------------------------------------------------------------------------------------------
// common_type
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Xp, class _Yp>
using __cond_res = decltype(false ? std::declval<_Xp (&)()>()() : std::declval<_Yp (&)()>()());

template <class _T1, class _T2>
struct __common_type_cref {};
template <class _T1, class _T2>
  requires requires { typename __cond_res<const _T1&, const _T2&>; }
struct __common_type_cref<_T1, _T2> {
  using type = __decay(__cond_res<const _T1&, const _T2&>);
};

template <class _D1, class _D2>
using __cond_decay = __decay(decltype(false ? std::declval<_D1>() : std::declval<_D2>()));

template <class _D1, class _D2>
struct __common_type_decayed : __common_type_cref<_D1, _D2> {};
template <class _D1, class _D2>
  requires requires { typename __cond_decay<_D1, _D2>; }
struct __common_type_decayed<_D1, _D2> {
  using type = __cond_decay<_D1, _D2>;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class... _Tp>
struct common_type {};
template <class _Tp>
struct common_type<_Tp> : common_type<_Tp, _Tp> {};

template <class _T1, class _T2>
struct common_type<_T1, _T2>
    : conditional_t<__is_same(_T1, __decay(_T1)) && __is_same(_T2, __decay(_T2)),
                    __ycxx::__detail::__common_type_decayed<_T1, _T2>, common_type<__decay(_T1), __decay(_T2)>> {};

template <class _T1, class _T2, class _T3, class... _Rp>
  requires requires { typename common_type<_T1, _T2>::type; }
struct common_type<_T1, _T2, _T3, _Rp...> : common_type<typename common_type<_T1, _T2>::type, _T3, _Rp...> {};

template <class... _Tp>
using common_type_t = typename common_type<_Tp...>::type;

// ---------------------------------------------------------------------------------------------
// common_reference
// ---------------------------------------------------------------------------------------------
template <class _Tp, class _Up, template <class> class _TQual, template <class> class _UQual>
struct basic_common_reference {};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Ap>
struct __xref {
  template <class _Up>
  using apply = __copy_cvref<_Ap, _Up>;
};

template <class _Ap, class _Bp>
struct __common_ref {};

template <class _Ap, class _Bp>
using __common_ref_t = typename __common_ref<_Ap, _Bp>::type;

// A = X&, B = Y&
template <class _Xp, class _Yp>
  requires requires { typename __cond_res<__copy_cv<_Xp, _Yp>&, __copy_cv<_Yp, _Xp>&>; } &&
           __is_reference(__cond_res<__copy_cv<_Xp, _Yp>&, __copy_cv<_Yp, _Xp>&>)
struct __common_ref<_Xp&, _Yp&> {
  using type = __cond_res<__copy_cv<_Xp, _Yp>&, __copy_cv<_Yp, _Xp>&>;
};

// A = X&&, B = Y&&
template <class _Xp, class _Yp>
  requires requires { typename __common_ref_t<_Xp&, _Yp&>; } &&
           __is_convertible(_Xp &&, ::__ycxx::__detail::__remove_ref_t<__common_ref_t<_Xp&, _Yp&>> &&) &&
           __is_convertible(_Yp &&, ::__ycxx::__detail::__remove_ref_t<__common_ref_t<_Xp&, _Yp&>> &&)
struct __common_ref<_Xp&&, _Yp&&> {
  using type = ::__ycxx::__detail::__remove_ref_t<__common_ref_t<_Xp&, _Yp&>> &&;
};

// A = X&&, B = Y&
template <class _Xp, class _Yp>
  requires requires { typename __common_ref_t<const _Xp&, _Yp&>; } &&
           __is_convertible(_Xp &&, __common_ref_t<const _Xp&, _Yp&>)
struct __common_ref<_Xp&&, _Yp&> {
  using type = __common_ref_t<const _Xp&, _Yp&>;
};
// A = X&, B = Y&&
template <class _Xp, class _Yp>
  requires requires { typename __common_ref<_Yp&&, _Xp&>::type; }
struct __common_ref<_Xp&, _Yp&&> : __common_ref<_Yp&&, _Xp&> {};

template <class _T1, class _T2>
using __basic_common_ref_t =
    typename std::basic_common_reference<__remove_cvref(_T1), __remove_cvref(_T2), __xref<_T1>::template apply,
                                         __xref<_T2>::template apply>::type;

template <class _T1, class _T2>
struct __common_reference2_d : std::common_type<_T1, _T2> {}; // bullet 4 (and 5)

template <class _T1, class _T2>
struct __common_reference2_c : __common_reference2_d<_T1, _T2> {};
template <class _T1, class _T2>
  requires requires { typename __cond_res<_T1, _T2>; }
struct __common_reference2_c<_T1, _T2> {
  using type = __cond_res<_T1, _T2>;
};

template <class _T1, class _T2>
struct __common_reference2_b : __common_reference2_c<_T1, _T2> {};
template <class _T1, class _T2>
  requires requires { typename __basic_common_ref_t<_T1, _T2>; }
struct __common_reference2_b<_T1, _T2> {
  using type = __basic_common_ref_t<_T1, _T2>;
};

template <class _T1, class _T2>
struct __common_reference2 : __common_reference2_b<_T1, _T2> {};
template <class _T1, class _T2>
  requires __is_reference(_T1) && __is_reference(_T2) && requires { typename __common_ref_t<_T1, _T2>; } &&
           __is_convertible(__add_pointer(_T1), __add_pointer(__common_ref_t<_T1, _T2>)) &&
           __is_convertible(__add_pointer(_T2), __add_pointer(__common_ref_t<_T1, _T2>))
struct __common_reference2<_T1, _T2> {
  using type = __common_ref_t<_T1, _T2>;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class... _Tp>
struct common_reference {};
template <class _Tp>
struct common_reference<_Tp> {
  using type = _Tp;
};
template <class _T1, class _T2>
struct common_reference<_T1, _T2> : __ycxx::__detail::__common_reference2<_T1, _T2> {};
template <class _T1, class _T2, class _T3, class... _Rp>
  requires requires { typename common_reference<_T1, _T2>::type; }
struct common_reference<_T1, _T2, _T3, _Rp...> : common_reference<typename common_reference<_T1, _T2>::type, _T3, _Rp...> {};

template <class... _Tp>
using common_reference_t = typename common_reference<_Tp...>::type;

// ---------------------------------------------------------------------------------------------
// [meta.const.eval]
// ---------------------------------------------------------------------------------------------
// Available where the compiler provides the builtin (Clang 23); a constraint failure otherwise.
// For U other than void, the cast to U must also be a constant subexpression (a downcast to a
// type the object is not): __builtin_constant_p of an expression using the cast answers that
// during constant evaluation.
template <class _Up = void, class _Tp>
  requires __ycxx::__detail::__y_builtin::__has_is_within_lifetime<_Tp>
consteval bool is_within_lifetime(const _Tp* p) noexcept {
  static_assert(requires { static_cast<const volatile _Up*>(p); },
                "std::is_within_lifetime: static_cast<const volatile U*>(p) must be well-formed");
  if (!__builtin_is_within_lifetime(p))
    return false;
  if constexpr (is_void_v<_Up>)
    return true;
  else
    return __builtin_constant_p(static_cast<const volatile _Up*>(p) == p);
}

} // namespace std

