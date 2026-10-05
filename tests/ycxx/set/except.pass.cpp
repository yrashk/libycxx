// [associative.reqmts.except]/1-3 for set and multiset, with a comparison object, an element
// type and an allocator that can each be armed to throw (support/reqs/assoc_except.hpp):
// a failed single-element insert / emplace / emplace_hint / insert(nh) has no effect (same
// elements at the same addresses, nothing leaked, a node handle keeps its element); clear,
// erase(k), erase(q), extract and swap throw nothing when the comparison object does not,
// even with every allocation and element construction failing; inserting a range keeps the
// basic guarantee ([res.on.exception.handling]/3).
// REQUIRES: exceptions
#include <set>
#include "test_allocators.hpp"
#include "reqs/assoc_except.hpp"
#include "check.hpp"

using namespace reqs::assoc_except;

using S = std::set<TKey, ThrowLess, CountingAlloc<TKey>>;
using MS = std::multiset<TKey, ThrowLess, CountingAlloc<TKey>>;

int main() {
  CHECK(single_element_insertion<S>());
  CHECK(single_element_insertion<MS>());
  CHECK(non_throwing_members<S>());
  CHECK(non_throwing_members<MS>());
  CHECK(range_insertion_basic<S>());
  CHECK(range_insertion_basic<MS>());
  CHECK(live == 0);
  return 0;
}
