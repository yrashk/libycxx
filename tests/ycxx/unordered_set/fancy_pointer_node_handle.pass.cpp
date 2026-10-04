// [container.node.overview], [container.node.cons]: a node handle holds the node through
// allocator_traits<allocator_type>::rebind_traits<container-node-type>::pointer, so node
// handles must work with an allocator whose pointer type is a class type
// (support/fancy_ptr.hpp); [container.reqmts]/64 Note 2.
// For unordered_set and unordered_multiset ([unord.set.overview], [unord.multiset.overview]):
// extract(k), extract(q), insert(nh), insert(p, nh) and merge, through the generic check
// support/reqs/fancy_alloc_assoc.hpp (node_handles).
#include <unordered_set>
#include <functional>
#include "container_values.hpp"
#include "fancy_ptr.hpp"
#include "reqs/fancy_alloc_assoc.hpp"
#include "check.hpp"

using reqs::assoc::VHash;
using reqs::fancy_alloc_assoc::node_handles;

int main() {
  CHECK((node_handles<std::unordered_set<int, VHash, std::equal_to<int>, FancyAlloc<int>>>()));
  CHECK((node_handles<std::unordered_multiset<Elem, VHash, std::equal_to<Elem>, FancyAlloc<Elem>>>()));
  return 0;
}
