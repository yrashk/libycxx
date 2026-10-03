// libycxx core: the complete <type_traits>.
#pragma once

#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/move.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/swap.hpp>

namespace std {

// ---------------------------------------------------------------------------------------------
// [meta.unary.prop]
// ---------------------------------------------------------------------------------------------
template <class T>
struct is_signed : bool_constant<::ycxx::detail::is_signed_v<T>> {};
template <class T>
inline constexpr bool is_signed_v = ::ycxx::detail::is_signed_v<T>;
template <class T>
struct is_unsigned : bool_constant<::ycxx::detail::is_unsigned_v<T>> {};
template <class T>
inline constexpr bool is_unsigned_v = ::ycxx::detail::is_unsigned_v<T>;

template <class T>
struct is_bounded_array : bool_constant<__is_bounded_array(T)> {};
template <class T>
inline constexpr bool is_bounded_array_v = __is_bounded_array(T);
template <class T>
struct is_unbounded_array : bool_constant<__is_unbounded_array(T)> {};
template <class T>
inline constexpr bool is_unbounded_array_v = __is_unbounded_array(T);

template <class T>
struct is_scoped_enum : bool_constant<__is_scoped_enum(T)> {};
template <class T>
inline constexpr bool is_scoped_enum_v = __is_scoped_enum(T);

template <class T>
struct is_standard_layout : bool_constant<__is_standard_layout(T)> {};
template <class T>
inline constexpr bool is_standard_layout_v = __is_standard_layout(T);
template <class T>
struct is_polymorphic : bool_constant<__is_polymorphic(T)> {};
template <class T>
inline constexpr bool is_polymorphic_v = __is_polymorphic(T);
template <class T>
struct is_abstract : bool_constant<__is_abstract(T)> {};
template <class T>
inline constexpr bool is_abstract_v = __is_abstract(T);
template <class T>
struct is_final : bool_constant<__is_final(T)> {};
template <class T>
inline constexpr bool is_final_v = __is_final(T);
template <class T>
struct is_aggregate : bool_constant<__is_aggregate(T)> {};
template <class T>
inline constexpr bool is_aggregate_v = __is_aggregate(T);

// is_trivial / is_trivial_v are deprecated in C++26 (P3247) and intentionally not provided.

template <class T>
struct is_implicit_lifetime : bool_constant<__builtin_is_implicit_lifetime(T)> {};
template <class T>
inline constexpr bool is_implicit_lifetime_v = __builtin_is_implicit_lifetime(T);

template <class T>
struct is_default_constructible : bool_constant<__is_constructible(T)> {};
template <class T>
inline constexpr bool is_default_constructible_v = __is_constructible(T);
template <class T>
struct is_copy_constructible : bool_constant<__is_constructible(T, __add_lvalue_reference(const T))> {};
template <class T>
inline constexpr bool is_copy_constructible_v = __is_constructible(T, __add_lvalue_reference(const T));
template <class T>
struct is_move_constructible : bool_constant<__is_constructible(T, __add_rvalue_reference(T))> {};
template <class T>
inline constexpr bool is_move_constructible_v = __is_constructible(T, __add_rvalue_reference(T));

template <class T>
struct is_copy_assignable
    : bool_constant<__is_assignable(__add_lvalue_reference(T), __add_lvalue_reference(const T))> {};
template <class T>
inline constexpr bool is_copy_assignable_v =
    __is_assignable(__add_lvalue_reference(T), __add_lvalue_reference(const T));
template <class T>
struct is_move_assignable : bool_constant<__is_assignable(__add_lvalue_reference(T), __add_rvalue_reference(T))> {};
template <class T>
inline constexpr bool is_move_assignable_v = __is_assignable(__add_lvalue_reference(T), __add_rvalue_reference(T));

template <class T>
struct is_trivially_default_constructible : bool_constant<__is_trivially_constructible(T)> {};
template <class T>
inline constexpr bool is_trivially_default_constructible_v = __is_trivially_constructible(T);
template <class T>
struct is_trivially_copy_constructible
    : bool_constant<__is_trivially_constructible(T, __add_lvalue_reference(const T))> {};
template <class T>
inline constexpr bool is_trivially_copy_constructible_v =
    __is_trivially_constructible(T, __add_lvalue_reference(const T));
template <class T>
struct is_trivially_move_constructible : bool_constant<__is_trivially_constructible(T, __add_rvalue_reference(T))> {
};
template <class T>
inline constexpr bool is_trivially_move_constructible_v = __is_trivially_constructible(T, __add_rvalue_reference(T));
template <class T>
struct is_trivially_copy_assignable
    : bool_constant<__is_trivially_assignable(__add_lvalue_reference(T), __add_lvalue_reference(const T))> {};
template <class T>
inline constexpr bool is_trivially_copy_assignable_v =
    __is_trivially_assignable(__add_lvalue_reference(T), __add_lvalue_reference(const T));
template <class T>
struct is_trivially_move_assignable
    : bool_constant<__is_trivially_assignable(__add_lvalue_reference(T), __add_rvalue_reference(T))> {};
template <class T>
inline constexpr bool is_trivially_move_assignable_v =
    __is_trivially_assignable(__add_lvalue_reference(T), __add_rvalue_reference(T));

template <class T>
struct is_nothrow_default_constructible : bool_constant<__is_nothrow_constructible(T)> {};
template <class T>
inline constexpr bool is_nothrow_default_constructible_v = __is_nothrow_constructible(T);
template <class T>
struct is_nothrow_copy_constructible
    : bool_constant<__is_nothrow_constructible(T, __add_lvalue_reference(const T))> {};
template <class T>
inline constexpr bool is_nothrow_copy_constructible_v =
    __is_nothrow_constructible(T, __add_lvalue_reference(const T));
template <class T>
struct is_nothrow_move_constructible : bool_constant<__is_nothrow_constructible(T, __add_rvalue_reference(T))> {};
template <class T>
inline constexpr bool is_nothrow_move_constructible_v = __is_nothrow_constructible(T, __add_rvalue_reference(T));
template <class T>
struct is_nothrow_copy_assignable
    : bool_constant<__is_nothrow_assignable(__add_lvalue_reference(T), __add_lvalue_reference(const T))> {};
template <class T>
inline constexpr bool is_nothrow_copy_assignable_v =
    __is_nothrow_assignable(__add_lvalue_reference(T), __add_lvalue_reference(const T));
template <class T>
struct is_nothrow_move_assignable
    : bool_constant<__is_nothrow_assignable(__add_lvalue_reference(T), __add_rvalue_reference(T))> {};
template <class T>
inline constexpr bool is_nothrow_move_assignable_v =
    __is_nothrow_assignable(__add_lvalue_reference(T), __add_rvalue_reference(T));

template <class T>
struct has_virtual_destructor : bool_constant<__has_virtual_destructor(T)> {};
template <class T>
inline constexpr bool has_virtual_destructor_v = __has_virtual_destructor(T);

template <class T>
struct has_unique_object_representations : bool_constant<__has_unique_object_representations(T)> {};
template <class T>
inline constexpr bool has_unique_object_representations_v = __has_unique_object_representations(T);

template <class T, class U>
struct reference_constructs_from_temporary : bool_constant<__reference_constructs_from_temporary(T, U)> {};
template <class T, class U>
inline constexpr bool reference_constructs_from_temporary_v = __reference_constructs_from_temporary(T, U);
template <class T, class U>
struct reference_converts_from_temporary : bool_constant<__reference_converts_from_temporary(T, U)> {};
template <class T, class U>
inline constexpr bool reference_converts_from_temporary_v = __reference_converts_from_temporary(T, U);

// ---------------------------------------------------------------------------------------------
// [meta.unary.prop.query]
// ---------------------------------------------------------------------------------------------
template <class T>
struct alignment_of : integral_constant<size_t, alignof(T)> {};
template <class T>
inline constexpr size_t alignment_of_v = alignof(T);

template <class T>
struct rank : integral_constant<size_t, __array_rank(T)> {};
template <class T>
inline constexpr size_t rank_v = __array_rank(T);

template <class T, unsigned I = 0>
inline constexpr size_t extent_v = 0;
template <class T, size_t N>
inline constexpr size_t extent_v<T[N], 0> = N;
template <class T, unsigned I>
inline constexpr size_t extent_v<T[], I> = extent_v<T, I - 1>;
template <class T, size_t N, unsigned I>
inline constexpr size_t extent_v<T[N], I> = extent_v<T, I - 1>;
template <class T, unsigned I = 0>
struct extent : integral_constant<size_t, extent_v<T, I>> {};

// ---------------------------------------------------------------------------------------------
// [meta.rel]
// ---------------------------------------------------------------------------------------------
template <class Base, class Derived>
struct is_virtual_base_of : bool_constant<__builtin_is_virtual_base_of(Base, Derived)> {};
template <class Base, class Derived>
inline constexpr bool is_virtual_base_of_v = __builtin_is_virtual_base_of(Base, Derived);

template <class T, class U>
struct is_layout_compatible : bool_constant<__is_layout_compatible(T, U)> {};
template <class T, class U>
inline constexpr bool is_layout_compatible_v = __is_layout_compatible(T, U);

template <class Base, class Derived>
struct is_pointer_interconvertible_base_of : bool_constant<__is_pointer_interconvertible_base_of(Base, Derived)> {};
template <class Base, class Derived>
inline constexpr bool is_pointer_interconvertible_base_of_v = __is_pointer_interconvertible_base_of(Base, Derived);

#if YCXX_HAS_MEMBER_INTERCONVERTIBILITY
template <class S, class M>
constexpr bool is_pointer_interconvertible_with_class(M S::* m) noexcept {
  return __builtin_is_pointer_interconvertible_with_class(m);
}
template <class S1, class S2, class M1, class M2>
constexpr bool is_corresponding_member(M1 S1::* m1, M2 S2::* m2) noexcept {
  return __builtin_is_corresponding_member(m1, m2);
}
#endif

// ---------------------------------------------------------------------------------------------
// [meta.trans.sign]
// ---------------------------------------------------------------------------------------------
} // namespace std

