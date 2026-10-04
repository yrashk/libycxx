// [sequence.reqmts]/20-22: a.emplace(p, args) "Inserts an object of type T constructed with
// std::forward<Args>(args)... before p. [Note 1: args can directly or indirectly refer to a
// value in a. - end note]" (LWG 2164). The same holds for emplace_back ([sequence.reqmts]
// "Appends an object of type T constructed with std::forward<Args>(args)...") and for
// push_back(const T&). Checked with spare capacity (elements shift in place) and without
// (reallocation), with the argument before, at and after the insertion point, directly and
// indirectly (a constructor argument that is a member of an element).
#include <vector>
#include <string>
#include <utility>
#include "check.hpp"

struct Pair {
  std::string s;
  int n;
  constexpr Pair(const std::string& str, int k) : s(str), n(k) {}
  constexpr bool operator==(const Pair&) const = default;
};

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
    v.emplace(v.cbegin(), v.back());
    if (v != V{d, a, b, c, d}) return false;
    v = fresh();
    v.emplace(v.cbegin() + 1, v[1]);  // the element that is shifted
    if (v != V{a, b, b, c, d}) return false;
    v = fresh();
    v.emplace(v.cbegin() + 2, v[3]);
    if (v != V{a, b, d, c, d}) return false;
    v = fresh();
    v.emplace(v.cbegin() + 3, v[0]);
    if (v != V{a, b, c, a, d}) return false;
    v = fresh();
    v.emplace(v.cend(), v[2]);
    if (v != V{a, b, c, d, c}) return false;
    v = fresh();
    v.emplace_back(v.front());
    if (v != V{a, b, c, d, a}) return false;
    v = fresh();
    v.push_back(v[1]);
    if (v != V{a, b, c, d, b}) return false;
    v = fresh();
    const V& cv = v;
    v.insert(v.cbegin() + 1, cv[2]);
    if (v != V{a, c, b, c, d}) return false;
  }
  return true;
}

constexpr bool test_indirect() {
  using V = std::vector<Pair>;
  std::string l1 = "a string that is longer than any small-string buffer, one";
  std::string l2 = "a string that is longer than any small-string buffer, two";
  for (int spare = 0; spare < 2; ++spare) {
    V v;
    v.emplace_back(l1, 1);
    v.emplace_back(l2, 2);
    if (spare) v.reserve(10);
    else v.shrink_to_fit();
    v.emplace(v.cbegin(), v[1].s, v[0].n);  // arguments are members of elements
    if (!(v[0] == Pair(l2, 1)) || !(v[1] == Pair(l1, 1)) || !(v[2] == Pair(l2, 2))) return false;
    v.shrink_to_fit();
    v.emplace_back(v[0].s, v[2].n);
    if (v.size() != 4 || !(v[3] == Pair(l2, 2))) return false;
  }
  return true;
}

static_assert(test<int>(1, 2, 3, 4));
static_assert(test<std::string>("first string, longer than a small buffer", "second string, longer than a small buffer",
                                "c", "fourth string, longer than a small buffer"));
static_assert(test_indirect());

int main() {
  CHECK(test<int>(1, 2, 3, 4));
  CHECK(test<std::string>("first string, longer than a small buffer", "second string, longer than a small buffer", "c",
                          "fourth string, longer than a small buffer"));
  CHECK(test<std::vector<int>>({1}, {2, 2}, {3, 3, 3}, {}));
  CHECK(test_indirect());
  return 0;
}
