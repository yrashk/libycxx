// [unord.req.except]/1-4 for unordered_map and unordered_multimap, with an equality predicate,
// an element type and an allocator that can each be armed to throw (the hash function does
// not throw; support/reqs/assoc_except.hpp): a failed single-element insert / emplace /
// emplace_hint / try_emplace / insert_or_assign / operator[] / insert(nh) has no effect (same
// elements at the same addresses in the same order, same bucket_count(), nothing leaked, a
// node handle keeps its element), also when the insertion has to rehash; a failed rehash() /
// reserve() has no effect; clear, erase(k), erase(q), extract and swap throw nothing when
// Hash and Pred do not, even with every allocation and element construction failing;
// inserting a range keeps the basic guarantee ([res.on.exception.handling]/3).
// REQUIRES: exceptions
#include <unordered_map>
#include "test_allocators.hpp"
#include "reqs/assoc_except.hpp"
#include "check.hpp"

using namespace reqs::assoc_except;

using A = CountingAlloc<std::pair<const TKey, TKey>>;
using M = std::unordered_map<TKey, TKey, TKeyHash, ThrowEq, A>;
using MM = std::unordered_multimap<TKey, TKey, TKeyHash, ThrowEq, A>;

int main() {
  CHECK(single_element_insertion<M>());
  CHECK(single_element_insertion<MM>());
  CHECK(non_throwing_members<M>());
  CHECK(non_throwing_members<MM>());
  CHECK(range_insertion_basic<M>());
  CHECK(range_insertion_basic<MM>());
  CHECK(rehash_no_effect<M>());
  CHECK(rehash_no_effect<MM>());
  CHECK(live == 0);
  return 0;
}
