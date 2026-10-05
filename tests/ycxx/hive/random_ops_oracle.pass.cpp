// Pseudo-random sequences of hive operations, checked after every step against a model that
// records the live elements and their addresses, with the element type counting its live objects.
// [hive.overview]: insertion and erasure keep every other element in place (references, pointers
// and iterators to other elements stay valid; [hive.modifiers]/5, /10, /14, /17), so each
// element's address is fixed from insertion to erasure. [hive.modifiers]: emplace / insert return
// an iterator to the new element (exactly one T constructed); insert(n, x), insert_range,
// insert(first, last) insert copies; erase(position) / erase(first, last) return the iterator
// following the erased elements ([sequence.reqmts]). [hive.operations]: splice moves the
// elements themselves and empties x (/3); unique erases all but the first of each consecutive
// group in iteration order and returns the number erased (/8-9); sort orders the elements
// (/14; may move them); get_iterator(p) points to *p (/18). [hive.capacity]: capacity() >=
// size(); reserve(n) gives capacity() >= n and keeps every pointer valid (/4, /7);
// trim_capacity keeps them too (/14); shrink_to_fit may move the elements (/11). [hive.erasure]:
// erase / erase_if return the number removed.
#include <hive>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <vector>
#include "check.hpp"

long live = 0;
struct E {
  int key;
  int id;
  E(int k, int i) : key(k), id(i) { ++live; }
  E(const E& o) : key(o.key), id(o.id) { ++live; }
  E(E&& o) noexcept : key(o.key), id(o.id) { ++live; }
  E& operator=(const E&) = default;
  E& operator=(E&&) noexcept = default;
  ~E() { --live; }
  friend bool operator==(const E& a, const E& b) { return a.key == b.key; }
  friend bool operator<(const E& a, const E& b) { return a.key < b.key; }
};

unsigned st = 5150u;
unsigned rnd(unsigned n) {
  st ^= st << 13;
  st ^= st >> 17;
  st ^= st << 5;
  return st % n;
}

struct Tester {
  std::hive<E> h;
  std::map<int, const E*> where;  // id -> address
  std::map<int, int> key_of;     // id -> key
  int next_id = 0;

  void record(const E& e) {
    where[e.id] = &e;
    key_of[e.id] = e.key;
  }
  void rebuild() {  // after an operation that may move elements
    where.clear();
    for (auto& e : h) where[e.id] = &e;
  }
  std::vector<const E*> order() const {
    std::vector<const E*> v;
    for (auto& e : h) v.push_back(&e);
    return v;
  }
  void verify() {
    CHECK(h.size() == where.size());
    CHECK(h.empty() == where.empty());
    CHECK(live == static_cast<long>(h.size()));
    CHECK(h.capacity() >= h.size());
    std::size_t n = 0;
    for (auto& e : h) {
      auto it = where.find(e.id);
      CHECK(it != where.end() && it->second == &e && key_of[e.id] == e.key);
      ++n;
    }
    CHECK(n == h.size());
    CHECK(static_cast<std::size_t>(std::distance(h.begin(), h.end())) == n);
    // reverse iteration visits the same elements in the opposite order
    std::vector<const E*> fwd = order(), bwd;
    for (auto it = h.rbegin(); it != h.rend(); ++it) bwd.push_back(&*it);
    std::reverse(bwd.begin(), bwd.end());
    CHECK(fwd == bwd);
    // get_iterator
    if (!fwd.empty()) {
      const E* p = fwd[rnd(static_cast<unsigned>(fwd.size()))];
      auto it = h.get_iterator(p);
      CHECK(&*it == p);
      const auto& ch = h;
      CHECK(&*ch.get_iterator(p) == p);
    }
  }

