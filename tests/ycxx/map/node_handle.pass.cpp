// [associative.reqmts.general]/84-117 and [container.node]: extract(q) / extract(k) move an
// element into a node handle (no copy: the element keeps its address), the handle exposes
// key() (modifiable) and mapped(), insert(nh) / insert(p, nh) put it back (map returns
// insert_return_type{position, inserted, node}, multimap an iterator), empty handles insert
// nothing, failed insertion keeps the handle. merge(source) moves the elements from another
// map or multimap with any comparator and the same allocator (Table 75), leaving behind in
// a map the elements whose keys are already present.
#include <map>
#include <functional>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

using namespace reqs::associative;

static_assert(merge<std::map<int, int>, std::map<int, int>>());
static_assert(merge<std::map<int, int>, std::multimap<int, int>>());
static_assert(merge<std::multimap<int, int>, std::map<int, int>>());
static_assert(merge<std::multimap<int, Elem>, std::multimap<int, Elem>>());

int main() {
  CHECK(node_handles<std::map<int, int>>());
  CHECK(node_handles<std::map<int, Elem>>());
  CHECK(node_handles<std::multimap<int, int>>());
  CHECK(node_handles<std::multimap<Elem, Elem>>());
  CHECK((merge<std::map<int, int>, std::map<int, int>>()));
  CHECK((merge<std::map<int, Elem>, std::map<int, Elem, std::greater<int>>>()));
  CHECK((merge<std::map<int, int>, std::multimap<int, int>>()));
  CHECK((merge<std::multimap<int, int>, std::map<int, int>>()));
  CHECK((merge<std::multimap<int, Elem>, std::multimap<int, Elem, std::greater<int>>>()));

  // a node extracted from a map can go into a multimap with another comparator
  std::map<int, int> m{{1, 10}, {2, 20}};
  std::multimap<int, int, std::greater<int>> mm{{2, 0}};
  auto nh = m.extract(2);
  const int* addr = &nh.mapped();
  auto it = mm.insert(std::move(nh));
  CHECK(it->first == 2 && it->second == 20 && &it->second == addr && mm.size() == 2 && m.size() == 1);
  CHECK(std::next(it) == mm.end());  // after the existing equivalent element
  auto back = m.insert(mm.extract(it));
  CHECK(back.inserted && back.position->second == 20 && &back.position->second == addr);
  return 0;
}
