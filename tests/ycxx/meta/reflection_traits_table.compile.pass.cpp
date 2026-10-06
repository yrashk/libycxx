// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.reflection.traits]/4 Table 64: each X_type(info...) returns std::X<T...>::value for the
// types represented by its arguments, and each transformation returns a reflection of
// std::X<T...>::type, never an alias (Note 3; Note 4: is_same_type(int, alias) is true).
// Checked against the class templates for many traits and types, including user
// specializations of common_type, tuple_size/tuple_element (/9-10), and variant_size and
// variant_alternative (/11-12); rank and extent (/7-8) of dealias(type); type_order (/13).
// /2: a reflection that is not a type throws meta::exception; /3.1.2: so does a transformation
// whose class template has no member type (common_type<int, Agg>). (/3.1.1 and /3.1.3 are in
// reflection_traits_errors.compile.pass.cpp.)
// REQUIRES: exceptions
#include <meta>
#include <compare>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace m = std::meta;

struct Empty {};
struct Poly {
  virtual ~Poly() = default;
};
struct Abstract {
  virtual void f() = 0;
};
struct Final final {};
struct Agg {
  int a;
  double b;
};
union U {
  int i;
  float f;
};
enum class Scoped : short { a };
enum Plain { p };
struct Derived : Poly {};
struct NoCopy {
  NoCopy(const NoCopy&) = delete;
};
using IntAlias = int;

template <class T>
consteval bool unary_agree() {
  constexpr m::info t = ^^T;
  return m::is_void_type(t) == std::is_void_v<T> && m::is_null_pointer_type(t) == std::is_null_pointer_v<T> &&
         m::is_integral_type(t) == std::is_integral_v<T> && m::is_floating_point_type(t) == std::is_floating_point_v<T> &&
         m::is_array_type(t) == std::is_array_v<T> && m::is_pointer_type(t) == std::is_pointer_v<T> &&
         m::is_lvalue_reference_type(t) == std::is_lvalue_reference_v<T> &&
         m::is_rvalue_reference_type(t) == std::is_rvalue_reference_v<T> &&
         m::is_member_object_pointer_type(t) == std::is_member_object_pointer_v<T> &&
         m::is_member_function_pointer_type(t) == std::is_member_function_pointer_v<T> &&
         m::is_enum_type(t) == std::is_enum_v<T> && m::is_union_type(t) == std::is_union_v<T> &&
         m::is_class_type(t) == std::is_class_v<T> && m::is_function_type(t) == std::is_function_v<T> &&
         m::is_reflection_type(t) == std::is_reflection_v<T> && m::is_reference_type(t) == std::is_reference_v<T> &&
         m::is_arithmetic_type(t) == std::is_arithmetic_v<T> && m::is_fundamental_type(t) == std::is_fundamental_v<T> &&
         m::is_object_type(t) == std::is_object_v<T> && m::is_scalar_type(t) == std::is_scalar_v<T> &&
         m::is_compound_type(t) == std::is_compound_v<T> && m::is_member_pointer_type(t) == std::is_member_pointer_v<T> &&
         m::is_const_type(t) == std::is_const_v<T> && m::is_volatile_type(t) == std::is_volatile_v<T> &&
         m::is_signed_type(t) == std::is_signed_v<T> && m::is_unsigned_type(t) == std::is_unsigned_v<T> &&
         m::is_bounded_array_type(t) == std::is_bounded_array_v<T> &&
         m::is_unbounded_array_type(t) == std::is_unbounded_array_v<T> &&
         m::is_scoped_enum_type(t) == std::is_scoped_enum_v<T> && m::rank(t) == std::rank_v<T> &&
         m::extent(t) == std::extent_v<T> && m::extent(t, 1) == std::extent_v<T, 1>;
}

