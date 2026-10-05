// Merging, set operations, partitions and unique on pseudo-random inputs with many equivalent
// elements, std:: and ranges:: (with projections), checked against group-by-group oracles.
// Elements carry a key and an id, so stability and the choice of copied elements are visible.
// [alg.merge]: merge and inplace_merge are stable ([algorithm.stable]: of equivalent elements
// those of the first range come first, each range in order).
// [set.union]/1: of m equivalent elements in the first range and n in the second, "all m
// elements from the first range are included in the union, in order, and then the final
// max(n - m, 0) elements from the second range"; [set.intersection]: "the first min(m, n)
// elements from the first range"; [set.difference]: "the last max(m - n, 0) elements from
// [first1, last1) ..., in order"; [set.symmetric.difference]: "the last m - n of these elements
// from [first1, last1), in order, if m > n, and the last n - m ... from [first2, last2)";
// [includes]: true iff every group of the second range has n <= m. The ranges forms return
// {last1, last2, result_end} (in, in1, in2, out as applicable).
// [alg.partitions]: partition puts every element satisfying the predicate first and returns the
// partition point; stable_partition keeps the relative order inside both groups;
// partition_copy copies, in order, to the true / false outputs. [alg.unique]: unique keeps the
// first element of each run of equivalent consecutive elements, in order.
#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <ranges>
#include <vector>
#include "check.hpp"

struct E {
  int key;
  int id;
  friend bool operator==(const E&, const E&) = default;
};
using Vec = std::vector<E>;
const auto lt = [](const E& a, const E& b) { return a.key < b.key; };

unsigned st = 4242u;
unsigned rnd(unsigned n) {
  st ^= st << 13;
  st ^= st >> 17;
  st ^= st << 5;
  return st % n;
}

Vec sorted_input(std::size_t n, unsigned distinct, int id_base) {
  std::vector<int> keys(n);
  for (auto& k : keys) k = static_cast<int>(rnd(distinct));
  // insertion sort (the oracle does not use the library's sort)
  for (std::size_t i = 1; i < n; ++i)
    for (std::size_t j = i; j > 0 && keys[j] < keys[j - 1]; --j) std::swap(keys[j], keys[j - 1]);
  Vec v;
  for (std::size_t i = 0; i < n; ++i) v.push_back({keys[i], id_base + static_cast<int>(i)});
  return v;
}

// The distinct keys of two vectors, ascending or descending (without the library's algorithms).
std::vector<int> distinct_keys(const Vec& a, const Vec& b, bool desc) {
  std::vector<int> keys;
  for (const Vec* v : {&a, &b})
    for (auto& e : *v) {
      bool seen = false;
      for (int k : keys) seen = seen || k == e.key;
      if (!seen) keys.push_back(e.key);
    }
  for (std::size_t i = 1; i < keys.size(); ++i)
    for (std::size_t j = i; j > 0 && (desc ? keys[j] > keys[j - 1] : keys[j] < keys[j - 1]); --j) std::swap(keys[j], keys[j - 1]);
  return keys;
}

// The run of elements with key k in a sorted vector.
Vec group(const Vec& v, int k) {
  Vec g;
  for (auto& e : v)
    if (e.key == k) g.push_back(e);
  return g;
}

enum Op { Union, Inter, Diff, Sym, Merge };
Vec oracle(const Vec& a, const Vec& b, Op op) {
  std::vector<int> keys = distinct_keys(a, b, false);
  Vec out;
  for (int k : keys) {
    Vec ga = group(a, k), gb = group(b, k);
    std::size_t m = ga.size(), n = gb.size();
    switch (op) {
      case Merge:
        out.insert(out.end(), ga.begin(), ga.end());
        out.insert(out.end(), gb.begin(), gb.end());
        break;
      case Union:
        out.insert(out.end(), ga.begin(), ga.end());
        if (n > m) out.insert(out.end(), gb.begin() + static_cast<long>(m), gb.end());
        break;
      case Inter: out.insert(out.end(), ga.begin(), ga.begin() + static_cast<long>(std::min(m, n))); break;
      case Diff:
        if (m > n) out.insert(out.end(), ga.begin() + static_cast<long>(n), ga.end());
        break;
      case Sym:
        if (m > n) out.insert(out.end(), ga.begin() + static_cast<long>(n), ga.end());
        if (n > m) out.insert(out.end(), gb.begin() + static_cast<long>(m), gb.end());
        break;
    }
  }
  return out;
}

