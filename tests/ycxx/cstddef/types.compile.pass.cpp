// [cstddef.syn]: ptrdiff_t, size_t, max_align_t, nullptr_t = decltype(nullptr).
// [support.types.layout]/2: ptrdiff_t is "an implementation-defined signed integer type that
// can hold the difference of two subscripts"; [expr.add]/5: the type of p - q "shall be the
// same type that is defined as std::ptrdiff_t in the <cstddef> header".
// /3: size_t is "an implementation-defined unsigned integer type"; [expr.sizeof]/6: "The
// result of sizeof and sizeof... is a prvalue of type std::size_t". [expr.alignof]/3 likewise.
// /5: "max_align_t is a trivially copyable standard-layout type whose alignment requirement is
// at least as great as that of every scalar type ... std::is_trivially_default_constructible_v
// <max_align_t> is true."
#include <cstddef>
#include <type_traits>

static_assert(std::is_same_v<std::nullptr_t, decltype(nullptr)>);
static_assert(std::is_integral_v<std::ptrdiff_t> && std::is_signed_v<std::ptrdiff_t>);
static_assert(std::is_integral_v<std::size_t> && std::is_unsigned_v<std::size_t>);
static_assert(!std::is_same_v<std::size_t, bool> && !std::is_same_v<std::ptrdiff_t, bool>);
static_assert(std::is_same_v<std::size_t, decltype(sizeof(int))>);
static_assert(std::is_same_v<std::size_t, decltype(alignof(int))>);
int arr[2];
static_assert(std::is_same_v<std::ptrdiff_t, decltype(&arr[1] - &arr[0])>);

using M = std::max_align_t;
static_assert(std::is_trivially_copyable_v<M>);
static_assert(std::is_standard_layout_v<M>);
static_assert(std::is_trivially_default_constructible_v<M>);
struct S { int m; };
enum En { x };
static_assert(alignof(M) >= alignof(long double) && alignof(M) >= alignof(double));
static_assert(alignof(M) >= alignof(long long) && alignof(M) >= alignof(void*));
static_assert(alignof(M) >= alignof(void (*)()) && alignof(M) >= alignof(int S::*));
static_assert(alignof(M) >= alignof(void (S::*)()) && alignof(M) >= alignof(std::nullptr_t));
static_assert(alignof(M) >= alignof(En) && alignof(M) >= alignof(wchar_t) && alignof(M) >= alignof(char32_t));
// "whose alignment requirement is supported in every context": a fundamental alignment
static_assert((alignof(M) & (alignof(M) - 1)) == 0);
alignas(M) static unsigned char storage[sizeof(M)];

// [support.types.byteops] / [cstddef.syn]: byte is in namespace std
static_assert(std::is_enum_v<std::byte>);
