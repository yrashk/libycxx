// [map.overview] synopsis: merge takes only map<Key, T, C2, Allocator> and
// multimap<Key, T, C2, Allocator> -- any comparison object C2 but the same allocator type
// ([associative.reqmts.general]/112-117, [container.node.overview] Table 75: compatible
// nodes need the same allocator). A source with another allocator type is ill-formed (with
// another comparator it is fine: map/node_handle.pass.cpp merges map<int, Elem, greater>).
#include <map>
#include <functional>
#include <utility>
#include "test_allocators.hpp"

int main() {
  std::map<int, int> a;
  std::map<int, int, std::greater<int>, MinimalAlloc<std::pair<const int, int>>> b;
  a.merge(b);
  return 0;
}
