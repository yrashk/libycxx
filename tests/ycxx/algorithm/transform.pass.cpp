// [alg.transform]: unary transform assigns op(*(first1 + i)) to *(result + i) and returns
// result + N; binary transform uses binary_op(*(first1 + i), *(first2 + i)) with N =
// last1 - first1. result may be equal to first1 (in-place). ranges::transform returns
// unary_transform_result {last1, result + N} / binary_transform_result {first1 + N,
// first2 + N, result + N} with N the shorter length, and supports projections.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 4};
  int b[] = {10, 20, 30, 40};
  int out[4] = {};
  int* r = std::transform(a, a + 4, out, [](int x) { return x * x; });
  if (r != out + 4 || out[3] != 16) return false;

  r = std::transform(a, a + 4, b, out, std::plus<>{});
  if (r != out + 4 || out[0] != 11 || out[3] != 44) return false;

  // in place
  std::transform(a, a + 4, a, [](int x) { return -x; });
  if (a[0] != -1 || a[3] != -4) return false;

  // ranges, unary with projection
  struct P {
    int k;
  };
  P ps[] = {{1}, {2}, {3}};
  int o2[3] = {};
  auto ur = std::ranges::transform(ps, o2, [](int k) { return k + 100; }, &P::k);
  static_assert(std::is_same_v<decltype(ur), std::ranges::unary_transform_result<P*, int*>>);
  if (ur.in != ps + 3 || ur.out != o2 + 3 || o2[2] != 103) return false;

  // ranges, binary with unequal lengths: stops at the shorter
  int x[] = {1, 2, 3, 4, 5};
  int y[] = {10, 20, 30};
  int o3[5] = {};
  auto br = std::ranges::transform(x, y, o3, std::multiplies<>{});
  static_assert(std::is_same_v<decltype(br), std::ranges::binary_transform_result<int*, int*, int*>>);
  if (br.in1 != x + 3 || br.in2 != y + 3 || br.out != o3 + 3 || o3[2] != 90 || o3[3] != 0) return false;

  auto br2 = std::ranges::transform(y, x, o3, std::minus<>{}, [](int v) { return v * 2; }, [](int v) { return v + 1; });
  if (br2.in1 != y + 3 || br2.in2 != x + 3 || o3[0] != 20 - 2) return false;

  // iterator-sentinel form
  int o4[3] = {};
  auto ir = std::ranges::transform(y, y + 3, o4, [](int v) { return v / 10; });
  if (ir.in != y + 3 || o4[1] != 2) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
