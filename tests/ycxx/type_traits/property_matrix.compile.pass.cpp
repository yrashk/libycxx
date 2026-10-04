// [meta.unary.prop] Table 54 over a matrix of types: is_const, is_volatile, is_trivially_copyable,
// is_standard_layout, is_empty, is_polymorphic, is_abstract, is_final, is_aggregate, is_signed,
// is_unsigned, is_bounded_array, is_unbounded_array, is_scoped_enum, is_destructible and
// is_trivially/nothrow_destructible, has_virtual_destructor, has_unique_object_representations.
//   is_const: "T is const-qualified ([basic.type.qualifier])" ([basic.type.qualifier]/3: an array
//     of const T is const-qualified; references and function types are never cv-qualified).
//   is_signed: "If is_arithmetic_v<T> is true, the same result as T(-1) < T(0); otherwise, false";
//   is_unsigned: "T(0) < T(-1)".
//   is_empty: "T is a class type, but not a union type, with no non-static data members other than
//     subobjects of zero size, no virtual member functions, no virtual base classes, and no base
//     class B for which is_empty_v<B> is false." ([intro.object]/9: an empty [[no_unique_address]]
//     member may be of zero size; GCC and Clang give it zero size on the Itanium ABI.)
//   is_final: "T is a class type marked with the class-property-specifier final" (unions too).
//   is_destructible: "Either T is a reference type, or T is a complete object type for which the
//     expression declval<U&>().~U() is well-formed ... where U is remove_all_extents_t<T>."
//   has_unique_object_representations: arrays use remove_all_extents_t<T>; [meta.unary.prop]/10.
#include <type_traits>
#include <cstddef>
#include <stdfloat>

struct Empty {};
struct EmptyDerived : Empty {};
struct NUA { [[no_unique_address]] Empty e; };
struct WithInt { int i; };
struct WithBitfield0 { int : 0; };
struct Poly { virtual void f(); };
struct PolyDerived : Poly {};
struct Abstract { virtual void f() = 0; };
struct AbstractDerived : Abstract {};
struct ConcreteDerived : Abstract { void f() override; };
struct VBase : virtual Empty {};
struct Final final {};
union FinalUnion final { int i; };
union U { int i; };
struct VDtor { virtual ~VDtor(); };
struct VDtorDerived : VDtor {};
struct PrivDtor { private: ~PrivDtor(); };
struct DelDtor { ~DelDtor() = delete; };
struct ThrowDtor { ~ThrowDtor() noexcept(false); };
struct UserDtor { ~UserDtor(); };
struct UserCopy { UserCopy(const UserCopy&); };
struct DelCopy { DelCopy(const DelCopy&) = delete; };   // trivially copyable: eligible copy/move ops are trivial
struct Padded { char c; int i; };
struct NoPad { int a, b; };
struct Mixed { int a; private: int b; };
struct RefMember { int& r; };
enum E { e };
enum class SE { s };
struct Incomplete;
enum class IncompleteScoped : int;   // opaque-enum-declaration: complete type
enum IncompleteUnscoped : int;
using Lambda = decltype([] {});
using CaptureLambda = decltype([x = 1] { return x; });

template <template <class> class T, class X> constexpr bool v = [] {
  static_assert(T<X>::value == T<X>{}, "integral_constant conversion");
  static_assert(std::is_base_of_v<std::bool_constant<T<X>::value>, T<X>>);
  return T<X>::value;
}();

// is_const / is_volatile
static_assert(v<std::is_const, const int> && std::is_const_v<const int>);
static_assert(!v<std::is_const, int> && !std::is_const_v<const int*>);
static_assert(std::is_const_v<int* const> && std::is_const_v<const volatile int>);
static_assert(v<std::is_const, const int[3]> && std::is_const_v<const int[]>);
static_assert(std::is_const_v<const int[2][3]>);
static_assert(!std::is_const_v<const int&> && !std::is_const_v<const int&&>);
static_assert(!std::is_const_v<void() const>);
static_assert(std::is_const_v<const void> && !std::is_const_v<void>);
static_assert(std::is_const_v<const Incomplete>);
static_assert(v<std::is_volatile, volatile int> && std::is_volatile_v<volatile int[3]>);
static_assert(!std::is_volatile_v<volatile int*> && std::is_volatile_v<int* volatile>);
static_assert(!std::is_volatile_v<volatile int&> && !std::is_volatile_v<void() volatile>);

