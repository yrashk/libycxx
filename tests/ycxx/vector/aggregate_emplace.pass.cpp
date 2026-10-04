// [container.alloc.reqmts]/2 note 2: a container constructs elements with
// allocator_traits<A>::construct(m, p, args); for std::allocator that is
// construct_at(p, args...) ([allocator.traits.members]/5), i.e.
// ::new(voidify(*p)) T(std::forward<Args>(args)...) ([specialized.construct]/2). Since C++20
// a parenthesized expression-list can initialize an aggregate ([dcl.init.general]/16.6.2.2),
// so emplace_back(args...) / emplace(p, args...) work for an aggregate without a
// constructor, including with fewer arguments than members (the rest value-initialized).
#include <vector>
#include <string>
#include "check.hpp"

struct Agg {
  int a;
  double b;
  std::string s;
};

struct Inner {
  int x;
  int y;
};
struct Outer {
  Inner in;
  int z;
};

int main() {
  std::vector<Agg> v;
  Agg& r = v.emplace_back(1, 2.5, "three");
  CHECK(&r == &v.back() && r.a == 1 && r.b == 2.5 && r.s == "three");
  v.emplace_back(4);
  CHECK(v.back().a == 4 && v.back().b == 0.0 && v.back().s.empty());
  auto it = v.emplace(v.begin(), 0, 0.5);
  CHECK(it == v.begin() && v[0].b == 0.5 && v.size() == 3);
  std::vector<Outer> o;
  o.emplace_back(Inner{1, 2}, 3);
  CHECK(o[0].in.y == 2 && o[0].z == 3);
  std::vector<Inner> in;
  for (int i = 0; i < 20; ++i) in.emplace_back(i, -i);
  CHECK(in.size() == 20 && in[19].x == 19 && in[19].y == -19);
  return 0;
}
