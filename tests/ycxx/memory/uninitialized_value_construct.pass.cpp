// [uninitialized.construct.value]/1: uninitialized_value_construct(first, last) is
// "for (; first != last; ++first) ::new (voidify(*first)) iterator_traits<I>::value_type();"
// (value-initialization: zero for scalars and for classes without a user-provided default
// constructor). /3: the _n form returns the advanced iterator. Both are constexpr.
#include <memory>
#include <cstring>
#include "check.hpp"

struct P {
  int x;
  double y;
};
struct WithCtor {
  int v;
  constexpr WithCtor() : v(5) {}
};

constexpr bool test() {
  std::allocator<WithCtor> a;
  WithCtor* p = a.allocate(3);
  std::uninitialized_value_construct(p, p + 3);
  if (p[0].v != 5 || p[2].v != 5) return false;
  std::destroy(p, p + 3);
  if (std::uninitialized_value_construct_n(p, 2) != p + 2) return false;
  if (p[1].v != 5) return false;
  std::destroy_n(p, 2);
  if (std::uninitialized_value_construct_n(p, 0) != p) return false;
  a.deallocate(p, 3);
  std::allocator<int> ai;
  int* q = ai.allocate(2);
  std::uninitialized_value_construct(q, q + 2);
  if (q[0] != 0 || q[1] != 0) return false;
  ai.deallocate(q, 2);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // value-initialization zeroes storage that held other bytes
  std::allocator<P> a;
  P* p = a.allocate(4);
  std::memset(static_cast<void*>(p), 0x5A, 4 * sizeof(P));
  std::uninitialized_value_construct(p, p + 4);
  for (int i = 0; i < 4; ++i) CHECK(p[i].x == 0 && p[i].y == 0.0);
  std::destroy(p, p + 4);
  std::memset(static_cast<void*>(p), 0x5A, 4 * sizeof(P));
  P* e = std::uninitialized_value_construct_n(p, 3);
  CHECK(e == p + 3);
  CHECK(p[0].x == 0 && p[2].y == 0.0);
  std::destroy_n(p, 3);
  a.deallocate(p, 4);

  std::allocator<unsigned> au;
  unsigned* u = au.allocate(8);
  std::memset(static_cast<void*>(u), 0xFF, 8 * sizeof(unsigned));
  std::uninitialized_value_construct_n(u, 8);
  for (int i = 0; i < 8; ++i) CHECK(u[i] == 0u);
  au.deallocate(u, 8);
  return 0;
}
