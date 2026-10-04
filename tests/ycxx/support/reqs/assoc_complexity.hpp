// Generic run-time check, instantiated per ordered associative container (map, multimap, set,
// multiset, and the flat adaptors) with key_type int and the counting comparator CountLess:
// the complexity requirements stated in comparisons of the comparison object.
//   [associative.reqmts.general]/25, /28, /31, /34: X(i, j[, c]), X(from_range, rg[, c]):
//     "NlogN in general ...; linear if [i, j) is sorted with respect to value_comp()".
//   /60, /74, /99: emplace_hint, insert(p, t), insert(p, nh): "Logarithmic in general, but
//     amortized constant if the element is inserted right before p".
//   /141-174: find, count (log(size) + count), contains, lower_bound, upper_bound,
//     equal_range: logarithmic -- for equal_range also in a multi container where the key has
//     many equivalent elements ("Equivalent to make_pair(b.lower_bound(k),
//     b.upper_bound(k))", logarithmic).
//   /106-111, /127-137: extract(q), erase(q), erase(r): amortized constant; erase(k):
//     log(a.size()) + a.count(k); /114-117: merge: N log(a.size() + N).
//   [flat.map.overview]/2: the flat adaptors meet the associative container requirements
//     except that single-element insertion and erasure (also with a hint) are linear.
//   [flat.map.cons]/2, [flat.set.cons]/2: construction from containers: "Linear in N if the
//     container arguments are already sorted with respect to value_comp()"; /4: from
//     sorted_unique containers: constant. [flat.map.modifiers]/7, /10, /12, /15 and
//     [flat.set.modifiers]: insert(first, last) / insert_range(rg): N + M log M;
//     insert(sorted_unique, first, last) / insert_range(sorted_unique, rg): linear in the
//     size after the operation.
// The draft gives no constants. With N = 2^14 elements, log2(N) = 14 and a balanced tree of
// height at most 2 log2(N + 1), the bounds below (an average of at most 6 comparisons per
// "amortized constant" operation, 4 log2(N) + 4 per "logarithmic" one, 8 per element for
// "linear") are far above any reasonable constant and far below the cost of a logarithmic
// (resp. linear, N log N) algorithm, so they tell the complexity classes apart without
// depending on implementation details.
#pragma once
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>
#include "check.hpp"  // dprintf

