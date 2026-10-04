// [associative.reqmts.general]/180 for set and multiset: with a transparent comparator the
// lookup, erase and extract templates compare the argument with the keys directly; without
// it the argument is converted to key_type; erase / extract with an iterator argument are
// the position forms. [set.modifiers]/1-4 (C++26): with a transparent comparator,
// insert(K&&) / insert(hint, K&&) construct value_type from the argument only if no
// equivalent element exists, and return the element equivalent to x.
#include <set>
#include <functional>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

using reqs::associative::Id;
using reqs::associative::IdLess;
using reqs::associative::IdLessPlain;

bool heterogeneous_insert() {
  std::set<Id, IdLess> s;
  s.insert(Id(1));
  s.insert(Id(3));
  Id::from_int = 0;
  auto [it, inserted] = s.insert(3);  // already there: no Id made
  if (inserted || it->v != 3 || Id::from_int != 0) return false;
  auto h = s.insert(s.cend(), 1);
  if (h != s.begin() || Id::from_int != 0 || s.size() != 2) return false;
  auto [it2, ins2] = s.insert(2);
  if (!ins2 || it2->v != 2 || Id::from_int != 1 || s.size() != 3) return false;
  auto h2 = s.insert(s.cend(), 5);
  return h2->v == 5 && std::next(h2) == s.end() && Id::from_int == 2;
}

int main() {
  CHECK((reqs::associative::transparent<std::set<Id, IdLess>, std::set<Id, IdLessPlain>>()));
  CHECK((reqs::associative::transparent<std::multiset<Id, IdLess>, std::multiset<Id, IdLessPlain>>()));
  CHECK(heterogeneous_insert());
  std::set<long, std::less<>> s{1, 2, 3};
  CHECK(s.contains(2) && s.erase(2) == 1 && !s.extract(3).empty() && s.size() == 1);
  return 0;
}
