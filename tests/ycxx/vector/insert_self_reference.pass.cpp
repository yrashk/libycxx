// [sequence.reqmts]/24-27: a.insert(p, t) "Inserts a copy of t before p"; /32-35:
// a.insert(p, n, t) "Inserts n copies of t before p". Unlike a.assign(n, t) (/67:
// "Preconditions: ... t is not a reference into a") these have no precondition excluding t
// referring to an element of a, so the value inserted is the value t had before the call,
// even when t is an element that the insertion shifts or reallocates. Checked before and
// after the insertion point, with spare capacity (elements shift in place) and without
// (reallocation).
#include <vector>
#include <string>
#include <utility>
#include "check.hpp"

template <class T>
constexpr bool test(T a, T b, T c, T d) {
  using V = std::vector<T>;
  for (int spare = 0; spare < 2; ++spare) {
    auto fresh = [&] {
      V v{a, b, c, d};
      if (spare) v.reserve(100);
      else v.shrink_to_fit();
      return v;
    };
    V v = fresh();
    v.insert(v.cbegin(), v[2]);  // value after the insertion point
    if (v != V{c, a, b, c, d}) return false;
    v = fresh();
    v.insert(v.cbegin() + 3, v[0]);  // value before the insertion point
    if (v != V{a, b, c, a, d}) return false;
    v = fresh();
    v.insert(v.cbegin() + 1, v.back());
    if (v != V{a, d, b, c, d}) return false;
    v = fresh();
    v.insert(v.cend(), v.front());
    if (v != V{a, b, c, d, a}) return false;

    v = fresh();
    v.insert(v.cbegin(), 3, v[1]);
    if (v != V{b, b, b, a, b, c, d}) return false;
    v = fresh();
    v.insert(v.cbegin() + 2, 2, v[3]);
    if (v != V{a, b, d, d, c, d}) return false;
    v = fresh();
    v.insert(v.cbegin() + 1, 5, v[0]);
    if (v != V{a, a, a, a, a, a, b, c, d}) return false;
    v = fresh();
    v.insert(v.cbegin() + 3, 1, v[2]);
    if (v != V{a, b, c, c, d}) return false;
    v = fresh();
    v.insert(v.cend(), 4, v[1]);
    if (v != V{a, b, c, d, b, b, b, b}) return false;
  }
  return true;
}

static_assert(test<int>(1, 2, 3, 4));
static_assert(test<std::string>("first string, longer than a small buffer",
                                "second string, longer than a small buffer", "c",
                                "fourth string, longer than a small buffer"));

int main() {
  CHECK(test<int>(1, 2, 3, 4));
  CHECK(test<long long>(10, 20, 30, 40));
  CHECK(test<std::string>("first string, longer than a small buffer",
                          "second string, longer than a small buffer", "c",
                          "fourth string, longer than a small buffer"));
  CHECK(test<std::vector<int>>({1}, {2, 2}, {3, 3, 3}, {}));
  return 0;
}
