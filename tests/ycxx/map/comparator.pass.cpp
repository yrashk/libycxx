// [associative.reqmts.general]/18-46, /179 for map and multimap with a stateful comparator:
// X(c), X(i, j, c), X(from_range, rg, c), X(il, c) order by a copy of c; key_comp() returns
// it and value_comp() orders pairs by their keys with it; copy construction, copy and move
// assignment and swap carry the comparison object of the source ("the target container
// shall then use the comparison object from the container being copied"); the container
// does not keep a reference to the argument.
#include <map>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

using reqs::associative::Dir;

static_assert(reqs::associative::comparator<std::map<int, int, Dir>>());
static_assert(reqs::associative::comparator<std::multimap<int, Elem, Dir>>());

int main() {
  CHECK(reqs::associative::comparator<std::map<int, int, Dir>>());
  CHECK(reqs::associative::comparator<std::map<Elem, Elem, Dir>>());
  CHECK(reqs::associative::comparator<std::multimap<int, Elem, Dir>>());
  CHECK(reqs::associative::comparator<std::multimap<long, int, Dir>>());
  return 0;
}
