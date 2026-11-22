// [flat.map.overview]/6 lets a flat_map that an exception leaves in an inconsistent state be
// emptied. libycxx's insert of a sized forward range into vector-backed containers first
// reserves room in both: when that reservation fails nothing has changed yet, so the flat_map
// keeps its elements (a quality-of-implementation guarantee of libycxx, not of the standard).
// REQUIRES: exceptions
#include <flat_map>
#include <functional>
#include <new>
#include <utility>
#include <vector>
#include "check.hpp"
#include "test_allocators.hpp"

using V = std::vector<int, CountingAlloc<int>>;
using M = std::flat_map<int, int, std::less<int>, V, V>;

int main() {
  const std::pair<int, int> more[] = {{7, 70}, {1, 10}, {9, 90}, {4, 40}, {8, 80}};
  for (int fail = 0; fail != 2; ++fail) { // the keys' reservation fails, then the values'
    M m;
    m.insert({{5, 50}, {2, 20}, {3, 30}});
    alloc_counters.fail_after = fail;
    bool threw = false;
    try {
      m.insert(std::begin(more), std::end(more));
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    alloc_counters.fail_after = -1;
    CHECK(threw);
    CHECK(m.size() == 3 && m.at(2) == 20 && m.at(3) == 30 && m.at(5) == 50);
  }
}
