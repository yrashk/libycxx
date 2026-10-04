// hive with a move-only element type (support/move_only_elem.hpp): every member used needs
// only Cpp17EmplaceConstructible / Cpp17MoveInsertable / Cpp17MoveAssignable / Cpp17Swappable
// elements. [hive.modifiers]/1-6: emplace, insert(T&&); /7-9: insert_range over
// views::as_rvalue (Cpp17EmplaceInsertable from *ranges::begin(rg)); [hive.capacity]/8-11:
// shrink_to_fit (Cpp17MoveInsertable), /12-14 trim_capacity, /23-27 reshape
// (Cpp17MoveInsertable; size() unchanged; block_capacity_limits() becomes the new limits;
// reallocated elements keep their values); [hive.operations]/2-6 splice, /7-12 unique,
// /13-16 sort (Cpp17MoveInsertable, Cpp17MoveAssignable, Cpp17Swappable; O(N log N)
// comparisons, checked against 4 N log2 N + 4 N); move construction / assignment and swap
// ([container.reqmts], [hive.cons]); erase_if ([hive.erasure]).
#include <hive>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>
#include "move_only_elem.hpp"
#include "check.hpp"

using H = std::hive<MOElem>;

std::vector<int> sorted_values(const H& h) {
  std::vector<int> v;
  for (const MOElem& e : h) v.push_back(e.value());
  for (std::size_t i = 1; i < v.size(); ++i)  // insertion sort: no <algorithm> needed
    for (std::size_t j = i; j > 0 && v[j - 1] > v[j]; --j) std::swap(v[j - 1], v[j]);
  return v;
}

int main() {
  H h;
  h.emplace(3);
  MOElem m(1);
  h.insert(std::move(m));
  h.insert(h.begin(), MOElem(2));
  MOElem more[3] = {7, 5, 6};
  h.insert_range(std::views::as_rvalue(more));
  CHECK(more[0].value() == -1 && more[2].value() == -1);
  h.insert_range(std::views::iota(8, 10) | std::views::transform([](int i) { return MOElem(i); }));
  CHECK((sorted_values(h) == std::vector<int>{1, 2, 3, 5, 6, 7, 8, 9}));

  // sort and unique
  h.emplace(5);
  h.emplace(5);
  h.sort();
  int prev = -100;
  for (const MOElem& e : h) {
    CHECK(e.value() >= prev);
    prev = e.value();
  }
  CHECK(h.unique() == 2 && h.size() == 8);
  long comparisons = 0;
  H big;
  for (int i = 0; i < 4096; ++i) big.emplace((i * 2654435761u) % 10007);
  big.sort([&](const MOElem& a, const MOElem& b) {
    ++comparisons;
    return a.value() > b.value();
  });
  prev = 1 << 30;
  for (const MOElem& e : big) {
    CHECK(e.value() <= prev);
    prev = e.value();
  }
  CHECK(comparisons <= 4L * 4096 * 12 + 4L * 4096);  // O(N log N), log2(4096) = 12

  // reshape, shrink_to_fit, trim_capacity keep the elements
  const std::hive_limits hl = H::block_capacity_hard_limits();
  std::hive_limits target(hl.min, hl.min);
  if (H::is_within_hard_limits(target)) {
    auto before = sorted_values(h);
    h.reshape(target);
    CHECK(h.block_capacity_limits().min == hl.min && h.block_capacity_limits().max == hl.min);
    CHECK(sorted_values(h) == before && h.size() == 8);
  }
  CHECK(std::erase_if(h, [](const MOElem& e) { return e.value() < 8; }) == 6);  // order may have changed
  h.shrink_to_fit();
  h.trim_capacity();
  CHECK(h.size() == 2 && h.capacity() >= 2);

  // splice, move, swap
  H other(h.block_capacity_limits());  // its blocks must be within h's limits ([hive.operations]/4)
  other.emplace(42);
  h.splice(other);
  CHECK(other.empty() && h.size() == 3);
  H moved(std::move(h));
  H assigned;
  assigned = std::move(moved);
  H s;
  s.emplace(1);
  assigned.swap(s);
  swap(assigned, s);
  CHECK(assigned.size() == 3 && s.size() == 1);
  CHECK((sorted_values(assigned) == std::vector<int>{8, 9, 42}));
  return 0;
}
