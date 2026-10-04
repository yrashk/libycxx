// [set.overview]/2, [multiset.overview]/2: set and multiset meet the associative container
// requirements, including the range members X(from_range, rg[, c]) and a.insert_range(rg)
// ([associative.reqmts.general]/29-34, /80-83) for every container-compatible-range
// ([container.intro.reqmts]/2). The generic check support/reqs/assoc_range_kinds.hpp: sized
// input-only ranges, approximately-sized ranges with a wrong hint, plain input ranges,
// prvalue ranges, move-only and non-const-only views, ranges whose reference type only
// converts to the key, and, with a move-only key type, views::as_rvalue and a transform
// producing prvalues.
#include <set>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;

int main() {
  CHECK(copyable<std::set<int>>());
  CHECK(copyable<std::set<Elem>>());
  CHECK(copyable<std::multiset<int>>());
  CHECK(copyable<std::multiset<Elem>>());
  CHECK(move_only<std::set<MOElem>>());
  CHECK(move_only<std::multiset<MOElem>>());
  return 0;
}
