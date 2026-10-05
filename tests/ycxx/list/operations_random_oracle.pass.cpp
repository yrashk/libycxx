// Pseudo-random sequences of the list operations on list and forward_list, with many equal keys,
// checked against a vector model that tracks each element's identity (id) and address.
// [list.ops] / [forward.list.ops]: sort is stable and "Does not affect the validity of iterators
// and references" (so every element keeps its address); merge is stable, uses at most
// size() + x.size() - 1 comparisons, leaves x empty and moves the elements themselves
// ("Pointers and references to the moved elements of x now refer to those same elements");
// reverse keeps every reference valid; remove / remove_if / unique return the number of
// elements erased and keep the order of the others; unique keeps the first element of each
// consecutive group with binary_pred(*i, *(i - 1)); splice / splice_after move the elements
// themselves (whole list, one element, a range; also within the same list).
#include <forward_list>
#include <list>
#include <cstddef>
#include <iterator>
#include <vector>
#include "check.hpp"

struct E {
  int key;
  int id;
  friend bool operator==(const E& a, const E& b) { return a.key == b.key; }  // remove(value)
};

unsigned st = 77u;
unsigned rnd(unsigned n) {
  st = st * 1103515245u + 12345u;
  return (st >> 16) % n;
}

struct Rec {
  int key, id;
  const E* addr;
};
using Model = std::vector<Rec>;

template <class C>
constexpr bool is_fwd = !requires(C c) { c.size(); };

template <class C>
bool matches(const C& c, const Model& m) {
  std::size_t i = 0;
  for (auto& e : c) {
    if (i >= m.size() || e.key != m[i].key || e.id != m[i].id || &e != m[i].addr) return false;
    ++i;
  }
  return i == m.size();
}
template <class C>
Model snapshot(const C& c) {
  Model m;
  for (auto& e : c) m.push_back({e.key, e.id, &e});
  return m;
}
void stable_sort_model(Model& m, bool desc) {
  for (std::size_t i = 1; i < m.size(); ++i)
    for (std::size_t j = i; j > 0 && (desc ? m[j].key > m[j - 1].key : m[j].key < m[j - 1].key); --j) std::swap(m[j], m[j - 1]);
}

template <class C>
void push_end(C& c, E e) {
  if constexpr (is_fwd<C>) {
    auto before = c.before_begin();
    for (auto it = c.begin(); it != c.end(); ++it) ++before;
    c.insert_after(before, e);
  } else {
    c.push_back(e);
  }
}

