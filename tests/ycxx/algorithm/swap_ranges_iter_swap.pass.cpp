// [alg.swap]: swap_ranges(first1, last1, first2) swaps *(first1 + n) with *(first2 + n) for
// n < last1 - first1 and returns first2 + (last1 - first1); ranges::swap_ranges swaps
// M = min(len1, len2) elements via ranges::iter_swap and returns {first1 + M, first2 + M}.
// iter_swap(a, b) performs swap(*a, *b).
#include <algorithm>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace user {
struct S {
  int v;
  int swaps = 0;
};
constexpr void swap(S& a, S& b) {  // found by ADL
  std::swap(a.v, b.v);
  ++a.swaps;
  ++b.swaps;
}
}  // namespace user

constexpr bool test() {
  int a[] = {1, 2, 3};
  int b[] = {4, 5, 6, 7};
  int* r = std::swap_ranges(a, a + 3, b);
  if (r != b + 3 || a[0] != 4 || b[0] != 1 || b[3] != 7) return false;

  std::iter_swap(a, a + 2);
  if (a[0] != 6 || a[2] != 4) return false;

  auto rr = std::ranges::swap_ranges(a, b);  // M = 3
  static_assert(std::is_same_v<decltype(rr), std::ranges::swap_ranges_result<int*, int*>>);
  if (rr.in1 != a + 3 || rr.in2 != b + 3 || a[0] != 1 || b[0] != 6) return false;
  auto rr2 = std::ranges::swap_ranges(b, b + 4, a, a + 2);
  if (rr2.in1 != b + 2 || rr2.in2 != a + 2) return false;

  user::S s1[2] = {{1}, {2}};
  user::S s2[2] = {{3}, {4}};
  std::ranges::swap_ranges(s1, s2);
  if (s1[0].v != 3 || s2[1].v != 2 || s1[0].swaps != 1) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
