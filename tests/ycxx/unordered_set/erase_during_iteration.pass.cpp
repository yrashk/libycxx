// Erasing while iterating over unordered_set / unordered_multiset (and the map forms), with a
// colliding hash and with a well-spread one.
// [unord.req.general] a.erase(q): "Returns: The iterator immediately following q prior to the
// erasure"; a.erase(q1, q2) erases [q1, q2) and returns q2 (the element following them);
// /242: "The erase members shall invalidate only iterators and references to the erased
// elements, and preserve the relative order of the elements that are not erased"; /244: the same
// for extract, whose node keeps the element. So a loop `it = pred(*it) ? c.erase(it) : ++it`
// visits every element exactly once in the original order, the survivors keep their order and
// their addresses, and erase_if ([unord.set.erasure]) removes the same elements and returns
// their number. Multi containers: elements with equivalent keys stay adjacent.
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <iterator>
#include <vector>
#include "check.hpp"

struct H {
  bool collide;
  std::size_t operator()(int k) const { return collide ? static_cast<std::size_t>(k & 3) : static_cast<std::size_t>(k) * 2654435761u; }
};

unsigned st = 7u;
unsigned rnd(unsigned n) {
  st = st * 1664525u + 1013904223u;
  return (st >> 8) % n;
}

template <class C>
int key_of(const typename C::value_type& v) {
  if constexpr (requires { v.first; }) return v.first;
  else return v;
}

template <class C>
std::vector<const typename C::value_type*> addresses(const C& c) {
  std::vector<const typename C::value_type*> r;
  for (auto& e : c) r.push_back(&e);
  return r;
}

template <class C>
bool adjacent_groups(const C& c) {
  std::vector<int> keys;
  for (auto& e : c) keys.push_back(key_of<C>(e));
  for (std::size_t i = 0; i < keys.size(); ++i)
    for (std::size_t j = i + 2; j < keys.size(); ++j)
      if (keys[i] == keys[j] && keys[j - 1] != keys[i]) return false;
  return true;
}

template <class C, class Make>
void run(bool collide, int n, int key_range, Make make) {
  for (int variant = 0; variant < 5; ++variant) {
    C c(0, H{collide});
    for (int i = 0; i < n; ++i) c.insert(make(static_cast<int>(rnd(static_cast<unsigned>(key_range))), i));
    CHECK(adjacent_groups(c));
    const auto before = addresses(c);
    const unsigned mod = 2 + static_cast<unsigned>(variant);
    auto pred = [&](const typename C::value_type& e) { return static_cast<unsigned>(key_of<C>(e)) % mod == 0; };
    std::vector<const typename C::value_type*> survivors;
    for (auto* p : before)
      if (!pred(*p)) survivors.push_back(p);

    std::vector<const typename C::value_type*> visited;
    std::size_t erased = 0;
    switch (variant) {
      case 0:  // erase(iterator)
        for (auto it = c.begin(); it != c.end();) {
          visited.push_back(&*it);
          if (pred(*it)) {
            auto nx = std::next(it);
            it = c.erase(it);
            CHECK(it == nx);
            ++erased;
          } else {
            ++it;
          }
        }
        break;
      case 1:  // erase(const_iterator)
        for (auto it = c.cbegin(); it != c.cend();) {
          visited.push_back(&*it);
          if (pred(*it)) {
            it = c.erase(it);
            ++erased;
          } else {
            ++it;
          }
        }
        break;
      case 2:  // erase(first, last) of each maximal run of matching elements
        for (auto it = c.cbegin(); it != c.cend();) {
          if (!pred(*it)) {
            visited.push_back(&*it);
            ++it;
            continue;
          }
          auto last = it;
          while (last != c.cend() && pred(*last)) {
            visited.push_back(&*last);
            ++last;
            ++erased;
          }
          auto r = c.erase(it, last);
          CHECK(r == last);
          it = r;
        }
        break;
      case 3:  // extract(iterator), advancing first
        for (auto it = c.begin(); it != c.end();) {
          visited.push_back(&*it);
          if (pred(*it)) {
            auto cur = it++;
            const auto* addr = &*cur;
            auto node = c.extract(cur);
            CHECK(!node.empty());
            if constexpr (requires { node.value(); }) CHECK(&node.value() == addr);
            ++erased;
          } else {
            ++it;
          }
        }
        break;
      default:  // erase_if
        visited = before;
        for (auto* p : before) erased += pred(*p);
        CHECK(std::erase_if(c, pred) == erased);
    }
    CHECK(visited == before);  // every element once, in the original order
    CHECK(c.size() == before.size() - erased);
    CHECK(addresses(c) == survivors);  // order and addresses of the others are kept
    CHECK(adjacent_groups(c));
    for (auto& e : c) CHECK(!pred(e));
    // erasing everything one element at a time from the front, then the back half by range
    std::size_t m = c.size();
    auto mid = std::next(c.begin(), static_cast<long>(m / 2));
    auto tail_addrs = std::vector<const typename C::value_type*>();
    for (auto it = mid; it != c.end(); ++it) tail_addrs.push_back(&*it);
    while (c.begin() != mid) c.erase(c.begin());
    CHECK(addresses(c) == tail_addrs);
    CHECK(c.erase(c.cbegin(), c.cend()) == c.end());
    CHECK(c.empty() && c.begin() == c.end());
  }
}

int main() {
  auto key = [](int k, int) { return k; };
  auto kv = [](int k, int v) { return std::pair<const int, int>(k, v); };
  for (bool collide : {true, false}) {
    for (int n : {0, 1, 2, 17, 300}) {
      for (int range : {1, 4, 50, 100000}) {
        run<std::unordered_set<int, H>>(collide, n, range, key);
        run<std::unordered_multiset<int, H>>(collide, n, range, key);
        run<std::unordered_map<int, int, H>>(collide, n, range, kv);
        run<std::unordered_multimap<int, int, H>>(collide, n, range, kv);
      }
    }
  }
}
