// [partial.sum]: *result = *first, then each further output is acc = std::move(acc) + *i
// (or binary_op); returns result + (last - first); exactly (last - first) - 1 applications;
// result may equal first. [inclusive.scan] / [exclusive.scan]: the K-th output is the
// (generalized noncommutative) sum of the first K + 1 / first K inputs (exclusive starting
// from init, which is output first); inclusive_scan optionally takes init, folded in first.
// [transform.inclusive.scan] / [transform.exclusive.scan] apply unary_op first.
// [adjacent.difference]: *result = *first, then *(result + k) = *(first + k) - *(first +
// k - 1) (or binary_op(curr, prev)); result may equal first. All return the output end.
#include <numeric>
#include <functional>
#include <initializer_list>
#include "test_iterators.hpp"
#include "check.hpp"

template <int N>
constexpr bool eq(const int (&a)[N], std::initializer_list<int> il) {
  int i = 0;
  for (int v : il)
    if (a[i++] != v) return false;
  return true;
}

constexpr bool test() {
  const int a[] = {1, 2, 3, 4};
  int out[4] = {};
  if (std::partial_sum(a, a + 4, out) != out + 4 || !eq(out, {1, 3, 6, 10})) return false;
  if (std::partial_sum(a, a, out) != out) return false;
  std::partial_sum(a, a + 4, out, std::multiplies<>{});
  if (!eq(out, {1, 2, 6, 24})) return false;
  int inplace[] = {1, 1, 1, 1};
  std::partial_sum(inplace, inplace + 4, inplace);
  if (!eq(inplace, {1, 2, 3, 4})) return false;
  // partial_sum accumulates in the input value type
  const double halves[] = {0.5, 0.5, 0.5};
  int hout[3] = {};
  std::partial_sum(halves, halves + 3, hout);  // acc is double: 0.5, 1.0, 1.5 -> 0, 1, 1
  if (!eq(hout, {0, 1, 1})) return false;
  std::partial_sum(InputIter<const int>(a), InputIter<const int>(a + 4), out);
  if (!eq(out, {1, 3, 6, 10})) return false;

  if (std::inclusive_scan(a, a + 4, out) != out + 4 || !eq(out, {1, 3, 6, 10})) return false;
  std::inclusive_scan(a, a + 4, out, std::plus<>{}, 100);
  if (!eq(out, {101, 103, 106, 110})) return false;
  if (std::exclusive_scan(a, a + 4, out, 0) != out + 4 || !eq(out, {0, 1, 3, 6})) return false;
  std::exclusive_scan(a, a + 4, out, 1, std::multiplies<>{});
  if (!eq(out, {1, 1, 2, 6})) return false;
  int io[] = {1, 2, 3, 4};
  std::exclusive_scan(io, io + 4, io, 10);  // in place is allowed (result may equal first)
  if (!eq(io, {10, 11, 13, 16})) return false;
  int io2[] = {1, 2, 3, 4};
  std::inclusive_scan(io2, io2 + 4, io2);
  if (!eq(io2, {1, 3, 6, 10})) return false;
  auto sq = [](int x) { return x * x; };
  std::transform_inclusive_scan(a, a + 4, out, std::plus<>{}, sq);
  if (!eq(out, {1, 5, 14, 30})) return false;
  std::transform_inclusive_scan(a, a + 4, out, std::plus<>{}, sq, -1);
  if (!eq(out, {0, 4, 13, 29})) return false;
  std::transform_exclusive_scan(a, a + 4, out, 0, std::plus<>{}, sq);
  if (!eq(out, {0, 1, 5, 14})) return false;

  const int b[] = {1, 4, 9, 16};
  if (std::adjacent_difference(b, b + 4, out) != out + 4 || !eq(out, {1, 3, 5, 7})) return false;
  std::adjacent_difference(b, b + 4, out, [](int cur, int prev) { return cur * 100 + prev; });
  if (!eq(out, {1, 401, 904, 1609})) return false;
  int ad[] = {3, 5, 8};
  std::adjacent_difference(ad, ad + 3, ad);  // in place
  if (ad[0] != 3 || ad[1] != 2 || ad[2] != 3) return false;
  if (std::adjacent_difference(b, b, out) != out) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[] = {1, 2, 3, 4, 5};
  int out[5];
  int calls = 0;
  std::partial_sum(a, a + 5, out, [&](int x, int y) { ++calls; return x + y; });
  CHECK(calls == 4);
  calls = 0;
  std::adjacent_difference(a, a + 5, out, [&](int x, int y) { ++calls; return x - y; });
  CHECK(calls == 4);
  return 0;
}
