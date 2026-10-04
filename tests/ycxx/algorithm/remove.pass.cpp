// [alg.remove]: remove / remove_if eliminate the elements equal to value / satisfying pred
// and return the end of the resulting range; "Remarks: Stable" -- the relative order of the
// kept elements is preserved. remove_copy(_if) copy the kept elements and return the end.
// ranges::remove(_if) return {j, last}; ranges::remove_copy(_if) return {last, result + N}.
// Complexity: exactly last - first applications of the predicate. C++26: braced value.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x, y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {1, 2, 1, 3, 1, 4};
  int* e = std::remove(a, a + 6, 1);
  if (e != a + 3 || a[0] != 2 || a[1] != 3 || a[2] != 4) return false;

  int b[] = {5, 6, 7, 8, 9};
  int calls = 0;
  e = std::remove_if(b, b + 5, [&calls](int v) {
    ++calls;
    return v % 2 == 0;
  });
  if (e != b + 3 || b[0] != 5 || b[1] != 7 || b[2] != 9 || calls != 5) return false;

  // nothing removed / everything removed
  int c[] = {1, 2};
  if (std::remove(c, c + 2, 3) != c + 2 || c[1] != 2) return false;
  if (std::remove(c, c, 1) != c) return false;
  int d[] = {4, 4, 4};
  if (std::remove(d, d + 3, 4) != d) return false;

  // forward iterators
  int f[] = {1, 0, 2, 0, 3};
  ForwardIter<int> fe = std::remove(ForwardIter<int>(f), ForwardIter<int>(f + 5), 0);
  if (fe.p != f + 3 || f[2] != 3) return false;

  int src[] = {1, 2, 3, 2, 1};
  int out[5] = {};
  int* oe = std::remove_copy(src, src + 5, out, 2);
  if (oe != out + 3 || out[0] != 1 || out[1] != 3 || out[2] != 1 || src[1] != 2) return false;
  oe = std::remove_copy_if(src, src + 5, out, [](int v) { return v < 2; });
  if (oe != out + 3 || out[0] != 2 || out[2] != 2) return false;

  // ranges
  int r[] = {1, 2, 3, 2};
  auto sr = std::ranges::remove(r, 2);
  if (sr.begin() != r + 2 || sr.end() != r + 4 || r[0] != 1 || r[1] != 3) return false;
  Pt ps[] = {{1, 0}, {2, 0}, {1, 1}, {3, 0}};
  auto sp = std::ranges::remove(ps, 1, &Pt::x);
  if (sp.begin() != ps + 2 || !(ps[0] == Pt{2, 0}) || !(ps[1] == Pt{3, 0})) return false;
  Pt qs[] = {{1, 1}, {2, 2}};
  if (std::ranges::remove(qs, {1, 1}).begin() != qs + 1) return false;
  if (std::remove(qs, qs + 1, {2, 2}) != qs) return false;

  int r2[] = {1, 2, 3, 4};
  auto si = std::ranges::remove_if(r2, r2 + 4, [](int v) { return v > 2; });
  if (si.begin() != r2 + 2 || si.end() != r2 + 4) return false;

  int o2[4] = {};
  int s2[] = {1, 2, 3, 4};
  auto rc = std::ranges::remove_copy(s2, o2, 3);
  static_assert(std::is_same_v<decltype(rc), std::ranges::remove_copy_result<int*, int*>>);
  if (rc.in != s2 + 4 || rc.out != o2 + 3 || o2[2] != 4) return false;
  auto rci = std::ranges::remove_copy_if(s2, o2, [](int v) { return v % 2; });
  static_assert(std::is_same_v<decltype(rci), std::ranges::remove_copy_if_result<int*, int*>>);
  if (rci.out != o2 + 2 || o2[0] != 2 || o2[1] != 4) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