namespace ycxx::detail {
template <class T>
struct sign_pair; // {signed, unsigned} for each standard integer type
template <class S, class U>
struct sign_pair_def {
  using s = S;
  using u = U;
};
template <> struct sign_pair<signed char> : sign_pair_def<signed char, unsigned char> {};
template <> struct sign_pair<unsigned char> : sign_pair_def<signed char, unsigned char> {};
template <> struct sign_pair<short> : sign_pair_def<short, unsigned short> {};
template <> struct sign_pair<unsigned short> : sign_pair_def<short, unsigned short> {};
template <> struct sign_pair<int> : sign_pair_def<int, unsigned int> {};
template <> struct sign_pair<unsigned int> : sign_pair_def<int, unsigned int> {};
template <> struct sign_pair<long> : sign_pair_def<long, unsigned long> {};
template <> struct sign_pair<unsigned long> : sign_pair_def<long, unsigned long> {};
template <> struct sign_pair<long long> : sign_pair_def<long long, unsigned long long> {};
template <> struct sign_pair<unsigned long long> : sign_pair_def<long long, unsigned long long> {};
template <> struct sign_pair<int128> : sign_pair_def<int128, uint128> {};
template <> struct sign_pair<uint128> : sign_pair_def<int128, uint128> {};

// Smallest-rank standard integer type with the same size (for character types and enums).
template <class T>
consteval auto same_size_signed() {
  if constexpr (sizeof(T) == sizeof(signed char))
    return static_cast<signed char*>(nullptr);
  else if constexpr (sizeof(T) == sizeof(short))
    return static_cast<short*>(nullptr);
  else if constexpr (sizeof(T) == sizeof(int))
    return static_cast<int*>(nullptr);
  else if constexpr (sizeof(T) == sizeof(long))
    return static_cast<long*>(nullptr);
  else if constexpr (sizeof(T) == sizeof(long long))
    return static_cast<long long*>(nullptr);
  else
    return static_cast<int128*>(nullptr);
}

template <class T>
struct sign_base {
  using type = sign_pair<__remove_pointer(decltype(same_size_signed<T>()))>;
};
template <class T>
  requires requires { typename sign_pair<T>::s; }
struct sign_base<T> {
  using type = sign_pair<T>;
};

template <class T>
concept sign_changeable = (is_integral_v<T> && !__is_same(__remove_cv(T), bool)) || __is_enum(T);
} // namespace ycxx::detail

