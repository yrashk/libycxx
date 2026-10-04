// [meta.unary.prop] is_scoped_enum: "T is a scoped enumeration ([dcl.enum])";
// [meta.unary.prop] is_bounded_array: "T is an array type of known bound";
// is_unbounded_array: "T is an array type of unknown bound".
#include <type_traits>

enum Unscoped { u };
enum UnscopedFixed : unsigned char { uf };
enum class Scoped { s };
enum struct ScopedStruct : long { ss };
enum class Opaque : int;                 // opaque-enum-declaration: complete type
struct Cls { enum class Nested { n }; };
struct Incomplete;

static_assert(std::is_base_of_v<std::true_type, std::is_scoped_enum<Scoped>>);
static_assert(std::is_base_of_v<std::false_type, std::is_scoped_enum<Unscoped>>);
static_assert(std::is_scoped_enum_v<Scoped>);
static_assert(std::is_scoped_enum_v<ScopedStruct>);
static_assert(std::is_scoped_enum_v<Opaque>);
static_assert(std::is_scoped_enum_v<Cls::Nested>);
static_assert(!std::is_scoped_enum_v<Unscoped>);
static_assert(!std::is_scoped_enum_v<UnscopedFixed>);
static_assert(!std::is_scoped_enum_v<int>);
static_assert(!std::is_scoped_enum_v<Cls>);
static_assert(!std::is_scoped_enum_v<Scoped&>);
static_assert(!std::is_scoped_enum_v<Scoped*>);
static_assert(!std::is_scoped_enum_v<Scoped[2]>);
static_assert(!std::is_scoped_enum_v<void>);
static_assert(!std::is_scoped_enum_v<Incomplete>);
static_assert(!std::is_scoped_enum_v<int()>);

static_assert(std::is_base_of_v<std::true_type, std::is_bounded_array<int[1]>>);
static_assert(std::is_base_of_v<std::false_type, std::is_bounded_array<int[]>>);
static_assert(std::is_bounded_array_v<int[3]>);
static_assert(std::is_bounded_array_v<const int[3]>);
static_assert(std::is_bounded_array_v<int[2][3]>);
static_assert(!std::is_bounded_array_v<int[]>);
static_assert(!std::is_bounded_array_v<int[][3]>);
static_assert(!std::is_bounded_array_v<int>);
static_assert(!std::is_bounded_array_v<int*>);
static_assert(!std::is_bounded_array_v<int (&)[3]>);
static_assert(!std::is_bounded_array_v<int (*)[3]>);
static_assert(!std::is_bounded_array_v<void>);

static_assert(std::is_unbounded_array_v<int[]>);
static_assert(std::is_unbounded_array_v<const volatile int[]>);
static_assert(std::is_unbounded_array_v<int[][3]>);
static_assert(std::is_unbounded_array_v<Incomplete[]>);
static_assert(!std::is_unbounded_array_v<int[3]>);
static_assert(!std::is_unbounded_array_v<int[2][3]>);
static_assert(!std::is_unbounded_array_v<int>);
static_assert(!std::is_unbounded_array_v<int (&)[]>);
static_assert(!std::is_unbounded_array_v<int (*)[]>);
static_assert(!std::is_unbounded_array_v<void>);
