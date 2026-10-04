// [associative.reqmts.general]/84-117 and [container.node] for set and multiset: extract
// into a node handle without copying, value() is modifiable (the extracted key can be
// changed before re-insertion), insert(nh) / insert(p, nh) results, empty handles, failed
// insertion keeping the handle; merge from set / multiset with any comparator (Table 75).
#include <set>
#include <functional>
#include <memory>
#include "container_values.hpp"
#include "reqs/associative.hpp"
#include "check.hpp"

using namespace reqs::associative;

static_assert(merge<std::set<int>, std::set<int>>());
static_assert(merge<std::set<Elem>, std::multiset<Elem>>());
static_assert(merge<std::multiset<int>, std::set<int, std::greater<int>>>());

struct MoveCount {
  int v;
  static inline int copies = 0, moves = 0;
  MoveCount(int x) : v(x) {}
  MoveCount(const MoveCount& o) : v(o.v) { ++copies; }
  MoveCount(MoveCount&& o) noexcept : v(o.v) { ++moves; }
  MoveCount& operator=(const MoveCount&) = default;
  bool operator<(const MoveCount& o) const { return v < o.v; }
};
struct RevLess {
  bool operator()(const MoveCount& a, const MoveCount& b) const { return b < a; }
};

int main() {
  CHECK(node_handles<std::set<int>>());
  CHECK(node_handles<std::set<Elem>>());
  CHECK(node_handles<std::multiset<int>>());
  CHECK(node_handles<std::multiset<Elem>>());
  CHECK((merge<std::set<int>, std::set<int>>()));
  CHECK((merge<std::set<int>, std::set<int, std::greater<int>>>()));
  CHECK((merge<std::set<Elem>, std::multiset<Elem>>()));
  CHECK((merge<std::multiset<int>, std::set<int, std::greater<int>>>()));
  CHECK((merge<std::multiset<Elem>, std::multiset<Elem>>()));

  // extract / insert / merge move no elements
  std::set<MoveCount> a;
  for (int i = 0; i < 10; ++i) a.emplace(i);
  std::multiset<MoveCount, RevLess> b;
  MoveCount::copies = MoveCount::moves = 0;
  auto nh = a.extract(a.begin());
  nh.value().v = 42;
  b.insert(std::move(nh));
  b.merge(a);
  a.merge(b);
  CHECK(MoveCount::copies == 0 && MoveCount::moves == 0);
  CHECK(a.size() == 10 && b.empty() && std::prev(a.end())->v == 42);
  return 0;
}
