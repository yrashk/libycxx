// XFAIL-COMPILER: clang  Clang 23 provides no way to detect structural types (std::is_structural absent)
// [meta.unary.prop] Table 54: is_structural<T>: "T is a structural type ([temp.param])";
// Preconditions: remove_all_extents_t<T> shall be a complete type or cv void.
// [temp.param]/12: "A structural type is one of the following: a scalar type, or an lvalue
// reference type, or a literal class type with the following properties: all base classes and
// non-static data members are public and non-mutable and the types of all base classes and
// non-static data members are structural types or (possibly multidimensional) arrays thereof."
// [basic.types.general]/9: scalar types include their cv-qualified versions.
// An array type itself is none of these. [version.syn]: __cpp_lib_is_structural.
#include <type_traits>
#include <version>
#include <cstddef>

#if !defined(__cpp_lib_is_structural) || __cpp_lib_is_structural < 202603L
#error "__cpp_lib_is_structural"
#endif

struct Agg { int i; double d; int* p; };
struct WithArray { int a[2][3]; };
struct WithRef { int& r; };
struct WithRRef { int&& r; };
struct Base { int b; };
struct PubDerived : Base { int x; };
struct ProtDerived : protected Base {};
struct PrivMember { private: int x; };
struct ProtMember { protected: int x; };
struct MutableMember { mutable int x; };
struct NonLiteral { ~NonLiteral(); int x; };
struct Nested { Agg a; WithArray w[2]; };
struct NestedBad { PrivMember p; };
struct ArrayOfBad { PrivMember p[2]; };
struct StaticPrivate { int x; private: static int s; };   // static members are not considered
struct Ctor { constexpr Ctor(int) {} int x; };
union Un { int i; float f; };
union PrivUn { private: int i; };
enum E { e };
enum class SE : char { s };
using Lambda = decltype([] {});

template <class T> constexpr bool st = std::is_structural_v<T> && std::is_structural<T>::value &&
    std::is_base_of_v<std::true_type, std::is_structural<T>>;
template <class T> constexpr bool nst = !std::is_structural_v<T> && !std::is_structural<T>::value &&
    std::is_base_of_v<std::false_type, std::is_structural<T>>;

static_assert(st<int> && st<const int> && st<volatile long> && st<bool> && st<char8_t>);
static_assert(st<float> && st<double> && st<long double>);
static_assert(st<int*> && st<const void*> && st<void (*)()> && st<std::nullptr_t>);
static_assert(st<int Agg::*> && st<void (Agg::*)() const>);
static_assert(st<E> && st<SE> && st<const SE>);
static_assert(st<int&> && st<const Agg&> && st<void (&)()> && st<int (&)[3]>);
static_assert(nst<int&&> && nst<Agg&&>);
static_assert(nst<void> && nst<const void>);
static_assert(nst<void()> && nst<void() const>);
static_assert(nst<int[3]> && nst<int[]> && nst<Agg[2]>);
static_assert(st<Agg> && st<const Agg> && st<WithArray> && st<WithRef> && st<PubDerived> && st<Nested>);
static_assert(st<StaticPrivate> && st<Ctor> && st<Un> && st<Lambda>);
static_assert(nst<WithRRef> && nst<ProtDerived> && nst<PrivMember> && nst<ProtMember> && nst<MutableMember>);
static_assert(nst<NonLiteral> && nst<NestedBad> && nst<ArrayOfBad> && nst<PrivUn>);

int main() {}
