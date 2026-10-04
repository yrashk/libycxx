// [alg.partitions]: is_partitioned "Returns: true if and only if the elements e of [first,
// last) are partitioned with respect to the expression bool(invoke(pred, invoke(proj, e)))"
// (at most last - first applications). partition "Places all the elements e in [first,
// last) that satisfy E(e) before all the elements that do not" and returns the boundary i
// (ranges: {i, last}); "exactly N applications of the predicate and projection"; it works
// on forward iterators. partition_point returns mid with E true on [first, mid) and false
// on [mid, last), in "O(log(last - first)) applications of pred and proj".
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct P {
  int k;
};
constexpr bool is_even(int x) { return x % 2 == 0; }

constexpr bool partitioned_at(const int* a, int n, int mid, bool (*pred)(int)) {
  for (int i = 0; i < n; ++i)
    if (pred(a[i]) != (i < mid)) return false;
  return true;
}

constexpr bool test() {
  {
    int a[] = {2, 4, 1, 3};
    if (!std::is_partitioned(a, a + 4, is_even)) return false;
    int b[] = {2, 1, 4};
    if (std::is_partitioned(b, b + 3, is_even)) return false;
    if (!std::is_partitioned(b, b, is_even)) return false;
    int c[] = {1, 3};  // all false
    if (!std::is_partitioned(c, c + 2, is_even)) return false;
    if (!std::is_partitioned(InputIter<int>(a), InputIter<int>(a + 4), is_even)) return false;
  }
  {
    int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    int orig[9];
    for (int i = 0; i < 9; ++i) orig[i] = a[i];
    int calls = 0;
    int* mid = std::partition(a, a + 9, [&calls](int x) {
      ++calls;
      return x % 2 == 0;
    });
    if (calls != 9) return false;
    if (mid != a + 4 || !partitioned_at(a, 9, 4, is_even) || !same_multiset(a, orig, 9)) return false;
  }
  {
    // forward iterators
    int a[] = {1, 1, 2, 3, 4, 6};
    int orig[6];
    for (int i = 0; i < 6; ++i) orig[i] = a[i];
    ForwardIter<int> m = std::partition(ForwardIter<int>(a), ForwardIter<int>(a + 6), is_even);
    if (m.p != a + 3 || !partitioned_at(a, 6, 3, is_even) || !same_multiset(a, orig, 6)) return false;
  }
  {
    // edge cases
    int a[] = {1, 3, 5};
    if (std::partition(a, a + 3, is_even) != a) return false;
    int b[] = {2, 4};
    if (std::partition(b, b + 2, is_even) != b + 2) return false;
    if (std::partition(b, b, is_even) != b) return false;
  }
  {
    // ranges::partition returns {i, last}; projection
    P ps[] = {{1}, {2}, {3}, {4}};
    auto r = std::ranges::partition(ps, is_even, &P::k);
    static_assert(std::is_same_v<decltype(r), std::ranges::subrange<P*>>);
    if (r.begin() != ps + 2 || r.end() != ps + 4) return false;
    if (ps[0].k % 2 != 0 || ps[1].k % 2 != 0 || ps[2].k % 2 == 0 || ps[3].k % 2 == 0) return false;
    if (!std::ranges::is_partitioned(ps, is_even, &P::k)) return false;
    if (std::ranges::is_partitioned(ps, [](const P& p) { return p.k % 2 != 0; })) return false;
    int a[] = {5, 6, 7, 8};
    ForwardRange<int> fr{a, a + 4};
    auto fr_r = std::ranges::partition(fr, is_even);
    if (fr_r.begin().p != a + 2 || fr_r.end().p != a + 4) return false;
    if (!std::ranges::is_partitioned(fr, is_even)) return false;
  }
  {
    // partition_point
    int a[] = {2, 4, 6, 1, 3};
    if (std::partition_point(a, a + 5, is_even) != a + 3) return false;
    if (std::partition_point(a, a + 3, is_even) != a + 3) return false;
    if (std::partition_point(a + 3, a + 5, is_even) != a + 3) return false;
    if (std::partition_point(a, a, is_even) != a) return false;
    ForwardIter<int> f = std::partition_point(ForwardIter<int>(a), ForwardIter<int>(a + 5), is_even);
    if (f.p != a + 3) return false;
    P ps[] = {{0}, {2}, {5}};
    if (std::ranges::partition_point(ps, is_even, &P::k) != ps + 2) return false;
    if (std::ranges::partition_point(a, a + 5, is_even) != a + 3) return false;
  }
  return true;
}

static_assert(test());

int big[1024];

int main() {
  CHECK(test());
  for (Pattern p : all_patterns) {
    fill_pattern(big, 1024, p);
    int pc = 0, jc = 0;
    auto r = std::ranges::partition(big, [&pc](int x) { ++pc; return x % 3 == 0; },
                                    [&jc](int x) { ++jc; return x; });
    CHECK(pc == 1024 && jc == 1024);
    for (int* i = big; i != r.begin(); ++i) CHECK(*i % 3 == 0);
    for (int* i = r.begin(); i != big + 1024; ++i) CHECK(*i % 3 != 0);
    // is_partitioned: at most N applications
    pc = 0;
    CHECK(std::is_partitioned(big, big + 1024, [&pc](int x) { ++pc; return x % 3 == 0; }));
    CHECK(pc <= 1024);
  }
  // partition_point: logarithmic
  for (int i = 0; i < 1024; ++i) big[i] = i < 300 ? 0 : 1;
  int pc = 0;
  CHECK(std::partition_point(big, big + 1024, [&pc](int x) { ++pc; return x == 0; }) == big + 300);
  CHECK(pc <= 2 * floor_log2(1024) + 2);
  return 0;
}
