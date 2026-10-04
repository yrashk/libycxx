// [utility.swap]/1-4: swap(T& a, T& b) is constexpr, constrained on is_move_constructible_v<T>
// && is_move_assignable_v<T>, exchanges the values, and has exception specification
// is_nothrow_move_constructible_v<T> && is_nothrow_move_assignable_v<T>.
// /5-7: swap(T (&a)[N], T (&b)[N]) noexcept(is_nothrow_swappable_v<T>), constrained on
// is_swappable_v<T>, "As if by swap_ranges(a, a + N, b)".
#include <utility>
#include <type_traits>
#include "check.hpp"

struct Moves {
  int v;
  int* moves;
  constexpr Moves(int x, int* m) : v(x), moves(m) {}
  constexpr Moves(Moves&& o) noexcept : v(o.v), moves(o.moves) { ++*moves; }
  constexpr Moves& operator=(Moves&& o) noexcept {
    v = o.v;
    moves = o.moves;
    ++*moves;
    return *this;
  }
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) {}
  ThrowingMove& operator=(ThrowingMove&&) noexcept { return *this; }
};
struct ThrowingAssign {
  ThrowingAssign() = default;
  ThrowingAssign(ThrowingAssign&&) noexcept {}
  ThrowingAssign& operator=(ThrowingAssign&&) { return *this; }
};
namespace adl {
struct Custom {
  int v;
  int* calls;
};
constexpr void swap(Custom& a, Custom& b) noexcept {
  int t = a.v;
  a.v = b.v;
  b.v = t;
  ++*a.calls;
}
}  // namespace adl

static_assert(noexcept(std::swap(std::declval<int&>(), std::declval<int&>())));
static_assert(!noexcept(std::swap(std::declval<ThrowingMove&>(), std::declval<ThrowingMove&>())));
static_assert(!noexcept(std::swap(std::declval<ThrowingAssign&>(), std::declval<ThrowingAssign&>())));
static_assert(std::is_same_v<decltype(std::swap(std::declval<int&>(), std::declval<int&>())), void>);
using IntArr = int[3];
using TMArr = ThrowingMove[2];
static_assert(noexcept(std::swap(std::declval<IntArr&>(), std::declval<IntArr&>())));
static_assert(!noexcept(std::swap(std::declval<TMArr&>(), std::declval<TMArr&>())));

constexpr bool test() {
  int a = 1, b = 2;
  std::swap(a, b);
  if (a != 2 || b != 1) return false;
  // class types that are only movable
  int moves = 0;
  Moves m1(1, &moves), m2(2, &moves);
  std::swap(m1, m2);
  if (m1.v != 2 || m2.v != 1) return false;
  // arrays
  int x[3] = {1, 2, 3}, y[3] = {4, 5, 6};
  std::swap(x, y);
  if (x[0] != 4 || x[2] != 6 || y[0] != 1 || y[2] != 3) return false;
  // arrays of arrays
  int p[2][2] = {{1, 2}, {3, 4}}, q[2][2] = {{5, 6}, {7, 8}};
  std::swap(p, q);
  if (p[1][1] != 8 || q[0][0] != 1) return false;
  // arrays swap their elements as if by swap_ranges, which finds ADL swap
  int calls = 0;
  adl::Custom c1[2] = {{1, &calls}, {2, &calls}}, c2[2] = {{3, &calls}, {4, &calls}};
  std::swap(c1, c2);
  if (calls != 2 || c1[0].v != 3 || c2[1].v != 2) return false;
  // self-swap leaves the value unchanged
  std::swap(a, a);
  return a == 2;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
