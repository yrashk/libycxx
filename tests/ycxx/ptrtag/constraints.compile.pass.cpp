// [ptrtag.pair.general]: the constructor template<tagging-compatible-pointee<pointer_type,
// bits_requested> U> pointer_tag_pair(U* p, tag_type t) and from_overaligned<PromisedAlignment>
// are constrained by tagging-compatible-pointee:
//   convertible_to<U*, PtrT> && pointer_bits_available(Alignment) >= BitsRequested &&
//   (is_void_v<element-of<PtrT>> || is_scalar_v<element-of<PtrT>> ||
//    is_union_v<element-of<PtrT>> || is_pointer_interconvertible_base_of_v<element-of<PtrT>, U>)
// with Alignment = alignof(U) for the constructor and PromisedAlignment for from_overaligned.
// The comparisons require three_way_comparable / equality_comparable of the tag ([ptrtag.pair.comp]).
#include <cstddef>
#include <memory>
#include <type_traits>

struct alignas(8) A {
  long a;
};
struct alignas(8) B {
  long b;
};
struct First : A {}; // standard-layout, A at offset 0: pointer-interconvertible
struct NotStandardLayout : A { // members in two classes: not standard-layout
  int x;
};
struct Second : B, A { // A is not at offset 0: not pointer-interconvertible
  int y;
};
struct Virt : virtual A {};
struct Incomplete;
struct alignas(2) Two {
  char c;
};

using PA = std::pointer_tag_pair<A*>; // 3 bits
static_assert(PA::bits_requested == 3);
static_assert(std::is_constructible_v<PA, A*, unsigned>);
static_assert(std::is_constructible_v<PA, First*, unsigned>);
static_assert(!std::is_constructible_v<PA, Second*, unsigned>);  // not pointer-interconvertible
static_assert(!std::is_constructible_v<PA, NotStandardLayout*, unsigned>);
static_assert(!std::is_constructible_v<PA, Virt*, unsigned>);    // virtual base
static_assert(!std::is_constructible_v<PA, const A*, unsigned>); // not convertible
static_assert(!std::is_constructible_v<PA, B*, unsigned>);
static_assert(std::is_constructible_v<PA, std::nullptr_t, unsigned>);
static_assert(!std::is_constructible_v<PA, A*>); // no one-argument constructor

// Alignment: Two gives 1 bit only.
static_assert(std::is_constructible_v<std::pointer_tag_pair<Two*, 1>, Two*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<Two*, 2>, Two*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<char*, 1>, char*, unsigned>);
static_assert(std::is_constructible_v<std::pointer_tag_pair<char*, 0>, char*, unsigned>);
static_assert(std::is_constructible_v<std::pointer_tag_pair<char*, 1>, std::nullptr_t, unsigned>);

// void* and scalars: any convertible U with the alignment; void itself has none.
using PV = std::pointer_tag_pair<void*, 2>;
static_assert(std::is_constructible_v<PV, int*, unsigned>);
static_assert(std::is_constructible_v<PV, Second*, unsigned>); // element is void
static_assert(!std::is_constructible_v<PV, char*, unsigned>);  // alignment 1
static_assert(!std::is_constructible_v<PV, void*, unsigned>);  // no alignof(void)
static_assert(!std::is_constructible_v<PV, const int*, unsigned>);
static_assert(std::is_constructible_v<std::pointer_tag_pair<const void*, 2>, const int*, unsigned>);
static_assert(std::is_constructible_v<std::pointer_tag_pair<const int*, 2>, int*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<long*, 2>, int*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<Incomplete*, 0>, Incomplete*, unsigned>);
// Arrays as pointees are neither scalar, union nor class.
static_assert(!std::is_constructible_v<std::pointer_tag_pair<int (*)[2], 2>, int (*)[2], unsigned>);

// from_overaligned: the promised alignment replaces alignof(U).
template <class P, std::size_t Al, class U>
concept overaligned = requires(U* u) { P::template from_overaligned<Al>(u, 0u); };
static_assert(overaligned<std::pointer_tag_pair<char*, 4>, 16, char>);
static_assert(!overaligned<std::pointer_tag_pair<char*, 4>, 8, char>);
static_assert(overaligned<std::pointer_tag_pair<void*, 4>, 16, void>); // void* through a promise
static_assert(overaligned<std::pointer_tag_pair<A*, 6>, 64, First>);
static_assert(!overaligned<std::pointer_tag_pair<A*, 6>, 64, Second>);

// Tag type conversion: the tag parameter is tag_type.
enum class Tag : unsigned char { a, b };
static_assert(std::is_constructible_v<std::pointer_tag_pair<A*, 1, Tag>, A*, Tag>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<A*, 1, Tag>, A*, unsigned>);