namespace std {
template <class T>
struct make_signed {};
template <class T>
  requires ycxx::detail::sign_changeable<T>
struct make_signed<T> {
  using type = ycxx::detail::copy_cv<T, typename ycxx::detail::sign_base<__remove_cv(T)>::type::s>;
};
template <class T>
using make_signed_t = typename make_signed<T>::type;
template <class T>
struct make_unsigned {};
template <class T>
  requires ycxx::detail::sign_changeable<T>
struct make_unsigned<T> {
  using type = ycxx::detail::copy_cv<T, typename ycxx::detail::sign_base<__remove_cv(T)>::type::u>;
};
template <class T>
using make_unsigned_t = typename make_unsigned<T>::type;


// ---------------------------------------------------------------------------------------------
// [meta.trans.arr], [meta.trans.ptr]
// ---------------------------------------------------------------------------------------------
template <class T>
struct remove_extent {
  using type = __remove_extent(T);
};
template <class T>
using remove_extent_t = __remove_extent(T);
template <class T>
struct remove_all_extents {
  using type = __remove_all_extents(T);
};
template <class T>
using remove_all_extents_t = __remove_all_extents(T);

template <class T>
struct remove_pointer {
  using type = __remove_pointer(T);
};
template <class T>
using remove_pointer_t = __remove_pointer(T);
template <class T>
struct add_pointer {
  using type = __add_pointer(T);
};
template <class T>
using add_pointer_t = __add_pointer(T);

// ---------------------------------------------------------------------------------------------
// [meta.trans.other]
// ---------------------------------------------------------------------------------------------
template <class T>
struct underlying_type {};
template <class T>
  requires __is_enum(T)
struct underlying_type<T> {
  using type = __underlying_type(T);
};
template <class T>
using underlying_type_t = typename underlying_type<T>::type;

template <class T>
struct unwrap_reference {
  using type = T;
};
template <class T>
struct unwrap_reference<reference_wrapper<T>> {
  using type = T&;
};
template <class T>
using unwrap_reference_t = typename unwrap_reference<T>::type;
template <class T>
struct unwrap_ref_decay : unwrap_reference<__decay(T)> {};
template <class T>
using unwrap_ref_decay_t = typename unwrap_ref_decay<T>::type;

// conjunction / disjunction / negation: short-circuiting, and derive from the deciding trait.
template <class... B>
struct conjunction : true_type {};
template <class B1, class... Bn>
struct conjunction<B1, Bn...> : conditional_t<!bool(B1::value) || sizeof...(Bn) == 0, B1, conjunction<Bn...>> {};
template <class... B>
inline constexpr bool conjunction_v = conjunction<B...>::value;

template <class... B>
struct disjunction : false_type {};
template <class B1, class... Bn>
struct disjunction<B1, Bn...> : conditional_t<bool(B1::value) || sizeof...(Bn) == 0, B1, disjunction<Bn...>> {};
template <class... B>
inline constexpr bool disjunction_v = disjunction<B...>::value;

template <class B>
struct negation : bool_constant<!bool(B::value)> {};
template <class B>
inline constexpr bool negation_v = !bool(B::value);

} // namespace std

