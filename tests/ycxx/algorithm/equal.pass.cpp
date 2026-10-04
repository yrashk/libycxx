// [alg.equal]: equal(first1, last1, first2[, pred]) compares the n = last1 - first1
// elements; equal(first1, last1, first2, last2[, pred]) is false if the lengths differ and
// otherwise compares elementwise. Complexity: for the four-iterator forms with
// random-access iterators (or sized sentinels) of different lengths, no applications of
// the predicate. ranges::equal uses projections and checks sizes first for sized ranges.
#include <algorithm>
#include <functional>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
};

constexpr bool test() {
  int a[] = {1, 2, 3};
  int b[] = {1, 2, 3, 4};
  int c[] = {1, 2, 4};
  if (!std::equal(a, a + 3, b) || std::equal(a, a + 3, c)) return false;
  if (std::equal(a, a + 3, b, b + 4) || !std::equal(a, a + 3, b, b + 3)) return false;
  if (!std::equal(a, a, c, c)) return false;
  if (!std::equal(a, a + 3, c, [](int x, int y) { return x <= y; })) return false;
  if (!std::equal(a, a + 3, c, c + 3, [](int x, int y) { return x <= y; })) return false;
  if (!std::equal(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b), InputIter<int>(b + 3))) return false;
  if (std::equal(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b), InputIter<int>(b + 4))) return false;

  if (!std::ranges::equal(a, a) || std::ranges::equal(a, b) || std::ranges::equal(a, c)) return false;
  Pt p[] = {{1, 0}, {2, 0}, {3, 0}};
  if (!std::ranges::equal(p, a, {}, &Pt::x)) return false;
  if (!std::ranges::equal(a, p, {}, {}, &Pt::x)) return false;
  if (!std::ranges::equal(a, a + 3, c, c + 3, std::ranges::less_equal{})) return false;
  InputRange<int> ia{a, a + 3};
  ForwardRange<int> fb{b, b + 3};
  if (!std::ranges::equal(ia, fb)) return false;
  ForwardRange<int> fb4{b, b + 4};
  if (std::ranges::equal(ia, fb4)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[] = {1, 2, 3};
  int b[] = {1, 2, 3, 4};
  int calls = 0;
  auto pred = [&](int x, int y) {
    ++calls;
    return x == y;
  };
  CHECK(!std::equal(a, a + 3, b, b + 4, pred));
  CHECK(calls == 0);  // random access, different lengths
  CHECK(!std::ranges::equal(a, b, pred));
  CHECK(calls == 0);  // sized ranges, different sizes
  return 0;
}
