// [associative.reqmts.except]/1-3 for map and multimap, with a comparison object, an element
// type and an allocator that can each be armed to throw (support/reqs/assoc_except.hpp):
// a failed single-element insert / emplace / emplace_hint / try_emplace / insert_or_assign /
// operator[] / insert(nh) has no effect (same elements at the same addresses, nothing leaked,
// a node handle keeps its element); clear, erase(k), erase(q), extract and swap throw nothing
// when the comparison object does not, even with every allocation and element construction
// failing; inserting a range keeps the basic guarantee ([res.on.exception.handling]/3).
#include <map>
#include "test_allocators.hpp"
#include "reqs/assoc_except.hpp"
#include "check.hpp"

using namespace reqs::assoc_except;

using M = std::map<TKey, TKey, ThrowLess, CountingAlloc<std::pair<const TKey, TKey>>>;
using MM = std::multimap<TKey, TKey, ThrowLess, CountingAlloc<std::pair<const TKey, TKey>>>;

int main() {
  CHECK(single_element_insertion<M>());
  CHECK(single_element_insertion<MM>());
  CHECK(non_throwing_members<M>());
  CHECK(non_throwing_members<MM>());
  CHECK(range_insertion_basic<M>());
  CHECK(range_insertion_basic<MM>());
  CHECK(live == 0);
  return 0;
}
