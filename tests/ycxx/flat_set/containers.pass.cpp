// [flat.set.cons]/1: flat_set(cont[, comp]) adopts cont, sorts it and keeps only the first of
// each group of equivalent elements; flat_set(sorted_unique, cont) adopts it unchanged.
// [flat.multiset.cons]/1: flat_multiset(cont) sorts and keeps duplicates.
// [flat.set.modifiers]/5, [flat.multiset.modifiers]/4: insert(first, last) appends, sorts
// the new elements and merges them after the existing ones; for flat_set "erases all but the
// first element from each group of consecutive equivalent elements", so an existing element
// wins over an equivalent new one; flat_multiset keeps the existing ones first.
// /16-19: extract() && returns the container and empties the set; replace(cont) installs it.
// [flat.set.erasure]: erase_if is stable and calls the predicate exactly size() times.
#include <flat_set>
#include <deque>
#include <functional>
#include <utility>
#include <vector>
#include "check.hpp"

// equal when the tens digit is equal; the units digit distinguishes equivalent elements
struct Tens {
  constexpr bool operator()(int a, int b) const { return a / 10 < b / 10; }
};

constexpr bool flat_set() {
  std::flat_set<int> s(std::vector<int>{5, 1, 3, 1, 5});
  if (std::move(s).extract() != std::vector<int>{1, 3, 5} || !s.empty()) return false;
  std::flat_set<int> u(std::sorted_unique, std::vector<int>{2, 4, 6});
  if (u.size() != 3 || *u.begin() != 2) return false;
  std::flat_set<int, Tens> t{12, 31};
  int more[] = {35, 18, 47, 41};
  t.insert(more, more + 4);
  std::vector<int> c = std::move(t).extract();
  if (c != std::vector<int>{12, 31, 41} && c != std::vector<int>{12, 31, 47}) return false;
  if (c[0] != 12 || c[1] != 31) return false;  // the pre-existing elements were kept
  t.replace(std::move(c));
  if (t.size() != 3 || !t.contains(45)) return false;
  t.insert(std::sorted_unique, {50, 60});
  if (t.size() != 5 || !t.contains(59)) return false;
  int calls = 0;
  auto n = std::erase_if(t, [&](int x) { return ++calls, x % 2 == 1; });
  return n == 2 && calls == 5 && t.size() == 3 && *t.begin() == 12;
}

constexpr bool flat_multiset() {
  std::flat_multiset<int, Tens> m(std::vector<int>{31, 12, 35});
  if (m.size() != 3 || *m.begin() != 12) return false;
  int more[] = {38, 14};
  m.insert(more, more + 2);
  std::vector<int> c = std::move(m).extract();
  if (!m.empty() || c.size() != 5 || c[0] != 12 || c[1] != 14 || c[4] != 38) return false;
  if (!((c[2] == 31 && c[3] == 35) || (c[2] == 35 && c[3] == 31))) return false;
  m.replace(std::move(c));
  auto it = m.emplace(33);  // after the equivalent elements
  if (it != m.begin() + 5 || m.count(30) != 4) return false;
  std::flat_multiset<int> e(std::sorted_equivalent, std::vector<int>{1, 1, 2});
  return e.count(1) == 2;
}

bool deque_container() {
  std::flat_set<int, std::less<int>, std::deque<int>> d(std::deque<int>{3, 1, 3});
  std::flat_multiset<int, std::less<int>, std::deque<int>> m(std::deque<int>{3, 1, 3});
  return d.size() == 2 && m.size() == 3 && std::move(d).extract() == std::deque<int>{1, 3};
}

static_assert(flat_set());
static_assert(flat_multiset());

int main() {
  CHECK(flat_set());
  CHECK(flat_multiset());
  CHECK(deque_container());
  return 0;
}
