// [default.allocator.general]/1: "All specializations of the default allocator meet the
// allocator completeness requirements ([allocator.requirements.completeness])" -- allocator<T>
// and its members (other than allocate etc.) can be named while T is incomplete.
// [allocator.requirements.completeness]: "allocator_traits<A>::value_type" and the member
// types are usable, and A is a complete type.
#include <memory>
#include <cstddef>
#include <type_traits>

struct Incomplete;
using A = std::allocator<Incomplete>;
static_assert(sizeof(A) > 0);  // A is complete
static_assert(std::is_same_v<A::value_type, Incomplete>);
static_assert(std::is_same_v<std::allocator_traits<A>::value_type, Incomplete>);
static_assert(std::is_same_v<std::allocator_traits<A>::pointer, Incomplete*>);
static_assert(std::is_same_v<std::allocator_traits<A>::size_type, std::size_t>);
static_assert(std::allocator_traits<A>::is_always_equal::value);
static_assert(std::is_nothrow_default_constructible_v<A>);
static_assert(std::is_nothrow_copy_constructible_v<A>);

// a node type that holds its own allocator, the classic use
struct Node {
  std::allocator<Node> alloc;
  Node* next;
};
static_assert(std::is_same_v<std::allocator_traits<std::allocator<int>>::rebind_alloc<Incomplete>, A>);
static_assert(std::is_same_v<std::allocator_traits<A>::rebind_alloc<int>, std::allocator<int>>);
struct Incomplete {};