namespace reqs::assoc_complexity {

inline long comparisons = 0;

struct CountLess {
  bool operator()(int a, int b) const {
    ++comparisons;
    return a < b;
  }
};

constexpr int N = 1 << 14;
constexpr long LOG_BOUND = 4 * 14 + 4;
constexpr long AMORTIZED = 6;  // per operation, on average
constexpr long LINEAR = 8;     // per element

template <class X>
concept is_map = requires { typename X::mapped_type; };
template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;
template <class X>
concept node_based = requires { typename X::node_type; };
template <class X>
concept is_flat = !node_based<X>;

template <class X>
typename X::value_type v(int k) {
  if constexpr (is_map<X>) return {k, k};
  else return k;
}

template <class X>
std::vector<typename X::value_type> sorted_values(int n, int step = 1) {
  std::vector<typename X::value_type> r;
  for (int i = 0; i < n; ++i) r.push_back(v<X>(i * step));
  return r;
}

// Reports a measurement above its bound (on stderr) so that a failure names the operation.
inline bool within(const char* what, long c, long bound) {
  if (c <= bound) return true;
  dprintf(2, "complexity: %s: %ld comparisons, bound %ld\n", what, c, bound);
  return false;
}

// Comparisons made by f().
template <class F>
long count(F f) {
  comparisons = 0;
  f();
  return comparisons;
}

template <class X>
bool sorted_construction() {
  auto s = sorted_values<X>(N);
  X* keep = nullptr;
  long c1 = count([&] { keep = new X(s.begin(), s.end()); });
  bool ok = keep->size() == static_cast<std::size_t>(N);
  delete keep;
  long c2 = count([&] { keep = new X(std::from_range, s); });
  ok = ok && keep->size() == static_cast<std::size_t>(N);
  delete keep;
  bool a = within("X(i, j), sorted", c1, LINEAR * N);
  bool b = within("X(from_range, rg), sorted", c2, LINEAR * N);
  return ok && a && b;
}

template <class X>
bool lookups() {
  auto s = sorted_values<X>(N, 2);
  X x(s.begin(), s.end());
  if constexpr (is_multi<X>) {
    // key 2 * N: N equivalent elements, between N smaller and N larger keys
    for (int i = 0; i < N; ++i) x.insert(x.end(), v<X>(4 * N));
    for (int i = 0; i < N; ++i) x.insert(x.end(), v<X>(4 * N + 2 + i));
  }
  const X& cx = x;
  for (int k : {0, 1, 2 * N - 2, N, N + 1, 4 * N, 6 * N}) {
    long c[6] = {count([&] { (void)x.find(k); }),
                 count([&] { (void)cx.contains(k); }),
                 count([&] { (void)x.lower_bound(k); }),
                 count([&] { (void)cx.upper_bound(k); }),
                 count([&] { (void)x.equal_range(k); }),
                 count([&] { (void)cx.equal_range(k); })};
    const char* names[6] = {"find", "contains", "lower_bound", "upper_bound", "equal_range", "equal_range const"};
    for (int i = 0; i < 6; ++i)
      if (!within(names[i], c[i], LOG_BOUND)) return false;
    long cnt = static_cast<long>(x.count(k));
    if (!within("count", count([&] { (void)x.count(k); }), LOG_BOUND + 2 * cnt)) return false;
  }
  if constexpr (is_multi<X>) {  // the equivalent range is found in logarithmic time
    auto [lo, hi] = x.equal_range(4 * N);
    if (std::distance(lo, hi) != N || x.lower_bound(4 * N) != lo || x.upper_bound(4 * N) != hi) return false;
  }
  return true;
}

// Hinted insertion right before p is amortized constant.
template <class X>
bool hints() {
  {  // ascending keys, hint end()
    X x;
    long c = count([&] {
      for (int i = 0; i < N; ++i) x.insert(x.end(), v<X>(i));
    });
    if (!within("insert(end(), t), ascending", c, AMORTIZED * N) || x.size() != static_cast<std::size_t>(N))
      return false;
    c = count([&] {  // emplace_hint, descending keys, hint begin()
      for (int i = 0; i < N; ++i) x.emplace_hint(x.begin(), v<X>(-1 - i));
    });
    if (!within("emplace_hint(begin(), t), descending", c, AMORTIZED * N) || x.size() != static_cast<std::size_t>(2 * N))
      return false;
  }
  {  // odd keys into the gaps between even keys, hint = the next element
    auto s = sorted_values<X>(N, 2);
    X x(s.begin(), s.end());
    std::vector<typename X::iterator> next;
    for (auto it = std::next(x.begin()); it != x.end(); ++it) next.push_back(it);
    long c = count([&] {
      for (int i = 0; i + 1 < N; ++i) x.insert(next[i], v<X>(2 * i + 1));
    });
    if (!within("insert(next, t)", c, AMORTIZED * N) || x.size() != static_cast<std::size_t>(2 * N - 1)) return false;
  }
  if constexpr (is_multi<X>) {  // equivalent keys: hint at the first / past the last of them
    X x;
    x.insert(v<X>(5));
    long c = count([&] {
      for (int i = 0; i < N; ++i) x.insert(x.begin(), v<X>(5));
      for (int i = 0; i < N; ++i) x.emplace_hint(x.end(), v<X>(5));
    });
    if (!within("insert / emplace_hint among equivalent keys", c, AMORTIZED * 2 * N) || x.count(5) != static_cast<std::size_t>(2 * N + 1)) return false;
  }
  if constexpr (node_based<X>) {  // insert(p, nh)
    auto s = sorted_values<X>(N);
    X src(s.begin(), s.end());
    X x;
    long c = count([&] {
      for (int i = 0; i < N; ++i) x.insert(x.end(), src.extract(src.begin()));
    });
    if (!within("insert(end(), nh)", c, 2 * AMORTIZED * N) || x.size() != static_cast<std::size_t>(N) || !src.empty()) return false;
  }
  return true;
}

// Erasure and extraction by position are amortized constant; erase(k) is log + count.
template <class X>
bool erasure() {
  auto s = sorted_values<X>(N);
  X x(s.begin(), s.end());
  long c = count([&] {
    for (int i = 0; i < N / 2; ++i) x.erase(x.begin());
  });
  if (!within("erase(begin())", c, AMORTIZED * N)) return false;
  c = count([&] { x.erase(std::next(x.begin(), 10), std::next(x.begin(), 1000)); });
  if (!within("erase(q1, q2)", c, AMORTIZED * 1000)) return false;
  if constexpr (node_based<X>) {
    c = count([&] {
      for (int i = 0; i < 1000; ++i) (void)x.extract(x.begin());
    });
    if (!within("extract(begin())", c, AMORTIZED * 1000)) return false;
  }
  c = count([&] { x.erase(N - 1); });
  if (!within("erase(k)", c, LOG_BOUND) || x.contains(N - 1)) return false;
  if constexpr (is_multi<X>) {
    for (int i = 0; i < 100; ++i) x.insert(v<X>(N - 3));
    c = count([&] { x.erase(N - 3); });
    if (!within("erase(k), 101 equivalent", c, LOG_BOUND + 2 * 101) || x.contains(N - 3)) return false;
  }
  return true;
}

template <class X>
bool merging() {
  auto s = sorted_values<X>(N, 2);
  X a(s.begin(), s.end());
  X b;
  for (int i = 0; i < 64; ++i) b.insert(v<X>(4 * i + 1));
  long c = count([&] { a.merge(b); });
  return within("merge", c, 64 * LOG_BOUND) && b.empty() && a.size() == static_cast<std::size_t>(N + 64);
}

// [flat.map.cons], [flat.set.cons], [flat.map.modifiers], [flat.set.modifiers]
template <class X>
bool flat_specific() {
  using KC = std::vector<int>;
  KC keys;
  for (int i = 0; i < N; ++i) keys.push_back(2 * i);
  auto tag = [] {
    if constexpr (is_multi<X>) return std::sorted_equivalent;
    else return std::sorted_unique;
  }();
  auto make = [&](auto&&... pre) {
    if constexpr (is_map<X>) return X(pre..., keys, keys);
    else return X(pre..., keys);
  };
  bool ok = true;
  long c = count([&] { (void)make(); });  // already sorted: linear
  ok = within("construction from sorted containers", c, LINEAR * N) && ok;
  c = count([&] { (void)make(tag); });  // sorted_unique / sorted_equivalent: constant
  ok = within("construction from sorted_unique / sorted_equivalent containers", c, 8) && ok;
  X x = make(tag);
  auto more = sorted_values<X>(N, 2);
  for (auto& e : more) {
    if constexpr (is_map<X>) e.first += 1, e.second += 1;
    else e += 1;
  }
  c = count([&] { x.insert(tag, more.begin(), more.end()); });  // linear in the size after
  ok = within("insert(sorted_unique, first, last)", c, LINEAR * 2 * N) && ok;
  if (x.size() != static_cast<std::size_t>(2 * N)) return false;
  auto again = sorted_values<X>(N, 2);
  for (auto& e : again) {
    if constexpr (is_map<X>) e.first += 4 * N, e.second += 4 * N;
    else e += 4 * N;
  }
  c = count([&] { x.insert_range(tag, again); });
  ok = within("insert_range(sorted_unique, rg)", c, LINEAR * 3 * N) && ok;
  if (x.size() != static_cast<std::size_t>(3 * N)) return false;
  std::vector<typename X::value_type> few;
  for (int i = 0; i < 16; ++i) few.push_back(v<X>(16 * N - 7 * i));
  c = count([&] { x.insert(few.begin(), few.end()); });  // N + M log M
  ok = within("insert(first, last)", c, LINEAR * (3 * N + 16 * 4)) && ok;
  c = count([&] { x.insert_range(few); });
  return within("insert_range(rg)", c, LINEAR * (3 * N + 16 + 16 * 4)) && ok;
}

// The flat adaptors: [flat.map.overview]/2.3 replaces the hint, insertion and erasure
// complexities by "linear", and their range constructors are specified through insert(first,
// last) (N + M log M), so only the lookups and the flat-specific statements are checked.
template <class X>
bool test() {
  bool ok = lookups<X>();  // every check runs, so that every violation is reported
  if constexpr (node_based<X>) {
    ok = sorted_construction<X>() && ok;
    ok = hints<X>() && ok;
    ok = erasure<X>() && ok;
    ok = merging<X>() && ok;
  } else {
    ok = flat_specific<X>() && ok;
  }
  return ok;
}

}  // namespace reqs::assoc_complexity
