// [hive.overview]/7: hive iterators meet the Cpp17BidirectionalIterator requirements and
// model three_way_comparable<strong_ordering>: i < j iff i comes before j in iteration
// order, also across element blocks and after erasures. [container.rev.reqmts]: rbegin /
// rend visit the elements in reverse order. [hive.operations]/1: i + n is next(i, n).
// (Interpretive: /7 names the ordering but not its meaning; the test takes the strong order
// to be the iteration order, as P0447 describes it.)
#include <hive>
#include <compare>
#include <cstddef>
#include <iterator>
#include <vector>
#include "check.hpp"

int main() {
  std::hive<int> h;
  for (int i = 0; i < 2000; ++i) h.insert(i);
  for (auto it = h.begin(); it != h.end();) {
    if (*it % 3 == 0) it = h.erase(it);
    else ++it;
  }
  std::vector<std::hive<int>::const_iterator> order;
  for (auto it = h.cbegin(); it != h.cend(); ++it) order.push_back(it);
  CHECK(order.size() == h.size());
  for (std::size_t a = 0; a < order.size(); a += 37) {
    for (std::size_t b = 0; b < order.size(); b += 41) {
      auto cmp = order[a] <=> order[b];
      CHECK((cmp < 0) == (a < b) && (cmp == 0) == (a == b) && (cmp > 0) == (a > b));
      CHECK((order[a] < order[b]) == (a < b) && (order[a] >= order[b]) == (a >= b));
    }
    CHECK(order[a] < h.cend() && order[a] != h.cend());
  }
  // mixed iterator / const_iterator, end() compares greater than every element
  auto mi = h.begin();
  CHECK((mi <=> h.cbegin()) == 0 && mi < h.cend() && h.end() > mi);
  // reverse iteration
  std::size_t k = order.size();
  for (auto r = h.crbegin(); r != h.crend(); ++r) {
    --k;
    CHECK(&*r == &*order[k]);
  }
  CHECK(k == 0);
  // bidirectional walk back from end
  auto it = h.end();
  for (std::size_t i = order.size(); i-- > 0;) {
    --it;
    CHECK(it == order[i]);
  }
  CHECK(std::distance(h.begin(), h.end()) == static_cast<std::ptrdiff_t>(h.size()));
  CHECK(std::hive<int>::iterator() == std::hive<int>::iterator());
  return 0;
}
