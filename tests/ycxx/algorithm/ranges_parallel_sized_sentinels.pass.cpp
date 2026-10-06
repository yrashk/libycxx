// The iterator-sentinel forms of the ranges:: parallel algorithms take a random_access_iterator
// and any sized_sentinel_for it, not only a common range ([algorithm.syn]; e.g. [alg.find],
// [alg.copy]/7, /10.2, [alg.fill], [sort], [alg.count], [alg.foreach]/17): here counted_iterator
// with default_sentinel, and a pointer with a sentinel type of its own. The ranges given by such
// pairs, and subranges of them, are sized-random-access-ranges for the range forms. Results are
// iterators of the iterator type (first + N), not the sentinel.
#include <algorithm>
#include <execution>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;

struct stop_at {
  const int* e;
  friend bool operator==(const int* p, stop_at s) { return p == s.e; }
  friend std::ptrdiff_t operator-(const int* p, stop_at s) { return p - s.e; }
  friend std::ptrdiff_t operator-(stop_at s, const int* p) { return s.e - p; }
};
static_assert(std::sized_sentinel_for<stop_at, int*> && std::sized_sentinel_for<std::default_sentinel_t, std::counted_iterator<int*>>);

template <class Pol>
void run(Pol&& pol) {
  int a[6] = {5, 3, 8, 1, 9, 2};
  std::counted_iterator<int*> ci(a, 4);  // 5 3 8 1
  auto f = rg::find(pol, ci, std::default_sentinel, 8);
  static_assert(std::is_same_v<decltype(f), std::counted_iterator<int*>>);
  CHECK(f.base() == a + 2 && f.count() == 2);
  auto nf = rg::find(pol, ci, std::default_sentinel, 9);  // 9 is past the counted range
  CHECK(nf.count() == 0 && nf.base() == a + 4);
  CHECK(rg::count_if(pol, ci, std::default_sentinel, [](int x) { return x > 2; }) == 3);

  int out[3] = {};
  auto c = rg::copy(pol, ci, std::default_sentinel, out, stop_at{out + 3});
  CHECK(c.in.base() == a + 3 && c.out == out + 3 && out[2] == 8);

  // a pointer and a sentinel type
  auto r = rg::fill(pol, a + 4, stop_at{a + 6}, 0);
  static_assert(std::is_same_v<decltype(r), int*>);
  CHECK(r == a + 6 && a[4] == 0 && a[5] == 0 && a[3] == 1);
  auto e = rg::for_each(pol, a, stop_at{a + 4}, [](int& x) { x *= 10; });
  CHECK(e == a + 4 && a[0] == 50 && a[3] == 10);
  CHECK(rg::sort(pol, a, stop_at{a + 4}) == a + 4);
  CHECK(a[0] == 10 && a[1] == 30 && a[2] == 50 && a[3] == 80 && a[4] == 0);

  // the range forms with a subrange of such a pair
  rg::subrange<int*, stop_at> sr(a, stop_at{a + 4});
  static_assert(rg::random_access_range<decltype(sr)> && rg::sized_range<decltype(sr)> && !rg::common_range<decltype(sr)>);
  CHECK(rg::find(pol, sr, 50) == a + 2);
  auto mm = rg::minmax_element(pol, sr);
  CHECK(mm.min == a && mm.max == a + 3);
  std::vector<int> dst(10);
  auto cr = rg::copy(pol, sr, dst);
  CHECK(cr.in == a + 4 && cr.out == dst.begin() + 4 && dst[3] == 80);
  auto cv = rg::copy(pol, std::views::counted(a, 2), dst);
  CHECK(cv.out == dst.begin() + 2);
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  return 0;
}
