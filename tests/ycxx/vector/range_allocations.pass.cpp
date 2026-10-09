// Normative: [vector.cons]/10, /12-13 and [vector.modifiers]/3 bound ELEMENT-STORAGE
// reallocations, not all allocator calls. For a range R, the guarantee applies to a forward
// range that is not approximately sized, or to an approximately sized range with
// distance(rg) <= reserve_hint(rg). [vector.capacity]/7 preserves storage during insertions
// after reserve until size would exceed capacity.
// Libycxx policy: these paths use exactly one allocator call to construct nonempty storage,
// at most one for forward-range insertion, and none while reserved capacity suffices. These
// allocator-call checks include any auxiliary buffers and are stronger than the draft.
// Normative contents, exactly-once dereferencing and reserved-storage stability are checked
// separately below. Allocation counts are performance regressions, not portable oracles.
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
    int derefs = 0;
    V v(std::from_range, ForwardRange<int>{a, a + 100, &derefs});  // forward, not sized
    CHECK(alloc_counters.allocations - before == 1 && v.size() == 100 && v[99] == 99);
    CHECK(derefs == 100);
    for (int i = 0; i < 100; ++i) CHECK(v[i] == i);
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
    const auto capacity = v.capacity();
    const int* storage = v.data();
    while (v.size() < capacity) {
      v.push_back(1);
      CHECK(v.data() == storage && v.capacity() == capacity);
    }
    CHECK(alloc_counters.allocations == before && v.size() >= 500);
    v.insert(v.begin(), 0);  // now exceeds capacity: reallocates once
    CHECK(alloc_counters.allocations - before == 1);
  }
  {
    // Both empty and nonempty forward ranges inserted with sufficient capacity preserve
    // the elements before the insertion point. Count dereferences independently of allocation.
    V v{7, 8, 9};
    v.reserve(v.size() + 100);
    int* first = &v.front();
    const auto capacity = v.capacity();
    int derefs = 0;
    auto empty_pos = v.insert_range(v.begin() + 1, ForwardRange<int>{a, a, &derefs});
    CHECK(empty_pos == v.begin() + 1 && v.size() == 3 && derefs == 0);
    auto pos = v.insert_range(v.begin() + 1, ForwardRange<int>{a, a + 100, &derefs});
    CHECK(pos == v.begin() + 1 && v.size() == 103 && derefs == 100);
    CHECK(&v.front() == first && *first == 7 && v.capacity() == capacity);
    for (int i = 0; i < 100; ++i) CHECK(v[i + 1] == i);
    CHECK(v[101] == 8 && v[102] == 9);
  }
  CHECK(alloc_counters.outstanding == 0);
  return 0;
}
