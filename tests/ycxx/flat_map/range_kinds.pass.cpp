// [flat.map.overview]/2, [flat.multimap.overview]: flat_map and flat_multimap meet the
// associative container requirements for X(from_range, rg[, c]) and a.insert_range(rg)
// ([associative.reqmts.general]/29-34, /80-83), with [flat.map.modifiers]/11-15 (insert_range
// converts each element to value_type, appends, sorts the new ones, merges, and for flat_map
// erases the later duplicates; insert_range(sorted_unique, rg) is equivalent) and
// [flat.multimap.defn] (insert_range(sorted_equivalent, rg)), for every
// container-compatible-range ([container.intro.reqmts]/2). The generic check
// support/reqs/assoc_range_kinds.hpp: sized input-only ranges, approximately-sized ranges with
// a wrong hint, plain input ranges, prvalue ranges, move-only and non-const-only views,
// pair-like elements (tuple, array). Also with deque as the underlying containers
// ([flat.map.overview]/7). Every member is constexpr. (Move-only mapped types:
// range_kinds_move_only.pass.cpp.)
#include <flat_map>
#include <deque>
#include <functional>
#include <vector>
#include "container_values.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;

using DM = std::flat_map<int, Elem, std::less<int>, std::deque<int>, std::deque<Elem>>;
using DMM = std::flat_multimap<Elem, int, std::less<Elem>, std::deque<Elem>, std::deque<int>>;

static_assert(copyable<std::flat_map<int, int>>());
static_assert(copyable<std::flat_multimap<int, int>>());

int main() {
  CHECK(copyable<std::flat_map<int, int>>());
  CHECK(copyable<std::flat_map<Elem, Elem>>());
  CHECK(copyable<std::flat_multimap<int, Elem>>());
  CHECK(copyable<std::flat_multimap<Elem, long>>());
  CHECK(copyable<DM>());
  CHECK(copyable<DMM>());
  return 0;
}
