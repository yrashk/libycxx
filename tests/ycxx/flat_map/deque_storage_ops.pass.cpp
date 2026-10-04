// flat containers over std::deque (any sequence container with random access iterators,
// [flat.map.overview], [flat.set.overview]) with a non-default comparator:
// [flat.map.cons]: flat_map(sorted_unique_t, key_container_type, mapped_container_type)
//   "Initializes c.keys with std::move(key_cont), c.values with std::move(mapped_cont)";
// [flat.map.access] keys()/values() return c.keys / c.values;
// [flat.map.modifiers]: insert(sorted_unique, first, last) and insert_range(sorted_unique, rg)
//   are "Equivalent to insert(first, last)" / insert_range(rg) (keys already present are not
//   inserted again); extract() && "Postconditions: *this is emptied ... Returns: std::move(c)";
//   replace(key_cont, mapped_cont) "Equivalent to: c.keys = std::move(key_cont); c.values =
//   std::move(mapped_cont);"; [flat.map.erasure] erase_if returns the number erased;
//   [flat.map.access] operator[] inserts value-initialized mapped values at the sorted
//   position. [flat.multimap.cons] sorted_equivalent; [flat.multimap] insertion of equivalent
//   keys goes after existing ones (emplace: upper_bound). [flat.set.cons], [flat.set.modifiers]
//   extract/replace, [flat.multiset] likewise.
#include <algorithm>
#include <deque>
#include <flat_map>
#include <flat_set>
#include <functional>
#include <list>
#include <string>
#include <vector>
#include "check.hpp"

using DM = std::flat_map<int, std::string, std::greater<int>, std::deque<int>, std::deque<std::string>>;
using DMM = std::flat_multimap<int, int, std::less<>, std::deque<int>, std::deque<int>>;
using DS = std::flat_set<int, std::less<int>, std::deque<int>>;

int main() {
  {
    std::deque<int> k{9, 5, 2};
    std::deque<std::string> v{"nine", "five", "two"};
    DM m(std::sorted_unique, k, v);
    CHECK(m.size() == 3 && m.begin()->first == 9);
    CHECK(m.keys() == k && m.values() == v);
    m.insert_range(std::sorted_unique, std::vector<std::pair<int, std::string>>{{8, "eight"}, {5, "x"}, {1, "one"}});
    CHECK(m.size() == 5 && m.at(8) == "eight" && m.at(1) == "one");
    CHECK(m.at(5) == "five" || m.at(5) == "x");
    std::list<std::pair<int, std::string>> l{{7, "seven"}, {3, "three"}};
    m.insert(std::sorted_unique, l.begin(), l.end());
    CHECK(m.size() == 7 && std::ranges::is_sorted(m.keys(), std::greater<int>{}));
    CHECK(std::ranges::equal(m.keys(), std::vector{9, 8, 7, 5, 3, 2, 1}));
    auto c = std::move(m).extract();
    CHECK(m.empty() && m.keys().empty() && m.values().empty());
    CHECK(c.keys.size() == 7 && c.values.size() == 7 && c.keys.front() == 9 && c.values.front() == "nine");
    m.replace(std::move(c.keys), std::move(c.values));
    CHECK(m.size() == 7 && m[3] == "three");
    CHECK(std::erase_if(m, [](const auto& p) { return p.first % 2 == 1; }) == 5);
    CHECK(m.size() == 2);
    m[4] = "four";
    CHECK((m.keys() == std::deque<int>{8, 4, 2}));
    CHECK(m[6].empty() && m.size() == 4 && m.keys()[1] == 6);
    auto it = m.erase(m.begin());
    CHECK(it->first == 6);
    DM n(std::sorted_unique, {{3, "c"}, {2, "b"}});
    CHECK(n.size() == 2 && n.begin()->second == "c");
  }
  {
    DMM mm(std::sorted_equivalent, std::deque<int>{1, 1, 2}, std::deque<int>{10, 11, 20});
    mm.insert({1, 12});
    CHECK(std::ranges::equal(mm.values(), std::vector{10, 11, 12, 20}));
    CHECK(mm.erase(1) == 3 && mm.size() == 1);
    mm.insert(std::sorted_equivalent, {{0, 0}, {3, 30}});
    CHECK(std::ranges::equal(mm.values(), std::vector{0, 20, 30}));
    mm.emplace(2, 21);
    CHECK(mm.count(2) == 2 && std::ranges::equal(mm.values(), std::vector{0, 20, 21, 30}));
  }
  {
    DS s(std::sorted_unique, std::deque<int>{1, 3, 5});
    s.insert(4);
    CHECK(std::ranges::equal(s, std::vector{1, 3, 4, 5}));
    auto d = std::move(s).extract();
    CHECK(s.empty() && (d == std::deque<int>{1, 3, 4, 5}));
    s.replace(std::move(d));
    CHECK(s.size() == 4);
    s.insert_range(std::sorted_unique, std::vector{0, 4, 6});
    CHECK(std::ranges::equal(s, std::vector{0, 1, 3, 4, 5, 6}));
    CHECK(std::erase_if(s, [](int x) { return x > 3; }) == 3);
    std::flat_multiset<int, std::greater<>, std::deque<int>> ms{3, 1, 3, 2};
    CHECK(std::ranges::equal(ms, std::vector{3, 3, 2, 1}));
  }
  return 0;
}
