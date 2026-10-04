// [associative.reqmts.general]/180 for flat_set / flat_multiset with a transparent
// comparator: find, count, contains, lower_bound, upper_bound, equal_range and erase accept
// a key-comparable value. [flat.set.modifiers]/1-4: insert(K&&) / insert(hint, K&&) leave
// *this and x unchanged when an equivalent element exists, and otherwise insert as if by
// emplace(std::forward<K>(x)).
#include <flat_set>
#include <functional>
#include <iterator>
#include "check.hpp"

struct Id {
  int v;
  static inline int from_int = 0;
  Id(int x) : v(x) { ++from_int; }
  Id(const Id&) = default;
  Id& operator=(const Id&) = default;
};
struct IdLess {
  using is_transparent = void;
  bool operator()(const Id& a, const Id& b) const { return a.v < b.v; }
  bool operator()(const Id& a, int b) const { return a.v < b; }
  bool operator()(int a, const Id& b) const { return a < b.v; }
};

int main() {
  std::flat_set<Id, IdLess> s;
  s.emplace(1);
  s.emplace(3);
  Id::from_int = 0;
  CHECK(s.contains(3) && s.find(2) == s.end() && s.count(1) == 1 && s.lower_bound(2) == s.begin() + 1);
  CHECK(std::distance(s.equal_range(3).first, s.equal_range(3).second) == 1 && s.upper_bound(1) == s.begin() + 1);
  auto [it, ins] = s.insert(3);
  CHECK(!ins && it->v == 3 && s.insert(s.cbegin(), 1)->v == 1 && Id::from_int == 0);
  auto [it2, ins2] = s.insert(2);
  CHECK(ins2 && it2->v == 2 && Id::from_int >= 1 && s.size() == 3);
  Id::from_int = 0;
  CHECK(s.erase(2) == 1 && s.size() == 2 && Id::from_int == 0);
  std::flat_multiset<Id, IdLess> m;
  for (int k : {1, 2, 2}) m.emplace(k);
  Id::from_int = 0;
  CHECK(m.count(2) == 2 && m.erase(2) == 2 && m.size() == 1 && Id::from_int == 0);
  return 0;
}
