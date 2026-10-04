// [alg.is.permutation]: is_permutation is false if the lengths differ, otherwise true iff
// some permutation of [first2, last2) is equal to [first1, last1) under pred.
// Complexity: no predicate applications for random-access iterators of different lengths;
// exactly last1 - first1 applications if equal(first1, last1, first2, last2, pred) would
// return true. ranges::is_permutation uses projections.
#include <algorithm>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
};

constexpr bool test() {
  int a[] = {1, 2, 3, 2};
  int b[] = {2, 3, 2, 1};
  int c[] = {2, 3, 1, 1};
  if (!std::is_permutation(a, a + 4, b) || std::is_permutation(a, a + 4, c)) return false;
  if (!std::is_permutation(a, a + 4, b, b + 4) || std::is_permutation(a, a + 4, b, b + 3)) return false;
  if (!std::is_permutation(a, a, b, b)) return false;
  auto mod2 = [](int l, int r) { return l % 2 == r % 2; };
  if (std::is_permutation(a, a + 4, c, mod2)) return false;  // parities {1,0,1,0} vs {0,1,1,1}
  int d[] = {4, 5, 7, 6};
  if (!std::is_permutation(a, a + 4, d, d + 4, mod2)) return false;
  if (!std::is_permutation(ForwardIter<int>(a), ForwardIter<int>(a + 4), ForwardIter<int>(b),
                           ForwardIter<int>(b + 4)))
    return false;

  if (!std::ranges::is_permutation(a, b) || std::ranges::is_permutation(a, c)) return false;
  Pt p[] = {{3, 0}, {1, 0}, {2, 0}, {2, 0}};
  if (!std::ranges::is_permutation(p, a, {}, &Pt::x)) return false;
  if (!std::ranges::is_permutation(a, p, {}, {}, &Pt::x)) return false;
  ForwardRange<int> fa{a, a + 4};
  ForwardRange<int> fb{b, b + 4};
  if (!std::ranges::is_permutation(fa, fb)) return false;
  ForwardRange<int> fb3{b, b + 3};
  if (std::ranges::is_permutation(fa, fb3)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[] = {1, 2, 3, 4};
  int b[] = {1, 2, 3, 4, 5};
  int calls = 0;
  auto pred = [&](int x, int y) {
    ++calls;
    return x == y;
  };
  CHECK(!std::is_permutation(a, a + 4, b, b + 5, pred) && calls == 0);
  CHECK(std::is_permutation(a, a + 4, b, b + 4, pred) && calls == 4);  // equal ranges
  calls = 0;
  CHECK(std::is_permutation(a, a + 4, b, pred) && calls == 4);
  return 0;
}