template <class T>
consteval bool object_agree() {
  constexpr m::info t = ^^T;
  return unary_agree<T>() && m::is_trivially_copyable_type(t) == std::is_trivially_copyable_v<T> &&
         m::is_standard_layout_type(t) == std::is_standard_layout_v<T> && m::is_empty_type(t) == std::is_empty_v<T> &&
         m::is_polymorphic_type(t) == std::is_polymorphic_v<T> && m::is_abstract_type(t) == std::is_abstract_v<T> &&
         m::is_final_type(t) == std::is_final_v<T> && m::is_aggregate_type(t) == std::is_aggregate_v<T> &&
         m::is_structural_type(t) == std::is_structural_v<T> &&
         m::is_default_constructible_type(t) == std::is_default_constructible_v<T> &&
         m::is_copy_constructible_type(t) == std::is_copy_constructible_v<T> &&
         m::is_move_constructible_type(t) == std::is_move_constructible_v<T> &&
         m::is_copy_assignable_type(t) == std::is_copy_assignable_v<T> &&
         m::is_move_assignable_type(t) == std::is_move_assignable_v<T> && m::is_swappable_type(t) == std::is_swappable_v<T> &&
         m::is_destructible_type(t) == std::is_destructible_v<T> &&
         m::is_trivially_default_constructible_type(t) == std::is_trivially_default_constructible_v<T> &&
         m::is_trivially_copy_constructible_type(t) == std::is_trivially_copy_constructible_v<T> &&
         m::is_trivially_move_constructible_type(t) == std::is_trivially_move_constructible_v<T> &&
         m::is_trivially_copy_assignable_type(t) == std::is_trivially_copy_assignable_v<T> &&
         m::is_trivially_move_assignable_type(t) == std::is_trivially_move_assignable_v<T> &&
         m::is_trivially_destructible_type(t) == std::is_trivially_destructible_v<T> &&
         m::is_nothrow_default_constructible_type(t) == std::is_nothrow_default_constructible_v<T> &&
         m::is_nothrow_copy_constructible_type(t) == std::is_nothrow_copy_constructible_v<T> &&
         m::is_nothrow_move_constructible_type(t) == std::is_nothrow_move_constructible_v<T> &&
         m::is_nothrow_copy_assignable_type(t) == std::is_nothrow_copy_assignable_v<T> &&
         m::is_nothrow_move_assignable_type(t) == std::is_nothrow_move_assignable_v<T> &&
         m::is_nothrow_swappable_type(t) == std::is_nothrow_swappable_v<T> &&
         m::is_nothrow_destructible_type(t) == std::is_nothrow_destructible_v<T> &&
         m::is_implicit_lifetime_type(t) == std::is_implicit_lifetime_v<T> &&
         m::has_virtual_destructor(t) == std::has_virtual_destructor_v<T> &&
         m::has_unique_object_representations(t) == std::has_unique_object_representations_v<T>;
}

static_assert(unary_agree<void>() && unary_agree<const volatile void>() && unary_agree<std::nullptr_t>());
static_assert(unary_agree<int&>() && unary_agree<const int&&>() && unary_agree<void()>() && unary_agree<int() const &>());
static_assert(unary_agree<m::info>() && unary_agree<int[]>());
static_assert(object_agree<int>() && object_agree<const unsigned char>() && object_agree<volatile double>());
static_assert(object_agree<int*>() && object_agree<int Agg::*>() && object_agree<void (Poly::*)()>());
static_assert(object_agree<int[3][4]>() && object_agree<Empty>() && object_agree<Poly>() && object_agree<Final>());
static_assert(object_agree<Agg>() && object_agree<U>() && object_agree<Scoped>() && object_agree<Plain>());
static_assert(object_agree<Derived>() && object_agree<NoCopy>());
// (Not object_agree<m::info>: GCC 16.2 has an internal compiler error on
// __has_unique_object_representations(decltype(^^int)).)
static_assert(unary_agree<m::info>() && m::is_trivially_copyable_type(^^m::info) && m::is_structural_type(^^m::info));
static_assert(unary_agree<Abstract>() && m::is_abstract_type(^^Abstract) && m::is_polymorphic_type(^^Abstract));

// Binary and variadic queries.
static_assert(m::is_same_type(^^int, ^^IntAlias) && (^^int) != (^^IntAlias) && m::dealias(^^IntAlias) == (^^int));
static_assert(m::is_base_of_type(^^Poly, ^^Derived) && !m::is_base_of_type(^^Derived, ^^Poly));
static_assert(!m::is_virtual_base_of_type(^^Poly, ^^Derived));
static_assert(m::is_convertible_type(^^Derived*, ^^Poly*) && !m::is_convertible_type(^^Poly*, ^^Derived*));
static_assert(m::is_nothrow_convertible_type(^^int, ^^long) == std::is_nothrow_convertible_v<int, long>);
static_assert(m::is_layout_compatible_type(^^Agg, ^^Agg) && !m::is_layout_compatible_type(^^int, ^^unsigned));
static_assert(m::is_pointer_interconvertible_base_of_type(^^Empty, ^^Empty));
static_assert(m::is_assignable_type(^^int&, ^^long) && !m::is_assignable_type(^^int, ^^int));
static_assert(m::is_trivially_assignable_type(^^int&, ^^int) && m::is_nothrow_assignable_type(^^Agg&, ^^const Agg&));
static_assert(m::is_swappable_with_type(^^int&, ^^int&) && !m::is_swappable_with_type(^^int, ^^int));
static_assert(m::is_nothrow_swappable_with_type(^^int&, ^^int&));
static_assert(m::is_constructible_type(^^Agg, {^^int, ^^double}) == std::is_constructible_v<Agg, int, double>);
static_assert(m::is_constructible_type(^^int, {}) && !m::is_constructible_type(^^NoCopy, {^^const NoCopy&}));
static_assert(m::is_trivially_constructible_type(^^int, {^^int}) && !m::is_trivially_constructible_type(^^Poly, {}));
static_assert(m::is_nothrow_constructible_type(^^long, {^^int}));
static_assert(m::reference_constructs_from_temporary(^^const int&, ^^long));
static_assert(m::reference_converts_from_temporary(^^const int&, ^^long));
static_assert(!m::reference_constructs_from_temporary(^^const int&, ^^int&));
using Fn = int (*)(long, char);
static_assert(m::is_invocable_type(^^Fn, {^^long, ^^char}) && !m::is_invocable_type(^^Fn, {^^long}));
static_assert(m::is_invocable_r_type(^^double, ^^Fn, {^^int, ^^int}) && !m::is_invocable_r_type(^^Agg, ^^Fn, {^^int, ^^int}));
static_assert(!m::is_nothrow_invocable_type(^^Fn, {^^long, ^^char}));
static_assert(m::is_nothrow_invocable_r_type(^^void, ^^void (*)() noexcept, {}));

