// Libycxx performance policy: 3 N ceil(log2 M) + 3 N comparisons for the fixed inputs.
// The finite comparison budget is a regression heuristic, not an exact draft bound or
// proof of asymptotic/average-case complexity. Normative effects remain independent.
// [partial.sort.copy]: with N = min(last - first, result_last - result_first), "Places the
// first N elements as sorted with respect to comp and proj2 into the range [result_first,
// result_first + N)"; returns result_first + N (std), {last, result_first + N} (ranges,
// ranges::partial_sort_copy_result). The source only needs input iterators; the ranges
// form takes separate projections proj1 (source) and proj2 (result).
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.sort/partial.sort.copy/partial_sort_copy(_comp)?.pass.cpp
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct Wrapped {
  int v;
  constexpr Wrapped() : v(0) {}
  constexpr Wrapped(int x) : v(x) {}  // the source int is writable to a Wrapped
};

constexpr bool test() {
  {
    int src[] = {7, 3, 9, 1, 5, 2};
    int out[3] = {};
    int* e = std::partial_sort_copy(src, src + 6, out, out + 3);
    if (e != out + 3 || out[0] != 1 || out[1] != 2 || out[2] != 3) return false;
    // the source is unchanged
    if (src[0] != 7 || src[5] != 2) return false;
  }
  {
    // result longer than the source: N = last - first
    int src[] = {4, 2, 3};
    int out[5] = {-1, -1, -1, -1, -1};
    int* e = std::partial_sort_copy(src, src + 3, out, out + 5);
    if (e != out + 3 || out[0] != 2 || out[1] != 3 || out[2] != 4 || out[3] != -1) return false;
  }
  {
    // input iterators, comparator, class-type random-access result
    int src[] = {1, 6, 3, 8, 2};
    int out[2] = {};
    RandomIter<int> e = std::partial_sort_copy(InputIter<int>(src), InputIter<int>(src + 5), RandomIter<int>(out),
                                               RandomIter<int>(out + 2), std::greater<>{});
    if (e.p != out + 2 || out[0] != 8 || out[1] != 6) return false;
  }
  {
    // empty result range
    int src[] = {1, 2};
    int out[1] = {42};
    if (std::partial_sort_copy(src, src + 2, out, out) != out || out[0] != 42) return false;
    if (std::partial_sort_copy(src, src, out, out + 1) != out || out[0] != 42) return false;
  }
  {
    // ranges: iterator/sentinel form with an input source, result type
    int src[] = {5, 4, 3, 2, 1};
    int out[2] = {};
    auto r = std::ranges::partial_sort_copy(InputIter<int>(src), PtrSentinel<int>{src + 5}, out, out + 2);
    static_assert(std::is_same_v<decltype(r), std::ranges::partial_sort_copy_result<InputIter<int>, int*>>);
    if (r.in.p != src + 5 || r.out != out + 2 || out[0] != 1 || out[1] != 2) return false;
  }
  {
    // ranges: range form, proj1 on the source and proj2 on the result, different types
    int src[] = {30, 10, 20, 40};
    Wrapped out[3];
    auto r = std::ranges::partial_sort_copy(src, out, std::ranges::greater{}, std::identity{}, &Wrapped::v);
    static_assert(std::is_same_v<decltype(r), std::ranges::partial_sort_copy_result<int*, Wrapped*>>);
    if (r.in != src + 4 || r.out != out + 3) return false;
    if (out[0].v != 40 || out[1].v != 30 || out[2].v != 20) return false;
  }
  {
    // ranges: the whole input is consumed even if the result is short
    int src[] = {9, 1, 8};
    int out[1] = {};
    auto r = std::ranges::partial_sort_copy(src, out);
    if (r.in != src + 3 || r.out != out + 1 || out[0] != 1) return false;
  }
  return true;
}

static_assert(test());

constexpr int N = 2000;
int big[N], out[N], original[N];

int main() {
  CHECK(test());
  // [partial.sort.copy]/7: "Approximately (last - first) * log N comparisons, and twice as
  // many projections" (generous constant).
  for (int m : {1, 10, 100, 2000}) {
    for (Pattern p : all_patterns) {
      fill_pattern(big, N, p);
      for (int i = 0; i < N; ++i) original[i] = big[i];
      int comps = 0, projs = 0;
      auto r = std::ranges::partial_sort_copy(big, big + N, out, out + m, CountingLess{&comps}, CountingProj{&projs},
                                              CountingProj{&projs});
      CHECK(r.in == big + N && r.out == out + m);
      for (int i = 0; i < N; ++i) CHECK(big[i] == original[i]);
      CHECK(sorted_by(out, out + m));
      CHECK(comps <= 3LL * N * ceil_log2(m) + 3LL * N);
      CHECK(projs <= 2 * comps);
      // the result holds the m smallest
      std::sort(big, big + N);
      for (int i = 0; i < m; ++i) CHECK(out[i] == big[i]);
    }
  }
  return 0;
}
