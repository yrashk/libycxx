// [set.symmetric.difference]: "If [first1, last1) contains m elements that are equivalent
// to each other and [first2, last2) contains n elements that are equivalent to them, then
// |m - n| of those elements are included in the symmetric difference: the last m - n of
// these elements from [first1, last1), in order, if m > n, and the last n - m of these
// elements from [first2, last2), in order, if m < n." Returns result_last (std) and {last1,
// last2, result + N} (ranges, set_symmetric_difference_result = in_in_out_result).
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
    int a[] = {1, 2, 2, 4, 6};
    int b[] = {2, 3, 4, 4, 6};
    int out[10] = {};
    int* e = std::set_symmetric_difference(a, a + 5, b, b + 5, out);
    int want[] = {1, 2, 3, 4};
    if (e != out + 4) return false;
    for (int i = 0; i < 4; ++i)
      if (out[i] != want[i]) return false;
  }
  {
    // the LAST |m - n| elements of the range holding more of them
    KV a[] = {{1, 0}, {2, 1}, {2, 2}, {2, 3}, {5, 4}};
    KV b[] = {{2, 10}, {5, 11}, {5, 12}, {5, 13}};
    KV out[9];
    KV* e = std::set_symmetric_difference(a, a + 5, b, b + 4, out);
    int ids[] = {0, 2, 3, 12, 13};
    if (e != out + 5) return false;
    for (int i = 0; i < 5; ++i)
      if (out[i].id != ids[i]) return false;
  }
  {
    // comparator, input iterators, empty inputs
    int a[] = {7, 5, 3};
    int b[] = {6, 5};
    int out[5] = {};
    int* e = std::set_symmetric_difference(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b),
                                           InputIter<int>(b + 2), out, std::greater<>{});
    if (e != out + 3 || out[0] != 7 || out[1] != 6 || out[2] != 3) return false;
    if (std::set_symmetric_difference(a, a, b, b, out) != out) return false;
    if (std::set_symmetric_difference(a, a, b, b + 2, out) != out + 2 || out[1] != 5) return false;
  }
  {
    // ranges: result type and members; projections of different types
    KV a[] = {{1, 0}, {4, 1}};
    Other b[] = {{1, 100}, {2, 101}, {2, 102}};
    Out out[4];
    auto r = std::ranges::set_symmetric_difference(a, b, out, {}, &KV::key, &Other::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::set_symmetric_difference_result<KV*, Other*, Out*>>);
    static_assert(std::is_same_v<std::ranges::set_symmetric_difference_result<KV*, Other*, Out*>,
                                 std::ranges::in_in_out_result<KV*, Other*, Out*>>);
    if (r.in1 != a + 2 || r.in2 != b + 3 || r.out != out + 3) return false;
    if (out[0].id != 101 || out[1].id != 102 || out[2].id != 1) return false;
    int x[] = {1, 2};
    int y[] = {2, 3};
    int o[2] = {};
    auto r2 = std::ranges::set_symmetric_difference(InputIter<int>(x), PtrSentinel<int>{x + 2}, InputIter<int>(y),
                                                    PtrSentinel<int>{y + 2}, o);
    if (r2.in1.p != x + 2 || r2.in2.p != y + 2 || r2.out != o + 2 || o[0] != 1 || o[1] != 3) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