// is_signed / is_unsigned
template <class T> constexpr bool sgn = std::is_signed_v<T> && v<std::is_signed, T> && !std::is_unsigned_v<T>;
template <class T> constexpr bool uns = std::is_unsigned_v<T> && v<std::is_unsigned, T> && !std::is_signed_v<T>;
template <class T> constexpr bool neither = !std::is_signed_v<T> && !std::is_unsigned_v<T>;
static_assert(sgn<signed char> && sgn<short> && sgn<int> && sgn<long> && sgn<long long>);
static_assert(sgn<const int> && sgn<volatile long> && sgn<const volatile short>);
static_assert(uns<unsigned char> && uns<unsigned short> && uns<unsigned> && uns<unsigned long> && uns<unsigned long long>);
static_assert(uns<bool> && uns<const bool>);
static_assert(uns<char8_t> && uns<char16_t> && uns<char32_t>);
static_assert(std::is_signed_v<char> == (char(-1) < char(0)));
static_assert(std::is_unsigned_v<char> == (char(0) < char(-1)));
static_assert(std::is_signed_v<wchar_t> == (wchar_t(-1) < wchar_t(0)));
static_assert(sgn<float> && sgn<double> && sgn<long double> && sgn<const float>);
#if defined(__STDCPP_FLOAT16_T__)
static_assert(sgn<std::float16_t>);
#endif
#if defined(__STDCPP_BFLOAT16_T__)
static_assert(sgn<std::bfloat16_t>);
#endif
#if defined(__STDCPP_FLOAT128_T__)
static_assert(sgn<std::float128_t>);
#endif
static_assert(neither<E> && neither<SE> && neither<int*> && neither<void> && neither<int&> &&
              neither<int[3]> && neither<std::nullptr_t> && neither<Empty> && neither<void()> &&
              neither<int Empty::*> && neither<Incomplete>);

// arrays, scoped enums
static_assert(v<std::is_bounded_array, int[3]> && std::is_bounded_array_v<const int[1][2]>);
static_assert(!std::is_bounded_array_v<int[]> && !std::is_bounded_array_v<int[][3]> && !std::is_bounded_array_v<int(&)[3]>);
static_assert(v<std::is_unbounded_array, int[]> && std::is_unbounded_array_v<const Incomplete[]>);
static_assert(std::is_unbounded_array_v<int[][3]> && !std::is_unbounded_array_v<int[3][2]>);
static_assert(!std::is_unbounded_array_v<int(&)[]> && !std::is_unbounded_array_v<int*>);
static_assert(v<std::is_scoped_enum, SE> && std::is_scoped_enum_v<const SE> && std::is_scoped_enum_v<IncompleteScoped>);
static_assert(!std::is_scoped_enum_v<E> && !std::is_scoped_enum_v<IncompleteUnscoped> && !std::is_scoped_enum_v<int>);
static_assert(!std::is_scoped_enum_v<SE&> && !std::is_scoped_enum_v<SE[2]> && !std::is_scoped_enum_v<Incomplete>);
static_assert(!std::is_scoped_enum_v<void> && !std::is_scoped_enum_v<void() const>);

