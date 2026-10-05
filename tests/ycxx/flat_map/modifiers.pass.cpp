// [flat.map.modifiers], [flat.map.access]: insert(first, last) / insert_range keep an
// existing element when a new one has an equivalent key (the new elements are merged after
// the pre-existing ones and only the first of each group is kept); insert(sorted_unique,
// ...) is equivalent; try_emplace leaves *this and the arguments unchanged when the key
// exists; insert_or_assign assigns to the existing mapped value; operator[] inserts a
// value-initialized mapped value; at throws out_of_range; erase(k / position / range);
// swap; erase_if is stable and applies the predicate exactly size() times
// ([flat.map.erasure]). flat_multimap keeps all equivalent elements, newly inserted ones
// after the existing ones ([flat.multimap.overview]/4, [associative.reqmts.general]/4).
// REQUIRES: exceptions
#include <flat_map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include "check.hpp"

constexpr bool map_ops() {
  std::flat_map<int, int> m{{1, 10}, {3, 30}};
  std::pair<int, int> more[] = {{3, 31}, {2, 20}, {0, 0}, {2, 21}};
  m.insert(more, more + 4);
  if (m.keys() != std::vector<int>{0, 1, 2, 3} || m.at(3) != 30) return false;
  if (m.at(2) != 20 && m.at(2) != 21) return false;
  m.insert_range(std::vector<std::pair<int, int>>{{1, 11}, {4, 40}});
  if (m.at(1) != 10 || m.at(4) != 40 || m.size() != 5) return false;
  m.insert(std::sorted_unique, {std::pair{4, 41}, std::pair{5, 50}});
  if (m.at(4) != 40 || m.at(5) != 50) return false;
  auto [it, ins] = m.try_emplace(5, 55);
  if (ins || it->second != 50) return false;
  auto [it2, ins2] = m.insert_or_assign(5, 56);
  if (ins2 || it2->second != 56 || m.at(5) != 56) return false;
  m[9] = 90;
  if (m[7] != 0 || m.size() != 8 || m.keys().back() != 9) return false;
  if (m.erase(7) != 1 || m.erase(7) != 0) return false;
  auto e = m.erase(m.begin());
  if (e != m.begin() || e->first != 1) return false;
  e = m.erase(m.begin() + 1, m.begin() + 3);
  if (e->first != 4 || m.keys() != std::vector<int>{1, 4, 5, 9}) return false;
  std::flat_map<int, int> o{{100, 1}};
  m.swap(o);
  if (m.size() != 1 || o.size() != 4) return false;
  swap(m, o);
  int calls = 0;
  auto n = std::erase_if(m, [&](const auto& p) {
    ++calls;
    return p.first % 2 == 1;
  });
  if (n != 3 || calls != 4 || m.keys() != std::vector<int>{4} || m.values() != std::vector<int>{40}) return false;
  int caught = 0;
  if !consteval {
    try {
      (void)m.at(8);
    } catch (const std::out_of_range&) {
      ++caught;
    }
    try {
      (void)std::as_const(m).at(8);
    } catch (const std::out_of_range&) {
      ++caught;
    }
  } else {
    caught = 2;
  }
  return caught == 2;
}

constexpr bool multimap_ops() {
  std::flat_multimap<int, int> m{{1, 10}, {3, 30}};
  auto i = m.emplace(3, 31);
  if (i != m.begin() + 2 || m.values() != std::vector<int>{10, 30, 31}) return false;
  std::pair<int, int> more[] = {{3, 32}, {1, 11}};
  m.insert(more, more + 2);
  if (m.keys() != std::vector<int>{1, 1, 3, 3, 3} || m.values() != std::vector<int>{10, 11, 30, 31, 32}) return false;
  m.insert(std::sorted_equivalent, {std::pair{0, 0}, std::pair{3, 33}});
  if (m.values() != std::vector<int>{0, 10, 11, 30, 31, 32, 33}) return false;
  if (m.erase(3) != 4 || m.size() != 3) return false;
  auto n = std::erase_if(m, [](const auto& p) { return p.second == 10; });
  return n == 1 && m.values() == std::vector<int>{0, 11};
}

static_assert(map_ops());
static_assert(multimap_ops());

int main() {
  CHECK(map_ops());
  CHECK(multimap_ops());
  return 0;
}
