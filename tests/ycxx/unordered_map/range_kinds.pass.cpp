// [unord.map.overview]/2, [unord.multimap.overview]/2: unordered_map and unordered_multimap
// meet the unordered associative container requirements, including X(from_range, rg[, n[,
// hf[, eq]]]) and a.insert_range(rg) ([unord.req.general]) for every
// container-compatible-range ([container.intro.reqmts]/2). The generic check
// support/reqs/assoc_range_kinds.hpp: sized input-only ranges, approximately-sized ranges with
// a wrong hint, plain input ranges, prvalue ranges, move-only and non-const-only views,
// pair-like elements (tuple, array) and, with a move-only mapped type, views::as_rvalue and a
// transform producing prvalues.
#include <unordered_map>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;
using reqs::assoc::VHash;

int main() {
  CHECK(copyable<std::unordered_map<int, int>>());
  CHECK((copyable<std::unordered_map<Elem, Elem, VHash>>()));
  CHECK(copyable<std::unordered_multimap<int, Elem>>());
  CHECK((copyable<std::unordered_multimap<Elem, long, VHash>>()));
  CHECK(move_only<std::unordered_map<int, MOElem>>());
  CHECK((move_only<std::unordered_multimap<Elem, MOElem, VHash>>()));
  return 0;
}
