// [uninitialized.construct.default]/2: ranges::uninitialized_default_construct(first, last)
// default-initializes remove_reference_t<iter_reference_t<I>> at each position "return
// first;"; the range overload returns borrowed_iterator_t<R>. /4: the _n form is
// uninitialized_default_construct(counted_iterator(first, n), default_sentinel).base().
// [uninitialized.construct.value]/2,4: likewise with value-initialization. All constexpr.
#include <memory>
#include <cstring>
#include <iterator>
#include <span>
#include "check.hpp"

struct D {
  int v = 7;
  constexpr D() = default;
};
struct P {
  int x;
  long y;
};

constexpr bool test() {
  std::allocator<D> a;
  D* p = a.allocate(4);
  D* r = std::ranges::uninitialized_default_construct(p, p + 4);
  if (r != p + 4 || p[3].v != 7) return false;
  std::ranges::destroy(p, p + 4);
  std::span<D> s(p, 3);
  auto it = std::ranges::uninitialized_default_construct(s);
  if (it != s.end() || p[2].v != 7) return false;
  std::ranges::destroy(s);
  r = std::ranges::uninitialized_default_construct_n(p, 2);
  if (r != p + 2 || p[1].v != 7) return false;
  std::ranges::destroy_n(p, 2);
  if (std::ranges::uninitialized_default_construct_n(p, 0) != p) return false;
  // a sentinel of a different type
  std::counted_iterator<D*> ci(p, 2);
  auto ce = std::ranges::uninitialized_default_construct(ci, std::default_sentinel);
  if (ce.base() != p + 2) return false;
  std::ranges::destroy_n(p, 2);

  r = std::ranges::uninitialized_value_construct(p, p + 4);
  if (r != p + 4 || p[0].v != 7) return false;
  std::ranges::destroy(p, p + 4);
  auto it2 = std::ranges::uninitialized_value_construct(std::span<D>(p, 2));  // borrowed
  if (it2 != std::span<D>(p, 2).end()) return false;
  std::ranges::destroy_n(p, 2);
  r = std::ranges::uninitialized_value_construct_n(p, 3);
  if (r != p + 3) return false;
  std::ranges::destroy_n(p, 3);
  a.deallocate(p, 4);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::allocator<P> a;
  P* p = a.allocate(3);
  std::memset(static_cast<void*>(p), 0x77, 3 * sizeof(P));
  std::ranges::uninitialized_value_construct(p, p + 3);
  CHECK(p[0].x == 0 && p[2].y == 0);
  std::ranges::destroy(p, p + 3);
  std::memset(static_cast<void*>(p), 0x77, 3 * sizeof(P));
  std::ranges::uninitialized_value_construct_n(p, 2);
  CHECK(p[1].x == 0 && p[1].y == 0);
  std::ranges::destroy_n(p, 2);
  a.deallocate(p, 3);
  return 0;
}
