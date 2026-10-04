// [map.overview]/2, [multimap.overview]/2: map and multimap meet the associative container
// requirements ([associative.reqmts]) for unique and equivalent keys respectively. The
// generic checks in support/reqs/associative.hpp: construction from iterator ranges,
// ranges and initializer lists (sorted, unique or equivalent keys), a = il; emplace /
// insert results and placement of equivalent keys; hinted insertion; find, count, contains,
// lower_bound, upper_bound, equal_range on X and const X; erase(k / q / r / q1, q2) and
// clear; iterator and reference stability across insertion and erasure ([associative.reqmts.general]/175);
// erase_if ([map.erasure], [multimap.erasure]). Every member is constexpr.
#include <map>
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

static_assert(all<std::map<int, int>>());
static_assert(all<std::map<int, Elem>>());
static_assert(all<std::multimap<int, int>>());
static_assert(all<std::multimap<Elem, Elem>>());

int main() {
  CHECK(all<std::map<int, int>>());
  CHECK(all<std::map<int, Elem>>());
  CHECK(all<std::map<Elem, int>>());
  CHECK(all<std::map<long, double, std::less<>>>());
  CHECK(all<std::multimap<int, int>>());
  CHECK(all<std::multimap<Elem, Elem>>());
  CHECK(all<std::multimap<int, long, std::less<>>>());
  return 0;
}