bool oracle_includes(const Vec& a, const Vec& b) {
  for (auto& e : b)
    if (group(b, e.key).size() > group(a, e.key).size()) return false;
  return true;
}

void merges_and_set_ops(std::size_t na, std::size_t nb, unsigned distinct) {
  Vec a = sorted_input(na, distinct, 0), b = sorted_input(nb, distinct, 100000);
  std::list<E> la(a.begin(), a.end()), lb(b.begin(), b.end());
  for (Op op : {Merge, Union, Inter, Diff, Sym}) {
    Vec want = oracle(a, b, op);
    Vec out(na + nb + 1, E{-1, -1});
    Vec::iterator end;
    switch (op) {
      case Merge: end = std::merge(la.begin(), la.end(), lb.begin(), lb.end(), out.begin(), lt); break;
      case Union: end = std::set_union(la.begin(), la.end(), lb.begin(), lb.end(), out.begin(), lt); break;
      case Inter: end = std::set_intersection(la.begin(), la.end(), lb.begin(), lb.end(), out.begin(), lt); break;
      case Diff: end = std::set_difference(la.begin(), la.end(), lb.begin(), lb.end(), out.begin(), lt); break;
      case Sym: end = std::set_symmetric_difference(la.begin(), la.end(), lb.begin(), lb.end(), out.begin(), lt); break;
    }
    CHECK(Vec(out.begin(), end) == want);
    CHECK(end == out.begin() + static_cast<long>(want.size()));

    // ranges, projections on both sides, greater-than order on reversed inputs
    Vec ra(a.rbegin(), a.rend()), rb(b.rbegin(), b.rend());
    Vec out2(na + nb + 1, E{-1, -1});
    auto g = std::ranges::greater{};
    switch (op) {
      case Merge: {
        auto r = std::ranges::merge(ra, rb, out2.begin(), g, &E::key, &E::key);
        CHECK(r.in1 == ra.end() && r.in2 == rb.end());
        end = r.out;
        break;
      }
      case Union: {
        auto r = std::ranges::set_union(ra, rb, out2.begin(), g, &E::key, &E::key);
        CHECK(r.in1 == ra.end() && r.in2 == rb.end());
        end = r.out;
        break;
      }
      case Inter: {
        auto r = std::ranges::set_intersection(ra, rb, out2.begin(), g, &E::key, &E::key);
        CHECK(r.in1 == ra.end() && r.in2 == rb.end());
        end = r.out;
        break;
      }
      case Diff: {
        auto r = std::ranges::set_difference(ra, rb, out2.begin(), g, &E::key, &E::key);
        CHECK(r.in == ra.end());
        end = r.out;
        break;
      }
      case Sym: {
        auto r = std::ranges::set_symmetric_difference(ra, rb, out2.begin(), g, &E::key, &E::key);
        CHECK(r.in1 == ra.end() && r.in2 == rb.end());
        end = r.out;
        break;
      }
    }
    // build the descending oracle directly from the reversed inputs
    Vec dwant;
    {
      std::vector<int> keys = distinct_keys(ra, rb, true);
      for (int k : keys) {
        Vec ga = group(ra, k), gb = group(rb, k);
        std::size_t m = ga.size(), n = gb.size();
        auto add = [&](const Vec& gr, std::size_t from, std::size_t to) {
          dwant.insert(dwant.end(), gr.begin() + static_cast<long>(from), gr.begin() + static_cast<long>(to));
        };
        switch (op) {
          case Merge: add(ga, 0, m); add(gb, 0, n); break;
          case Union: add(ga, 0, m); if (n > m) add(gb, m, n); break;
          case Inter: add(ga, 0, std::min(m, n)); break;
          case Diff: if (m > n) add(ga, n, m); break;
          case Sym: if (m > n) add(ga, n, m); if (n > m) add(gb, m, n); break;
        }
      }
    }
    CHECK(Vec(out2.begin(), end) == dwant);
  }
  CHECK(std::includes(a.begin(), a.end(), b.begin(), b.end(), lt) == oracle_includes(a, b));
  CHECK(std::ranges::includes(a, b, {}, &E::key, &E::key) == oracle_includes(a, b));
  CHECK(std::ranges::includes(b, a, {}, &E::key, &E::key) == oracle_includes(b, a));
  CHECK(std::includes(a.begin(), a.end(), a.begin(), a.end(), lt));

  // inplace_merge of a ++ b (bidirectional list and vector, std and ranges)
  Vec want = oracle(a, b, Merge);
  Vec ab = a;
  ab.insert(ab.end(), b.begin(), b.end());
  Vec v = ab;
  std::inplace_merge(v.begin(), v.begin() + static_cast<long>(na), v.end(), lt);
  CHECK(v == want);
  std::list<E> l(ab.begin(), ab.end());
  auto mid = std::next(l.begin(), static_cast<long>(na));
  CHECK(std::ranges::inplace_merge(l, mid, {}, &E::key) == l.end());
  CHECK(Vec(l.begin(), l.end()) == want);
  v = ab;
  CHECK(std::ranges::inplace_merge(v.begin(), v.begin() + static_cast<long>(na), v.end(), std::ranges::less{}, &E::key) == v.end());
  CHECK(v == want);
}

