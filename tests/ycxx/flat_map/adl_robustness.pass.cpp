// [contents]/3, [container.alloc.reqmts]/2, [utility.arg.requirements], [iterator.requirements]
// for flat_map and flat_multimap (also over deque) ([flat.map.overview], [flat.multimap.overview]): no
// unqualified call inside the library may find, by argument-dependent lookup, a function
// template of the key / mapped type's namespace (support/adl_poison.hpp: evil::move,
// evil::find, evil::lower_bound, ::addressof, ...), and keys, values and iterators with a
// deleted unary & and comma operator must work. The generic check
// support/reqs/adl_robustness_assoc.hpp: every constructor form, assignment, comparison,
// swap, insert / emplace / hint / range members, lookups, erasure, erase_if,
// extract / replace, operator[] / at / try_emplace / insert_or_assign. Every member is constexpr.
#include <flat_map>
#include <deque>
#include <functional>
#include "adl_poison.hpp"
#include "reqs/adl_robustness_assoc.hpp"
#include "check.hpp"

using reqs::adl_robustness_assoc::test;
using reqs::adl_robustness_assoc::ValHash;

static_assert(test<std::flat_map<evil::Val, evil::Val>>());

int main() {
  CHECK((test<std::flat_map<evil::Val, evil::Val>>()));
  CHECK((test<std::flat_multimap<evil::Val, evil::Val>>()));
  CHECK((test<std::flat_map<GVal, GVal>>()));
  CHECK((test<std::flat_multimap<evil::Val, evil::Val, std::less<evil::Val>, std::deque<evil::Val>, std::deque<evil::Val>>>()));
  return 0;
}
