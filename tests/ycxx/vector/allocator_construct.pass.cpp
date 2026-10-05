// [container.alloc.reqmts]/2 note 2: "A container calls allocator_traits<A>::construct(m, p,
// args) to construct an element at p using args, with m == get_allocator()", and elements
// are destroyed with allocator_traits<A>::destroy. [container.reqmts]/64: all memory is
// obtained through the allocator; [container.reqmts]/25: the destructor destroys every
// element and deallocates all memory.
// REQUIRES: exceptions
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"

struct Tracked {
  static inline int live = 0;
  int v;
  Tracked(int x = 0) : v(x) { ++live; }
  Tracked(const Tracked& o) : v(o.v) { ++live; }
  Tracked(Tracked&& o) noexcept : v(o.v) { ++live; }
  Tracked& operator=(const Tracked&) = default;
  Tracked& operator=(Tracked&&) = default;
  ~Tracked() { --live; }
};

int main() {
  {
    std::vector<Tracked, CountingAlloc<Tracked>> v;
    v.emplace_back(1);
    CHECK(alloc_counters.constructs >= 1);
    int c = alloc_counters.constructs;
    v.reserve(v.capacity() + 10);  // relocating the element goes through construct too
    CHECK(alloc_counters.constructs >= c + 1);
    c = alloc_counters.constructs;
    v.push_back(Tracked(2));
    v.emplace_back(3);
    v.insert(v.end(), 2, Tracked(4));
    CHECK(alloc_counters.constructs >= c + 4);
    std::vector<Tracked, CountingAlloc<Tracked>> w(3);
    v.resize(8);
    CHECK(v.size() == 8 && w.size() == 3);
    CHECK(alloc_counters.constructs - alloc_counters.destroys == Tracked::live);
  }
  CHECK(Tracked::live == 0);
  CHECK(alloc_counters.constructs == alloc_counters.destroys);
  CHECK(alloc_counters.outstanding == 0);
  CHECK(alloc_counters.allocations == alloc_counters.deallocations);
  return 0;
}
