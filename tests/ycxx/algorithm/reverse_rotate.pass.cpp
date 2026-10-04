// [alg.reverse]: reverse swaps *(first + i) and *(last - i - 1) for i < (last - first) / 2;
// ranges::reverse returns last. reverse_copy writes *(result + (last - first) - 1 - i) =
// *(first + i) and returns result + N (ranges: {last, result + N}). [alg.rotate]: rotate
// "places the element from the position first + i into position first + (i + (last -
// middle)) % (last - first)" and returns first + (last - middle) (ranges: {that, last}).
// rotate_copy copies [middle, last) then [first, middle) and returns result + N (ranges:
// {last, result + N}).
#include <algorithm>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

// bidirectional, not random access
struct Bidi {
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using pointer = int*;
  using reference = int&;
  int* p = nullptr;
  constexpr int& operator*() const { return *p; }
  constexpr Bidi& operator++() {
    ++p;
    return *this;
  }
  constexpr Bidi operator++(int) { return Bidi{p++}; }
  constexpr Bidi& operator--() {
    --p;
    return *this;
  }
  constexpr Bidi operator--(int) { return Bidi{p--}; }
  constexpr bool operator==(const Bidi&) const = default;
};
static_assert(std::bidirectional_iterator<Bidi> && !std::random_access_iterator<Bidi>);

constexpr bool test() {
  int a[] = {1, 2, 3, 4, 5};
  std::reverse(a, a + 5);
  if (a[0] != 5 || a[2] != 3 || a[4] != 1) return false;
  int b[] = {1, 2, 3, 4};
  std::reverse(b, b + 4);
  if (b[0] != 4 || b[1] != 3 || b[3] != 1) return false;
  std::reverse(b, b);
  std::reverse(b, b + 1);
  if (b[0] != 4) return false;

  int* rl = std::ranges::reverse(b);
  if (rl != b + 4 || b[0] != 1) return false;

  int out[5] = {};
  int src[] = {1, 2, 3};
  if (std::reverse_copy(src, src + 3, out) != out + 3 || out[0] != 3 || out[2] != 1) return false;
  auto rc = std::ranges::reverse_copy(src, out);
  static_assert(std::is_same_v<decltype(rc), std::ranges::reverse_copy_result<int*, int*>>);
  if (rc.in != src + 3 || rc.out != out + 3) return false;

  // rotate
  int r[] = {1, 2, 3, 4, 5, 6, 7};
  int* m = std::rotate(r, r + 3, r + 7);
  if (m != r + 4) return false;
  int expect[] = {4, 5, 6, 7, 1, 2, 3};
  for (int i = 0; i < 7; ++i)
    if (r[i] != expect[i]) return false;
  // degenerate cases
  if (std::rotate(r, r, r + 7) != r + 7) return false;
  if (std::rotate(r, r + 7, r + 7) != r) return false;
  if (r[0] != 4) return false;

  // forward iterators
  int f[] = {1, 2, 3, 4, 5};
  ForwardIter<int> fm = std::rotate(ForwardIter<int>(f), ForwardIter<int>(f + 1), ForwardIter<int>(f + 5));
  if (fm.p != f + 4 || f[0] != 2 || f[4] != 1) return false;

  int g[] = {1, 2, 3, 4, 5, 6};
  auto sr = std::ranges::rotate(g, g + 2);
  if (sr.begin() != g + 4 || sr.end() != g + 6 || g[0] != 3 || g[5] != 2) return false;
  auto sr2 = std::ranges::rotate(g, g + 5, g + 6);
  if (sr2.begin() != g + 1 || g[0] != 2 || g[1] != 3) return false;

  int ro[6] = {};
  int s2[] = {1, 2, 3, 4};
  if (std::rotate_copy(s2, s2 + 1, s2 + 4, ro) != ro + 4 || ro[0] != 2 || ro[3] != 1) return false;
  auto rr = std::ranges::rotate_copy(s2, s2 + 3, ro);
  static_assert(std::is_same_v<decltype(rr), std::ranges::rotate_copy_result<int*, int*>>);
  if (rr.in != s2 + 4 || rr.out != ro + 4 || ro[0] != 4 || ro[1] != 1) return false;
  int l[] = {1, 2, 3, 4, 5};
  std::reverse(Bidi{l}, Bidi{l + 5});
  if (l[0] != 5 || l[4] != 1 || l[2] != 3) return false;
  Bidi bm = std::rotate(Bidi{l}, Bidi{l + 1}, Bidi{l + 5});
  if (bm.p != l + 4 || l[0] != 4 || l[4] != 5) return false;
  if (std::ranges::reverse(Bidi{l}, Bidi{l + 4}).p != l + 4 || l[0] != 1) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
