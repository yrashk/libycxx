// [alg.count]: count returns the number of i in [first, last) with *i == value, count_if
// the number with pred(*i); the return type is iter_difference_t / difference_type. The
// ranges forms use projections, and T defaults to the projected value type (braced
// values work). Exactly last - first applications.
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {1, 2, 1, 3, 1};
  static_assert(std::is_same_v<decltype(std::count(a, a + 5, 1)), std::ptrdiff_t>);
  if (std::count(a, a + 5, 1) != 3 || std::count(a, a + 5, 9) != 0 || std::count(a, a, 1) != 0) return false;
  if (std::count_if(a, a + 5, [](int x) { return x > 1; }) != 2) return false;
  if (std::count(InputIter<int>(a), InputIter<int>(a + 5), 1) != 3) return false;
  Pt pts[] = {{1, 1}, {1, 2}, {2, 2}};
  if (std::count(pts, pts + 3, {1, 2}) != 1) return false;
  if (std::ranges::count(pts, {2, 2}) != 1) return false;
  if (std::ranges::count(pts, 1, &Pt::x) != 2) return false;
  if (std::ranges::count(pts, pts + 3, 2, &Pt::y) != 2) return false;
  if (std::ranges::count_if(pts, [](int y) { return y == 1; }, &Pt::y) != 1) return false;
  InputRange<int> in{a, a + 5};
  if (std::ranges::count(in, 1) != 3) return false;
  static_assert(std::is_same_v<decltype(std::ranges::count(a, 1)), std::ptrdiff_t>);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[] = {1, 2, 3};
  int calls = 0;
  CHECK(std::ranges::count_if(a, [&](int) { ++calls; return true; }) == 3 && calls == 3);
  return 0;
}
