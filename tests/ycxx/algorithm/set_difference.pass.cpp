// [set.difference]: "If [first1, last1) contains m elements that are equivalent to each
// other and [first2, last2) contains n elements that are equivalent to them, the last
// max(m - n, 0) elements from [first1, last1) are included in the sorted difference, in
// order." Returns result_last (std) and {last1, result + N} for the non-parallel ranges
// overloads (set_difference_result = in_out_result). "Remarks: Stable".
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct Other {
  int key;
};
struct Out {
  int id = -1;
  constexpr Out() = default;
  constexpr Out(const KV& k) : id(k.id) {}
  constexpr Out(const Other&) : id(-2) {}  // never written: mergeable requires the conversion
};

constexpr bool test() {
  {
    int a[] = {1, 2, 2, 2, 4, 6};
    int b[] = {2, 3, 4, 7};
    int out[6] = {};
    int* e = std::set_difference(a, a + 6, b, b + 4, out);
    if (e != out + 4 || out[0] != 1 || out[1] != 2 || out[2] != 2 || out[3] != 6) return false;
  }
  {
    // the LAST m - n of the first range's equivalent elements
    KV a[] = {{2, 0}, {2, 1}, {2, 2}, {3, 3}};
    KV b[] = {{2, 10}, {3, 11}, {3, 12}};
    KV out[4];
    KV* e = std::set_difference(a, a + 4, b, b + 3, out);
    if (e != out + 2 || out[0].id != 1 || out[1].id != 2) return false;
  }
  {
    // comparator, input iterators, empty inputs
    int a[] = {9, 6, 3};
    int b[] = {6};
    int out[3] = {};
    int* e = std::set_difference(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b), InputIter<int>(b + 1), out,
                                 std::greater<>{});
    if (e != out + 2 || out[0] != 9 || out[1] != 3) return false;
    if (std::set_difference(a, a, b, b + 1, out) != out) return false;
    if (std::set_difference(b, b + 1, a, a, out) != out + 1 || out[0] != 6) return false;
  }
  {
    // ranges: {last1, result + N}; projections of different types
    KV a[] = {{1, 0}, {2, 1}, {2, 2}, {9, 3}};
    Other b[] = {{2}, {3}, {4}};
    Out out[4];
    auto r = std::ranges::set_difference(a, b, out, {}, &KV::key, &Other::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::set_difference_result<KV*, Out*>>);
    static_assert(std::is_same_v<std::ranges::set_difference_result<KV*, Out*>, std::ranges::in_out_result<KV*, Out*>>);
    if (r.in != a + 4 || r.out != out + 3) return false;
    if (out[0].id != 0 || out[1].id != 2 || out[2].id != 3) return false;
    int x[] = {1, 2};
    int y[] = {5, 6, 7};
    int o[2] = {};
    auto r2 = std::ranges::set_difference(InputIter<int>(x), PtrSentinel<int>{x + 2}, InputIter<int>(y), PtrSentinel<int>{y + 3}, o);
    if (r2.in.p != x + 2 || r2.out != o + 2) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
