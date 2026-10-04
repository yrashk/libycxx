// [contents]/3, [container.alloc.reqmts]/2, [utility.arg.requirements], [iterator.requirements]
// for set and multiset ([set.overview], [multiset.overview]): no
// unqualified call inside the library may find, by argument-dependent lookup, a function
// template of the key / mapped type's namespace (support/adl_poison.hpp: evil::move,
// evil::find, evil::lower_bound, ::addressof, ...), and keys, values and iterators with a
// deleted unary & and comma operator must work. The generic check
// support/reqs/adl_robustness_assoc.hpp: every constructor form, assignment, comparison,
// swap, insert / emplace / hint / range members, lookups, erasure, erase_if,
// node handles and merge.
#include <set>
#include <functional>
#include "adl_poison.hpp"
#include "reqs/adl_robustness_assoc.hpp"
#include "check.hpp"

using reqs::adl_robustness_assoc::test;
using reqs::adl_robustness_assoc::ValHash;

int main() {
  CHECK((test<std::set<evil::Val>>()));
  CHECK((test<std::multiset<evil::Val>>()));
  CHECK((test<std::set<GVal, std::less<>>>()));
  CHECK((test<std::multiset<GVal>>()));
  return 0;
}
