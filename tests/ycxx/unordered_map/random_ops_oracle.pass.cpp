// Long pseudo-random sequences of operations on unordered_map and unordered_multimap, with a
// colliding hash (h(k) = k % 7), an identity hash and a hash that maps everything to 0, checked
// after every step against an oracle that records each key's equivalent-key group in order.
// [unord.req.general]/6: in containers with equivalent keys, elements with equivalent keys are
// adjacent; mutating operations preserve the relative order within each group. /9: keys with
// the same hash code appear in the same bucket; rehashing does not invalidate pointers or
// references and keeps the relative order of equivalent elements. /242: erase invalidates only
// the erased elements and keeps the relative order of the others; /244: likewise extract.
// Bucket interface (/198-/218): bucket(k) is in [0, bucket_count()) and names the bucket where
// k's elements are found; bucket_size(n) is the number of elements of bucket n, which
// [begin(n), end(n)) visits; load_factor() is size() / bucket_count(); /231: size() <=
// bucket_count() * max_load_factor(). find, count, contains and equal_range agree with the
// oracle; erase(k) returns the number erased; erase(q) the iterator after q; emplace_hint and
// insert(node) with a hint place the element; extract + insert(node) moves the element.
#include <unordered_map>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <vector>
#include "check.hpp"

struct ModHash {
  int mode;
  std::size_t operator()(int k) const {
    switch (mode) {
      case 0: return static_cast<std::size_t>(k % 7 + 7);
      case 1: return static_cast<std::size_t>(k);
      default: return 0;
    }
  }
};

unsigned st = 88172645u;
unsigned rnd(unsigned n) {
  st ^= st << 13;
  st ^= st >> 17;
  st ^= st << 5;
  return st % n;
}

template <class C>
struct Tester {
  static constexpr bool multi = !requires(C c) { c.try_emplace(0, 0); };
  C c;
  std::map<int, std::vector<int>> oracle;  // key -> values in group order
  std::size_t total = 0;
  int next_val = 0;
  int key_range;
  // max_load_factor(z) itself need not rehash: the bound is checked again after the next
  // insertion, rehash or reserve.
  bool mlf_changed = false;

  Tester(int mode, int keys) : c(0, ModHash{mode}), key_range(keys) {}

  std::vector<int> group(int k) {
    std::vector<int> g;
    auto [b, e] = c.equal_range(k);
    for (; b != e; ++b) {
      CHECK(b->first == k);
      g.push_back(b->second);
    }
    return g;
  }

  // After inserting value v under key k: the group is the old group with v inserted somewhere.
  void inserted(int k, int v) {
    std::vector<int> g = group(k);
    std::vector<int>& old = oracle[k];
    CHECK(g.size() == old.size() + 1);
    auto pos = std::find(g.begin(), g.end(), v);
    CHECK(pos != g.end());
    std::vector<int> without = g;
    without.erase(without.begin() + (pos - g.begin()));
    CHECK(without == old);
    old = g;
    ++total;
    mlf_changed = false;
  }

