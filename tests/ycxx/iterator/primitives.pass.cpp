// [iterator.operations]: std::advance, distance, next, prev are constexpr.
// [range.iter.ops]: ranges::advance(i, n), advance(i, bound), advance(i, n, bound) returning
// the unused distance; ranges::next / prev with counts and bounds; ranges::distance(first,
// last) for sized sentinels and by counting otherwise (and for a range r).
#include <iterator>
#include <array>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

// A forward-only iterator (no operator-) to exercise the counting paths.
struct Fwd {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  constexpr int& operator*() const { return *p; }
  constexpr Fwd& operator++() { ++p; return *this; }
  constexpr Fwd operator++(int) { auto t = *this; ++p; return t; }
  constexpr bool operator==(const Fwd&) const = default;
};
static_assert(std::forward_iterator<Fwd>);

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  int* p = a;
  std::advance(p, 3);
  if (p != a + 3) return false;
  std::advance(p, -2);
  if (p != a + 1) return false;
  if (std::next(a) != a + 1 || std::next(a, 4) != a + 4 || std::prev(a + 4) != a + 3 || std::prev(a + 4, 2) != a + 2)
    return false;
  if (std::distance(a, a + 6) != 6 || std::distance(a + 6, a) != -6) return false;

  int* q = a;
  std::ranges::advance(q, 2);
  if (q != a + 2) return false;
  std::ranges::advance(q, a + 5);
  if (q != a + 5) return false;
  q = a;
  if (std::ranges::advance(q, 10, a + 6) != 4 || q != a + 6) return false;  // unused distance
  if (std::ranges::advance(q, -10, a) != -4 || q != a) return false;
  if (std::ranges::advance(q, 3, a + 6) != 0 || q != a + 3) return false;
  if (std::ranges::next(a, 2) != a + 2 || std::ranges::next(a, a + 4) != a + 4) return false;
  if (std::ranges::next(a, 9, a + 6) != a + 6) return false;
  if (std::ranges::prev(a + 3) != a + 2 || std::ranges::prev(a + 3, 2) != a + 1) return false;
  if (std::ranges::prev(a + 3, 9, a) != a) return false;
  if (std::ranges::distance(a, a + 6) != 6) return false;

  Fwd f{a}, l{a + 6};
  if (std::ranges::distance(f, l) != 6) return false;
  Fwd g = f;
  std::ranges::advance(g, 4);
  if (g.p != a + 4) return false;
  if (std::ranges::advance(g, 5, l) != 3 || g != l) return false;
  if (std::ranges::next(f, l).p != a + 6) return false;
  if (std::distance(f, l) != 6) return false;
  std::array<int, 4> arr{};
  if (std::ranges::distance(arr) != 4) return false;
  // sentinel of a different type: counted_iterator / default_sentinel
  std::counted_iterator<int*> c(a, 4);
  if (std::ranges::distance(c, std::default_sentinel) != 4) return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::ranges::advance(std::declval<int*&>(), 1, std::declval<int*>())),
                             std::ptrdiff_t>);
static_assert(std::is_same_v<decltype(std::ranges::advance(std::declval<int*&>(), 1)), void>);

int main() {
  CHECK(test());
  return 0;
}
