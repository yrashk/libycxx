// [meta.rel] is_layout_compatible: "T and U are layout-compatible ([basic.types.general])".
// [basic.types.general]/11: "Two types cv1 T1 and cv2 T2 are layout-compatible types if T1
// and T2 are the same type, layout-compatible enumerations, or layout-compatible
// standard-layout class types." [dcl.enum]: enumerations are layout-compatible if they have
// the same underlying type. [class.mem]: standard-layout structs are layout-compatible if
// their common initial sequence comprises all members.
#include <type_traits>

struct A { int x; char y; };
struct B { int a; char b; };
struct C { char y; int x; };
struct D { int x; char y; char z; };
struct BitA { int x : 3; };
struct BitB { int x : 3; };
struct BitC { int x : 4; };
struct NonStd1 { int x; private: int y; };
struct NonStd2 { int x; private: int y; };
enum E1 : int { e1 };
enum class E2 : int { e2 };
enum class E3 : long { e3 };
union U1 { int i; float f; };
union U2 { float f; int i; };
union U3 { int i; double d; };

static_assert(std::is_base_of_v<std::true_type, std::is_layout_compatible<A, B>>);
static_assert(std::is_base_of_v<std::false_type, std::is_layout_compatible<A, C>>);

// Same type (cv ignored).
static_assert(std::is_layout_compatible_v<int, int>);
static_assert(std::is_layout_compatible_v<const int, int>);
static_assert(std::is_layout_compatible_v<volatile A, const A>);
static_assert(std::is_layout_compatible_v<int[3], int[3]>);
static_assert(std::is_layout_compatible_v<NonStd1, NonStd1>);
// Different scalar types are not layout-compatible.
static_assert(!std::is_layout_compatible_v<int, unsigned>);
static_assert(!std::is_layout_compatible_v<int, long>);
static_assert(!std::is_layout_compatible_v<char, signed char>);
static_assert(!std::is_layout_compatible_v<int[3], unsigned[3]>);
static_assert(!std::is_layout_compatible_v<int[3], int[4]>);
// Enumerations.
static_assert(std::is_layout_compatible_v<E1, E2>);
static_assert(!std::is_layout_compatible_v<E1, E3>);
static_assert(!std::is_layout_compatible_v<E1, int>);
// Standard-layout structs.
static_assert(std::is_layout_compatible_v<A, B>);
static_assert(std::is_layout_compatible_v<const A, B>);
static_assert(!std::is_layout_compatible_v<A, C>);
static_assert(!std::is_layout_compatible_v<A, D>);
static_assert(std::is_layout_compatible_v<BitA, BitB>);
static_assert(!std::is_layout_compatible_v<BitA, BitC>);
static_assert(!std::is_layout_compatible_v<NonStd1, NonStd2>);   // not standard-layout
// Unions: members in any order ([class.mem]).
static_assert(std::is_layout_compatible_v<U1, U2>);
static_assert(!std::is_layout_compatible_v<U1, U3>);
// Struct vs. union, and references.
static_assert(!std::is_layout_compatible_v<A, U1>);
static_assert(!std::is_layout_compatible_v<int&, int>);
