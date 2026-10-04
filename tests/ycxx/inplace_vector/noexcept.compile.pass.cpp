// [inplace.vector.overview] synopsis: the default constructor is noexcept; the move
// constructor is noexcept(N == 0 || is_nothrow_move_constructible_v<T>); swap (member and
// non-member) is noexcept(N == 0 || (is_nothrow_swappable_v<T> &&
// is_nothrow_move_constructible_v<T>)); capacity(), max_size() and shrink_to_fit() are
// static and noexcept; begin / end / size / empty / data / clear are noexcept.
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
using I = std::inplace_vector<int, 8>;
using N = std::inplace_vector<NonTrivial, 8>;

static_assert(std::is_nothrow_default_constructible_v<N>);
static_assert(std::is_nothrow_move_constructible_v<I> && !std::is_nothrow_move_constructible_v<N>);
static_assert(std::is_nothrow_move_constructible_v<Z>);
static_assert(noexcept(std::declval<I&>().swap(std::declval<I&>())));
static_assert(!noexcept(std::declval<N&>().swap(std::declval<N&>())));
static_assert(noexcept(std::declval<I&>().clear()) && noexcept(std::declval<I&>().data()));

static_assert(noexcept(swap(std::declval<I&>(), std::declval<I&>())));
static_assert(noexcept(std::declval<Z&>().swap(std::declval<Z&>())));
static_assert(noexcept(I::capacity()) && noexcept(I::max_size()) && noexcept(I::shrink_to_fit()));
static_assert(noexcept(std::declval<const I&>().size()) && noexcept(std::declval<const I&>().empty()));
static_assert(noexcept(std::declval<I&>().begin()) && noexcept(std::declval<const I&>().cend()));

int main() { return 0; }