// class properties
static_assert(v<std::is_empty, Empty> && std::is_empty_v<EmptyDerived> && std::is_empty_v<const Empty>);
static_assert(std::is_empty_v<NUA>);
static_assert(std::is_empty_v<Lambda> && !std::is_empty_v<CaptureLambda>);
static_assert(std::is_empty_v<WithBitfield0>);   // an unnamed bit-field is not a member ([class.bit]/2)
static_assert(!std::is_empty_v<WithInt> && !std::is_empty_v<Poly> && !std::is_empty_v<VBase>);
static_assert(!std::is_empty_v<U> && !std::is_empty_v<int> && !std::is_empty_v<Empty[1]> && !std::is_empty_v<Empty&>);
static_assert(std::is_empty_v<Final> && std::is_empty_v<std::true_type>);
static_assert(v<std::is_polymorphic, Poly> && std::is_polymorphic_v<PolyDerived> && std::is_polymorphic_v<Abstract>);
static_assert(std::is_polymorphic_v<VDtor> && std::is_polymorphic_v<const VDtorDerived>);
static_assert(!std::is_polymorphic_v<VBase>);   // a virtual base alone does not make a class polymorphic
static_assert(!std::is_polymorphic_v<Empty> && !std::is_polymorphic_v<U> && !std::is_polymorphic_v<Poly*> && !std::is_polymorphic_v<Poly&>);
static_assert(v<std::is_abstract, Abstract> && std::is_abstract_v<AbstractDerived> && !std::is_abstract_v<ConcreteDerived>);
static_assert(!std::is_abstract_v<Poly> && !std::is_abstract_v<Abstract*> && !std::is_abstract_v<int> && !std::is_abstract_v<U>);
static_assert(v<std::is_final, Final> && std::is_final_v<FinalUnion> && std::is_final_v<const Final>);
static_assert(!std::is_final_v<Empty> && !std::is_final_v<U> && !std::is_final_v<Final[1]> && !std::is_final_v<Final&>);
static_assert(!std::is_final_v<int> && !std::is_final_v<Final*>);
static_assert(v<std::has_virtual_destructor, VDtor> && std::has_virtual_destructor_v<VDtorDerived>);
static_assert(!std::has_virtual_destructor_v<Poly> && !std::has_virtual_destructor_v<VDtor&> && !std::has_virtual_destructor_v<int>);
static_assert(!std::has_virtual_destructor_v<VDtor[2]>);

// is_aggregate ([dcl.init.aggr]/1: arrays and classes with no user-declared or inherited ctors,
// no private/protected direct non-static data members, no virtual functions, no virtual,
// private or protected base classes).
static_assert(v<std::is_aggregate, Empty> && std::is_aggregate_v<WithInt> && std::is_aggregate_v<EmptyDerived>);
static_assert(std::is_aggregate_v<int[3]> && std::is_aggregate_v<int[]> && std::is_aggregate_v<Poly[2]>);
static_assert(std::is_aggregate_v<const WithInt> && std::is_aggregate_v<U> && std::is_aggregate_v<RefMember>);
static_assert(std::is_aggregate_v<Incomplete[]>);
static_assert(std::is_aggregate_v<Lambda> == false);   // closure types are not aggregates ([expr.prim.lambda.closure]/1)
static_assert(!std::is_aggregate_v<Poly> && !std::is_aggregate_v<Mixed> && !std::is_aggregate_v<VBase>);
static_assert(!std::is_aggregate_v<UserCopy> && !std::is_aggregate_v<DelCopy>);
static_assert(!std::is_aggregate_v<int> && !std::is_aggregate_v<void> && !std::is_aggregate_v<WithInt&> && !std::is_aggregate_v<E>);

