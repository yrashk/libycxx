// [vector.cons]/10: vector(first, last) with forward iterators makes "no reallocations"
// (exactly one allocation for a non-empty range). [vector.cons]/13: vector(from_range, rg)
// performs no reallocations if R is a forward range or is approximately sized.
// [vector.modifiers]/3: insert_range / append_range / insert(p, i, j) perform at most one
// reallocation for a forward or sized range. [vector.capacity]/7: no reallocation during
// insertions after reserve() until the size would exceed capacity().
// REQUIRES: exceptions
#include <vector>
#include <list>
#include <ranges>
#include "test_allocators.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

using V = std::vector<int, CountingAlloc<int>>;

int main() {
  int a[100];
  for (int i = 0; i < 100; ++i) a[i] = i;
  std::list<int> l(a, a + 100);
  {
    int before = alloc_counters.allocations;
    V v(l.begin(), l.end());
    CHECK(alloc_counters.allocations - before == 1 && v.size() == 100);
  }
  {
    int before = alloc_counters.allocations;
    V v(std::from_range, ForwardRange<int>{a, a + 100});  // forward, not sized
    CHECK(alloc_counters.allocations - before == 1 && v.size() == 100 && v[99] == 99);
  }
  {
    int before = alloc_counters.allocations;
    V v(std::from_range, std::views::iota(0, 100));  // sized
    CHECK(alloc_counters.allocations - before == 1 && v.size() == 100);
  }
  {
    V v{1, 2, 3};
    int before = alloc_counters.allocations;
    v.insert_range(v.begin() + 1, ForwardRange<int>{a, a + 100});
    CHECK(alloc_counters.allocations - before <= 1 && v.size() == 103 && v[100] == 99);
    before = alloc_counters.allocations;
    v.append_range(l);
    CHECK(alloc_counters.allocations - before <= 1 && v.size() == 203);
    before = alloc_counters.allocations;
    v.insert(v.end(), l.begin(), l.end());
    CHECK(alloc_counters.allocations - before <= 1 && v.size() == 303);
  }
  {
    V v;
    v.reserve(500);
    int before = alloc_counters.allocations;
    while (v.size() < v.capacity()) v.push_back(1);
    CHECK(alloc_counters.allocations == before && v.size() >= 500);
    v.insert(v.begin(), 0);  // now exceeds capacity: reallocates once
    CHECK(alloc_counters.allocations - before == 1);
  }
  CHECK(alloc_counters.outstanding == 0);
  return 0;
}
