// The key argument of erase may refer to an element that is being erased:
// [associative.reqmts.general]/119: a.erase(k) "Erases all elements in the container with key
// equivalent to k. Returns: The number of erased elements."; [unord.req.general] a.erase(k):
// "Erases all elements with key equivalent to k. Returns: The number of elements erased.";
// [flat.map.overview]/2 etc. bring the same requirement to the flat containers. Nothing
// requires k to stay valid while elements are destroyed, so the container must determine the
// whole set of elements before destroying the one k refers to. Likewise
// [list.ops]/15 list::remove(const T& value) "Erases all the elements in the list referred to
// by a list iterator i for which ... *i == value" (also when value is one of them).
// The element type here poisons itself in its destructor, so comparing against a destroyed
// element finds no further matches and the counts below would be wrong.
#include <flat_map>
#include <flat_set>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "check.hpp"

struct Key {
  int v;
  Key(int x) : v(x) {}
  Key(const Key&) = default;
  Key& operator=(const Key&) = default;
  ~Key() { v = -1; }
  friend bool operator==(const Key& a, const Key& b) { return a.v == b.v; }
  friend auto operator<=>(const Key& a, const Key& b) { return a.v <=> b.v; }
};
struct KeyHash {
  std::size_t operator()(const Key& k) const { return static_cast<std::size_t>(k.v) % 7; }
};

template <class C>
void set_like(std::size_t ones) {
  C c{Key(1), Key(1), Key(1), Key(2), Key(0), Key(1)};
  std::size_t before = c.size();
  CHECK(c.count(Key(1)) == ones);
  std::size_t n = c.erase(*c.find(Key(1)));
  CHECK(n == ones && c.size() == before - ones && c.count(Key(1)) == 0);
}

template <class C>
void map_like(std::size_t ones) {
  C c{{Key(1), 1}, {Key(1), 2}, {Key(2), 3}, {Key(1), 4}};
  std::size_t before = c.size();
  std::size_t n = c.erase(c.find(Key(1))->first);
  CHECK(n == ones && c.size() == before - ones && c.count(Key(1)) == 0);
}

int main() {
  set_like<std::multiset<Key>>(4);
  set_like<std::set<Key>>(1);
  set_like<std::unordered_multiset<Key, KeyHash>>(4);
  set_like<std::unordered_set<Key, KeyHash>>(1);
  set_like<std::flat_multiset<Key>>(4);
  set_like<std::flat_set<Key>>(1);
  map_like<std::multimap<Key, int>>(3);
  map_like<std::map<Key, int>>(1);
  map_like<std::unordered_multimap<Key, int, KeyHash>>(3);
  map_like<std::flat_multimap<Key, int>>(3);
  {
    std::list<Key> l{Key(1), Key(2), Key(1), Key(1)};
    CHECK(l.remove(l.front()) == 3 && l.size() == 1 && l.front() == Key(2));
  }
  return 0;
}
