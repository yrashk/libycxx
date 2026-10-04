// [associative.reqmts.general]/7.22-7.23: heterogeneous keys "ke"/"kx" need only partition the
// container; such a key may be equivalent to SEVERAL elements even in a container with unique
// keys (here: every string with the same first character). Then, for map, set, flat_map and
// flat_set as much as for the multi-containers:
//   a_tran.count(ke) "Returns: The number of elements with key r such that !c(r, ke) &&
//     !c(ke, r)" (/151);
//   a_tran.equal_range(ke) is the whole range (/173), lower_bound/upper_bound (/161, /167);
//   a_tran.contains(ke) (/156);
//   a_tran.find(ke) returns "an element" of it (/145);
//   a_tran.erase(kx) "Erases all elements in the container with key r such that !c(r, kx) &&
//     !c(kx, r) is true. Returns: The number of erased elements." (/123-124);
//   a_tran.extract(kx) "Removes the first element in the container with key r such that ..."
//     (/105).
// [flat.map.overview]/2, [flat.set.overview]/2, [flat.multimap.overview]/2,
// [flat.multiset.overview]/2: the flat containers meet the associative container requirements
// (except node handles, iterator invalidation and single-element complexity); checked also with
// deque storage.
#include <deque>
#include <flat_map>
#include <flat_set>
#include <map>
#include <set>
#include <string>
#include "check.hpp"

struct First {
  char c;
};
struct ByFirst {
  using is_transparent = void;
  bool operator()(const std::string& a, const std::string& b) const { return a < b; }
  bool operator()(const std::string& a, First b) const { return a[0] < b.c; }
  bool operator()(First a, const std::string& b) const { return a.c < b[0]; }
};

template <class M>
void test_map() {
  M m{{"apple", 1}, {"avocado", 2}, {"banana", 3}, {"axe", 4}, {"cherry", 5}, {"blue", 6}};
  CHECK(m.count(First{'a'}) == 3);
  CHECK(m.count(First{'b'}) == 2);
  CHECK(m.count(First{'z'}) == 0);
  CHECK(m.contains(First{'c'}) && !m.contains(First{'d'}));
  auto [lo, hi] = m.equal_range(First{'a'});
  CHECK(lo == m.begin() && std::distance(lo, hi) == 3 && hi->first == "banana");
  CHECK(m.lower_bound(First{'b'})->first == "banana");
  CHECK(m.upper_bound(First{'b'})->first == "cherry");
  auto f = m.find(First{'b'});
  CHECK(f != m.end() && f->first[0] == 'b');
  CHECK(m.erase(First{'a'}) == 3);
  CHECK(m.size() == 3 && m.begin()->first == "banana");
  CHECK(m.erase(First{'z'}) == 0 && m.size() == 3);
  CHECK(m.erase(First{'c'}) == 1 && m.size() == 2);
}

template <class S>
void test_set() {
  S s{"apple", "avocado", "banana", "axe", "cherry", "blue"};
  CHECK(s.count(First{'a'}) == 3);
  auto [lo, hi] = s.equal_range(First{'b'});
  CHECK(std::distance(lo, hi) == 2 && *lo == "banana");
  CHECK(s.erase(First{'b'}) == 2 && s.size() == 4);
  CHECK(!s.contains(First{'b'}) && s.contains(First{'a'}));
}

template <class C>
void test_extract() {
  C c{"axe", "apple", "b", "avocado"};
  auto nh = c.extract(First{'a'});
  CHECK(!nh.empty() && nh.value() == "apple" && c.size() == 3);  // the first one
  nh = c.extract(First{'a'});
  CHECK(nh.value() == "avocado");
  CHECK(c.extract(First{'z'}).empty() && c.size() == 2);
}

int main() {
  test_map<std::map<std::string, int, ByFirst>>();
  test_map<std::multimap<std::string, int, ByFirst>>();
  test_map<std::flat_map<std::string, int, ByFirst>>();
  test_map<std::flat_multimap<std::string, int, ByFirst>>();
  test_map<std::flat_map<std::string, int, ByFirst, std::deque<std::string>, std::deque<int>>>();
  test_set<std::set<std::string, ByFirst>>();
  test_set<std::multiset<std::string, ByFirst>>();
  test_set<std::flat_set<std::string, ByFirst>>();
  test_set<std::flat_multiset<std::string, ByFirst, std::deque<std::string>>>();
  test_extract<std::set<std::string, ByFirst>>();
  test_extract<std::multiset<std::string, ByFirst>>();
  {
    std::map<std::string, int, ByFirst> m{{"axe", 1}, {"apple", 2}, {"b", 3}};
    auto nh = m.extract(First{'a'});
    CHECK(nh.key() == "apple" && nh.mapped() == 2);
    std::multimap<std::string, int, ByFirst> mm{{"x", 1}, {"x", 2}, {"x", 3}};
    CHECK(mm.extract(First{'x'}).mapped() == 1);  // first among equivalent elements
    CHECK(mm.extract(std::string("x")).mapped() == 2);
  }
  return 0;
}
