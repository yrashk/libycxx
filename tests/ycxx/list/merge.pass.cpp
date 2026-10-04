// [list.ops]/26-30: merge(x) / merge(x, comp) (lvalue and rvalue x) merge two sorted lists
// into one sorted with respect to comp (less<> by default). "Stable ([algorithm.stable])":
// for equivalent elements, those from *this precede those from x and each keeps its relative
// order. x is empty afterwards; "No elements are copied by this operation"; pointers,
// references and iterators to the moved elements now refer to them as members of *this.
// "At most size() + x.size() - 1 comparisons". "If addressof(x) == this, there are no
// effects" and no comparisons are performed.
#include <list>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>
#include "check.hpp"

struct K {
  int key;
  int tag;
  static inline int copies = 0;
  constexpr K(int k, int t) : key(k), tag(t) {}
  constexpr K(const K& o) : key(o.key), tag(o.tag) {
    if !consteval { ++copies; }
  }
  constexpr K& operator=(const K&) = default;
  friend constexpr bool operator<(const K& a, const K& b) { return a.key < b.key; }
};

template <class L>
constexpr bool keys_tags(const L& l, std::initializer_list<int> keys, std::initializer_list<int> tags) {
  if (l.size() != keys.size()) return false;
  auto k = keys.begin();
  auto t = tags.begin();
  for (const K& e : l) {
    if (e.key != *k++ || e.tag != *t++) return false;
  }
  return true;
}

constexpr bool basic() {
  std::list<K> a{{1, 0}, {3, 0}, {3, 1}, {7, 0}};
  std::list<K> b{{0, 10}, {3, 10}, {3, 11}, {8, 10}};
  auto b3 = std::next(b.begin());
  const K* p3 = std::addressof(*b3);
  a.merge(b);
  if (!keys_tags(a, {0, 1, 3, 3, 3, 3, 7, 8}, {10, 0, 0, 1, 10, 11, 0, 10})) return false;
  if (!b.empty() || b.size() != 0) return false;
  // the iterator to {3, 10} now walks a
  if (std::addressof(*b3) != p3 || std::next(b3, 4) != a.end() || std::prev(b3, 4) != a.begin()) return false;
  // rvalue argument, with a comparator, descending order
  std::list<K> c{{9, 0}, {5, 0}, {5, 1}};
  c.merge(std::list<K>{{8, 20}, {5, 20}, {1, 20}}, [](const K& x, const K& y) { return y.key < x.key; });
  if (!keys_tags(c, {9, 8, 5, 5, 5, 1}, {0, 20, 0, 1, 20, 20})) return false;
  // merging with itself has no effect
  c.merge(c, [](const K& x, const K& y) { return y.key < x.key; });
  if (!keys_tags(c, {9, 8, 5, 5, 5, 1}, {0, 20, 0, 1, 20, 20})) return false;
  a.merge(a);
  if (a.size() != 8) return false;
  // empty operands
  std::list<K> e;
  e.merge(a);
  if (e.size() != 8 || !a.empty()) return false;
  e.merge(a);
  if (e.size() != 8) return false;
  std::list<int> i1{1, 4}, i2{2, 3, 5};
  i1.merge(std::move(i2), std::less<>());
  return i1 == std::list<int>{1, 2, 3, 4, 5};
}

bool counts() {
  std::list<K> a, b;
  for (int i = 0; i < 50; ++i) a.emplace_back(2 * i, 0);
  for (int i = 0; i < 30; ++i) b.emplace_back(2 * i + 1, 1);
  int comparisons = 0;
  K::copies = 0;
  a.merge(b, [&](const K& x, const K& y) {
    ++comparisons;
    return x.key < y.key;
  });
  if (K::copies != 0 || comparisons > 50 + 30 - 1) return false;
  int prev = -1;
  for (const K& k : a) {
    if (k.key <= prev) return false;
    prev = k.key;
  }
  comparisons = 0;
  a.merge(a, [&](const K&, const K&) { return ++comparisons, false; });
  return comparisons == 0 && a.size() == 80;
}

static_assert(basic());

int main() {
  CHECK(basic());
  CHECK(counts());
  return 0;
}
