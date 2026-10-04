// [reverse.iter.nonmember]: iter_move(i) is "auto tmp = i.base(); return
// ranges::iter_move(--tmp);" with noexcept(is_nothrow_copy_constructible_v<Iterator> &&
// noexcept(ranges::iter_move(--declval<Iterator&>()))); iter_swap(x, y) swaps *--xtmp and
// *--ytmp with the analogous exception specification.
#include <iterator>
#include <utility>
#include <type_traits>
#include "check.hpp"

struct MoveCount {
  int v;
  int moved = 0;
  constexpr MoveCount(int x) : v(x) {}
  constexpr MoveCount(MoveCount&& o) : v(o.v), moved(o.moved + 1) {}
  constexpr MoveCount& operator=(MoveCount&& o) {
    v = o.v;
    moved = o.moved + 1;
    return *this;
  }
};

using R = std::reverse_iterator<int*>;
static_assert(std::is_same_v<decltype(std::ranges::iter_move(std::declval<const R&>())), int&&>);
static_assert(noexcept(std::ranges::iter_move(std::declval<const R&>())));
static_assert(noexcept(std::ranges::iter_swap(std::declval<const R&>(), std::declval<const R&>())));
static_assert(std::is_same_v<std::iter_rvalue_reference_t<R>, int&&>);

constexpr bool test() {
  int a[3] = {1, 2, 3};
  R r(a + 3), s(a + 1);
  int&& x = std::ranges::iter_move(r);
  if (&x != a + 2) return false;
  std::ranges::iter_swap(r, s);  // swaps a[2] and a[0]
  if (a[0] != 3 || a[2] != 1 || a[1] != 2) return false;
  iter_swap(r, s);  // the hidden friend found by ADL
  if (a[0] != 1 || a[2] != 3) return false;
  MoveCount m[2] = {MoveCount(5), MoveCount(6)};
  std::reverse_iterator<MoveCount*> rm(m + 2);
  MoveCount taken = std::ranges::iter_move(rm);
  if (taken.v != 6 || taken.moved != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
