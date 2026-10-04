// [unord.req.general]/115-147, /244 and [container.node] for unordered_set and
// unordered_multiset: extract into a node handle without copying (value() can be changed
// before re-insertion), insert(nh) / insert(q, nh) results, empty handles, failed insertion
// leaving the handle unchanged; merge between sets and multisets (Table 75) keeps element
// addresses and, for unique keys, leaves behind the elements already present.
#include <unordered_set>
#include "container_values.hpp"
#include "reqs/unordered.hpp"
#include "check.hpp"

using reqs::unordered::Hash;
using reqs::unordered::Eq;
using US = std::unordered_set<int, Hash, Eq>;
using UMS = std::unordered_multiset<int, Hash, Eq>;

int main() {
  CHECK(reqs::unordered::node_handles<US>());
  CHECK(reqs::unordered::node_handles<UMS>());
  CHECK(reqs::unordered::node_handles<std::unordered_set<Elem, Hash, Eq>>());
  CHECK((reqs::unordered::merge<US, US>()));
  CHECK((reqs::unordered::merge<US, UMS>()));
  CHECK((reqs::unordered::merge<UMS, US>()));
  CHECK((reqs::unordered::merge<UMS, UMS>()));
  return 0;
}
