// [associative.reqmts.general]/114: a.merge(a2) "Attempts to extract each element in a2 and
// insert it into a using the comparison object of a. In containers with unique keys, if there
// is an element in a with key equivalent to the key of an element from a2, then that element
// is not extracted from a2." /115: "Pointers and references to the transferred elements of a2
// refer to those same elements but as members of a." Merging a multimap into a map therefore
// transfers the first element of each equivalent group (the later ones then find an
// equivalent element in a) and leaves the rest, in their order, in the source. Merging into a
// multi-container inserts like a_eq.insert(t) (/68: "t is inserted at the end of that range"):
// transferred elements follow the existing equivalent ones, in source order. A unique-keys
// container merged into itself transfers nothing.
// [unord.req.general] Table 92 a.merge(a2): the same rules for the unordered containers
// (equivalent elements of a multi-container stay adjacent; the order within the group is not
// specified there).
// /110 a.extract(q) + a_uniq.insert(nh): the element keeps its address
// ([container.node.overview]/1: the node is transferred, not copied).
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "check.hpp"

using P = std::pair<int, int>;
template <class C>
std::vector<P> items(const C& c) {
  return {c.begin(), c.end()};
}

int main() {
  {
    std::map<int, int> dst{{2, 20}};
    std::multimap<int, int> src{{1, 1}, {1, 2}, {2, 3}, {3, 4}, {3, 5}, {3, 6}};
    const int* first1 = &src.find(1)->second;
    dst.merge(src);
    CHECK((items(dst) == std::vector<P>{{1, 1}, {2, 20}, {3, 4}}));
    CHECK(&dst.at(1) == first1);
    CHECK((items(src) == std::vector<P>{{1, 2}, {2, 3}, {3, 5}, {3, 6}}));
    dst.merge(dst);
    CHECK(dst.size() == 3);
  }
  {
    std::multimap<int, int> dst{{1, 0}, {3, 0}};
    std::map<int, int> src{{1, 9}, {3, 9}};
    std::multimap<int, int> src2{{1, 7}, {1, 8}};
    dst.merge(src);
    dst.merge(src2);
    CHECK(src.empty() && src2.empty());
    CHECK((items(dst) == std::vector<P>{{1, 0}, {1, 9}, {1, 7}, {1, 8}, {3, 0}, {3, 9}}));
  }
  {
    std::set<std::string> dst{"b"};
    std::multiset<std::string> src{"a", "a", "b", "c"};
    dst.merge(src);
    CHECK(dst.size() == 3 && src.size() == 2 && src.count("a") == 1 && src.count("b") == 1);
  }
  {
    std::unordered_map<int, int> dst{{2, 20}};
    std::unordered_multimap<int, int> src{{1, 1}, {1, 2}, {2, 3}, {3, 4}};
    dst.merge(src);
    CHECK(dst.size() == 3 && dst.at(2) == 20 && src.size() == 2 && src.count(1) == 1 && src.count(2) == 1);
    CHECK(dst.at(1) + src.find(1)->second == 3);  // one of the two 1s moved, the other stayed
  }
  {
    std::map<int, std::string> m{{1, "a"}, {2, "b"}, {3, "c"}};
    std::string* addr = &m.at(2);
    auto nh = m.extract(2);
    CHECK(&nh.mapped() == addr);
    nh.key() = 20;
    auto r = m.insert(std::move(nh));
    CHECK(r.inserted && &r.position->second == addr && r.position->first == 20);
    std::map<int, std::string> other;
    auto nh2 = m.extract(m.begin());
    std::string* a2 = &nh2.mapped();
    auto it = other.insert(other.end(), std::move(nh2));
    CHECK(&it->second == a2 && other.size() == 1);
  }
  return 0;
}
