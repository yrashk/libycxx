// [alg.shift]: shift_left(first, last, n): "If n == 0 or n >= last - first, does nothing.
// Otherwise, moves the element from position first + n + i into position first + i" in
// increasing i; returns NEW_LAST = first + (last - first - n) if n < last - first, otherwise
// first (ranges: {first, NEW_LAST}). shift_right moves first + i into first + n + i; returns
// NEW_FIRST = first + n if n < last - first, otherwise last (ranges: {NEW_FIRST, last}).
#include <algorithm>
#include <ranges>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 4, 5};
  int* nl = std::shift_left(a, a + 5, 2);
  if (nl != a + 3 || a[0] != 3 || a[1] != 4 || a[2] != 5) return false;

  int b[] = {1, 2, 3, 4, 5};
  int* nf = std::shift_right(b, b + 5, 2);
  if (nf != b + 2 || b[2] != 1 || b[3] != 2 || b[4] != 3) return false;

  int c[] = {1, 2, 3};
  if (std::shift_left(c, c + 3, 0) != c + 3 || c[0] != 1) return false;
  if (std::shift_left(c, c + 3, 3) != c || c[0] != 1) return false;
  if (std::shift_left(c, c + 3, 7) != c || c[2] != 3) return false;
  if (std::shift_right(c, c + 3, 0) != c || c[0] != 1) return false;
  if (std::shift_right(c, c + 3, 3) != c + 3 || c[0] != 1) return false;

  // forward iterators (shift_right must still work)
  int f[] = {1, 2, 3, 4};
  ForwardIter<int> ff = std::shift_right(ForwardIter<int>(f), ForwardIter<int>(f + 4), 1);
  if (ff.p != f + 1 || f[1] != 1 || f[2] != 2 || f[3] != 3) return false;
  int f2[] = {1, 2, 3, 4};
  ForwardIter<int> fl = std::shift_left(ForwardIter<int>(f2), ForwardIter<int>(f2 + 4), 3);
  if (fl.p != f2 + 1 || f2[0] != 4) return false;

  int r[] = {1, 2, 3, 4, 5};
  auto sl = std::ranges::shift_left(r, 1);
  if (sl.begin() != r || sl.end() != r + 4 || r[0] != 2 || r[3] != 5) return false;
  int r2[] = {1, 2, 3, 4, 5};
  auto sr = std::ranges::shift_right(r2, r2 + 5, 4);
  if (sr.begin() != r2 + 4 || sr.end() != r2 + 5 || r2[4] != 1) return false;
  auto sr0 = std::ranges::shift_right(r2, 9);
  if (sr0.begin() != r2 + 5 || sr0.end() != r2 + 5) return false;
  auto sl0 = std::ranges::shift_left(r2, 5);
  if (sl0.begin() != r2 || sl0.end() != r2) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
