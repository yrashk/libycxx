// [container.node.overview]/1, Table 75: unordered_map<K, T, H1, E1, A> and
// unordered_multimap<K, T, H2, E2, A> have compatible nodes for any hash functions H1, H2
// and predicates E1, E2: "Containers with compatible nodes have the same node handle type"
// and elements may be transferred in either direction (insert(node_type&&), merge).
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "reqs/unordered.hpp"
#include "check.hpp"

using reqs::unordered::Hash;
using reqs::unordered::Eq;
using UMM = std::unordered_multimap<int, int, Hash, Eq>;

static_assert(std::is_same_v<std::unordered_map<int, int>::node_type, UMM::node_type>);
static_assert(std::is_same_v<std::unordered_map<int, int, Hash>::node_type,
                             std::unordered_map<int, int, std::hash<int>, Eq>::node_type>);

int main() {
  std::unordered_map<int, int> std_map{{1, 10}, {2, 20}};
  std::unordered_multimap<int, int, Hash, Eq> other;
  auto nh = std_map.extract(1);
  const int* addr = &nh.mapped();
  static_assert(std::is_same_v<decltype(nh), UMM::node_type>);
  auto it = other.insert(std::move(nh));
  CHECK(&it->second == addr && other.size() == 1 && std_map.size() == 1);
  std_map.merge(other);
  CHECK(std_map.size() == 2 && &std_map.at(1) == addr && other.empty());
  return 0;
}
