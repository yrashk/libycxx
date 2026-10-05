// [flat.set.overview]/5-6, [flat.multiset.overview]: if a member function of flat_set /
// flat_multiset exits via an exception, the invariant (the keys are sorted with respect to
// the comparison object) of the object argument -- and for the move constructor and move
// assignment of both arguments -- is restored. The generic check
// support/reqs/flat_except.hpp arms a failure of an element copy or move, of a comparison or
// of an allocation after 0, 1, 2, ... steps of insert / emplace / emplace_hint / insert(first,
// last) / insert_range / erase / erase_if / copy and move construction and assignment, and
// checks the invariant (and that no element leaked, [res.on.exception.handling]/3) after
// every failure. Also with deque as the underlying container.
// REQUIRES: exceptions
#include <flat_set>
#include <deque>
#include <vector>
#include "test_allocators.hpp"
#include "reqs/flat_except.hpp"
#include "check.hpp"

using namespace reqs::flat_except;

int main() {
  bool ok = true;  // run every instantiation, so that every violation is reported
  ok = test<std::flat_set<FKey, FLess, std::vector<FKey, CountingAlloc<FKey>>>>() && ok;
  ok = test<std::flat_multiset<FKey, FLess, std::vector<FKey, CountingAlloc<FKey>>>>() && ok;
  ok = test<std::flat_set<FKey, FLess, std::deque<FKey, CountingAlloc<FKey>>>>() && ok;
  CHECK(ok);
  CHECK(live == 0);
  return 0;
}
