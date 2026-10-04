// [flat.map.cons]/1-4: flat_map(key_cont, mapped_cont[, comp]) adopts the containers, sorts
// by key and removes elements with duplicate keys; flat_map(sorted_unique, key_cont,
// mapped_cont) adopts them unchanged. [flat.multimap.cons]: the multimap keeps duplicates;
// sorted_equivalent adopts unchanged. keys() / values() expose the underlying containers,
// which keep the invariant that the value at offset off belongs to the key at offset off
// ([flat.map.overview]/5). [flat.map.modifiers]/35-38: extract() && returns the containers
// and leaves the map empty; replace(keys, values) installs new containers.
#include <flat_map>
#include <deque>
#include <utility>
#include <vector>
#include "check.hpp"

constexpr bool flat_map() {
  std::vector<int> k{5, 1, 3, 1};
  std::vector<int> v{50, 10, 30, 11};
  std::flat_map<int, int> m(std::move(k), std::move(v));
  if (m.keys() != std::vector<int>{1, 3, 5} || m.size() != 3) return false;
  if (m.values()[1] != 30 || m.values()[2] != 50 || (m.values()[0] != 10 && m.values()[0] != 11)) return false;
  // values follow their keys
  for (std::size_t i = 0; i < m.size(); ++i)
    if (m.values()[i] / 10 != m.keys()[i]) return false;
  std::flat_map<int, int> s(std::sorted_unique, std::vector<int>{1, 2, 4}, std::vector<int>{7, 8, 9});
  if (s.keys() != std::vector<int>{1, 2, 4} || s.values() != std::vector<int>{7, 8, 9}) return false;
  if (s.at(4) != 9 || (*(s.begin() + 1)).second != 8) return false;
  std::flat_map<int, int, std::greater<int>> g(std::vector<int>{1, 3, 2}, std::vector<int>{10, 30, 20}, std::greater<int>());
  if (g.keys() != std::vector<int>{3, 2, 1} || g.values() != std::vector<int>{30, 20, 10}) return false;
  // iterator dereference yields references into the containers
  auto it = m.begin();
  if (&(*it).first != &m.keys()[0] || &it->second != &m.values()[0]) return false;
  it->second = 99;
  if (m.values()[0] != 99) return false;
  // extract empties the map
  auto c = std::move(m).extract();
  if (c.keys != std::vector<int>{1, 3, 5} || c.values.size() != 3 || !m.empty() || m.size() != 0) return false;
  c.keys[2] = 6;
  m.replace(std::move(c.keys), std::move(c.values));
  if (m.size() != 3 || !m.contains(6) || m.contains(5) || m.at(6) != 50) return false;
  m.emplace(4, 40);
  return m.keys() == std::vector<int>{1, 3, 4, 6};
}

constexpr bool flat_multimap() {
  std::flat_multimap<int, int> m(std::vector<int>{2, 1, 2}, std::vector<int>{20, 10, 21});
  if (m.keys() != std::vector<int>{1, 2, 2} || m.values()[0] != 10) return false;
  std::flat_multimap<int, int> s(std::sorted_equivalent, std::vector<int>{1, 1, 3}, std::vector<int>{5, 4, 3});
  if (s.values() != std::vector<int>{5, 4, 3}) return false;
  auto c = std::move(s).extract();
  if (!s.empty() || c.keys.size() != 3) return false;
  s.replace(std::move(c.keys), std::move(c.values));
  return s.count(1) == 2;
}

bool deque_containers() {
  std::flat_multimap<int, int, std::less<int>, std::deque<int>, std::deque<int>> d(
      std::deque<int>{3, 1}, std::deque<int>{30, 10});
  std::flat_map<int, int, std::less<int>, std::deque<int>, std::deque<int>> u(
      std::deque<int>{3, 1, 3}, std::deque<int>{30, 10, 31});
  return d.keys().front() == 1 && d.values().back() == 30 && u.size() == 2 && u.keys().back() == 3;
}

static_assert(flat_map());
static_assert(flat_multimap());

int main() {
  CHECK(flat_map());
  CHECK(flat_multimap());
  CHECK(deque_containers());
  return 0;
}
