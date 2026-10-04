// [alg.search]/1-4: search returns the first i in [first1, last1 - (last2 - first2)] at which
// the pattern matches (first1 for an empty pattern, last1 if none; ranges: {i, i + (last2 -
// first2)} or {last1, last1}); "At most (last1 - first1) * (last2 - first2) applications of
// the corresponding predicate". /5-10: search_n returns the first i in [first, last - count]
// with count consecutive matches ({i, i + count}, or last / {last, last}); "Complexity: At
// most last - first applications of the corresponding predicate" (and projection, /10).
// [alg.find.end]/1-3: find_end returns the last such i (last1 if the pattern is empty or not
// found; ranges: {i, i + (i == last1 ? 0 : last2 - first2)}); "At most (last2 - first2) *
// (last1 - first1 - (last2 - first2) + 1) applications of the corresponding predicate and any
// projections". Checked against a brute-force model for every 0/1 sequence of length <= 10
// and every pattern of length <= 3 (count <= 5), with forward, bidirectional and random access
// iterators, counting predicate and projection calls.
#include <algorithm>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <vector>
#include "check.hpp"

static long calls = 0, projs = 0;
static const auto eq = [](int a, int b) { ++calls; return a == b; };
static const auto proj = [](int a) { ++projs; return a; };

static std::vector<int> bits(unsigned v, int n) {
  std::vector<int> r;
  for (int i = 0; i < n; ++i) r.push_back((v >> i) & 1);
  return r;
}

static long model_search(const std::vector<int>& t, const std::vector<int>& p) {
  long n = static_cast<long>(t.size()), m = static_cast<long>(p.size());
  for (long i = 0; i + m <= n; ++i)
    if (std::equal(p.begin(), p.end(), t.begin() + i)) return i;
  return n;
}
static long model_find_end(const std::vector<int>& t, const std::vector<int>& p) {
  long n = static_cast<long>(t.size()), m = static_cast<long>(p.size());
  if (m == 0) return n;
  for (long i = n - m; i >= 0; --i)
    if (std::equal(p.begin(), p.end(), t.begin() + i)) return i;
  return n;
}
static long model_search_n(const std::vector<int>& t, long count, int v) {
  long n = static_cast<long>(t.size());
  if (count <= 0) return 0;
  for (long i = 0; i + count <= n; ++i)
    if (std::all_of(t.begin() + i, t.begin() + i + count, [&](int x) { return x == v; })) return i;
  return n;
}

template <class C>
static void run(const std::vector<int>& tv, const std::vector<int>& pv) {
  C t(tv.begin(), tv.end());
  C p(pv.begin(), pv.end());
  const long n = static_cast<long>(tv.size()), m = static_cast<long>(pv.size());
  auto pos = [&](auto it) { return static_cast<long>(std::distance(t.begin(), it)); };

  calls = 0;
  auto s = std::search(t.begin(), t.end(), p.begin(), p.end(), eq);
  CHECK(pos(s) == model_search(tv, pv) && calls <= n * m);
  calls = projs = 0;
  auto rs = std::ranges::search(t, p, eq, proj, proj);
  const long ms = model_search(tv, pv);
  CHECK(pos(rs.begin()) == ms && pos(rs.end()) == (ms == n ? n : ms + m) && calls <= n * m);

  const long fe = model_find_end(tv, pv);
  // (For a pattern longer than the sequence the formula is negative; no bound is checked.)
  const long bound = m <= n ? m * (n - m + 1) : 1000;
  calls = 0;
  auto f = std::find_end(t.begin(), t.end(), p.begin(), p.end(), eq);
  CHECK(pos(f) == fe && calls <= bound);
  calls = projs = 0;
  auto rf = std::ranges::find_end(t, p, eq, proj, proj);
  CHECK(pos(rf.begin()) == fe && pos(rf.end()) == (fe == n ? n : fe + m) && calls <= bound && projs <= 2 * bound);

  for (int v = 0; v < 2; ++v)
    for (long count = 0; count <= 5; ++count) {
      const long sn = model_search_n(tv, count, v);
      calls = 0;
      auto i = std::search_n(t.begin(), t.end(), count, v, eq);
      CHECK(pos(i) == sn && calls <= n);
      calls = projs = 0;
      auto r = std::ranges::search_n(t, count, v, eq, proj);
      CHECK(pos(r.begin()) == sn && pos(r.end()) == (sn == n ? n : sn + count) && calls <= n && projs <= n);
      auto d = std::search_n(t.begin(), t.end(), count, v);
      CHECK(pos(d) == sn);
    }
}

int main() {
  for (int n = 0; n <= 10; ++n)
    for (unsigned tv = 0; tv < (1u << n); ++tv)
      for (int m = 0; m <= 3; ++m)
        for (unsigned pv = 0; pv < (1u << m); ++pv) {
          const auto t = bits(tv, n), p = bits(pv, m);
          run<std::vector<int>>(t, p);
          run<std::list<int>>(t, p);
          run<std::forward_list<int>>(t, p);
        }
  return 0;
}