// Transformations return types, not aliases.
static_assert(m::remove_const(^^const volatile int) == (^^volatile int) && m::remove_volatile(^^const volatile int) == (^^const int));
static_assert(m::remove_cv(^^const volatile IntAlias) == (^^int) && m::add_const(^^IntAlias) == (^^const int));
static_assert(m::add_volatile(^^int) == (^^volatile int) && m::add_cv(^^int&) == (^^int&));
static_assert(m::remove_reference(^^int&&) == (^^int) && m::add_lvalue_reference(^^int&&) == (^^int&));
static_assert(m::add_rvalue_reference(^^int&) == (^^int&) && m::add_rvalue_reference(^^void) == (^^void));
static_assert(m::make_signed(^^unsigned char) == (^^signed char) && m::make_unsigned(^^Scoped) == (^^unsigned short));
static_assert(m::remove_extent(^^int[2][3]) == (^^int[3]) && m::remove_all_extents(^^int[2][3]) == (^^int));
static_assert(m::remove_pointer(^^int* const) == (^^int) && m::add_pointer(^^int&) == (^^int*));
static_assert(m::remove_cvref(^^const IntAlias&) == (^^int) && m::decay(^^int[3]) == (^^int*) && m::decay(^^int(long)) == (^^int (*)(long)));
static_assert(m::underlying_type(^^Scoped) == (^^short));
static_assert(m::common_type({^^int, ^^long, ^^short}) == (^^long) && m::common_type({^^IntAlias}) == (^^int));
static_assert(m::common_reference({^^int&, ^^const int&}) == (^^const int&));
static_assert(m::invoke_result(^^Fn, {^^int, ^^int}) == (^^int));
static_assert(m::invoke_result(^^int Agg::*, {^^Agg&}) == (^^int&));
static_assert(m::unwrap_reference(^^std::reference_wrapper<Agg>) == (^^Agg&) && m::unwrap_reference(^^int) == (^^int));
static_assert(m::unwrap_ref_decay(^^const std::reference_wrapper<int>&) == (^^int&));

// User specializations are honoured.
struct A1 {};
struct B1 {};
struct C1 {};
template <> struct std::common_type<A1, B1> { using type = C1; };
template <> struct std::common_type<B1, A1> { using type = C1; };
static_assert(m::common_type({^^A1, ^^B1}) == (^^C1));

struct Pair2 {};
template <> struct std::tuple_size<Pair2> : std::integral_constant<std::size_t, 2> {};
template <> struct std::tuple_element<0, Pair2> { using type = long; };
template <> struct std::tuple_element<1, Pair2> { using type = Agg; };
static_assert(m::tuple_size(^^Pair2) == 2 && m::tuple_element(0, ^^Pair2) == (^^long) && m::tuple_element(1, ^^Pair2) == (^^Agg));
static_assert(m::tuple_size(^^std::tuple<int, char, long>) == 3 && m::tuple_element(1, ^^std::pair<int, char>) == (^^char));
static_assert(m::tuple_size(^^const std::array<int, 4>) == 4);
using V = std::variant<int, IntAlias, Agg>;
static_assert(m::variant_size(^^V) == 3 && m::variant_size(^^const V) == 3);
static_assert(m::variant_alternative(2, ^^V) == (^^Agg) && m::variant_alternative(1, ^^V) == (^^int));
static_assert(m::variant_alternative(0, ^^const V) == (^^const int));

// type_order agrees with std::type_order, is a total order, and sees through aliases.
static_assert(m::type_order(^^int, ^^IntAlias) == std::strong_ordering::equal);
static_assert(m::type_order(^^int, ^^long) == std::type_order_v<int, long>);
static_assert(m::type_order(^^long, ^^int) == (0 <=> m::type_order(^^int, ^^long)));
static_assert(std::is_same_v<decltype(m::type_order(^^int, ^^int)), std::strong_ordering>);

// Errors.
template <class F>
consteval bool throws(F f) {
  try {
    f();
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
constexpr int var = 0;
static_assert(throws([] { (void)m::is_void_type(^^var); }));                      // /2.1: not a type
static_assert(throws([] { (void)m::is_constructible_type(^^int, {^^var}); }));    // /2.2
static_assert(throws([] { (void)m::common_type({^^int, ^^Agg}); }));               // /3.1.2: no member type
static_assert(throws([] { (void)m::remove_cv(^^m); }));                             // a namespace
static_assert(!throws([] { (void)m::is_void_type(^^IntAlias); }));

int main() {}
