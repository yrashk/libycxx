// [set.overview]/2, [multiset.overview]/2: set and multiset meet the associative container
// requirements ([associative.reqmts]) for unique and equivalent keys respectively. The
// generic checks in support/reqs/associative.hpp: construction from iterator ranges, ranges
// and initializer lists, a = il; emplace / insert results and placement of equivalent keys;
// hinted insertion (including the position among equivalent keys); find, count, contains,
// lower_bound, upper_bound, equal_range on X and const X; erase(k / q / r / q1, q2) and
// clear; stability across insertion and erasure ([associative.reqmts.general]/175);
// erase_if ([set.erasure], [multiset.erasure]); stateful comparison objects
// ([associative.reqmts.general]/179). Every member is constexpr.
#include <set>
#include <functional>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

template <class X>
constexpr bool all() {
  using namespace reqs::associative;
  return construct<X>() && insert_emplace<X>() && hint<X>() && lookup<X>() && erase<X>() && stability<X>() &&
         erase_if<X>();
}

using reqs::associative::Dir;

static_assert(all<std::set<int>>());
static_assert(all<std::set<Elem>>());
static_assert(all<std::multiset<int>>());
static_assert(all<std::multiset<Elem>>());
static_assert(reqs::associative::comparator<std::set<int, Dir>>());
static_assert(reqs::associative::comparator<std::multiset<Elem, Dir>>());

int main() {
  CHECK(all<std::set<int>>());
  CHECK(all<std::set<Elem>>());
  CHECK(all<std::set<long, std::less<>>>());
  CHECK(all<std::multiset<int>>());
  CHECK(all<std::multiset<Elem>>());
  CHECK(reqs::associative::comparator<std::set<int, Dir>>());
  CHECK(reqs::associative::comparator<std::multiset<Elem, Dir>>());
  return 0;
}