template <class C>
void run(int steps) {
  C a, b;
  int id = 0;
  for (int i = 0; i < 30; ++i) push_end(a, E{static_cast<int>(rnd(6)), id++});
  for (int i = 0; i < 20; ++i) push_end(b, E{static_cast<int>(rnd(6)), id++});
  for (int step = 0; step < steps; ++step) {
    Model ma = snapshot(a), mb = snapshot(b);
    const bool desc = rnd(2);
    auto cmp = [desc](const E& x, const E& y) { return desc ? x.key > y.key : x.key < y.key; };
    switch (rnd(9)) {
      case 0: {  // sort both (stable, addresses kept)
        a.sort(cmp);
        stable_sort_model(ma, desc);
        CHECK(matches(a, ma));
        break;
      }
      case 1: {  // merge sorted lists
        a.sort(cmp);
        b.sort(cmp);
        stable_sort_model(ma, desc);
        stable_sort_model(mb, desc);
        int comparisons = 0;
        auto ccmp = [&](const E& x, const E& y) {
          ++comparisons;
          return cmp(x, y);
        };
        if (rnd(2)) a.merge(b, ccmp);
        else a.merge(static_cast<C&&>(b), ccmp);
        // the stable merge: of equivalent elements, a's come first
        Model want;
        std::size_t i = 0, j = 0;
        while (i < ma.size() && j < mb.size()) {
          if (cmp(E{mb[j].key, 0}, E{ma[i].key, 0})) want.push_back(mb[j++]);
          else want.push_back(ma[i++]);
        }
        while (i < ma.size()) want.push_back(ma[i++]);
        while (j < mb.size()) want.push_back(mb[j++]);
        CHECK(matches(a, want));
        CHECK(b.empty());
        std::size_t total = ma.size() + mb.size();
        CHECK(static_cast<std::size_t>(comparisons) + 1 <= (total == 0 ? 1 : total));
        for (int k = 0; k < 15; ++k) push_end(b, E{static_cast<int>(rnd(6)), id++});
        break;
      }
      case 2: {  // reverse
        a.reverse();
        Model want(ma.rbegin(), ma.rend());
        CHECK(matches(a, want));
        break;
      }
      case 3: {  // remove / remove_if
        int k = static_cast<int>(rnd(6));
        Model want;
        for (auto& r : ma)
          if (r.key != k) want.push_back(r);
        std::size_t n;
        if (rnd(2)) {
          n = a.remove_if([k](const E& e) { return e.key == k; });
        } else {
          n = a.remove(E{k, -1});  // *i == value compares keys
        }
        CHECK(n == ma.size() - want.size());
        CHECK(matches(a, want));
        break;
      }
      case 4: {  // unique with an equivalence on key / key parity
        bool parity = rnd(2);
        auto eq = [parity](const E& x, const E& y) { return parity ? x.key % 2 == y.key % 2 : x.key == y.key; };
        Model want;
        for (std::size_t i = 0; i < ma.size(); ++i)
          if (i == 0 || !eq(E{ma[i].key, 0}, E{ma[i - 1].key, 0})) want.push_back(ma[i]);
        std::size_t n = a.unique(eq);
        CHECK(n == ma.size() - want.size());
        CHECK(matches(a, want));
        break;
      }
      case 5: {  // splice the whole of b at a random position
        std::size_t p = rnd(static_cast<unsigned>(ma.size() + 1));
        Model want(ma.begin(), ma.begin() + static_cast<long>(p));
        want.insert(want.end(), mb.begin(), mb.end());
        want.insert(want.end(), ma.begin() + static_cast<long>(p), ma.end());
        if constexpr (is_fwd<C>) {
          a.splice_after(std::next(a.before_begin(), static_cast<long>(p)), b);
        } else {
          a.splice(std::next(a.begin(), static_cast<long>(p)), b);
        }
        CHECK(matches(a, want) && b.empty());
        for (int k = 0; k < 10; ++k) push_end(b, E{static_cast<int>(rnd(6)), id++});
        break;
      }
      case 6: {  // splice one element, from b or within a
        bool within = rnd(2) && !ma.empty();
        Model& src = within ? ma : mb;
        if (src.empty()) break;
        std::size_t i = rnd(static_cast<unsigned>(src.size()));
        std::size_t p = rnd(static_cast<unsigned>(ma.size() + 1));
        Rec moved = src[i];
        Model want = ma;
        if (within) {
          // position p is "before element p"; moving element i there
          if (p == i || p == i + 1) {
            // unchanged
          } else {
            want.erase(want.begin() + static_cast<long>(i));
            std::size_t q = p > i ? p - 1 : p;
            want.insert(want.begin() + static_cast<long>(q), moved);
          }
        } else {
          want.insert(want.begin() + static_cast<long>(p), moved);
          mb.erase(mb.begin() + static_cast<long>(i));
        }
        C& from = within ? a : b;
        if constexpr (is_fwd<C>) {
          // splice_after(position, x, i): moves the element after i, inserted after position
          auto before_elem = std::next(from.before_begin(), static_cast<long>(i));
          auto pos = std::next(a.before_begin(), static_cast<long>(p));
          a.splice_after(pos, from, before_elem);  // unchanged if position == i or position == ++i
        } else {
          a.splice(std::next(a.begin(), static_cast<long>(p)), from, std::next(from.begin(), static_cast<long>(i)));
        }
        CHECK(matches(a, want));
        if (!within) CHECK(matches(b, mb));
        break;
      }
      case 7: {  // splice a range of b
        if (mb.empty()) break;
        std::size_t f = rnd(static_cast<unsigned>(mb.size() + 1));
        std::size_t l = f + rnd(static_cast<unsigned>(mb.size() - f + 1));
        std::size_t p = rnd(static_cast<unsigned>(ma.size() + 1));
        Model want(ma.begin(), ma.begin() + static_cast<long>(p));
        want.insert(want.end(), mb.begin() + static_cast<long>(f), mb.begin() + static_cast<long>(l));
        want.insert(want.end(), ma.begin() + static_cast<long>(p), ma.end());
        Model brest(mb.begin(), mb.begin() + static_cast<long>(f));
        brest.insert(brest.end(), mb.begin() + static_cast<long>(l), mb.end());
        if constexpr (is_fwd<C>) {
          // splice_after(position, x, first, last): moves (first, last)
          a.splice_after(std::next(a.before_begin(), static_cast<long>(p)), b,
                         std::next(b.before_begin(), static_cast<long>(f)), std::next(b.before_begin(), static_cast<long>(l + 1)));
        } else {
          a.splice(std::next(a.begin(), static_cast<long>(p)), b, std::next(b.begin(), static_cast<long>(f)),
                   std::next(b.begin(), static_cast<long>(l)));
        }
        CHECK(matches(a, want));
        CHECK(matches(b, brest));
        break;
      }
      default: {  // swap a and b, or add elements
        if (rnd(2)) {
          a.swap(b);
          CHECK(matches(a, mb) && matches(b, ma));
        } else {
          for (int k = 0; k < 8; ++k) push_end(a, E{static_cast<int>(rnd(6)), id++});
        }
      }
    }
    if constexpr (!is_fwd<C>) {
      CHECK(static_cast<std::size_t>(std::distance(a.begin(), a.end())) == a.size());
      CHECK(static_cast<std::size_t>(std::distance(b.begin(), b.end())) == b.size());
      // backward iteration agrees
      std::size_t n = 0;
      for (auto it = a.rbegin(); it != a.rend(); ++it) ++n;
      CHECK(n == a.size());
    }
    // keep the lists from growing without bound
    if (std::distance(a.begin(), a.end()) > 300) a.clear();
  }
}

int main() {
  run<std::list<E>>(3000);
  run<std::forward_list<E>>(3000);
}
