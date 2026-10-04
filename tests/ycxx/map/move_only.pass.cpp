// [associative.reqmts.general]/47-74, /84-137, [map.modifiers]/7-31, [map.access]/1-4:
// map / multimap with a move-only mapped type (support/move_only_elem.hpp). Every member
// used needs only Cpp17EmplaceConstructible / Cpp17MoveInsertable / Cpp17DefaultInsertable /
// Cpp17MoveAssignable elements. The generic check support/reqs/assoc_move_only.hpp: emplace,
// emplace_hint, insert(t&&), insert(p, t&&), try_emplace, insert_or_assign, operator[],
// move construction / assignment, swap, extract / insert(nh), merge, erase, clear.
#include <map>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_move_only.hpp"
#include "check.hpp"

int main() {
  CHECK((reqs::assoc_move_only::test<std::map<int, MOElem>>()));
  CHECK((reqs::assoc_move_only::test<std::map<Elem, MOElem>>()));
  CHECK((reqs::assoc_move_only::test<std::multimap<int, MOElem>>()));
  return 0;
}
