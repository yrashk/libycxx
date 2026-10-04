// [map.overview]/2, [multimap.overview]/2: map and multimap meet the associative container
// requirements, including the range members X(from_range, rg[, c]) and a.insert_range(rg)
// ([associative.reqmts.general]/29-34, /80-83) for every container-compatible-range
// ([container.intro.reqmts]/2). The generic check support/reqs/assoc_range_kinds.hpp: sized
// input-only ranges, approximately-sized ranges with a wrong hint, plain input ranges,
// prvalue ranges, move-only and non-const-only views, pair-like elements (tuple, array) and,
// with a move-only mapped type, views::as_rvalue and a transform producing prvalues.
#include <map>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;

int main() {
  CHECK(copyable<std::map<int, int>>());
  CHECK(copyable<std::map<Elem, Elem>>());
  CHECK(copyable<std::multimap<int, Elem>>());
  CHECK(copyable<std::multimap<Elem, long>>());
  CHECK(move_only<std::map<int, MOElem>>());
  CHECK(move_only<std::multimap<Elem, MOElem>>());
  return 0;
}
