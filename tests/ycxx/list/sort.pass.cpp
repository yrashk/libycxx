// [list.ops]/33-35: sort preserves iterator/reference identity and is stable, with
// approximately N log N comparisons. These requirements do not prohibit temporary copies
// when T is copyable. Unlike list::merge, sort has no explicit no-copy wording.
// Libycxx policy: sorting copyable elements makes zero temporary copies. Retain this
// performance check separately from identity/stability and the normative immovable-type case.
// Runtime only: the declared draft's detailed sort declarations omit constexpr despite the
// list overview synopsis including it.
#include <list>
#include <iterator>
#include <memory>
#include "check.hpp"

struct K {
  int key;
  int tag;
  static inline int copies = 0;
  K(int k, int t) : key(k), tag(t) {}
  K(const K& o) : key(o.key), tag(o.tag) { ++copies; }
  K& operator=(const K&) = default;
  friend bool operator<(const K& a, const K& b) { return a.key < b.key; }
};

bool test(int n, int distinct) {
  std::list<K> l;
  unsigned seed = static_cast<unsigned>(n) * 7919u + 1u;
  for (int i = 0; i < n; ++i) {
    seed = seed * 1103515245u + 12345u;
    l.emplace_back(static_cast<int>((seed >> 8) % static_cast<unsigned>(distinct)), i);
  }
  // remember an iterator and address for every element
  std::list<K>::iterator its[2000];
  const K* addr[2000];
  int k = 0;
  for (auto it = l.begin(); it != l.end(); ++it, ++k) {
    its[k] = it;
    addr[k] = std::addressof(*it);
  }
  K::copies = 0;
  l.sort();
  if (K::copies != 0 || l.size() != static_cast<std::size_t>(n)) return false;
  const K* prev = nullptr;
  for (const K& e : l) {
    if (prev && (e.key < prev->key || (e.key == prev->key && e.tag < prev->tag))) return false;  // stable
    prev = &e;
  }
  for (int i = 0; i < n; ++i)
    if (std::addressof(*its[i]) != addr[i] || its[i]->tag != i) return false;
  // the saved iterators are iterators into the sorted list: they reach end()
  if (n > 0) {
    int steps = 0;
    for (auto it = its[0]; it != l.end(); ++it) ++steps;
    if (steps < 1 || steps > n) return false;
  }
  // descending with a comparator, still stable
  l.sort([](const K& a, const K& b) { return b.key < a.key; });
  prev = nullptr;
  for (const K& e : l) {
    if (prev && (prev->key < e.key || (e.key == prev->key && e.tag < prev->tag))) return false;
    prev = &e;
  }
  for (int i = 0; i < n; ++i)
    if (std::addressof(*its[i]) != addr[i] || its[i]->tag != i) return false;
  return K::copies == 0;  // libycxx policy, independent of the identity checks

}

struct Immovable {
  int key, tag;
  Immovable(int k, int t) : key(k), tag(t) {}
  Immovable(const Immovable&) = delete;
  Immovable(Immovable&&) = delete;
  Immovable& operator=(const Immovable&) = delete;
  Immovable& operator=(Immovable&&) = delete;
  friend bool operator<(const Immovable& a, const Immovable& b) { return a.key < b.key; }
};

int main() {
  // sort requires comparison, not element copy/move construction or assignment.
  std::list<Immovable> nodes;
  nodes.emplace_back(2, 0);
  nodes.emplace_back(1, 1);
  nodes.emplace_back(2, 2);
  auto first = nodes.begin();
  const Immovable* address = std::addressof(*first);
  nodes.sort();
  auto it = nodes.begin();
  CHECK(it++->tag == 1 && it++->tag == 0 && it++->tag == 2 && it == nodes.end());
  CHECK(std::addressof(*first) == address && first->key == 2 && first->tag == 0);
  nodes.sort([](const Immovable& a, const Immovable& b) { return a.key > b.key; });
  it = nodes.begin();
  CHECK(it++->tag == 0 && it++->tag == 2 && it++->tag == 1 && it == nodes.end());
  CHECK(std::addressof(*first) == address && first->key == 2 && first->tag == 0);
  for (int n : {0, 1, 2, 3, 5, 16, 17, 100, 1000, 1999})
    for (int d : {1, 2, 10, 1000}) CHECK(test(n, d));
  std::list<int> l{3, 1, 2};
  l.sort([](int a, int b) { return a > b; });
  CHECK(l == std::list<int>({3, 2, 1}));
  return 0;
}
