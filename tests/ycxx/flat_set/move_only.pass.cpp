// [flat.set.modifiers], [associative.reqmts.general]/47-74: flat_set / flat_multiset with a
// move-only key type (support/move_only_elem.hpp): emplace, emplace_hint, insert(t&&),
// insert(p, t&&), move construction / assignment, swap, extract() && / replace (the
// container is moved), erase, clear. Also with deque as the underlying container. Every
// member is constexpr. The generic check support/reqs/assoc_move_only.hpp.
#include <flat_set>
#include <deque>
#include <functional>
#include "move_only_elem.hpp"
#include "reqs/assoc_move_only.hpp"
#include "check.hpp"

static_assert(reqs::assoc_move_only::test<std::flat_set<MOElem>>());
static_assert(reqs::assoc_move_only::test<std::flat_multiset<MOElem>>());

int main() {
  CHECK(reqs::assoc_move_only::test<std::flat_set<MOElem>>());
  CHECK(reqs::assoc_move_only::test<std::flat_multiset<MOElem>>());
  CHECK((reqs::assoc_move_only::test<std::flat_set<MOElem, std::less<MOElem>, std::deque<MOElem>>>()));
  return 0;
}
