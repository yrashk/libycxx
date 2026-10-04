// [forward.list.ops]/13-33: remove / remove_if return the number erased, apply the predicate
// exactly distance(begin(), end()) times and are stable; unique / unique(pred) keep the first
// of every consecutive group of equivalent elements, return the number erased and apply the
// predicate exactly distance - 1 times (none when empty); merge (lvalue and rvalue, with and
// without comp) is stable with elements of *this first among equivalents, leaves x empty,
// copies no element, needs at most distance + x-distance - 1 comparisons, and merging with
// itself does nothing; sort / sort(comp) are stable, copy nothing and keep iterators valid;
// reverse() is noexcept and keeps iterators valid. All are constexpr.
#include <forward_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
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
  friend constexpr bool operator==(const K& a, const K& b) { return a.key == b.key; }
};

constexpr bool kt(const std::forward_list<K>& l, std::initializer_list<int> keys, std::initializer_list<int> tags) {
  auto k = keys.begin();
  auto t = tags.begin();
  for (const K& e : l) {
    if (k == keys.end() || e.key != *k++ || e.tag != *t++) return false;
  }
  return k == keys.end();
}

constexpr bool remove_unique() {
  using L = std::forward_list<int>;
  static_assert(std::is_same_v<decltype(std::declval<L&>().remove(1)), L::size_type>);
  static_assert(std::is_same_v<decltype(std::declval<L&>().unique()), L::size_type>);
  L l{1, 2, 1, 3, 1, 1, 4};
  auto i2 = std::next(l.begin());
  if (l.remove(1) != 4 || l != L{2, 3, 4} || i2 != l.begin()) return false;
  int calls = 0;
  l = {5, 6, 7, 8, 9};
  if (l.remove_if([&](int x) { return ++calls, x % 2 == 0; }) != 2 || calls != 5 || l != L{5, 7, 9}) return false;
  l = {3, 1, 3};
  if (l.remove(l.front()) != 2 || l != L{1}) return false;  // value aliases an erased element
  l = {1, 1, 2, 2, 2, 1, 3, 3, 1};
  if (l.unique() != 4 || l != L{1, 2, 1, 3, 1}) return false;
  calls = 0;
  l = {1, 2, 3, 4, 5, 6};
  if (l.unique([&](int a, int b) { return ++calls, (a < 4) == (b < 4); }) != 4 || calls != 5 || l != L{1, 4}) return false;
  L e;
  calls = 0;
  return e.unique([&](int, int) { return ++calls, true; }) == 0 && calls == 0 && e.remove(0) == 0;
}

constexpr bool merge() {
  using L = std::forward_list<K>;
  L a{{1, 0}, {3, 0}, {3, 1}, {7, 0}};
  L b{{0, 10}, {3, 10}, {8, 10}};
  auto b3 = std::next(b.begin());
  const K* p = std::addressof(*b3);
  a.merge(b);
  if (!kt(a, {0, 1, 3, 3, 3, 7, 8}, {10, 0, 0, 1, 10, 0, 10}) || !b.empty()) return false;
  if (std::addressof(*b3) != p || std::next(b3, 3) != a.end()) return false;
  L c{{9, 0}, {5, 0}};
  c.merge(L{{8, 1}, {5, 1}, {1, 1}}, [](const K& x, const K& y) { return y.key < x.key; });
  if (!kt(c, {9, 8, 5, 5, 1}, {0, 1, 0, 1, 1})) return false;
  c.merge(c);
  if (!kt(c, {9, 8, 5, 5, 1}, {0, 1, 0, 1, 1})) return false;
  std::forward_list<int> i1{1, 4}, i2{2, 3, 5};
  i1.merge(std::move(i2), [](int x, int y) { return x < y; });
  return i1 == std::forward_list<int>{1, 2, 3, 4, 5};
}

constexpr bool sort_reverse(int n) {
  std::forward_list<K> l;
  unsigned seed = 7u + static_cast<unsigned>(n);
  for (int i = n - 1; i >= 0; --i) {
    seed = seed * 1103515245u + 12345u;
    l.emplace_front(static_cast<int>((seed >> 8) % 5u), i);
  }
  auto first = l.begin();
  const K* pf = n ? std::addressof(*first) : nullptr;
  l.sort();
  const K* prev = nullptr;
  int count = 0;
  for (const K& e : l) {
    if (prev && (e.key < prev->key || (e.key == prev->key && e.tag < prev->tag))) return false;
    prev = &e;
    ++count;
  }
  if (count != n || (n && (std::addressof(*first) != pf || first->tag != 0))) return false;
  l.sort([](const K& a, const K& b) { return b.key < a.key; });
  prev = nullptr;
  for (const K& e : l) {
    if (prev && (prev->key < e.key || (e.key == prev->key && e.tag < prev->tag))) return false;
    prev = &e;
  }
  // reverse: order flips, iterators keep their elements
  std::forward_list<int> r{1, 2, 3, 4};
  auto r1 = r.begin();
  r.reverse();
  if (r != std::forward_list<int>{4, 3, 2, 1} || std::next(r1) != r.end() || *r1 != 1) return false;
  return true;
}

bool counts() {
  std::forward_list<K> a, b;
  for (int i = 49; i >= 0; --i) a.emplace_front(2 * i, 0);
  for (int i = 29; i >= 0; --i) b.emplace_front(2 * i + 1, 1);
  int comparisons = 0;
  K::copies = 0;
  a.merge(b, [&](const K& x, const K& y) { return ++comparisons, x.key < y.key; });
  if (K::copies != 0 || comparisons > 79) return false;
  a.sort([](const K& x, const K& y) { return y.key < x.key; });
  a.reverse();
  int prev = -1, n = 0;
  for (const K& k : a) {
    if (k.key <= prev) return false;
    prev = k.key;
    ++n;
  }
  return K::copies == 0 && n == 80;
}

static_assert(noexcept(std::declval<std::forward_list<int>&>().reverse()));
static_assert(remove_unique());
static_assert(merge());
static_assert(sort_reverse(0) && sort_reverse(1) && sort_reverse(40));

int main() {
  CHECK(remove_unique());
  CHECK(merge());
  for (int n : {0, 1, 2, 3, 17, 500}) CHECK(sort_reverse(n));
  CHECK(counts());
  return 0;
}