// ---------------------------------------------------------------------------------------------
// common_type
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail {

template <class X, class Y>
using cond_res = decltype(false ? std::declval<X (&)()>()() : std::declval<Y (&)()>()());

template <class T1, class T2>
struct common_type_cref {};
template <class T1, class T2>
  requires requires { typename cond_res<const T1&, const T2&>; }
struct common_type_cref<T1, T2> {
  using type = __decay(cond_res<const T1&, const T2&>);
};

template <class D1, class D2>
using cond_decay = __decay(decltype(false ? std::declval<D1>() : std::declval<D2>()));

template <class D1, class D2>
struct common_type_decayed : common_type_cref<D1, D2> {};
template <class D1, class D2>
  requires requires { typename cond_decay<D1, D2>; }
struct common_type_decayed<D1, D2> {
  using type = cond_decay<D1, D2>;
};

} // namespace ycxx::detail

namespace std {

template <class... T>
struct common_type {};
template <class T>
struct common_type<T> : common_type<T, T> {};

template <class T1, class T2>
struct common_type<T1, T2>
    : conditional_t<__is_same(T1, __decay(T1)) && __is_same(T2, __decay(T2)),
                    ycxx::detail::common_type_decayed<T1, T2>, common_type<__decay(T1), __decay(T2)>> {};

template <class T1, class T2, class T3, class... R>
  requires requires { typename common_type<T1, T2>::type; }
struct common_type<T1, T2, T3, R...> : common_type<typename common_type<T1, T2>::type, T3, R...> {};

template <class... T>
using common_type_t = typename common_type<T...>::type;

// ---------------------------------------------------------------------------------------------
// common_reference
// ---------------------------------------------------------------------------------------------
template <class T, class U, template <class> class TQual, template <class> class UQual>
struct basic_common_reference {};

} // namespace std

