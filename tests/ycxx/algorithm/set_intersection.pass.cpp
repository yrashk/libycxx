// [set.intersection]: "If [first1, last1) contains m elements that are equivalent to each
// other and [first2, last2) contains n elements that are equivalent to them, the first
// min(m, n) elements from the first range are included in the sorted intersection." Returns
// result_last (std) and {last1, last2, result + N} for the non-parallel ranges overloads
// (set_intersection_result = in_in_out_result) -- both inputs reported at their ends even
// when one is not fully examined. "Remarks: Stable".
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.set.operations/set.intersection/set_intersection_complexity.pass.cpp
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
    int a[] = {1, 2, 2, 2, 4, 6};
    int b[] = {2, 2, 3, 4, 7};
    int out[6] = {};
    int* e = std::set_intersection(a, a + 6, b, b + 5, out);
    if (e != out + 3 || out[0] != 2 || out[1] != 2 || out[2] != 4) return false;
  }
  {
    // the first min(m, n) elements of the FIRST range
    KV a[] = {{2, 0}, {2, 1}, {2, 2}, {4, 3}};
    KV b[] = {{2, 10}, {2, 11}, {4, 12}, {4, 13}};
    KV out[4];
    KV* e = std::set_intersection(a, a + 4, b, b + 4, out);
    if (e != out + 3 || out[0].id != 0 || out[1].id != 1 || out[2].id != 3) return false;
    e = std::set_intersection(b, b + 4, a, a + 4, out);
    if (e != out + 3 || out[0].id != 10 || out[1].id != 11 || out[2].id != 12) return false;
  }
  {
    // comparator, input iterators, disjoint and empty inputs
    int a[] = {9, 6, 3};
    int b[] = {8, 6, 3, 1};
    int out[3] = {};
    int* e = std::set_intersection(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b), InputIter<int>(b + 4),
                                   out, std::greater<>{});
    if (e != out + 2 || out[0] != 6 || out[1] != 3) return false;
    int c[] = {1, 5};
    int d[] = {2, 7};
    if (std::set_intersection(c, c + 2, d, d + 2, out) != out) return false;
    if (std::set_intersection(c, c, d, d + 2, out) != out) return false;
  }
  {
    // ranges: {last1, last2, result + N} even though the first range ends early
    KV a[] = {{1, 0}, {2, 1}};
    Other b[] = {{2, 100}, {5, 101}, {6, 102}, {7, 103}};
    Out out[2];
    auto r = std::ranges::set_intersection(a, b, out, {}, &KV::key, &Other::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::set_intersection_result<KV*, Other*, Out*>>);
    static_assert(std::is_same_v<std::ranges::set_intersection_result<KV*, Other*, Out*>,
                                 std::ranges::in_in_out_result<KV*, Other*, Out*>>);
    if (r.in1 != a + 2 || r.in2 != b + 4 || r.out != out + 1 || out[0].id != 1) return false;
    int x[] = {1, 2, 3, 4};
    int y[] = {1};
    int o[1] = {};
    auto r2 = std::ranges::set_intersection(InputIter<int>(x), PtrSentinel<int>{x + 4}, InputIter<int>(y),
                                            PtrSentinel<int>{y + 1}, o);
    if (r2.in1.p != x + 4 || r2.in2.p != y + 1 || r2.out != o + 1) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