  void verify_all() {
    CHECK(c.size() == total);
    CHECK(c.empty() == (total == 0));
    // iteration: every element once, groups adjacent and in oracle order
    std::size_t n = 0;
    std::map<int, std::vector<int>> seen;
    int prev_key = 0;
    bool first = true;
    for (auto it = c.begin(); it != c.end(); ++it, ++n) {
      if (!first && it->first != prev_key) CHECK(seen.find(it->first) == seen.end());  // adjacency
      seen[it->first].push_back(it->second);
      prev_key = it->first;
      first = false;
    }
    CHECK(n == total);
    std::erase_if(oracle, [](const auto& p) { return p.second.empty(); });
    CHECK(seen == oracle);
    CHECK(static_cast<std::size_t>(std::distance(c.cbegin(), c.cend())) == total);
    // bucket interface
    const std::size_t bc = c.bucket_count();
    // bucket(k) has the precondition bucket_count() > 0, so an empty container may have none
    if (bc == 0) {
      CHECK(total == 0);
      return;
    }
    std::size_t sum = 0;
    for (std::size_t b = 0; b < bc; ++b) {
      std::size_t in_bucket = 0;
      for (auto l = c.begin(b); l != c.end(b); ++l, ++in_bucket) CHECK(c.bucket(l->first) == b);
      CHECK(in_bucket == c.bucket_size(b));
      const C& cc = c;
      CHECK(static_cast<std::size_t>(std::distance(cc.begin(b), cc.end(b))) == in_bucket);
      CHECK(static_cast<std::size_t>(std::distance(c.cbegin(b), c.cend(b))) == in_bucket);
      sum += in_bucket;
    }
    CHECK(sum == total);
    for (auto& [k, g] : oracle) {
      CHECK(c.bucket(k) < bc);
      // keys with the same hash code share a bucket
      CHECK(c.bucket(k) == c.bucket(k + 7 * 1000) || c.hash_function().mode == 1);
      CHECK(c.count(k) == g.size());
      CHECK(c.contains(k));
      CHECK(c.find(k) != c.end() && c.find(k)->first == k);
    }
    CHECK(c.load_factor() == static_cast<float>(total) / static_cast<float>(bc));
    if (!mlf_changed) CHECK(static_cast<double>(total) <= static_cast<double>(bc) * c.max_load_factor());
  }

