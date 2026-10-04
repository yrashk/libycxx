// [unord.map.overview]/2, [unord.multimap.overview]/2: unordered_map and unordered_multimap are allocator-aware containers
// ([container.alloc.reqmts]). The generic check support/reqs/allocator_construct_assoc.hpp
// ([container.alloc.reqmts]/2 and its Note 2, [container.reqmts]/64): every key and mapped
// object is constructed with allocator_traits<A>::construct and destroyed with
// allocator_traits<A>::destroy -- across the constructors (also allocator-extended move with
// an unequal allocator), assignments, insert(t) / insert(p, t), emplace / emplace_hint / insert(P&&) (also with a duplicate key), try_emplace,
// insert_or_assign, operator[], rehash / reserve,
// range insertion, erasure, node handles, merge, erase_if, swap and clear.
#include <unordered_map>
#include <functional>
#include "reqs/allocator_construct_assoc.hpp"
#include "check.hpp"

using namespace reqs::allocator_construct_assoc;

int main() {
  CHECK((test<std::unordered_map<Tracked, Tracked, TrackedHash, std::equal_to<Tracked>, ConstructAlloc<std::pair<const Tracked, Tracked>>>>()));
  CHECK((test<std::unordered_multimap<Tracked, Tracked, TrackedHash, std::equal_to<Tracked>, ConstructAlloc<std::pair<const Tracked, Tracked>>>>()));
  return 0;
}