  void step() {
    switch (rnd(14)) {
      case 0:
      case 1:
      case 2: {  // emplace / insert / emplace_hint
        int id = next_id++, key = static_cast<int>(rnd(20));
        std::hive<E>::iterator it;
        switch (rnd(3)) {
          case 0: it = h.emplace(key, id); break;
          case 1: it = h.insert(E(key, id)); break;
          default: it = h.emplace_hint(h.begin(), key, id);
        }
        CHECK(it->id == id && it->key == key);
        record(*it);
        break;
      }
      case 3: {  // insert(n, x), insert_range, insert(first, last), insert(il)
        std::vector<E> src;
        for (unsigned k = 0, m = rnd(12); k < m; ++k) src.emplace_back(static_cast<int>(rnd(20)), next_id++);
        auto before = order();
        switch (rnd(3)) {
          case 0: h.insert_range(src); break;
          case 1: h.insert(src.begin(), src.end()); break;
          default: {
            if (!src.empty()) {
              const E x = src[0];
              const int copies = 1 + static_cast<int>(rnd(5));
              h.insert(static_cast<std::size_t>(copies), x);
              // the copies share x's id: give all but one a fresh id, so that ids stay unique
              bool first = true;
              for (auto& e : h)
                if (e.id == x.id) {
                  if (!first) e.id = next_id++;
                  first = false;
                }
            }
          }
        }
        // every element not seen before is new and stays where it is
        for (auto& e : h)
          if (where.find(e.id) == where.end()) record(e);
        for (auto* p : before) CHECK(where[p->id] == p);
        src.clear();
        break;
      }
      case 4:
      case 5: {  // erase(position)
        if (h.empty()) break;
        auto v = order();
        std::size_t i = rnd(static_cast<unsigned>(v.size()));
        auto it = h.get_iterator(v[i]);
        int id = it->id;
        auto r = h.erase(it);
        if (i + 1 < v.size()) CHECK(&*r == v[i + 1]);
        else CHECK(r == h.end());
        where.erase(id);
        break;
      }
      case 6: {  // erase(first, last)
        if (h.empty()) break;
        auto v = order();
        std::size_t f = rnd(static_cast<unsigned>(v.size()));
        std::size_t l = f + rnd(static_cast<unsigned>(v.size() - f + 1));
        auto fi = h.get_iterator(v[f]);
        auto li = l < v.size() ? h.get_iterator(v[l]) : h.end();
        for (std::size_t k = f; k < l; ++k) where.erase(v[k]->id);
        auto r = h.erase(fi, li);
        if (l < v.size()) CHECK(&*r == v[l]);
        else CHECK(r == h.end());
        break;
      }
      case 7: {  // erase_if / erase
        int mod = 3 + static_cast<int>(rnd(4));
        std::size_t want = 0;
        for (auto& e : h) want += e.key % mod == 0;
        for (auto it = where.begin(); it != where.end();)
          it = key_of[it->first] % mod == 0 ? where.erase(it) : std::next(it);
        CHECK(std::erase_if(h, [mod](const E& e) { return e.key % mod == 0; }) == want);
        int k = static_cast<int>(rnd(20));
        want = 0;
        for (auto& e : h) want += e.key == k;
        for (auto it = where.begin(); it != where.end();)
          it = key_of[it->first] == k ? where.erase(it) : std::next(it);
        CHECK(std::erase(h, E(k, -1)) == want);
        break;
      }
      case 8: {  // splice from another hive
        std::hive<E> other;
        std::vector<const E*> moved;
        for (unsigned k = 0, m = rnd(15); k < m; ++k) moved.push_back(&*other.emplace(static_cast<int>(rnd(20)), next_id++));
        auto before = order();
        if (rnd(2)) h.splice(other);
        else h.splice(std::move(other));
        CHECK(other.empty());
        for (auto* p : moved) record(*p);  // the same objects, now in h
        for (auto* p : before) CHECK(where[p->id] == p);
        break;
      }
      case 9: {  // unique, in iteration order
        auto v = order();
        std::size_t want = 0;
        for (std::size_t i = 1; i < v.size(); ++i)
          if (v[i]->key == v[i - 1]->key) {
            ++want;
            where.erase(v[i]->id);
          }
        CHECK(h.unique() == want);
        break;
      }
      case 10: {  // sort (may move elements)
        if (rnd(2)) h.sort();
        else h.sort([](const E& a, const E& b) { return a.key > b.key; });
        rebuild();
        break;
      }
      case 11: {  // reserve / trim_capacity keep addresses
        auto before = order();
        std::size_t n = h.size() + rnd(200);
        h.reserve(n);
        CHECK(h.capacity() >= n);
        CHECK(order() == before);
        if (rnd(2)) h.trim_capacity();
        else h.trim_capacity(h.size() + rnd(50));
        CHECK(order() == before);
        break;
      }
      case 12: {  // shrink_to_fit (may move elements)
        auto cap = h.capacity();
        h.shrink_to_fit();
        CHECK(h.capacity() <= cap);
        rebuild();
        break;
      }
      default: {  // occasionally clear; swap with a copy
        if (rnd(30) == 0) {
          h.clear();
          where.clear();
        } else {
          std::hive<E> copy(h);
          CHECK(copy.size() == h.size());
          h.swap(copy);  // the copy's elements are new objects
          rebuild();
        }
      }
    }
    verify();
  }
};

int main() {
  {
    Tester t;
    for (int i = 0; i < 5000; ++i) t.step();
  }
  CHECK(live == 0);
}