// is_trivially_copyable, is_standard_layout
static_assert(v<std::is_trivially_copyable, int> && std::is_trivially_copyable_v<const int>);
static_assert(std::is_trivially_copyable_v<volatile int>);   // [basic.types.general]/9: cv-qualified scalar types
static_assert(std::is_trivially_copyable_v<int[3]> && std::is_trivially_copyable_v<int[]> && std::is_trivially_copyable_v<WithInt[2][2]>);
static_assert(std::is_trivially_copyable_v<Empty> && std::is_trivially_copyable_v<DelCopy> && std::is_trivially_copyable_v<E>);
static_assert(std::is_trivially_copyable_v<int*> && std::is_trivially_copyable_v<std::nullptr_t> && std::is_trivially_copyable_v<int Empty::*>);
static_assert(!std::is_trivially_copyable_v<UserCopy> && !std::is_trivially_copyable_v<UserDtor> && !std::is_trivially_copyable_v<Poly>);
static_assert(!std::is_trivially_copyable_v<VBase> && !std::is_trivially_copyable_v<int&> && !std::is_trivially_copyable_v<void>);
static_assert(!std::is_trivially_copyable_v<void()>);
static_assert(v<std::is_standard_layout, WithInt> && std::is_standard_layout_v<int> && std::is_standard_layout_v<Padded>);
static_assert(std::is_standard_layout_v<int[]> && std::is_standard_layout_v<UserDtor>);
static_assert(!std::is_standard_layout_v<Mixed> && !std::is_standard_layout_v<Poly> && !std::is_standard_layout_v<VBase>);
static_assert(!std::is_standard_layout_v<int&> && !std::is_standard_layout_v<void>);

// destructibility
static_assert(v<std::is_destructible, int> && std::is_destructible_v<int&> && std::is_destructible_v<int&&>);
static_assert(std::is_destructible_v<int[3]> && std::is_destructible_v<UserDtor[2][2]>);
static_assert(!std::is_destructible_v<int[]> && !std::is_destructible_v<void> && !std::is_destructible_v<const void>);
static_assert(!std::is_destructible_v<void()> && !std::is_destructible_v<void() const>);
static_assert(!std::is_destructible_v<PrivDtor> && !std::is_destructible_v<DelDtor> && !std::is_destructible_v<DelDtor[2]>);
static_assert(std::is_destructible_v<PrivDtor&> && std::is_destructible_v<DelDtor*>);
static_assert(std::is_destructible_v<ThrowDtor> && !std::is_nothrow_destructible_v<ThrowDtor>);
static_assert(!std::is_nothrow_destructible_v<ThrowDtor[3]> && std::is_nothrow_destructible_v<ThrowDtor&>);
static_assert(std::is_nothrow_destructible_v<UserDtor> && !std::is_trivially_destructible_v<UserDtor>);
static_assert(std::is_trivially_destructible_v<int> && std::is_trivially_destructible_v<int&> && std::is_trivially_destructible_v<Empty[4]>);
static_assert(!std::is_trivially_destructible_v<int[]> && !std::is_trivially_destructible_v<void>);
static_assert(!std::is_trivially_destructible_v<VDtor> && !std::is_trivially_destructible_v<DelDtor>);
static_assert(!std::is_nothrow_destructible_v<int[]> && !std::is_nothrow_destructible_v<PrivDtor>);
static_assert(std::is_destructible_v<const volatile UserDtor> && std::is_destructible_v<Lambda>);

// has_unique_object_representations
static_assert(v<std::has_unique_object_representations, int> && std::has_unique_object_representations_v<unsigned>);
static_assert(std::has_unique_object_representations_v<const int> && std::has_unique_object_representations_v<NoPad>);
static_assert(std::has_unique_object_representations_v<int[3]> && std::has_unique_object_representations_v<NoPad[]>);
static_assert(std::has_unique_object_representations_v<int[][2]>);
static_assert(std::has_unique_object_representations_v<E> && std::has_unique_object_representations_v<int*>);
static_assert(!std::has_unique_object_representations_v<Padded> && !std::has_unique_object_representations_v<Padded[2]>);
static_assert(!std::has_unique_object_representations_v<float> && !std::has_unique_object_representations_v<double>);
static_assert(!std::has_unique_object_representations_v<UserCopy>);   // not trivially copyable
static_assert(!std::has_unique_object_representations_v<int&> && !std::has_unique_object_representations_v<void>);
static_assert(!std::has_unique_object_representations_v<Poly>);
static_assert(!std::has_unique_object_representations_v<void()>);

int main() {}
