// Whole-program integration: a matrix-vector kernel and a ReLU over an mdspan, vectorised with
// <simd>, compared with scalar loops. Values are small integers (exact in float).
//   [simd.loadstore]: unchecked_load(first, n) / partial_load(first, n): elements i < n are
//     loaded, the others value-initialised (/10 ff.); partial_store(v, first, n) stores only
//     elements i < n (/19).
//   [simd.reductions]: reduce(v) is the sum (GENERALIZED_SUM with plus<>); reduce_count(mask);
//     reduce_max.
//   [simd.binary], [simd.comparison]: element-wise *, + and > give a vec / a mask.
//   [simd.mask.where] select(mask, a, b): element-wise mask[i] ? a[i] : b[i].
//   [mdspan.layout.right]: row i of a layout_right matrix is contiguous at &A[i, 0];
//   [mdspan.sub]: submdspan(A, i, full_extent) is that row.
//   [numeric.ops.transform.reduce] transform_reduce over the same row as the scalar reference.
#include <algorithm>
#include <cstddef>
#include <mdspan>
#include <numeric>
#include <simd>
#include <vector>
#include "check.hpp"

namespace simd = std::simd;
using V = simd::vec<float>;

static float simd_dot(const float* a, const float* b, std::size_t n) {
  V acc(0.0f);
  std::size_t i = 0;
  for (; i + V::size() <= n; i += V::size())
    acc += simd::unchecked_load<V>(a + i, V::size()) * simd::unchecked_load<V>(b + i, V::size());
  if (i < n) acc += simd::partial_load<V>(a + i, n - i) * simd::partial_load<V>(b + i, n - i);
  return simd::reduce(acc);
}

static std::size_t simd_relu(float* p, std::size_t n) {  // returns the number of positives
  std::size_t pos = 0;
  for (std::size_t i = 0; i < n; i += V::size()) {
    std::size_t k = std::min<std::size_t>(V::size(), n - i);
    V v = simd::partial_load<V>(p + i, k);
    auto m = v > V(0.0f);
    pos += static_cast<std::size_t>(simd::reduce_count(m));  // tail elements are 0: not counted
    simd::partial_store(simd::select(m, v, V(0.0f)), p + i, k);
  }
  return pos;
}

int main() {
  constexpr std::size_t R = 5, K = 2 * V::size() + 3;  // a tail in every row
  std::vector<float> ad(R * K + 1, -1000.0f), xd(K + 1, -1000.0f);  // sentinels after the end
  std::mdspan A(ad.data(), std::dextents<std::size_t, 2>(R, K));
  for (std::size_t i = 0; i < R; ++i)
    for (std::size_t j = 0; j < K; ++j) A[i, j] = static_cast<float>(int((i * 7 + j * 3) % 11) - 5);
  for (std::size_t j = 0; j < K; ++j) xd[j] = static_cast<float>(int(j % 5) - 2);

  std::vector<float> y(R), yref(R);
  for (std::size_t i = 0; i < R; ++i) {
    auto row = std::submdspan(A, i, std::full_extent);
    CHECK(row.data_handle() == &A[i, 0] && row.extent(0) == K);
    y[i] = simd_dot(row.data_handle(), xd.data(), K);
    yref[i] = std::transform_reduce(&A[i, 0], &A[i, 0] + K, xd.data(), 0.0f);
  }
  CHECK(y == yref);
  // reduce_max over one full chunk of row 0.
  CHECK(simd::reduce_max(simd::unchecked_load<V>(&A[0, 0], V::size())) == *std::max_element(&A[0, 0], &A[0, 0] + V::size()));

  // ReLU in place over the whole matrix storage (R * K elements), the sentinel untouched.
  std::vector<float> ref(ad.begin(), ad.begin() + R * K);
  std::size_t want_pos = static_cast<std::size_t>(std::ranges::count_if(ref, [](float f) { return f > 0; }));
  for (float& f : ref) f = std::max(f, 0.0f);
  CHECK(simd_relu(ad.data(), R * K) == want_pos);
  CHECK(std::equal(ref.begin(), ref.end(), ad.begin()));
  CHECK(ad[R * K] == -1000.0f);
  CHECK(want_pos > 0 && want_pos < R * K);
  return 0;
}
