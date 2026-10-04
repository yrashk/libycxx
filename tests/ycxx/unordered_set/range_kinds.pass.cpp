// [unord.set.overview]/2, [unord.multiset.overview]/2: unordered_set and unordered_multiset
// meet the unordered associative container requirements, including X(from_range, rg[, n[,
// hf[, eq]]]) and a.insert_range(rg) ([unord.req.general]) for every
// container-compatible-range ([container.intro.reqmts]/2). The generic check
// support/reqs/assoc_range_kinds.hpp: sized input-only ranges, approximately-sized ranges with
// a wrong hint, plain input ranges, prvalue ranges, move-only and non-const-only views,
// ranges whose reference type only converts to the key, and, with a move-only key type,
// views::as_rvalue and a transform producing prvalues.
#include <unordered_set>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;
using reqs::assoc::VHash;

int main() {
  CHECK(copyable<std::unordered_set<int>>());
  CHECK((copyable<std::unordered_set<Elem, VHash>>()));
  CHECK(copyable<std::unordered_multiset<int>>());
  CHECK((copyable<std::unordered_multiset<Elem, VHash>>()));
  CHECK((move_only<std::unordered_set<MOElem, VHash>>()));
  CHECK((move_only<std::unordered_multiset<MOElem, VHash>>()));
  return 0;
}