namespace ycxx::detail {

template <class A>
struct xref {
  template <class U>
  using apply = copy_cvref<A, U>;
};

template <class A, class B>
struct common_ref {};

template <class A, class B>
using common_ref_t = typename common_ref<A, B>::type;

// A = X&, B = Y&
template <class X, class Y>
  requires requires { typename cond_res<copy_cv<X, Y>&, copy_cv<Y, X>&>; } &&
           __is_reference(cond_res<copy_cv<X, Y>&, copy_cv<Y, X>&>)
struct common_ref<X&, Y&> {
  using type = cond_res<copy_cv<X, Y>&, copy_cv<Y, X>&>;
};

// A = X&&, B = Y&&
template <class X, class Y>
  requires requires { typename common_ref_t<X&, Y&>; } &&
           __is_convertible(X &&, ::ycxx::detail::remove_ref_t<common_ref_t<X&, Y&>> &&) &&
           __is_convertible(Y &&, ::ycxx::detail::remove_ref_t<common_ref_t<X&, Y&>> &&)
struct common_ref<X&&, Y&&> {
  using type = ::ycxx::detail::remove_ref_t<common_ref_t<X&, Y&>> &&;
};

// A = X&&, B = Y&
template <class X, class Y>
  requires requires { typename common_ref_t<const X&, Y&>; } &&
           __is_convertible(X &&, common_ref_t<const X&, Y&>)
struct common_ref<X&&, Y&> {
  using type = common_ref_t<const X&, Y&>;
};
// A = X&, B = Y&&
template <class X, class Y>
  requires requires { typename common_ref<Y&&, X&>::type; }
struct common_ref<X&, Y&&> : common_ref<Y&&, X&> {};

template <class T1, class T2>
using basic_common_ref_t =
    typename std::basic_common_reference<__remove_cvref(T1), __remove_cvref(T2), xref<T1>::template apply,
                                         xref<T2>::template apply>::type;

template <class T1, class T2>
struct common_reference2_d : std::common_type<T1, T2> {}; // bullet 4 (and 5)

template <class T1, class T2>
struct common_reference2_c : common_reference2_d<T1, T2> {};
template <class T1, class T2>
  requires requires { typename cond_res<T1, T2>; }
struct common_reference2_c<T1, T2> {
  using type = cond_res<T1, T2>;
};

template <class T1, class T2>
struct common_reference2_b : common_reference2_c<T1, T2> {};
template <class T1, class T2>
  requires requires { typename basic_common_ref_t<T1, T2>; }
struct common_reference2_b<T1, T2> {
  using type = basic_common_ref_t<T1, T2>;
};

template <class T1, class T2>
struct common_reference2 : common_reference2_b<T1, T2> {};
template <class T1, class T2>
  requires __is_reference(T1) && __is_reference(T2) && requires { typename common_ref_t<T1, T2>; } &&
           __is_convertible(__add_pointer(T1), __add_pointer(common_ref_t<T1, T2>)) &&
           __is_convertible(__add_pointer(T2), __add_pointer(common_ref_t<T1, T2>))
struct common_reference2<T1, T2> {
  using type = common_ref_t<T1, T2>;
};

} // namespace ycxx::detail

namespace std {

template <class... T>
struct common_reference {};
template <class T>
struct common_reference<T> {
  using type = T;
};
template <class T1, class T2>
struct common_reference<T1, T2> : ycxx::detail::common_reference2<T1, T2> {};
template <class T1, class T2, class T3, class... R>
  requires requires { typename common_reference<T1, T2>::type; }
struct common_reference<T1, T2, T3, R...> : common_reference<typename common_reference<T1, T2>::type, T3, R...> {};

template <class... T>
using common_reference_t = typename common_reference<T...>::type;

// ---------------------------------------------------------------------------------------------
// [meta.const.eval]
// ---------------------------------------------------------------------------------------------
#if YCXX_HAS_IS_WITHIN_LIFETIME
template <class T>
consteval bool is_within_lifetime(const T* p) noexcept {
  return __builtin_is_within_lifetime(p);
}
#endif

} // namespace std

