// [flat.map.overview]/2-3, [flat.multimap.overview]/2-4: flat_map / flat_multimap meet the
// associative container requirements ([associative.reqmts]) for unique / equivalent keys,
// except for node handles and iterator invalidation. The applicable generic checks from
// support/reqs/associative.hpp: construction from iterator ranges, ranges and initializer
// lists, a = il; emplace / insert results and the placement of equivalent keys; hinted
// insertion (including the position among equivalent keys); find, count, contains,
// lower_bound, upper_bound, equal_range; erase_if; a stateful comparison object
// ([associative.reqmts.general]/179). Also with deque as the underlying containers
// ([flat.map.overview]/7). Every member is constexpr.
#include <flat_map>
#include <deque>
#include <functional>
#include <vector>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

template <class X>
constexpr bool all() {
  using namespace reqs::associative;
  return construct<X>() && insert_emplace<X>() && hint<X>() && lookup<X>() && erase_if<X>();
}

using reqs::associative::Dir;

static_assert(all<std::flat_map<int, int>>());
static_assert(all<std::flat_map<int, Elem>>());
static_assert(all<std::flat_multimap<int, int>>());
static_assert(all<std::flat_multimap<Elem, Elem>>());
static_assert(reqs::associative::comparator<std::flat_map<int, int, Dir>>());
static_assert(reqs::associative::comparator<std::flat_multimap<int, Elem, Dir>>());

int main() {
  CHECK(all<std::flat_map<int, int>>());
  CHECK(all<std::flat_map<int, Elem>>());
  CHECK(all<std::flat_map<Elem, long, std::less<>>>());
  CHECK((all<std::flat_map<int, int, std::less<int>, std::deque<int>, std::deque<int>>>()));
  CHECK(all<std::flat_multimap<int, int>>());
  CHECK(all<std::flat_multimap<Elem, Elem>>());
  CHECK((all<std::flat_multimap<int, Elem, std::less<int>, std::deque<int>, std::vector<Elem>>>()));
  CHECK(reqs::associative::comparator<std::flat_map<int, int, Dir>>());
  CHECK(reqs::associative::comparator<std::flat_multimap<int, Elem, Dir>>());
  return 0;
}
