// [unord.req.general] (emplace, emplace_hint, insert(t&&), insert(p, t&&), extract,
// insert(nh), merge, erase), [unord.map.modifiers] (try_emplace, insert_or_assign),
// [unord.map.elem] (operator[]): unordered_map / unordered_multimap with a move-only mapped
// type (support/move_only_elem.hpp); every member used needs only
// Cpp17EmplaceConstructible / Cpp17MoveInsertable / Cpp17DefaultInsertable /
// Cpp17MoveAssignable elements. The generic check support/reqs/assoc_move_only.hpp.
#include <unordered_map>
#include "container_values.hpp"
#include "move_only_elem.hpp"
#include "reqs/assoc_move_only.hpp"
#include "check.hpp"

using reqs::assoc::VHash;

int main() {
  CHECK((reqs::assoc_move_only::test<std::unordered_map<int, MOElem>>()));
  CHECK((reqs::assoc_move_only::test<std::unordered_map<Elem, MOElem, VHash>>()));
  CHECK((reqs::assoc_move_only::test<std::unordered_multimap<int, MOElem>>()));
  return 0;
}
