// [unord.req.general]/115-147, /244 and [container.node] for unordered_map and
// unordered_multimap: extract(q) / extract(k) into a node handle without copying (the key
// can be changed through key()), insert(nh) / insert(q, nh) results, empty handles, failed
// insertion leaving the handle unchanged; merge from containers with compatible nodes
// (Table 75: map or multimap) keeps element addresses and, for unique keys, leaves behind
// the elements whose keys are already present.
#include <unordered_map>
#include "container_values.hpp"
#include "reqs/unordered.hpp"
#include "check.hpp"

using reqs::unordered::Hash;
using reqs::unordered::Eq;
using UM = std::unordered_map<int, int, Hash, Eq>;
using UMM = std::unordered_multimap<int, int, Hash, Eq>;

int main() {
  CHECK(reqs::unordered::node_handles<UM>());
  CHECK(reqs::unordered::node_handles<UMM>());
  CHECK(reqs::unordered::node_handles<std::unordered_map<int, Elem, Hash, Eq>>());
  CHECK((reqs::unordered::merge<UM, UM>()));
  CHECK((reqs::unordered::merge<UM, UMM>()));
  CHECK((reqs::unordered::merge<UMM, UM>()));
  CHECK((reqs::unordered::merge<UMM, UMM>()));
  return 0;
}
