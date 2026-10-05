// [alg.copy]: copy(first, last, result) copies in order from first and returns result + N;
// copy_n copies max(0, n) elements and returns result + N (ranges: {first + N, result + N});
// copy_if copies the elements for which pred is true (stable) and returns the end;
// copy_backward copies into [result - N, result) starting from last - 1 and returns
// result - N. The ranges forms return in_out_result {last, result + N}; ranges::copy_backward
// returns {last, result - N}. All are constexpr. Overlap to the left is fine for copy, to the
// right for copy_backward.
// COUNTERPART: libstdcxx:25_algorithms/copy/34595.cc
#include <algorithm>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool is_odd(int x) { return x % 2 != 0; }

constexpr bool test() {
  int src[] = {1, 2, 3, 4, 5};
  int dst[5] = {};
  int* r = std::copy(src, src + 5, dst);
  if (r != dst + 5 || dst[0] != 1 || dst[4] != 5) return false;

  // copy from an input iterator
  int d2[5] = {};
  if (std::copy(InputIter<int>(src), InputIter<int>(src + 5), d2) != d2 + 5 || d2[2] != 3) return false;

  // overlapping, shifting left
  int ov[] = {1, 2, 3, 4, 5};
  std::copy(ov + 1, ov + 5, ov);
  if (ov[0] != 2 || ov[3] != 5 || ov[4] != 5) return false;

  // copy_backward, overlapping shifting right
  int ob[] = {1, 2, 3, 4, 5};
  int* rb = std::copy_backward(ob, ob + 4, ob + 5);
  if (rb != ob + 1 || ob[0] != 1 || ob[1] != 1 || ob[4] != 4) return false;

  // copy_n, including n <= 0
  int dn[5] = {};
  if (std::copy_n(src, 3, dn) != dn + 3 || dn[2] != 3 || dn[3] != 0) return false;
  if (std::copy_n(src, 0, dn) != dn) return false;
  if (std::copy_n(src, -2, dn) != dn) return false;

  // copy_if, stable
  int dc[5] = {};
  int* rc = std::copy_if(src, src + 5, dc, is_odd);
  if (rc != dc + 3 || dc[0] != 1 || dc[1] != 3 || dc[2] != 5) return false;

  // ranges forms
  int rd[5] = {};
  auto res = std::ranges::copy(src, rd);
  static_assert(std::is_same_v<decltype(res), std::ranges::copy_result<int*, int*>>);
  if (res.in != src + 5 || res.out != rd + 5 || rd[4] != 5) return false;

  int rd2[5] = {};
  ForwardRange<int> fr{src, src + 5};
  auto res2 = std::ranges::copy(fr, rd2);
  if (res2.in.p != src + 5 || res2.out != rd2 + 5) return false;

  int rn[5] = {};
  auto resn = std::ranges::copy_n(src, 2, rn);
  static_assert(std::is_same_v<decltype(resn), std::ranges::copy_n_result<int*, int*>>);
  if (resn.in != src + 2 || resn.out != rn + 2 || rn[1] != 2) return false;

  int ri[5] = {};
  auto resi = std::ranges::copy_if(src, ri, [](int x) { return x > 2; });
  if (resi.in != src + 5 || resi.out != ri + 3 || ri[0] != 3) return false;
  // projection
  struct P {
    int k;
    int v;
  };
  P ps[] = {{1, 10}, {0, 20}, {1, 30}};
  P po[3] = {};
  auto resp = std::ranges::copy_if(ps, po, [](int k) { return k == 1; }, &P::k);
  if (resp.out != po + 2 || po[1].v != 30) return false;

  int rbk[] = {1, 2, 3, 4, 5};
  auto resb = std::ranges::copy_backward(rbk, rbk + 3, rbk + 5);
  static_assert(std::is_same_v<decltype(resb), std::ranges::copy_backward_result<int*, int*>>);
  if (resb.in != rbk + 3 || resb.out != rbk + 2 || rbk[2] != 1 || rbk[4] != 3) return false;

  // back_insert_iterator-like output: ostream not needed; use a counting output iterator
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
