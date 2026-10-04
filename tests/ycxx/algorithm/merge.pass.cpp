// [alg.merge]: merge copies both sorted ranges into [result, result + N); "If an element a
// precedes b in an input range, a is copied into the output range before b. If e1 is an
// element of [first1, last1) and e2 of [first2, last2), e2 is copied into the output range
// before e1 if and only if E is true", E = bool(invoke(comp, invoke(proj2, e2),
// invoke(proj1, e1))) -- so equivalent elements of the first range come first. Returns
// result + N (std) and {last1, last2, result + N} (ranges, merge_result =
// in_in_out_result). "at most N - 1 comparisons and applications of each projection".
// Input iterators suffice.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct Other {
  int key;
  int tag;
};
struct Out {
  int id = -1;
  constexpr Out() = default;
  constexpr Out(const KV& k) : id(k.id) {}
  constexpr Out(const Other& o) : id(o.tag) {}
};

constexpr bool test() {
  {
    int a[] = {1, 3, 5, 7};
    int b[] = {2, 3, 6};
    int out[7] = {};
    int* e = std::merge(a, a + 4, b, b + 3, out);
    int want[] = {1, 2, 3, 3, 5, 6, 7};
    if (e != out + 7) return false;
    for (int i = 0; i < 7; ++i)
      if (out[i] != want[i]) return false;
  }
  {
    // stability: equivalent elements from the first range precede those from the second
    KV a[] = {{1, 0}, {2, 1}, {2, 2}};
    KV b[] = {{1, 10}, {2, 11}, {3, 12}};
    KV out[6];
    std::merge(a, a + 3, b, b + 3, out);
    int ids[] = {0, 10, 1, 2, 11, 12};
    for (int i = 0; i < 6; ++i)
      if (out[i].id != ids[i]) return false;
  }
  {
    // comparator; input iterators; one range empty
    int a[] = {9, 5, 1};
    int b[] = {8, 2};
    int out[5] = {};
    int* e = std::merge(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b), InputIter<int>(b + 2), out,
                        std::greater<>{});
    if (e != out + 5 || out[0] != 9 || out[1] != 8 || out[2] != 5 || out[3] != 2 || out[4] != 1) return false;
    e = std::merge(a, a, b, b + 2, out);
    if (e != out + 2 || out[0] != 8) return false;
    e = std::merge(a, a, b, b, out);
    if (e != out) return false;
  }
  {
    // ranges: {last1, last2, result + N}; projections of different source types
    KV a[] = {{1, 0}, {4, 1}};
    Other b[] = {{1, 100}, {2, 101}, {5, 102}};
    Out out[5];
    auto r = std::ranges::merge(a, b, out, {}, &KV::key, &Other::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::merge_result<KV*, Other*, Out*>>);
    static_assert(std::is_same_v<std::ranges::merge_result<KV*, Other*, Out*>, std::ranges::in_in_out_result<KV*, Other*, Out*>>);
    if (r.in1 != a + 2 || r.in2 != b + 3 || r.out != out + 5) return false;
    int ids[] = {0, 100, 101, 1, 102};
    for (int i = 0; i < 5; ++i)
      if (out[i].id != ids[i]) return false;
  }
  {
    // ranges: iterator/sentinel form with input iterators
    int a[] = {1, 4};
    int b[] = {2, 3};
    int out[4] = {};
    auto r = std::ranges::merge(InputIter<int>(a), PtrSentinel<int>{a + 2}, InputIter<int>(b), PtrSentinel<int>{b + 2}, out);
    if (r.in1.p != a + 2 || r.in2.p != b + 2 || r.out != out + 4) return false;
    if (out[0] != 1 || out[1] != 2 || out[2] != 3 || out[3] != 4) return false;
  }
  return true;
}

static_assert(test());

int x[1000], y[1500], out[2500];

int main() {
  CHECK(test());
  for (unsigned seed = 1; seed <= 5; ++seed) {
    fill_pattern(x, 1000, Pattern::random, seed);
    fill_pattern(y, 1500, Pattern::few_values, seed);
    std::sort(x, x + 1000);
    std::sort(y, y + 1500);
    int comps = 0;
    std::merge(x, x + 1000, y, y + 1500, out, CountingLess{&comps});
    CHECK(comps <= 2499);
    CHECK(sorted_by(out, out + 2500));
    comps = 0;
    int p1 = 0, p2 = 0;
    auto r = std::ranges::merge(x, y, out, CountingLess{&comps}, CountingProj{&p1}, CountingProj{&p2});
    CHECK(r.out == out + 2500);
    CHECK(comps <= 2499 && p1 <= 2499 && p2 <= 2499);
  }
  return 0;
}
