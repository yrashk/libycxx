// [container.node.overview], [container.node.cons]: a node handle holds the node through
// allocator_traits<allocator_type>::rebind_traits<container-node-type>::pointer, so node
// handles must work with an allocator whose pointer type is a class type
// (support/fancy_ptr.hpp); [container.reqmts]/64 Note 2.
// For unordered_map and unordered_multimap ([unord.map.overview], [unord.multimap.overview]):
// extract(k), extract(q), insert(nh), insert(p, nh) and merge, through the generic check
// support/reqs/fancy_alloc_assoc.hpp (node_handles).
#include <unordered_map>
#include <functional>
#include "container_values.hpp"
#include "fancy_ptr.hpp"
#include "reqs/fancy_alloc_assoc.hpp"
#include "check.hpp"

using reqs::assoc::VHash;
using reqs::fancy_alloc_assoc::node_handles;

int main() {
  CHECK((node_handles<std::unordered_map<int, Elem, VHash, std::equal_to<int>, FancyAlloc<std::pair<const int, Elem>>>>()));
  CHECK((node_handles<std::unordered_multimap<Elem, int, VHash, std::equal_to<Elem>, FancyAlloc<std::pair<const Elem, int>>>>()));
  return 0;
}
