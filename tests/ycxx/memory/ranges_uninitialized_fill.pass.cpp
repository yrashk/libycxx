// [uninitialized.fill]/2: ranges::uninitialized_fill(first, last, x) constructs
// remove_reference_t<iter_reference_t<I>>(x) at each position and returns last; the range
// overload returns borrowed_iterator_t<R>; T defaults to iter_value_t<I> / range_value_t<R>.
// /4: uninitialized_fill_n(first, n, x) via counted_iterator. All constexpr.
#include <memory>
#include <span>
#include "check.hpp"

struct Pt {
  int x, y;
};

constexpr bool test() {
  std::allocator<int> a;
  int* p = a.allocate(4);
  if (std::ranges::uninitialized_fill(p, p + 4, 3) != p + 4) return false;
  if (p[0] != 3 || p[3] != 3) return false;
  std::ranges::destroy(p, p + 4);
  std::span<int> s(p, 2);
  if (std::ranges::uninitialized_fill(s, 8) != s.end() || p[1] != 8) return false;
  std::ranges::destroy(s);
  if (std::ranges::uninitialized_fill_n(p, 3, 5) != p + 3 || p[2] != 5) return false;
  std::ranges::destroy_n(p, 3);
  if (std::ranges::uninitialized_fill_n(p, 0, 5) != p) return false;
  a.deallocate(p, 4);

  std::allocator<Pt> ap;
  Pt* q = ap.allocate(2);
  std::ranges::uninitialized_fill(q, q + 2, {1, 2});  // T defaults to the value type
  if (q[1].x != 1 || q[1].y != 2) return false;
  std::ranges::destroy(q, q + 2);
  std::ranges::uninitialized_fill(std::span<Pt>(q, 2), {3, 4});
  if (q[0].y != 4) return false;
  std::ranges::destroy(q, q + 2);
  std::ranges::uninitialized_fill_n(q, 2, {5, 6});
  if (q[1].x != 5) return false;
  std::ranges::destroy(q, q + 2);
  ap.deallocate(q, 2);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
