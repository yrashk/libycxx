// [vector.capacity]/15: resize(sz) "appends sz - size() default-inserted elements";
// [container.alloc.reqmts]/2.2: default-insertion is allocator_traits<A>::construct(m, p),
// which for std::allocator is construct_at(p) = ::new(p) T() — value-initialization, so
// scalars become zero ([dcl.init.general]/9). This holds even when the storage is reused
// within the existing capacity after shrinking, so old values must not reappear.
// [vector.cons]/3: vector(n) likewise has n default-inserted elements.
// [vector.bool.pspc]: vector<bool>::resize(sz, c = false).
#include <vector>
#include "check.hpp"

struct Agg {
  int a;
  double b;
};

constexpr bool test() {
  std::vector<int> v{1, 2, 3, 4, 5};
  v.resize(2);
  v.resize(5);
  if (v[2] != 0 || v[3] != 0 || v[4] != 0 || v[0] != 1) return false;
  std::vector<int> w(4);
  for (int x : w)
    if (x != 0) return false;
  std::vector<Agg> a{{1, 1.5}, {2, 2.5}};
  a.resize(1);
  a.resize(3);
  if (a[1].a != 0 || a[1].b != 0.0 || a[2].a != 0) return false;
  std::vector<int*> p{&v[0], &v[1]};
  p.resize(0);
  p.resize(2);
  if (p[0] != nullptr || p[1] != nullptr) return false;
  std::vector<bool> b{true, true, true};
  b.resize(1);
  b.resize(3);
  if (b[1] || b[2] || !b[0]) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // also at run time with storage that held non-zero values
  std::vector<long> v(100, -1);
  v.resize(10);
  v.resize(100);
  for (std::size_t i = 10; i < 100; ++i) CHECK(v[i] == 0);
  v.clear();
  v.resize(50);
  for (long x : v) CHECK(x == 0);
  return 0;
}
