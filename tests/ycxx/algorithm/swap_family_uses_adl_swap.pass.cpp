// The swapping algorithms are specified by the swap they apply, so a user swap found by
// argument-dependent lookup runs for every pair, also for a trivially copyable type held in
// contiguous storage (no byte-wise exchange):
//   [alg.swap] swap_ranges: "swap(*(first1 + n), *(first2 + n))" for each n < M (std),
//     "ranges::iter_swap(first1 + n, first2 + n)" (ranges); iter_swap(a, b): swap(*a, *b);
//   [alg.reverse] reverse: "applies std::iter_swap, or ranges::iter_swap for the overloads in
//     namespace ranges, to all pairs of iterators first + i, (last - i) - 1", exactly
//     (last - first)/2 swaps;
//   [array.members] array::swap(y): "Equivalent to swap_ranges(begin(), end(), y.begin())";
//     [array.special] swap(x, y): x.swap(y);
//   [utility.swap] swap(T (&a)[N], T (&b)[N]): "As if by swap_ranges(a, a + N, b)";
//   [concept.swappable]/2: ranges::swap(E1, E2) uses an ADL swap when one is found (2.1), and
//     for arrays of equal extent ranges::swap_ranges(E1, E2) (2.2); [iterator.cust.swap]
//     ranges::iter_swap(a, b) is ranges::swap(*a, *b) for these iterators.
// The ADL swap here exchanges the values and counts its calls on each object.
#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace user {
struct S {
  int v;
  int swaps;
};
static_assert(std::is_trivially_copyable_v<S>);
constexpr void swap(S& a, S& b) noexcept {
  int t = a.v;
  a.v = b.v;
  b.v = t;
  ++a.swaps;
  ++b.swaps;
}
}  // namespace user
using user::S;

template <class It>
void fill(It first, It last, int base) {
  for (int i = 0; first != last; ++first, ++i) *first = S{base + i, 0};
}

// [first, last) reversed from base..., each element swapped exactly once except the middle
template <class It>
void expect_reversed(It first, It last, int base) {
  int n = static_cast<int>(std::distance(first, last));
  for (int i = 0; first != last; ++first, ++i) {
    CHECK(first->v == base + n - 1 - i);
    CHECK(first->swaps == ((n % 2 == 1 && i == n / 2) ? 0 : 1));
  }
}

template <class C>
void reverse_in(C& c) {
  for (std::size_t n : {0u, 1u, 2u, 7u, 16u, 33u, 100u}) {
    if constexpr (requires { c.resize(n); }) c.resize(n);
    else if (n != c.size()) continue;
    fill(c.begin(), c.end(), 0);
    std::reverse(c.begin(), c.end());
    expect_reversed(c.begin(), c.end(), 0);
    fill(c.begin(), c.end(), 0);
    auto r = std::ranges::reverse(c);
    CHECK(r == c.end());
    expect_reversed(c.begin(), c.end(), 0);
    fill(c.begin(), c.end(), 0);
    std::ranges::reverse(c.begin(), c.end());
    expect_reversed(c.begin(), c.end(), 0);
  }
}

template <class C>
void swap_ranges_in(C& a, C& b) {
  for (std::size_t n : {1u, 5u, 40u}) {
    if constexpr (requires { a.resize(n); }) {
      a.resize(n);
      b.resize(n);
    } else if (n != a.size()) continue;
    fill(a.begin(), a.end(), 0);
    fill(b.begin(), b.end(), 1000);
    auto e = std::swap_ranges(a.begin(), a.end(), b.begin());
    CHECK(e == b.end());
    for (std::size_t i = 0; i < n; ++i) {
      CHECK(a[i].v == 1000 + static_cast<int>(i) && a[i].swaps == 1);
      CHECK(b[i].v == static_cast<int>(i) && b[i].swaps == 1);
    }
    auto r = std::ranges::swap_ranges(a, b);
    CHECK(r.in1 == a.end() && r.in2 == b.end());
    for (std::size_t i = 0; i < n; ++i) {
      CHECK(a[i].v == static_cast<int>(i) && a[i].swaps == 2);
      CHECK(b[i].swaps == 2);
    }
    std::iter_swap(a.begin(), b.begin());
    CHECK(a[0].v == 1000 && a[0].swaps == 3 && b[0].v == 0 && b[0].swaps == 3);
    std::ranges::iter_swap(a.begin(), b.begin());
    CHECK(a[0].v == 0 && a[0].swaps == 4 && b[0].v == 1000);
  }
}

int main() {
  std::vector<S> v;
  reverse_in(v);
  std::deque<S> d;
  reverse_in(d);
  std::array<S, 33> arr{};
  reverse_in(arr);
  S raw[16];
  std::span<S> sp(raw);
  reverse_in(sp);

  std::vector<S> va, vb;
  swap_ranges_in(va, vb);
  std::deque<S> da, db;
  swap_ranges_in(da, db);
  std::array<S, 40> aa{}, ab{};
  swap_ranges_in(aa, ab);

  // array::swap and the swap of std::array / C arrays / ranges::swap on C arrays
  std::array<S, 9> x{}, y{};
  fill(x.begin(), x.end(), 0);
  fill(y.begin(), y.end(), 100);
  x.swap(y);
  for (std::size_t i = 0; i < 9; ++i) CHECK(x[i].v == 100 + static_cast<int>(i) && x[i].swaps == 1);
  swap(x, y);  // [array.special]
  for (std::size_t i = 0; i < 9; ++i) CHECK(x[i].v == static_cast<int>(i) && x[i].swaps == 2);
  std::ranges::swap(x, y);  // ADL finds std::swap(array&, array&)
  for (std::size_t i = 0; i < 9; ++i) CHECK(y[i].v == static_cast<int>(i) && y[i].swaps == 3);
  S ca[12], cb[12];
  fill(ca, ca + 12, 0);
  fill(cb, cb + 12, 50);
  std::swap(ca, cb);
  for (int i = 0; i < 12; ++i) CHECK(ca[i].v == 50 + i && ca[i].swaps == 1 && cb[i].swaps == 1);
  std::ranges::swap(ca, cb);
  for (int i = 0; i < 12; ++i) CHECK(ca[i].v == i && ca[i].swaps == 2 && cb[i].v == 50 + i);
  S m1[2][3], m2[2][3];  // nested arrays: element-wise down to S
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j) {
      m1[i][j] = S{i * 3 + j, 0};
      m2[i][j] = S{10 + i * 3 + j, 0};
    }
  std::ranges::swap(m1, m2);
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j) CHECK(m1[i][j].v == 10 + i * 3 + j && m1[i][j].swaps == 1);
  std::swap(m1, m2);
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j) CHECK(m1[i][j].v == i * 3 + j && m1[i][j].swaps == 2);
}
