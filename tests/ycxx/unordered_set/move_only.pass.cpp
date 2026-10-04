// [unord.req.general]: unordered_set / unordered_multiset with a move-only key type
// (support/move_only_elem.hpp). emplace, emplace_hint, insert(t&&), insert(p, t&&),
// extract, insert(nh), merge, erase, move construction / assignment and swap need only
// Cpp17EmplaceConstructible / Cpp17MoveInsertable elements. The generic check
// support/reqs/assoc_move_only.hpp.
#include <unordered_set>
#include "move_only_elem.hpp"
#include "reqs/assoc_move_only.hpp"
#include "check.hpp"

using reqs::assoc::VHash;

int main() {
  CHECK((reqs::assoc_move_only::test<std::unordered_set<MOElem, VHash>>()));
  CHECK((reqs::assoc_move_only::test<std::unordered_multiset<MOElem, VHash>>()));
  return 0;
}
