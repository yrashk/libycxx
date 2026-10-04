// Exceptions thrown by user callbacks (comparator, hash, equality predicate) from inside
// associative and unordered container operations. Each callback throws on its n-th call, for
// every n until the operation completes, so every call inside the operation is a throw point.
//   [associative.reqmts.except]/2: "if an exception is thrown by any operation from within an
//     insert or emplace function inserting a single element, the insertion has no effect"
//     (insert, emplace, emplace_hint, try_emplace, insert_or_assign, operator[]: the latter
//     three insert via try_emplace/insert_or_assign, [map.access], [map.modifiers]).
//   [associative.reqmts.except]/1: erase(k) throws only what Compare throws.
//   [unord.req.except]/2: likewise for an exception thrown "by any operation other than the
//     container's hash function" (here: the key-equality predicate) - no effect;
//   [unord.req.except]/4: an exception from the hash function inside rehash() gives no
//     guarantee beyond [res.on.exception.handling]: the container stays valid.
// "Valid" is checked as: size() equals the iteration length, every element is found by find(),
// no key appears twice in a unique-key container, ordered containers iterate in order,
// unordered buckets agree with bucket(k), every element came from the inputs, the container
// keeps working afterwards, and every element object is destroyed exactly once (a live count
// returns to zero; destroying an object twice or using a destroyed one is caught by a magic
// field).
#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "check.hpp"

static int budget = -1;
static void tick() {
  if (budget >= 0 && budget-- == 0) throw 7;
}

static int live = 0;
struct K {
  int v;
  int magic = 0x5eed;
  K(int x) : v(x) { ++live; }
  K(const K& o) : v(o.v) {
    CHECK(o.magic == 0x5eed);
    ++live;
  }
  K& operator=(const K& o) {
    CHECK(o.magic == 0x5eed && magic == 0x5eed);
    v = o.v;
    return *this;
  }
  ~K() {
    CHECK(magic == 0x5eed);
    magic = 0;
    --live;
  }
  friend bool operator==(const K& a, const K& b) { return a.v == b.v; }
};
struct TLess {
  bool operator()(const K& a, const K& b) const {
    tick();
    return a.v < b.v;
  }
};
struct THash {
  bool hash_throws = true;
  std::size_t operator()(const K& k) const {
    if (hash_throws) tick();
    return static_cast<std::size_t>(k.v) * 7u;  // collisions modulo small bucket counts
  }
};
struct TEq {
  bool operator()(const K& a, const K& b) const {
    tick();
    return a.v == b.v;
  }
};

template <class C>
int key(const C&, const typename C::value_type& e) {
  if constexpr (requires { e.first; }) return e.first.v;
  else return e.v;
}

template <class C>
void check_valid(C& c, const std::vector<int>& universe) {
  CHECK(budget == -1);
  std::vector<int> keys;
  for (const auto& e : c) keys.push_back(key(c, e));
  CHECK(keys.size() == c.size());
  CHECK(static_cast<std::size_t>(std::distance(c.begin(), c.end())) == c.size());
  for (int k : keys) {
    CHECK(c.find(K(k)) != c.end());
    CHECK(std::ranges::find(universe, k) != universe.end());
  }
  std::vector<int> sorted = keys;
  std::ranges::sort(sorted);
  constexpr bool multi = requires { c.equal_range(K(0)); } && !requires { c.insert_or_assign(K(0), 0); } &&
                         !requires { c.insert(K(0)).second; };
  if constexpr (!multi) CHECK(std::ranges::adjacent_find(sorted) == sorted.end());
  if constexpr (requires { c.key_comp(); }) CHECK(std::ranges::is_sorted(keys));
  if constexpr (requires { c.bucket_count(); }) {
    std::size_t n = 0;
    for (std::size_t b = 0; b < c.bucket_count(); ++b)
      for (auto it = c.begin(b); it != c.end(b); ++it) {
        ++n;
        CHECK(c.bucket(K(key(c, *it))) == b);
      }
    CHECK(n == c.size());
  }
  // still works
  auto probe = K(1000);
  if constexpr (requires { typename C::mapped_type; }) c.insert(typename C::value_type(probe, "x"));
  else c.insert(probe);
  CHECK(c.find(probe) != c.end());
  c.erase(probe);
  CHECK(c.find(probe) == c.end());
}

// Runs op on copies of c0 with every throw point; single = no-effect guarantee applies.
template <class C, class Op>
void sweep(const C& c0, std::vector<int> universe, bool single, Op op) {
  for (const auto& e : c0) universe.push_back(key(c0, e));
  bool completed = false;
  for (int limit = 0; limit < 2000 && !completed; ++limit) {
    {
      C c(c0);
      std::size_t buckets0 = 0;
      if constexpr (requires { c.bucket_count(); }) buckets0 = c.bucket_count();
      budget = limit;
      bool threw = false;
      try {
        op(c);
      } catch (int e) {
        CHECK(e == 7);
        threw = true;
      }
      budget = -1;
      completed = !threw;
      if (threw && single) {
        CHECK(c == c0);
        if constexpr (requires { c.bucket_count(); }) CHECK(c.bucket_count() == buckets0);
      }
      check_valid(c, universe);
    }
  }
  CHECK(completed);
}

