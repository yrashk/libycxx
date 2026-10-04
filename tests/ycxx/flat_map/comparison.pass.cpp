// [flat.map.defn]: operator== and operator<=> (synth-three-way over value_type, so the
// result category comes from Key and T) compare the elements in order. Heterogeneous
// lookup and access with a transparent comparator ([associative.reqmts.general]/180,
// [flat.map.access], [flat.map.modifiers]).
#include <flat_map>
#include <compare>
#include <functional>
#include <type_traits>
#include "check.hpp"

constexpr bool compare() {
  using M = std::flat_map<int, double>;
  static_assert(std::is_same_v<decltype(M() <=> M()), std::partial_ordering>);
  M a{{1, 1.0}, {2, 2.0}}, b{{2, 2.0}, {1, 1.0}}, c{{1, 1.0}, {2, 3.0}};
  if (!(a == b) || a == c || !(a < c) || (a <=> b) != 0) return false;
  std::flat_multimap<int, int> m{{1, 2}, {1, 1}}, n{{1, 2}, {1, 1}};
  static_assert(std::is_same_v<decltype(m <=> n), std::strong_ordering>);
  return m == n;
}

constexpr bool transparent() {
  std::flat_map<long, int, std::less<>> m{{1, 10}, {3, 30}};
  if (m.find(3) == m.end() || !m.contains(1) || m.count(2) != 0 || m.lower_bound(2) != m.find(3)) return false;
  if (m.erase(1) != 1 || m.size() != 1) return false;
  m[5] = 50;
  return m.at(5) == 50 && m.try_emplace(5, 0).second == false;
}

static_assert(compare());
static_assert(transparent());

int main() {
  CHECK(compare());
  CHECK(transparent());
  return 0;
}
