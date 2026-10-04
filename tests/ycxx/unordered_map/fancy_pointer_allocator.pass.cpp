// [container.reqmts]/64 and Note 2, [allocator.requirements.general]/2-5, [unord.map.overview], [unord.multimap.overview]:
// unordered_map and unordered_multimap work with an allocator whose pointer type is a class type
// (support/fancy_ptr.hpp); pointer / const_pointer are the allocator's. The generic check
// support/reqs/fancy_alloc_assoc.hpp (test): construction, copy / move / assignment / swap,
// insertion and erasure (rebalancing / rehashing), iteration, lookups, erase_if, the bucket
// interface (unordered), clear. Node handles and merge: fancy_pointer_node_handle.pass.cpp. (Run time only: constexpr is covered by
// associative_reqs / unord_reqs.)
#include <unordered_map>
#include <functional>
#include "container_values.hpp"
#include "fancy_ptr.hpp"
#include "reqs/fancy_alloc_assoc.hpp"
#include "check.hpp"

using reqs::assoc::VHash;
using reqs::fancy_alloc_assoc::test;

int main() {
  CHECK((test<std::unordered_map<int, Elem, VHash, std::equal_to<int>, FancyAlloc<std::pair<const int, Elem>>>>()));
  CHECK((test<std::unordered_multimap<Elem, int, VHash, std::equal_to<Elem>, FancyAlloc<std::pair<const Elem, int>>>>()));
  return 0;
}
