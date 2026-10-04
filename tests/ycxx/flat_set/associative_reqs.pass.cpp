// [flat.set.overview], [flat.multiset.overview]: flat_set / flat_multiset meet the
// associative container requirements ([associative.reqmts]) for unique / equivalent keys
// except for node handles and iterator invalidation. The applicable generic checks from
// support/reqs/associative.hpp: construction from iterator ranges, ranges and initializer
// lists, a = il; emplace / insert results; hinted insertion including the position among
// equivalent keys; find, count, contains, lower_bound, upper_bound, equal_range; erase_if;
// stateful comparison objects. Also with deque as the underlying container. Every member is
// constexpr.
#include <flat_set>
#include <deque>
#include <functional>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

template <class X>
constexpr bool all() {
  using namespace reqs::associative;
  return construct<X>() && insert_emplace<X>() && hint<X>() && lookup<X>() && erase_if<X>();
}

using reqs::associative::Dir;

static_assert(all<std::flat_set<int>>());
static_assert(all<std::flat_set<Elem>>());
static_assert(all<std::flat_multiset<int>>());
static_assert(all<std::flat_multiset<Elem>>());
static_assert(reqs::associative::comparator<std::flat_set<int, Dir>>());
static_assert(reqs::associative::comparator<std::flat_multiset<Elem, Dir>>());

int main() {
  CHECK(all<std::flat_set<int>>());
  CHECK(all<std::flat_set<Elem>>());
  CHECK((all<std::flat_set<int, std::less<int>, std::deque<int>>>()));
  CHECK(all<std::flat_multiset<int>>());
  CHECK((all<std::flat_multiset<Elem, std::less<>, std::deque<Elem>>>()));
  CHECK(reqs::associative::comparator<std::flat_set<int, Dir>>());
  CHECK(reqs::associative::comparator<std::flat_multiset<Elem, Dir>>());
  return 0;
}
