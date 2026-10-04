// [flat.set.overview], [flat.multiset.overview]: flat_set and flat_multiset meet the
// associative container requirements for X(from_range, rg[, c]) and a.insert_range(rg)
// ([associative.reqmts.general]/29-34, /80-83), with [flat.set.modifiers] (insert_range
// appends, sorts the new elements, merges, and for flat_set keeps only the first of each
// group of equivalent elements; insert_range(sorted_unique, rg) is equivalent) and
// [flat.multiset.defn] (insert_range(sorted_equivalent, rg)), for every
// container-compatible-range ([container.intro.reqmts]/2). The generic check
// support/reqs/assoc_range_kinds.hpp: sized input-only ranges, approximately-sized ranges with
// a wrong hint, plain input ranges, prvalue ranges, move-only and non-const-only views,
// ranges whose reference type only converts to the key and, with a move-only key type,
// views::as_rvalue and a transform producing prvalues. Also with deque as the underlying
// container. Every member is constexpr.
#include <flat_set>
#include <deque>
#include <functional>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;

static_assert(copyable<std::flat_set<int>>());
static_assert(copyable<std::flat_multiset<int>>());
static_assert(move_only<std::flat_set<MOElem>>());

int main() {
  CHECK(copyable<std::flat_set<int>>());
  CHECK(copyable<std::flat_set<Elem>>());
  CHECK(copyable<std::flat_multiset<int>>());
  CHECK(copyable<std::flat_multiset<Elem>>());
  CHECK((copyable<std::flat_set<Elem, std::less<Elem>, std::deque<Elem>>>()));
  CHECK((copyable<std::flat_multiset<int, std::less<int>, std::deque<int>>>()));
  CHECK(move_only<std::flat_set<MOElem>>());
  CHECK(move_only<std::flat_multiset<MOElem>>());
  return 0;
}
