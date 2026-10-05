// [hive.operations]/3-4: splice(x): "If an exception is thrown, there are no effects";
// "Throws: length_error if any of x's active blocks are not within the bounds of
// current-limits" (an element block is within the bounds of limits when min <= its capacity
// <= max, [hive.overview]/5.5); when it succeeds, x becomes empty and pointers, references
// and iterators to the moved elements now refer to them as members of *this.
// [hive.modifiers]/2: emplace: "If an exception is thrown, there are no effects" -- checked
// with an element constructor and an allocator that can be armed to fail.
// [hive.cons], [hive.capacity]/15: a hive constructed with limits keeps them as
// block_capacity_limits(); [hive.overview]/5.4: limits are usable only when
// is_within_hard_limits(limits) (otherwise erroneous), so the limits are chosen from
// block_capacity_hard_limits().
// REQUIRES: exceptions
#include <hive>
#include <cstddef>
#include <iterator>
#include <new>
#include <stdexcept>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"

template <class H>
std::vector<const void*> addresses(const H& h) {
  std::vector<const void*> r;
  for (auto& e : h) r.push_back(&e);
  return r;
}

struct Boom {};
inline int ctor_countdown = -1;
struct T {
  int v;
  T(int x) : v(x) {
    if (ctor_countdown == 0) throw Boom();
    if (ctor_countdown > 0) --ctor_countdown;
  }
};

int main() {
  using H = std::hive<int>;
  const std::hive_limits hl = H::block_capacity_hard_limits();
  const std::size_t small = hl.min;
  const std::size_t big = hl.min * 4;
  const std::hive_limits ls(small, small), lb(big, big);
  CHECK(H::is_within_hard_limits(ls));
  if (H::is_within_hard_limits(lb)) {
    // x's only active block has capacity big, outside a's limits [small, small]
    H a(ls);
    H x(lb);
    for (int i = 0; i < 3; ++i) a.insert(i);
    x.insert(100);
    CHECK(a.block_capacity_limits().max == small && x.block_capacity_limits().min == big);
    auto aa = addresses(a);
    auto xa = addresses(x);
    bool thrown = false;
    try {
      a.splice(x);
    } catch (const std::length_error&) {
      thrown = true;
    }
    CHECK(thrown);
    CHECK(addresses(a) == aa && addresses(x) == xa && a.size() == 3 && x.size() == 1);  // no effects
    // the other way round x's blocks are within the bounds [big, big]? a's are not: also throws
    thrown = false;
    try {
      x.splice(a);
    } catch (const std::length_error&) {
      thrown = true;
    }
    CHECK(thrown && addresses(a) == aa && addresses(x) == xa);
    // compatible limits: x's blocks (capacity big) are within [small, big]
    H c(std::hive_limits(small, big));
    c.insert(7);
    auto it = x.begin();
    const int* p = &*it;
    c.splice(x);
    CHECK(x.empty() && c.size() == 2 && &*it == p && *it == 100);
    int seen = 0;
    for (auto j = c.begin(); j != c.end(); ++j) seen += (j == it);
    CHECK(seen == 1);
  }

  // emplace: no effects when the element's constructor or the allocator throws
  std::hive<T, CountingAlloc<T>> h;
  for (int i = 0; i < 5; ++i) h.emplace(i);
  for (int mode = 0; mode < 2; ++mode) {
    for (int n = 0;; ++n) {
      auto before = addresses(h);
      auto size = h.size();
      if (mode == 0) ctor_countdown = n;
      else alloc_counters.fail_after = n;
      bool done = true;
      int emplaced = 0;
      try {
        for (int k = 0; k < 40; ++k, ++emplaced) h.emplace(10 + k);  // forces new element blocks
      } catch (const Boom&) {
        done = false;
      } catch (const std::bad_alloc&) {
        done = false;
      }
      ctor_countdown = -1;
      alloc_counters.fail_after = -1;
      if (done) break;
      // the emplacements before the failing one took effect; the failing one did not
      auto after = addresses(h);
      CHECK(h.size() == size + static_cast<std::size_t>(emplaced));  // the failing one: no effect
      std::size_t kept = 0;
      for (const void* p : before)
        for (const void* q : after) kept += (p == q);
      CHECK(kept == before.size());  // nothing moved or lost
      std::size_t count = 0;
      for (auto& e : h) count += (e.v >= 0);
      CHECK(count == h.size());
      while (h.size() > 5) h.erase(std::prev(h.end()));
      CHECK(n < 1000);
    }
  }
  return 0;
}
