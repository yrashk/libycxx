// [unord.req.general]/245 for unordered_set / unordered_multiset: the lookup, erase,
// extract and bucket templates take part only when both Hash::is_transparent and
// Pred::is_transparent name types, and then do not convert the argument to key_type.
// [unord.set.modifiers]/1-4 (C++26): with both transparent, insert(K&&) and
// insert(hint, K&&) construct value_type only if no equivalent element exists, and return
// the element equivalent to the argument.
#include <unordered_set>
#include <cstddef>
#include <iterator>
#include "check.hpp"

struct Id {
  int v;
  static inline int from_int = 0;
  Id(int x) : v(x) { ++from_int; }
  Id(const Id&) = default;
  bool operator==(const Id&) const = default;
};
struct THash {
  using is_transparent = void;
  std::size_t operator()(const Id& i) const { return static_cast<std::size_t>(i.v) * 31u; }
  std::size_t operator()(int i) const { return static_cast<std::size_t>(i) * 31u; }
};
struct TEq {
  using is_transparent = void;
  bool operator()(const Id& a, const Id& b) const { return a.v == b.v; }
  bool operator()(const Id& a, int b) const { return a.v == b; }
  bool operator()(int a, const Id& b) const { return a == b.v; }
};
struct PEq {
  bool operator()(const Id& a, const Id& b) const { return a.v == b.v; }
};

template <class S>
bool lookup() {
  S s;
  for (int k : {1, 2, 3, 2}) s.emplace(k);
  const std::size_t n2 = s.size() == 4 ? 2 : 1;
  Id::from_int = 0;
  if (s.find(2) == s.end() || s.count(2) != n2 || !s.contains(1) || s.contains(7)) return false;
  auto [lo, hi] = s.equal_range(2);
  if (static_cast<std::size_t>(std::distance(lo, hi)) != n2 || s.bucket(3) >= s.bucket_count()) return false;
  if (s.erase(2) != n2 || s.extract(3).empty() || !s.extract(8).empty()) return false;
  return Id::from_int == 0 && s.size() == 1;
}

bool heterogeneous_insert() {
  std::unordered_set<Id, THash, TEq> s;
  s.emplace(1);
  Id::from_int = 0;
  auto [it, ins] = s.insert(1);
  auto h = s.insert(s.cbegin(), 1);
  if (ins || it->v != 1 || h != it || Id::from_int != 0) return false;
  auto [it2, ins2] = s.insert(2);
  auto h2 = s.insert(s.cend(), 3);
  return ins2 && it2->v == 2 && h2->v == 3 && Id::from_int == 2 && s.size() == 3;
}

int main() {
  CHECK(lookup<std::unordered_set<Id, THash, TEq>>());
  CHECK(lookup<std::unordered_multiset<Id, THash, TEq>>());
  CHECK(heterogeneous_insert());
  // predicate not transparent: the argument is converted
  std::unordered_set<Id, THash, PEq> p;
  p.emplace(1);
  Id::from_int = 0;
  CHECK(p.contains(1) && p.find(1) != p.end() && Id::from_int == 2);
  return 0;
}
