// [container.reqmts]/15-16: "X u(rv);" Complexity: "Linear for array and inplace_vector and
// constant for all other standard containers", and [container.alloc.reqmts]/17,20: X u(rv)
// is constant and X u(rv, m) is constant if m == rv.get_allocator(). Constant complexity
// independent of size means no per-element operations: the elements are neither copied nor
// moved, so they stay at the same addresses and are now owned by u. [container.reqmts]/50:
// t.swap(s) is constant for the same containers.
// COUNTERPART: libstdcxx:23_containers/[a-z_]+/debug/60499.cc
// COUNTERPART: libstdcxx:23_containers/forward_list/debug/60499.cc
#include <vector>
#include <memory>
#include <utility>
#include "check.hpp"

struct Counted {
  static inline int copies = 0, moves = 0;
  int v;
  Counted(int x) : v(x) {}
  Counted(const Counted& o) : v(o.v) { ++copies; }
  Counted(Counted&& o) noexcept : v(o.v) { ++moves; }
  Counted& operator=(const Counted& o) { v = o.v; ++copies; return *this; }
  Counted& operator=(Counted&& o) noexcept { v = o.v; ++moves; return *this; }
};

int main() {
  std::vector<Counted> v;
  v.reserve(100);
  for (int i = 0; i < 100; ++i) v.emplace_back(i);
  const Counted* p = v.data();
  Counted::copies = Counted::moves = 0;

  std::vector<Counted> u(std::move(v));
  CHECK(Counted::copies == 0 && Counted::moves == 0);
  CHECK(u.size() == 100 && u.data() == p && u[99].v == 99);

  std::vector<Counted> w(std::move(u), std::allocator<Counted>());
  CHECK(Counted::copies == 0 && Counted::moves == 0);
  CHECK(w.size() == 100 && w.data() == p);

  std::vector<Counted> x = std::move(w);
  CHECK(Counted::copies == 0 && Counted::moves == 0 && x.data() == p);

  std::vector<Counted> y{Counted(1)};
  Counted::copies = Counted::moves = 0;
  x.swap(y);
  CHECK(Counted::copies == 0 && Counted::moves == 0 && y.data() == p && x.size() == 1);
  return 0;
}
