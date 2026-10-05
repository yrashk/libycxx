// Heterogeneous lookup and insertion under pseudo-random workloads, for set, multiset, map,
// multimap (Compare::is_transparent) and their unordered counterparts (Hash::is_transparent and
// Pred::is_transparent). The lookup type Probe converts to the key type only explicitly, and
// every such conversion is counted:
// [associative.reqmts.general]/7 and [unord.req.general]/245: the member templates find, count,
// contains, equal_range, lower_bound / upper_bound (ordered), bucket (unordered), erase and
// extract take the argument as is, so they never make a key. [set.modifiers]/3,
// [unord.set.modifiers]/3: insert(K&&) / insert(hint, K&&) have no effect if an equivalent element
// exists, and otherwise construct one value_type u from the argument. [map.modifiers],
// [unord.map.modifiers]: try_emplace(K&&, args) and insert_or_assign(K&&, obj) likewise make a key
// only when inserting; [map.access], [unord.map.elem]: operator[](K&&) inserts
// value_type(std::forward<K>(x), T()) when absent; at(const K&) throws out_of_range when absent.
// Results are checked against per-key counts.
// REQUIRES: exceptions
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "check.hpp"

int conversions = 0;

struct Probe {
  int v;
};
struct Key {
  int v;
  explicit Key(int x) : v(x) {}
  explicit Key(Probe p) : v(p.v) { ++conversions; }
};

struct Less {
  using is_transparent = void;
  static int k(const Key& x) { return x.v; }
  static int k(const Probe& x) { return x.v; }
  template <class A, class B>
  bool operator()(const A& a, const B& b) const {
    return k(a) < k(b);
  }
};
struct Hash {
  using is_transparent = void;
  std::size_t operator()(const Key& x) const { return static_cast<std::size_t>(x.v) * 31u; }
  std::size_t operator()(const Probe& x) const { return static_cast<std::size_t>(x.v) * 31u; }
};
struct Eq {
  using is_transparent = void;
  template <class A, class B>
  bool operator()(const A& a, const B& b) const {
    return Less::k(a) == Less::k(b);
  }
};

unsigned st = 31337u;
unsigned rnd(unsigned n) {
  st = st * 1664525u + 1013904223u;
  return (st >> 9) % n;
}

template <class C>
const Key& key_of(const typename C::value_type& v) {
  if constexpr (requires { v.first; }) return v.first;
  else return v;
}

template <class C>
void run(int key_range, int steps) {
  constexpr bool is_map = requires { typename C::mapped_type; };
  constexpr bool ordered = requires(C c) { c.lower_bound(Probe{0}); };
  constexpr bool unique = requires(C c, typename C::value_type v) { c.insert(v).second; };
  C c;
  std::vector<std::size_t> cnt(static_cast<std::size_t>(key_range) + 5, 0);
  std::size_t total = 0;
  for (int step = 0; step < steps; ++step) {
    const int k = static_cast<int>(rnd(static_cast<unsigned>(key_range)));
    const Probe p{k};
    const std::size_t n = cnt[static_cast<std::size_t>(k)];
    conversions = 0;
    switch (rnd(8)) {
      case 0: {  // plain insertion of a key (not counted)
        if constexpr (is_map) c.insert(typename C::value_type(Key(k), step));
        else c.insert(Key(k));
        if (!unique || n == 0) {
          ++cnt[static_cast<std::size_t>(k)];
          ++total;
        }
        break;
      }
      case 1: {  // heterogeneous insertion forms of the unique-key containers
        if constexpr (unique) {
          bool inserted;
          if constexpr (is_map) {
            switch (rnd(4)) {
              case 0: inserted = c.try_emplace(p, step).second; break;
              case 1: inserted = c.insert_or_assign(p, step).second; break;
              case 2: {
                auto before = c.size();
                c[p] = step;
                inserted = c.size() != before;
                break;
              }
              default: {
                auto before = c.size();
                auto it = c.try_emplace(c.begin(), p, step);
                CHECK(key_of<C>(*it).v == k);
                inserted = c.size() != before;
              }
            }
          } else {
            if (rnd(2)) {
              inserted = c.insert(p).second;
            } else {
              auto before = c.size();
              auto it = c.insert(c.end(), p);
              CHECK(it->v == k);
              inserted = c.size() != before;
            }
          }
          CHECK(inserted == (n == 0));
          CHECK(conversions == (n == 0 ? 1 : 0));
          if (inserted) {
            ++cnt[static_cast<std::size_t>(k)];
            ++total;
          }
        }
        break;
      }
      case 2: {  // erase(K)
        CHECK(c.erase(p) == n);
        CHECK(conversions == 0);
        total -= n;
        cnt[static_cast<std::size_t>(k)] = 0;
        break;
      }
      case 3: {  // extract(K)
        auto node = c.extract(p);
        CHECK(conversions == 0);
        CHECK(node.empty() == (n == 0));
        if (n) {
          if constexpr (is_map) CHECK(node.key().v == k);
          else CHECK(node.value().v == k);
          --cnt[static_cast<std::size_t>(k)];
          --total;
        }
        break;
      }
      case 4: {  // at(K)
        if constexpr (is_map && unique) {
          bool threw = false;
          try {
            (void)c.at(p);
          } catch (const std::out_of_range&) {
            threw = true;
          }
          CHECK(threw == (n == 0));
          CHECK(conversions == 0);
        }
        break;
      }
      default: {  // lookups
        auto it = c.find(p);
        CHECK((it == c.end()) == (n == 0));
        if (n) CHECK(key_of<C>(*it).v == k);
        CHECK(c.count(p) == n);
        CHECK(c.contains(p) == (n != 0));
        auto [b, e] = c.equal_range(p);
        CHECK(static_cast<std::size_t>(std::distance(b, e)) == n);
        for (; b != e; ++b) CHECK(key_of<C>(*b).v == k);
        if constexpr (ordered) {
          std::size_t below = 0;
          for (int j = 0; j < k; ++j) below += cnt[static_cast<std::size_t>(j)];
          CHECK(static_cast<std::size_t>(std::distance(c.begin(), c.lower_bound(p))) == below);
          CHECK(static_cast<std::size_t>(std::distance(c.begin(), c.upper_bound(p))) == below + n);
        } else {
          if (c.bucket_count() > 0) {
            std::size_t bk = c.bucket(p);
            CHECK(bk < c.bucket_count());
            if (n) {
              bool there = false;
              for (auto l = c.begin(bk); l != c.end(bk); ++l) there = there || key_of<C>(*l).v == k;
              CHECK(there);
            }
          }
        }
        CHECK(conversions == 0);
      }
    }
    CHECK(c.size() == total);
  }
}

int main() {
  for (int range : {3, 50, 400}) {
    run<std::set<Key, Less>>(range, 4000);
    run<std::multiset<Key, Less>>(range, 4000);
    run<std::map<Key, int, Less>>(range, 4000);
    run<std::multimap<Key, int, Less>>(range, 4000);
    run<std::unordered_set<Key, Hash, Eq>>(range, 4000);
    run<std::unordered_multiset<Key, Hash, Eq>>(range, 4000);
    run<std::unordered_map<Key, int, Hash, Eq>>(range, 4000);
    run<std::unordered_multimap<Key, int, Hash, Eq>>(range, 4000);
  }
}
