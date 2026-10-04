// inplace_vector members whose value argument refers to an element of the same container.
// [sequence.reqmts]/24-27, /32-35: insert(p, t) / insert(p, n, t) insert copies of t (no
// precondition excluding t referring into a, unlike assign(n, t), /67); /22 Note 1: emplace's
// "args can directly or indirectly refer to a value in a". [inplace.vector.modifiers]:
// push_back/emplace_back/try_push_back/try_emplace_back/unchecked_push_back/
// unchecked_emplace_back append an object constructed from the argument(s).
// [inplace.vector.capacity]: resize(sz, c) appends sz - size() copies of c.
#include <inplace_vector>
#include <string>
#include "check.hpp"

template <class T>
constexpr bool test(T a, T b, T c, T d) {
  using V = std::inplace_vector<T, 12>;
  V v{a, b, c, d};
  v.insert(v.cbegin(), v[2]);
  if (v != V{c, a, b, c, d}) return false;
  v = V{a, b, c, d};
  v.insert(v.cbegin() + 1, v[1]);
  if (v != V{a, b, b, c, d}) return false;
  v = V{a, b, c, d};
  v.insert(v.cbegin() + 3, v[0]);
  if (v != V{a, b, c, a, d}) return false;
  v = V{a, b, c, d};
  v.insert(v.cbegin() + 1, 3, v[3]);
  if (v != V{a, d, d, d, b, c, d}) return false;
  v = V{a, b, c, d};
  v.insert(v.cbegin() + 2, 2, v[2]);
  if (v != V{a, b, c, c, c, d}) return false;
  v = V{a, b, c, d};
  v.insert(v.cbegin(), 5, v[1]);
  if (v != V{b, b, b, b, b, a, b, c, d}) return false;
  v = V{a, b, c, d};
  v.emplace(v.cbegin(), v.back());
  if (v != V{d, a, b, c, d}) return false;
  v = V{a, b, c, d};
  v.emplace(v.cbegin() + 2, v[2]);
  if (v != V{a, b, c, c, d}) return false;
  v = V{a, b, c, d};
  v.push_back(v[0]);
  v.emplace_back(v[1]);
  if (v != V{a, b, c, d, a, b}) return false;
  if (!v.try_push_back(v[2]) || !v.try_emplace_back(v[3])) return false;
  v.unchecked_push_back(v[0]);
  v.unchecked_emplace_back(v[5]);
  if (v != V{a, b, c, d, a, b, c, d, a, b}) return false;
  v = V{a, b, c, d};
  v.resize(9, v.back());
  if (v != V{a, b, c, d, d, d, d, d, d}) return false;
  v = V{a, b, c, d};
  v.resize(7, v.front());
  if (v != V{a, b, c, d, a, a, a}) return false;
  return true;
}

static_assert(test<int>(1, 2, 3, 4));

int main() {
  CHECK(test<int>(1, 2, 3, 4));
  CHECK(test<std::string>("first string, longer than a small buffer", "second string, longer than a small buffer", "c",
                          "fourth string, longer than a small buffer"));
  return 0;
}
