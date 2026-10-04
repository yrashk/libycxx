// [container.reqmts]/64 and Note 2, [allocator.requirements.general]/2-5, [set.overview], [multiset.overview]:
// set and multiset work with an allocator whose pointer type is a class type
// (support/fancy_ptr.hpp); pointer / const_pointer are the allocator's. The generic check
// support/reqs/fancy_alloc_assoc.hpp (test): construction, copy / move / assignment / swap,
// insertion and erasure (rebalancing / rehashing), iteration, lookups, erase_if, the bucket
// interface (unordered), clear. Node handles and merge: fancy_pointer_node_handle.pass.cpp. (Run time only: constexpr is covered by
// associative_reqs / unord_reqs.)
#include <set>
#include <functional>
#include "container_values.hpp"
#include "fancy_ptr.hpp"
#include "reqs/fancy_alloc_assoc.hpp"
#include "check.hpp"

using reqs::assoc::VHash;
using reqs::fancy_alloc_assoc::test;

int main() {
  CHECK((test<std::set<int, std::less<int>, FancyAlloc<int>>>()));
  CHECK((test<std::multiset<Elem, std::less<Elem>, FancyAlloc<Elem>>>()));
  return 0;
}