template <class M>
void ordered_maps() {
  M m0;
  for (int i = 0; i < 12; i += 2) m0.emplace(K(i), std::to_string(i));
  sweep(m0, {5}, true, [](M& m) { m.insert({K(5), "five"}); });
  sweep(m0, {5}, true, [](M& m) { m.emplace(5, "five"); });
  sweep(m0, {5}, true, [](M& m) { m.emplace_hint(m.begin(), 5, "five"); });
  sweep(m0, {5}, true, [](M& m) { m.insert(std::next(m.begin(), 3), {K(5), "five"}); });
  sweep(m0, {13}, true, [](M& m) { m.insert({K(13), "thirteen"}); });
  if constexpr (requires(M& m) { m[K(5)]; }) {
    sweep(m0, {5}, true, [](M& m) { m[K(5)] = "five"; });
    sweep(m0, {5}, true, [](M& m) { m.try_emplace(K(5), "five"); });
    sweep(m0, {5}, true, [](M& m) { m.insert_or_assign(K(5), "five"); });
  }
  sweep(m0, {1, 3, 5, 7, 4}, false, [](M& m) {
    m.insert({{K(1), "a"}, {K(3), "b"}, {K(5), "c"}, {K(7), "d"}, {K(4), "e"}});
  });
  sweep(m0, {}, false, [](M& m) { m.erase(K(6)); });
  sweep(m0, {}, false, [](M& m) { (void)m.count(K(4)); });
}

template <class S>
void ordered_sets() {
  S s0{K(1), K(4), K(9), K(16), K(25)};
  sweep(s0, {10}, true, [](S& s) { s.insert(K(10)); });
  sweep(s0, {10}, true, [](S& s) { s.emplace(10); });
  sweep(s0, {10}, true, [](S& s) { s.emplace_hint(s.end(), 10); });
  sweep(s0, {4}, true, [](S& s) { s.insert(K(4)); });
  sweep(s0, {2, 3, 4, 30}, false, [](S& s) { s.insert({K(2), K(30), K(3), K(4)}); });
  sweep(s0, {}, false, [](S& s) { s.erase(K(9)); });
}

template <class U>
void unordered() {
  U u0;
  u0.max_load_factor(1.0f);
  for (int i = 0; i < 10; ++i) {
    if constexpr (requires { typename U::mapped_type; }) u0.emplace(K(i * 3), std::to_string(i));
    else u0.emplace(i * 3);
  }
  auto make = [](int k) {
    if constexpr (requires { typename U::mapped_type; }) return typename U::value_type(K(k), "v");
    else return K(k);
  };
  // Only the equality predicate throws: no-effect guarantee for a single insertion.
  {
    U p0(u0.begin(), u0.end(), 0, THash{false}, TEq{});
    sweep(p0, {6, 7}, true, [&](U& u) { u.insert(make(6)); });
    sweep(p0, {7}, true, [&](U& u) { u.insert(make(7)); });
    sweep(p0, {6}, true, [&](U& u) { u.emplace(make(6)); });
    sweep(p0, {6}, true, [&](U& u) { u.emplace_hint(u.begin(), make(6)); });
    sweep(p0, {}, false, [&](U& u) { u.erase(K(9)); });
  }
  // The hash throws too: valid only.
  sweep(u0, {7}, false, [&](U& u) { u.insert(make(7)); });
  sweep(u0, {}, false, [](U& u) { u.rehash(u.bucket_count() * 4 + 1); });
  sweep(u0, {}, false, [](U& u) { u.reserve(100); });
  sweep(u0, {}, false, [](U& u) {
    u.max_load_factor(0.25f);  // may rehash
  });
  sweep(u0, {}, false, [](U& u) { u.erase(K(12)); });
  sweep(u0, {100, 101, 102, 103, 104, 105, 106, 107}, false, [&](U& u) {
    for (int i = 100; i < 108; ++i) u.insert(make(i));
  });
}

int main() {
  ordered_maps<std::map<K, std::string, TLess>>();
  ordered_maps<std::multimap<K, std::string, TLess>>();
  ordered_sets<std::set<K, TLess>>();
  ordered_sets<std::multiset<K, TLess>>();
  unordered<std::unordered_map<K, std::string, THash, TEq>>();
  unordered<std::unordered_set<K, THash, TEq>>();
  unordered<std::unordered_multimap<K, std::string, THash, TEq>>();
  unordered<std::unordered_multiset<K, THash, TEq>>();
  CHECK(live == 0);
  return 0;
}
