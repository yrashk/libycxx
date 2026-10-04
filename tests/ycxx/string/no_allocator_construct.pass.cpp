// [string.require]/3 note 1: basic_string "does not use the allocator's construct and destroy
// member functions"; normatively, [container.alloc.reqmts]/2: for a specialization of
// basic_string the insertion terms are defined as if A were allocator<T>. Storage is
// still obtained from the allocator ([string.require]/3), and everything allocated is
// returned.
#include <string>
#include "test_allocators.hpp"
#include "check.hpp"

using S = std::basic_string<char, std::char_traits<char>, CountingAlloc<char>>;

int main() {
  {
    S s(500, 'x');
    s.append(1000, 'y');
    s.insert(10, "inserted");
    s.replace(0, 100, 50, 'z');
    s.resize(3000, 'r');
    s.erase(5, 200);
    S t = s;
    t.push_back('!');
    t.shrink_to_fit();
    CHECK(alloc_counters.allocations > 0);
  }
  CHECK(alloc_counters.constructs == 0);
  CHECK(alloc_counters.destroys == 0);
  CHECK(alloc_counters.outstanding == 0);
  CHECK(alloc_counters.allocations == alloc_counters.deallocations);
  return 0;
}