void partitions_and_unique(std::size_t n, unsigned distinct) {
  Vec in;
  for (std::size_t i = 0; i < n; ++i) in.push_back({static_cast<int>(rnd(distinct)), static_cast<int>(i)});
  for (int modulus : {1, 2, 3}) {
    auto pred = [&](int k) { return k % modulus == 0; };
    auto epred = [&](const E& e) { return pred(e.key); };
    Vec yes, no;
    for (auto& e : in) (pred(e.key) ? yes : no).push_back(e);
    Vec stable_want = yes;
    stable_want.insert(stable_want.end(), no.begin(), no.end());

    Vec v = in;
    auto p = std::stable_partition(v.begin(), v.end(), epred);
    CHECK(v == stable_want && p == v.begin() + static_cast<long>(yes.size()));
    std::list<E> l(in.begin(), in.end());
    auto rp = std::ranges::stable_partition(l, pred, &E::key);
    CHECK(Vec(l.begin(), l.end()) == stable_want);
    CHECK(rp.begin() == std::next(l.begin(), static_cast<long>(yes.size())) && rp.end() == l.end());

    v = in;
    p = std::partition(v.begin(), v.end(), epred);
    CHECK(p == v.begin() + static_cast<long>(yes.size()));
    CHECK(std::all_of(v.begin(), p, epred) && std::none_of(p, v.end(), epred));
    CHECK(std::is_permutation(v.begin(), v.end(), in.begin(), in.end()));
    CHECK(std::partition_point(v.begin(), v.end(), epred) == p);
    v = in;
    auto rq = std::ranges::partition(v, pred, &E::key);
    CHECK(rq.begin() == v.begin() + static_cast<long>(yes.size()) && rq.end() == v.end());
    CHECK(std::ranges::is_partitioned(v, pred, &E::key));

    Vec t(n, E{-1, -1}), f(n, E{-1, -1});
    auto pc = std::ranges::partition_copy(in, t.begin(), f.begin(), pred, &E::key);
    CHECK(pc.in == in.end());
    CHECK(Vec(t.begin(), pc.out1) == yes && Vec(f.begin(), pc.out2) == no);
  }

  // unique / unique_copy: first of each run of equal keys
  Vec want;
  for (std::size_t i = 0; i < n; ++i)
    if (i == 0 || in[i].key != in[i - 1].key) want.push_back(in[i]);
  Vec v = in;
  auto ue = std::unique(v.begin(), v.end(), [](const E& a, const E& b) { return a.key == b.key; });
  CHECK(Vec(v.begin(), ue) == want);
  v = in;
  auto ur = std::ranges::unique(v, {}, &E::key);
  CHECK(Vec(v.begin(), ur.begin()) == want && ur.end() == v.end());
  Vec out(n, E{-1, -1});
  auto uc = std::ranges::unique_copy(in, out.begin(), {}, &E::key);
  CHECK(uc.in == in.end() && Vec(out.begin(), uc.out) == want);
}

int main() {
  for (int trial = 0; trial < 400; ++trial) {
    std::size_t na = rnd(40), nb = rnd(40);
    unsigned distinct = 1 + rnd(trial % 4 == 0 ? 2 : 12);
    merges_and_set_ops(na, nb, distinct);
    partitions_and_unique(rnd(80), 1 + rnd(6));
  }
  merges_and_set_ops(1500, 1700, 40);
  merges_and_set_ops(0, 1000, 3);
  merges_and_set_ops(1000, 0, 3);
  partitions_and_unique(5000, 4);
}
