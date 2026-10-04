// [flat.map.modifiers]/11-13: insert_range(rg) adds the elements "as if by
// ranges::for_each(rg, [&](value_type e) { c.keys.insert(c.keys.end(), std::move(e.first));
// c.values.insert(c.values.end(), std::move(e.second)); })", then sorts the new elements,
// merges them with the existing ones and removes duplicates; X(from_range, rg) inserts each
// element of rg ([associative.reqmts.general]/29-34, [flat.map.defn]: flat_map(from_range_t,
// R&&) calls insert_range). Nothing in [flat.map] requires a copyable mapped type: with a
// move-only mapped type, ranges of rvalues (views::as_rvalue) and of prvalues (a transform)
// are container-compatible ([container.intro.reqmts]/2), and sorting / merging need only
// move the elements ([alg.sort], [alg.merge]: sortable, i.e. indirectly_movable_storable).
// The generic check support/reqs/assoc_range_kinds.hpp (move_only).
#include <flat_map>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_range_kinds.hpp"
#include "check.hpp"

using namespace reqs::assoc_range_kinds;

static_assert(move_only<std::flat_map<int, MOElem>>());

int main() {
  CHECK(move_only<std::flat_map<int, MOElem>>());
  CHECK(move_only<std::flat_multimap<Elem, MOElem>>());
  return 0;
}
