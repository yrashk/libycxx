// [contents]/3, [container.alloc.reqmts]/2, [utility.arg.requirements], [iterator.requirements]
// for flat_set and flat_multiset (also over deque) ([flat.set.overview], [flat.multiset.overview]): no
// unqualified call inside the library may find, by argument-dependent lookup, a function
// template of the key / mapped type's namespace (support/adl_poison.hpp: evil::move,
// evil::find, evil::lower_bound, ::addressof, ...), and keys, values and iterators with a
// deleted unary & and comma operator must work. The generic check
// support/reqs/adl_robustness_assoc.hpp: every constructor form, assignment, comparison,
// swap, insert / emplace / hint / range members, lookups, erasure, erase_if,
// extract / replace. Every member is constexpr.
#include <flat_set>
#include <deque>
#include <functional>
#include "adl_poison.hpp"
#include "reqs/adl_robustness_assoc.hpp"
#include "check.hpp"

using reqs::adl_robustness_assoc::test;
using reqs::adl_robustness_assoc::ValHash;

static_assert(test<std::flat_set<evil::Val>>());

int main() {
  CHECK((test<std::flat_set<evil::Val>>()));
  CHECK((test<std::flat_multiset<evil::Val>>()));
  CHECK((test<std::flat_set<GVal>>()));
  CHECK((test<std::flat_multiset<GVal, std::less<GVal>, std::deque<GVal>>>()));
  return 0;
}
