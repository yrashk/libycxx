// [flat.map.modifiers], [flat.map.access], [associative.reqmts.general]/47-74: flat_map /
// flat_multimap with a move-only mapped type (support/move_only_elem.hpp): emplace,
// emplace_hint, insert(t&&), insert(p, t&&), try_emplace, insert_or_assign, operator[],
// move construction / assignment, swap, extract() && / replace (the containers are moved,
// [flat.map.modifiers]/35-38), erase, clear. Nothing used requires a copyable mapped type.
// Also with deque as the underlying containers ([flat.map.overview]/7). Every member is
// constexpr. The generic check support/reqs/assoc_move_only.hpp.
#include <flat_map>
#include <deque>
#include <functional>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_move_only.hpp"
#include "check.hpp"

using DM = std::flat_map<int, MOElem, std::less<int>, std::deque<int>, std::deque<MOElem>>;

static_assert(reqs::assoc_move_only::test<std::flat_map<int, MOElem>>());
static_assert(reqs::assoc_move_only::test<std::flat_multimap<int, MOElem>>());

int main() {
  CHECK((reqs::assoc_move_only::test<std::flat_map<int, MOElem>>()));
  CHECK((reqs::assoc_move_only::test<std::flat_map<Elem, MOElem>>()));
  CHECK((reqs::assoc_move_only::test<std::flat_multimap<int, MOElem>>()));
  CHECK(reqs::assoc_move_only::test<DM>());
  return 0;
}
