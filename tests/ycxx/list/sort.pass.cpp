// [list.ops]/33-35: sort() sorts by operator<, sort(comp) by comp; "Remarks: Stable." and the
// operation "Does not affect the validity of iterators and references": an iterator to an
// element still refers to that element (at its new position) afterwards, and no element is
// copied. Checked on many sizes with many equivalent keys. (The detailed description in
// [list.ops] declares sort without constexpr while the [list.overview] synopsis has it, so
// sort is checked only at run time here.)
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
  return K::copies == 0;
}

int main() {
  for (int n : {0, 1, 2, 3, 5, 16, 17, 100, 1000, 1999})
    for (int d : {1, 2, 10, 1000}) CHECK(test(n, d));
  std::list<int> l{3, 1, 2};
  l.sort([](int a, int b) { return a > b; });
  CHECK(l == std::list<int>({3, 2, 1}));
  return 0;
}