  void step() {
    int k = static_cast<int>(rnd(static_cast<unsigned>(key_range)));
    switch (rnd(12)) {
      case 0:
      case 1:
      case 2: {  // insert / emplace
        int v = next_val++;
        if constexpr (multi) {
          auto it = rnd(2) ? c.insert({k, v}) : c.emplace(k, v);
          CHECK(it->first == k && it->second == v);
          inserted(k, v);
        } else {
          auto [it, ok] = rnd(2) ? c.insert({k, v}) : c.emplace(k, v);
          CHECK(it->first == k);
          CHECK(ok == oracle[k].empty());
          if (ok) inserted(k, v);
        }
        break;
      }
      case 3: {  // emplace_hint with a random hint (or one into the group)
        int v = next_val++;
        auto hint = c.end();
        if (!c.empty()) {
          hint = rnd(2) ? c.find(k) : std::next(c.begin(), static_cast<long>(rnd(static_cast<unsigned>(c.size()))));
        }
        bool had = !oracle[k].empty();
        auto it = c.emplace_hint(hint, k, v);
        CHECK(it->first == k);
        if (multi || !had) inserted(k, v);
        else CHECK(it->second == oracle[k][0]);
        break;
      }
      case 4:
      case 5: {  // erase(k)
        std::size_t want = oracle[k].size();
        CHECK(c.erase(k) == want);
        total -= want;
        oracle[k].clear();
        break;
      }
      case 6:
      case 7: {  // erase(q) of a random element of the group
        auto& g = oracle[k];
        if (g.empty()) break;
        auto [b, e] = c.equal_range(k);
        std::size_t idx = rnd(static_cast<unsigned>(g.size()));
        auto q = std::next(b, static_cast<long>(idx));
        auto after = std::next(q);
        auto r = c.erase(q);
        CHECK(r == after);
        g.erase(g.begin() + static_cast<long>(idx));
        --total;
        (void)e;
        break;
      }
      case 8: {  // extract one element and put it back with a different key
        auto& g = oracle[k];
        if (g.empty()) break;
        auto node = c.extract(k);  // "an element ... with key equivalent to k"
        CHECK(!node.empty() && node.key() == k);
        auto where = std::find(g.begin(), g.end(), node.mapped());
        CHECK(where != g.end());
        g.erase(where);
        --total;
        int nk = static_cast<int>(rnd(static_cast<unsigned>(key_range)));
        node.key() = nk;
        int v = node.mapped();
        bool had = !oracle[nk].empty();
        if constexpr (multi) {
          auto it = rnd(2) ? c.insert(std::move(node)) : c.insert(c.begin(), std::move(node));
          CHECK(it->first == nk && it->second == v);
          inserted(nk, v);
        } else {
          auto res = c.insert(std::move(node));
          CHECK(res.inserted == !had);
          CHECK(res.position->first == nk);
          if (res.inserted) {
            CHECK(res.node.empty());
            inserted(nk, v);
          } else {
            CHECK(!res.node.empty() && res.node.mapped() == v);
          }
        }
        break;
      }
      case 9: {  // rehash / reserve / max_load_factor, with pointer stability
        std::vector<std::pair<const int, int>*> ptrs;
        for (auto& e : c) ptrs.push_back(&e);
        std::vector<std::pair<int, int>> vals;
        for (auto* p : ptrs) vals.emplace_back(p->first, p->second);
        switch (rnd(4)) {
          case 0: {
            c.rehash(rnd(3) == 0 ? 0 : rnd(500));
            mlf_changed = false;
            break;
          }
          case 1: {
            std::size_t n = rnd(300);
            c.reserve(n);  // rehash(ceil(n / max_load_factor()))
            CHECK(static_cast<double>(c.bucket_count()) >= static_cast<double>(n) / c.max_load_factor());
            mlf_changed = false;
            break;
          }
          case 2: {
            c.max_load_factor(0.25f + static_cast<float>(rnd(16)) * 0.25f);
            mlf_changed = true;
            break;
          }
          default: {
            std::size_t n = rnd(200);
            c.rehash(n);
            CHECK(c.bucket_count() >= n);
            mlf_changed = false;
          }
        }
        // rehash(n): bucket_count() >= size() / max_load_factor() (reserve is a rehash)
        if (!mlf_changed)
          CHECK(static_cast<double>(c.bucket_count()) >= static_cast<double>(c.size()) / c.max_load_factor());
        for (std::size_t i = 0; i < ptrs.size(); ++i)
          CHECK(ptrs[i]->first == vals[i].first && ptrs[i]->second == vals[i].second);
        break;
      }
      case 10: {  // insert_range of a few pairs
        std::vector<std::pair<int, int>> r;
        for (unsigned i = 0, m = rnd(6); i < m; ++i) r.emplace_back(static_cast<int>(rnd(static_cast<unsigned>(key_range))), next_val++);
        c.insert_range(r);
        if constexpr (multi) {
          std::map<int, std::vector<int>> added;
          for (auto& [rk, rv] : r) added[rk].push_back(rv);
          for (auto& [rk, vs] : added) {
            // the group is the old one with the new values inserted somewhere
            std::vector<int> g = group(rk), without;
            CHECK(g.size() == oracle[rk].size() + vs.size());
            for (int x : g)
              if (std::find(vs.begin(), vs.end(), x) == vs.end()) without.push_back(x);
            CHECK(without == oracle[rk]);
            oracle[rk] = g;
            total += vs.size();
          }
        } else {
          // each element is inserted iff its key is absent at that point: the first one wins
          for (auto& [rk, rv] : r)
            if (oracle[rk].empty()) inserted(rk, rv);
        }
        break;
      }
      default: {  // lookups of absent keys
        int absent = key_range + static_cast<int>(rnd(50));
        CHECK(c.find(absent) == c.end() && c.count(absent) == 0 && !c.contains(absent));
        auto [b, e] = c.equal_range(absent);
        CHECK(b == e);
      }
    }
  }
};

template <class C>
void run(int mode, int keys, int steps) {
  Tester<C> t(mode, keys);
  for (int i = 0; i < steps; ++i) {
    t.step();
    if (steps < 3000 || i % 37 == 0) t.verify_all();
  }
  t.verify_all();
  // clear, then reuse
  t.c.clear();
  t.oracle.clear();
  t.total = 0;
  t.verify_all();
  for (int i = 0; i < 200; ++i) t.step();
  t.verify_all();
}

int main() {
  using UM = std::unordered_map<int, int, ModHash>;
  using UMM = std::unordered_multimap<int, int, ModHash>;
  for (int mode : {0, 1, 2}) {
    for (int keys : {1, 5, 40, 1000}) {
      int steps = mode == 2 ? 600 : 2000;
      run<UM>(mode, keys, steps);
      run<UMM>(mode, keys, steps);
    }
  }
  run<UMM>(0, 300, 20000);
  run<UM>(1, 3000, 20000);
}
