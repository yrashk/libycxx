// [flat.map.overview]/5-6, [flat.multimap.overview]: if a member function of flat_map /
// flat_multimap exits via an exception, the invariants (as many keys as values, keys sorted,
// the value at each offset belonging to the key at that offset) of the object argument --
// and for the move constructor and move assignment of both arguments -- are restored
// (possibly by emptying the container). The generic check support/reqs/flat_except.hpp
// arms a failure of a key / value copy or move, of a comparison or of an allocation after 0,
// 1, 2, ... steps of insert / emplace / emplace_hint / try_emplace / insert_or_assign /
// operator[] / insert(first, last) / insert_range / erase / erase_if / copy and move
// construction and assignment, and checks the invariants (and that no element leaked,
// [res.on.exception.handling]/3) after every failure. Also with deque as the underlying
// containers ([flat.map.overview]/7).
#include <flat_map>
#include <deque>
#include <vector>
#include "test_allocators.hpp"
#include "reqs/flat_except.hpp"
#include "check.hpp"

using namespace reqs::flat_except;

using VC = std::vector<FKey, CountingAlloc<FKey>>;
using DC = std::deque<FKey, CountingAlloc<FKey>>;

int main() {
  bool ok = true;  // run every instantiation, so that every violation is reported
  ok = test<std::flat_map<FKey, FKey, FLess, VC, VC>>() && ok;
  ok = test<std::flat_multimap<FKey, FKey, FLess, VC, VC>>() && ok;
  ok = test<std::flat_map<FKey, FKey, FLess, DC, VC>>() && ok;
  ok = test<std::flat_multimap<FKey, FKey, FLess, DC, DC>>() && ok;
  CHECK(ok);
  CHECK(live == 0);
  return 0;
}
