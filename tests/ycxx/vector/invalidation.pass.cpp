// [vector.modifiers]/2: "If no reallocation happens, then references, pointers, and
// iterators before the insertion point remain valid". /4: erase "invalidates iterators and
// references at or after the point of the erase" (so those before stay valid).
// [vector.capacity]/7: no reallocation during insertions after reserve() until the size
// would exceed capacity(). [container.reqmts]/66.6: swap does not invalidate references.
// [container.reqmts]/67: other operations (e.g. element access, size) do not invalidate.
#include <vector>
#include "check.hpp"

int main() {
  std::vector<int> v;
  v.reserve(64);
  for (int i = 0; i < 10; ++i) v.push_back(i);
  int* p0 = &v[0];
  int* p4 = &v[4];
  auto it3 = v.begin() + 3;
  v.insert(v.begin() + 5, 99);
  v.emplace(v.begin() + 6, 98);
  v.insert(v.end(), 3, 7);
  v.push_back(1);
  v.emplace_back(2);
  CHECK(p0 == v.data() && *p0 == 0 && *p4 == 4 && *it3 == 3 && it3 == v.begin() + 3);
  v.erase(v.begin() + 6);
  v.pop_back();
  CHECK(*p4 == 4 && *it3 == 3 && p0 == v.data());
  (void)v.at(2);
  (void)v.size();
  (void)v.capacity();
  CHECK(*p4 == 4);
  std::vector<int> w{100};
  v.swap(w);
  CHECK(*p4 == 4 && &w[4] == p4);
  return 0;
}
