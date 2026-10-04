// [over.match.list]/1: list-initialization first considers the initializer-list
// constructors — "constexpr vector(initializer_list<T>, const Allocator& = Allocator());"
// ([vector.overview]) — and only if none is viable all constructors. So vector<int>{3} has
// one element 3 while vector<int>(3) has three value-initialized elements
// ([vector.cons]/3: n default-inserted elements), vector<int>{3, 4} is {3, 4} while
// vector<int>(3, 4) is three 4s, and for a T not constructible from an integer,
// vector<T>{3} falls back to explicit vector(size_type) (direct-list-initialization may use
// explicit constructors). An empty braced list value-initializes via the default
// constructor ([dcl.init.list]/3.5).
#include <vector>
#include <string>
#include "check.hpp"

struct NotFromInt {
  int v = 7;
};

constexpr bool test() {
  std::vector<int> a{3};
  std::vector<int> b(3);
  std::vector<int> c{3, 4};
  std::vector<int> d(3, 4);
  if (a.size() != 1 || a[0] != 3) return false;
  if (b.size() != 3 || b[0] != 0 || b[2] != 0) return false;
  if (c.size() != 2 || c[1] != 4) return false;
  if (d.size() != 3 || d[0] != 4) return false;
  std::vector<NotFromInt> e{3};
  if (e.size() != 3 || e[2].v != 7) return false;
  std::vector<std::string> f{3};
  if (f.size() != 3 || !f[0].empty()) return false;
  std::vector<std::string> g{"3"};
  if (g.size() != 1 || g[0] != "3") return false;
  std::vector<int> h{};
  if (!h.empty()) return false;
  std::vector<std::vector<int>> nested{{1, 2}, {3}};
  if (nested.size() != 2 || nested[0].size() != 2) return false;
  // vector<int>(size_type) is explicit, so 3 cannot copy-initialize a vector<int> element: the
  // initializer_list<vector<int>> constructor is not viable and the outer vector's explicit
  // vector(size_type) is used.
  std::vector<std::vector<int>> one{3};
  if (one.size() != 3 || !one[0].empty()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
