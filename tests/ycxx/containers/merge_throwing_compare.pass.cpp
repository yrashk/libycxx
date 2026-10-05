// [associative.reqmts.general]/116: a.merge(a2) "Throws: Nothing unless the comparison object
// throws." [unord.req.general] (a.merge(a2)): "Throws: Nothing unless the hash function or key
// equality predicate throws." So an exception from the comparator / hasher must propagate out
// of merge (it is not swallowed, and merge is not noexcept: no std::terminate), and the
// containers involved stay usable ([res.on.exception.handling]; the invariants of a container
// hold after an exception): they iterate in order (ordered containers), size() agrees with the
// iteration, find() finds every element, and no element appears that was not in one of them
// before. The comparator/hasher here throws on its n-th call, for every n until merge
// completes, so each comparison inside merge is a throw point once.
// REQUIRES: exceptions
#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "check.hpp"

static int budget = -1;
static void tick() {
  if (budget >= 0 && budget-- == 0) throw 7;
}
struct TLess {
  bool operator()(int a, int b) const {
    tick();
    return a < b;
  }
};
struct THash {
  std::size_t operator()(int a) const {
    tick();
    return static_cast<std::size_t>(a) * 2654435761u;
  }
};

template <class C>
int key_of(const C&, const typename C::value_type& v) {
  if constexpr (requires { v.first; }) return v.first;
  else return v;
}

template <class C>
void check_valid(const C& c, const std::vector<int>& universe) {
  std::vector<int> keys;
  for (const auto& e : c) keys.push_back(key_of(c, e));
  CHECK(keys.size() == c.size() && static_cast<std::size_t>(std::distance(c.begin(), c.end())) == c.size());
  if constexpr (requires { c.key_comp(); }) CHECK(std::is_sorted(keys.begin(), keys.end()));
  for (int k : keys) {
    CHECK(c.find(k) != c.end());
    CHECK(std::find(universe.begin(), universe.end(), k) != universe.end());
  }
}

template <class A, class B>
void run(const A& a0, const B& b0) {
  std::vector<int> universe;
  for (const auto& e : a0) universe.push_back(key_of(a0, e));
  for (const auto& e : b0) universe.push_back(key_of(b0, e));
  bool completed = false;
  for (int limit = 0; limit < 500 && !completed; ++limit) {
    A a(a0);
    B b(b0);
    budget = limit;
    bool threw = false;
    try {
      a.merge(b);
    } catch (int e) {
      threw = e == 7;
      CHECK(threw);
    }
    budget = -1;
    completed = !threw;
    check_valid(a, universe);
    check_valid(b, universe);
    if (completed) CHECK(a.size() + b.size() == universe.size());
  }
  CHECK(completed);
}

int main() {
  run(std::set<int, TLess>{1, 3, 5, 7}, std::set<int, TLess>{2, 3, 4, 7, 8, 9});
  run(std::multiset<int, TLess>{1, 1, 2}, std::set<int, TLess>{1, 2, 3});
  run(std::map<int, int, TLess>{{1, 0}, {4, 0}}, std::multimap<int, int, TLess>{{1, 0}, {2, 0}, {2, 1}, {5, 0}});
  run(std::unordered_set<int, THash>{1, 3, 5, 7}, std::unordered_set<int, THash>{2, 3, 4, 7, 8, 9});
  run(std::unordered_map<int, int, THash>{{1, 0}, {4, 0}},
      std::unordered_multimap<int, int, THash>{{1, 0}, {2, 0}, {2, 1}, {5, 0}});
  run(std::unordered_multiset<int, THash>{1, 1}, std::unordered_multiset<int, THash>{1, 2, 2});
  return 0;
}
