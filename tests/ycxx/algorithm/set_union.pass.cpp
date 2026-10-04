// [set.union]: "If [first1, last1) contains m elements that are equivalent to each other and
// [first2, last2) contains n elements that are equivalent to them, then all m elements from
// the first range are included in the union, in order, and then the final max(n - m, 0)
// elements from the second range are included in the union, in order." Returns result_last
// (std) and {last1, last2, result + N} (ranges, set_union_result = in_in_out_result).
// "Remarks: Stable".
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
    int a[] = {1, 2, 2, 4};
    int b[] = {2, 3, 4, 4, 5};
    int out[9] = {};
    int* e = std::set_union(a, a + 4, b, b + 5, out);
    int want[] = {1, 2, 2, 3, 4, 4, 5};
    if (e != out + 7) return false;
    for (int i = 0; i < 7; ++i)
      if (out[i] != want[i]) return false;
  }
  {
    // which elements: all m from the first, then the LAST n - m from the second
    KV a[] = {{1, 0}, {2, 1}, {2, 2}, {5, 3}};
    KV b[] = {{2, 10}, {2, 11}, {2, 12}, {2, 13}, {3, 14}, {5, 15}};
    KV out[10];
    KV* e = std::set_union(a, a + 4, b, b + 6, out);
    int ids[] = {0, 1, 2, 12, 13, 14, 3};
    if (e != out + 7) return false;
    for (int i = 0; i < 7; ++i)
      if (out[i].id != ids[i]) return false;
    // when m >= n only the first range's copies appear
    e = std::set_union(b, b + 6, a, a + 4, out);
    int ids2[] = {0, 10, 11, 12, 13, 14, 15};
    if (e != out + 7) return false;
    for (int i = 0; i < 7; ++i)
      if (out[i].id != ids2[i]) return false;
  }
  {
    // comparator, input iterators, empty inputs
    int a[] = {5, 3};
    int b[] = {4, 3, 1};
    int out[5] = {};
    int* e = std::set_union(InputIter<int>(a), InputIter<int>(a + 2), InputIter<int>(b), InputIter<int>(b + 3), out,
                            std::greater<>{});
    if (e != out + 4 || out[0] != 5 || out[1] != 4 || out[2] != 3 || out[3] != 1) return false;
    if (std::set_union(a, a, b, b, out) != out) return false;
    if (std::set_union(a, a, b, b + 3, out) != out + 3 || out[2] != 1) return false;
  }
  {
    // ranges: result type and members; projections of different types
    KV a[] = {{1, 0}, {3, 1}};
    Other b[] = {{1, 100}, {1, 101}, {2, 102}};
    Out out[5];
    auto r = std::ranges::set_union(a, b, out, {}, &KV::key, &Other::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::set_union_result<KV*, Other*, Out*>>);
    static_assert(std::is_same_v<std::ranges::set_union_result<KV*, Other*, Out*>, std::ranges::in_in_out_result<KV*, Other*, Out*>>);
    if (r.in1 != a + 2 || r.in2 != b + 3 || r.out != out + 4) return false;
    int ids[] = {0, 101, 102, 1};
    for (int i = 0; i < 4; ++i)
      if (out[i].id != ids[i]) return false;
    int x[] = {1, 2};
    int y[] = {2, 3};
    int o[4] = {};
    auto r2 = std::ranges::set_union(InputIter<int>(x), PtrSentinel<int>{x + 2}, InputIter<int>(y), PtrSentinel<int>{y + 2}, o);
    if (r2.in1.p != x + 2 || r2.in2.p != y + 2 || r2.out != o + 3) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
