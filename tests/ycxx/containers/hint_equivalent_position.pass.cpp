// [associative.reqmts.general]: for containers with equivalent keys the hint decides where a
// new element goes among the equivalent ones:
//   a.emplace_hint(p, args) /58: "Equivalent to a.emplace(std::forward<Args>(args)...),
//   except that the element is inserted as close as possible to the position just prior to
//   p"; a.insert(p, t) /72: "always inserts t in containers with equivalent keys. t is
//   inserted as close as possible to the position just prior to p"; a.insert(p, nh) /96: the
//   same for the element owned by nh; a_eq.insert(nh) /91: at the end of the equal range.
// So with an equal range [b, e) and p inside [b, e], the new element lands immediately before
// p; with p == begin() (before the range) it lands first in the range, with p == end() (after
// it) last. Without a hint, a_eq.insert(t) inserts "at the end of that range" (/68).
// [flat.multiset.overview]/2, [flat.multimap.overview]/2: the flat multi-containers meet these
// associative-container requirements (also with deque storage).
#include <deque>
#include <flat_map>
#include <flat_set>
#include <iterator>
#include <map>
#include <set>
#include <vector>
#include "check.hpp"

struct K {
  int k;
  int tag;
};
struct ByK {
  bool operator()(const K& a, const K& b) const { return a.k < b.k; }
};
template <class S>
std::vector<int> tags(const S& s) {
  std::vector<int> r;
  for (const auto& e : s) r.push_back(e.tag);
  return r;
}
template <class M>
std::vector<int> mapped(const M& m) {
  std::vector<int> r;
  for (const auto& [k, v] : m) r.push_back(v);
  return r;
}

template <class S>
void test_set() {
  S s;
  s.insert(K{1, 1});
  s.insert(K{1, 2});
  s.insert(K{1, 3});
  s.insert(K{0, 0});
  s.insert(K{2, 9});
  CHECK((tags(s) == std::vector<int>{0, 1, 2, 3, 9}));
  auto at = [&](int i) { return std::next(s.begin(), i); };
  auto r = s.insert(at(2), K{1, 4});  // before tag 2
  CHECK(r->tag == 4);
  CHECK((tags(s) == std::vector<int>{0, 1, 4, 2, 3, 9}));
  s.emplace_hint(at(1), K{1, 5});  // before tag 1 (start of the range)
  CHECK((tags(s) == std::vector<int>{0, 5, 1, 4, 2, 3, 9}));
  s.emplace_hint(s.begin(), K{1, 6});  // closest to begin(): first in the range
  CHECK((tags(s) == std::vector<int>{0, 6, 5, 1, 4, 2, 3, 9}));
  s.emplace_hint(s.end(), K{1, 7});  // closest to end(): last in the range
  CHECK((tags(s) == std::vector<int>{0, 6, 5, 1, 4, 2, 3, 7, 9}));
  s.insert(at(8), K{1, 8});  // p is the first element after the range
  CHECK((tags(s) == std::vector<int>{0, 6, 5, 1, 4, 2, 3, 7, 8, 9}));
  const K k{1, 10};
  s.insert(at(4), k);  // lvalue
  CHECK((tags(s) == std::vector<int>{0, 6, 5, 1, 10, 4, 2, 3, 7, 8, 9}));
}

template <class M>
void test_map() {
  M m{{1, 1}, {1, 2}, {1, 3}};
  auto at = [&](int i) { return std::next(m.begin(), i); };
  m.insert(at(1), {1, 4});
  CHECK((mapped(m) == std::vector<int>{1, 4, 2, 3}));
  m.emplace_hint(at(0), 1, 5);
  CHECK((mapped(m) == std::vector<int>{5, 1, 4, 2, 3}));
  m.emplace_hint(m.end(), 1, 6);
  CHECK((mapped(m) == std::vector<int>{5, 1, 4, 2, 3, 6}));
  m.insert(at(3), std::pair<const int, int>(1, 7));
  CHECK((mapped(m) == std::vector<int>{5, 1, 4, 7, 2, 3, 6}));
  m.emplace_hint(m.end(), 0, 0);  // a useless hint is still only a hint
  CHECK(m.begin()->second == 0 && m.size() == 8);
}

int main() {
  test_set<std::multiset<K, ByK>>();
  test_set<std::flat_multiset<K, ByK>>();
  test_set<std::flat_multiset<K, ByK, std::deque<K>>>();
  test_map<std::multimap<int, int>>();
  test_map<std::flat_multimap<int, int>>();
  test_map<std::flat_multimap<int, int, std::less<int>, std::deque<int>, std::deque<int>>>();
  {
    std::multiset<K, ByK> s{K{1, 1}, K{1, 2}};
    std::multiset<K, ByK> o{K{1, 3}, K{1, 4}};
    auto nh = o.extract(o.begin());
    s.insert(std::next(s.begin()), std::move(nh));  // node handle with hint
    CHECK((tags(s) == std::vector<int>{1, 3, 2}));
    nh = o.extract(o.begin());
    s.insert(std::move(nh));  // no hint: end of the range
    CHECK((tags(s) == std::vector<int>{1, 3, 2, 4}));
  }
  return 0;
}
