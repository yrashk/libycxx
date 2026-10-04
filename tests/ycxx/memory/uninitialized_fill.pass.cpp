// [uninitialized.fill]/1: template<class NoThrowForwardIterator, class T =
// iterator_traits<NoThrowForwardIterator>::value_type> uninitialized_fill(first, last, const
// T& x): "for (; first != last; ++first) ::new (voidify(*first)) value_type(x);" /3:
// uninitialized_fill_n(first, n, x): "for (; n--; ++first) ...; return first;". The default
// template argument for T lets a braced-init-list be passed (P2248). Both are constexpr.
#include <memory>
#include "check.hpp"

struct Pt {
  int x, y;
};
struct Conv {
  long v;
  constexpr explicit Conv(int x) : v(x) {}
};

constexpr bool test() {
  std::allocator<int> a;
  int* p = a.allocate(5);
  std::uninitialized_fill(p, p + 5, 9);
  for (int i = 0; i < 5; ++i)
    if (p[i] != 9) return false;
  std::destroy(p, p + 5);
  int* e = std::uninitialized_fill_n(p, 3, 4);
  if (e != p + 3 || p[0] != 4 || p[2] != 4) return false;
  std::destroy(p, p + 3);
  if (std::uninitialized_fill_n(p, 0, 1) != p) return false;
  a.deallocate(p, 5);

  std::allocator<Pt> ap;
  Pt* q = ap.allocate(3);
  std::uninitialized_fill(q, q + 3, {1, 2});  // T defaults to Pt
  if (q[2].x != 1 || q[2].y != 2) return false;
  std::destroy(q, q + 3);
  if (std::uninitialized_fill_n(q, 2, {3, 4}) != q + 2) return false;
  if (q[1].y != 4) return false;
  std::destroy(q, q + 2);
  ap.deallocate(q, 3);

  // heterogeneous T: direct-initialization of the value type from x
  std::allocator<Conv> ac;
  Conv* c = ac.allocate(2);
  std::uninitialized_fill(c, c + 2, 6);
  if (c[0].v != 6 || c[1].v != 6) return false;
  std::destroy(c, c + 2);
  ac.deallocate(c, 2);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
