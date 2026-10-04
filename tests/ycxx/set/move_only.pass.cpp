// [associative.reqmts.general]/47-74, /84-137: set / multiset with a move-only key type
// (support/move_only_elem.hpp). Every member used needs only Cpp17EmplaceConstructible /
// Cpp17MoveInsertable elements. The generic check support/reqs/assoc_move_only.hpp: emplace,
// emplace_hint, insert(t&&), insert(p, t&&), move construction / assignment, swap, extract /
// insert(nh), merge, erase, clear.
#include <set>
#include "move_only_elem.hpp"
#include "reqs/assoc_move_only.hpp"
#include "check.hpp"

int main() {
  CHECK(reqs::assoc_move_only::test<std::set<MOElem>>());
  CHECK(reqs::assoc_move_only::test<std::multiset<MOElem>>());
  return 0;
}
