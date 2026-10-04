// [inplace.vector.overview]/5: if N is zero, inplace_vector<T, N> is trivially copyable and
// empty and trivially default constructible. Otherwise: a trivially copy constructible T
// gives a trivial copy constructor, a trivially move constructible T a trivial move
// constructor, and a trivially destructible T a trivial destructor, plus trivial copy /
// move assignment when T's copy / move construction and assignment are trivial.
#include <inplace_vector>
#include <type_traits>
#include <utility>

struct NonTrivial {
  NonTrivial() {}
  NonTrivial(const NonTrivial&) {}
  NonTrivial(NonTrivial&&) noexcept(false) {}
  NonTrivial& operator=(const NonTrivial&) { return *this; }
  ~NonTrivial() {}
};

using Z = std::inplace_vector<NonTrivial, 0>;
static_assert(std::is_trivially_copyable_v<Z> && std::is_empty_v<Z> && std::is_trivially_default_constructible_v<Z>);
static_assert(std::is_trivially_copyable_v<std::inplace_vector<int, 0>> && std::is_empty_v<std::inplace_vector<int, 0>>);

using I = std::inplace_vector<int, 8>;
static_assert(std::is_trivially_copy_constructible_v<I> && std::is_trivially_move_constructible_v<I>);
static_assert(std::is_trivially_destructible_v<I>);
static_assert(std::is_trivially_copy_assignable_v<I> && std::is_trivially_move_assignable_v<I>);
static_assert(std::is_trivially_copyable_v<I>);

using N = std::inplace_vector<NonTrivial, 8>;
static_assert(!std::is_trivially_copy_constructible_v<N> && !std::is_trivially_destructible_v<N>);
static_assert(std::is_copy_constructible_v<N> && std::is_move_constructible_v<N>);

int main() { return 0; }
