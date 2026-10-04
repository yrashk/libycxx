// [vector.cons]: vector(initializer_list<T>, const Allocator& = Allocator()) is
// vector(il.begin(), il.end(), a); with nested braced-init-lists each level's elements are
// copy-initialized from the inner lists by the same constructor ([dcl.init.list]/3.7,
// [over.match.list]). Three and four levels, empty inner lists, const and non-const objects,
// and the same in constant evaluation ([vector.overview]: the members are constexpr).
#include <vector>
#include "check.hpp"

using V3 = std::vector<std::vector<std::vector<int>>>;

constexpr bool test() {
  V3 a{{{1, 2}}};
  if (a.size() != 1 || a[0].size() != 1 || a[0][0].size() != 2 || a[0][0][1] != 2) return false;
  const V3 b{{{1, 2}, {3}}, {}};
  if (b.size() != 2 || b[0].size() != 2 || !b[1].empty() || b[0][1][0] != 3) return false;
  std::vector<std::vector<std::vector<char>>> c{{{'a', 'b'}}, {}};
  if (c[0][0][0] != 'a' || c[0][0][1] != 'b' || !c[1].empty()) return false;
  std::vector<V3> d{{{{4}}}, {{{}}}};
  if (d.size() != 2 || d[0][0][0][0] != 4 || !d[1][0][0].empty()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  V3 a{{{1, 2}}, {{3}, {4, 5, 6}}};
  CHECK(a.size() == 2 && a[1].size() == 2 && a[1][1].size() == 3 && a[1][1][2] == 6);
  const std::vector<std::vector<std::vector<char>>> c{{{'a', 'b'}}, {}};
  CHECK(c.size() == 2 && c[0][0].size() == 2 && c[1].empty());
  return 0;
}
