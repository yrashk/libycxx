// [unord.set.overview]/2, [unord.multiset.overview]/2: unordered_set and unordered_multiset
// meet the unordered associative container requirements ([unord.req]) for unique and
// equivalent keys. The generic checks in support/reqs/unordered.hpp: member types; the
// constructors with bucket count / hash function / predicate (copies carry the hash
// function, predicate and max_load_factor, which starts at 1.0); emplace / insert results;
// adjacency of equivalent keys; find, count, contains, equal_range; erase forms keeping
// the order of the remaining elements; the bucket interface, load factor, rehash and
// reserve; reference and iterator validity across insertion; == across different bucket
// counts and insertion orders; erase_if. Every member is constexpr (with a constexpr hash).
#include <unordered_set>
#include "container_values.hpp"
#include "reqs/unordered.hpp"
#include "check.hpp"

using reqs::unordered::Hash;
using reqs::unordered::Eq;

template <class X>
constexpr bool all() {
  using namespace reqs::unordered;
  return types<X>() && construct<X>() && insert_emplace<X>() && lookup<X>() && erase<X>() && buckets<X>() &&
         validity<X>() && equality<X>() && erase_if<X>();
}

static_assert(all<std::unordered_set<int, Hash, Eq>>());
static_assert(all<std::unordered_multiset<Elem, Hash, Eq>>());

int main() {
  CHECK(all<std::unordered_set<int, Hash, Eq>>());
  CHECK(all<std::unordered_set<Elem, Hash, Eq>>());
  CHECK(all<std::unordered_set<long, Hash, Eq>>());
  CHECK(all<std::unordered_multiset<int, Hash, Eq>>());
  CHECK(all<std::unordered_multiset<Elem, Hash, Eq>>());
  CHECK(reqs::unordered::types<std::unordered_set<int>>());
  CHECK(reqs::unordered::types<std::unordered_multiset<long>>());
  return 0;
}
