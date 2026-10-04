// [unord.req.except]/1-4 for unordered_set and unordered_multiset, with an equality
// predicate, an element type and an allocator that can each be armed to throw (the hash
// function does not throw; support/reqs/assoc_except.hpp): a failed single-element insert /
// emplace / emplace_hint / insert(nh) has no effect (same elements at the same addresses in
// the same order, same bucket_count(), nothing leaked, a node handle keeps its element), also
// when the insertion has to rehash; a failed rehash() / reserve() has no effect; clear,
// erase(k), erase(q), extract and swap throw nothing when Hash and Pred do not, even with
// every allocation and element construction failing; inserting a range keeps the basic
// guarantee ([res.on.exception.handling]/3).
#include <unordered_set>
#include "test_allocators.hpp"
#include "reqs/assoc_except.hpp"
#include "check.hpp"

using namespace reqs::assoc_except;

using S = std::unordered_set<TKey, TKeyHash, ThrowEq, CountingAlloc<TKey>>;
using MS = std::unordered_multiset<TKey, TKeyHash, ThrowEq, CountingAlloc<TKey>>;

int main() {
  CHECK(single_element_insertion<S>());
  CHECK(single_element_insertion<MS>());
  CHECK(non_throwing_members<S>());
  CHECK(non_throwing_members<MS>());
  CHECK(range_insertion_basic<S>());
  CHECK(range_insertion_basic<MS>());
  CHECK(rehash_no_effect<S>());
  CHECK(rehash_no_effect<MS>());
  CHECK(live == 0);
  return 0;
}
