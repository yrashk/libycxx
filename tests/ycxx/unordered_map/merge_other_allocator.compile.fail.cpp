// [unord.map.overview] synopsis: merge takes only unordered_map<Key, T, H2, P2, Allocator>
// and unordered_multimap<Key, T, H2, P2, Allocator> -- any hash and predicate, but the same
// allocator type ([unord.req.general], [container.node.overview] Table 75). A source with
// another allocator type is ill-formed (with another hash it is fine:
// unordered_map/node_handle.pass.cpp, support/reqs/unordered.hpp merge).
// REQUIRES: exceptions
#include <unordered_map>
#include <functional>
#include <utility>
#include "test_allocators.hpp"

int main() {
  std::unordered_map<int, int> a;
  std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, MinimalAlloc<std::pair<const int, int>>> b;
  a.merge(b);
  return 0;
}
