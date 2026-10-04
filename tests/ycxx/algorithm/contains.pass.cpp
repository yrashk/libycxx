// [alg.contains]: ranges::contains(first, last, value, proj) returns ranges::find(first,
// last, value, proj) != last; ranges::contains_subrange returns first2 == last2 ||
// !ranges::search(first1, last1, first2, last2, pred, proj1, proj2).empty(). T defaults to
// the projected value type.
#include <algorithm>
#include <functional>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {3, 1, 4, 1, 5};
  if (!std::ranges::contains(a, 4) || std::ranges::contains(a, 2)) return false;
  if (!std::ranges::contains(a, a + 5, 5) || std::ranges::contains(a, a + 4, 5)) return false;
  if (std::ranges::contains(a, a, 3)) return false;
  Pt p[] = {{1, 2}, {3, 4}};
  if (!std::ranges::contains(p, {3, 4}) || std::ranges::contains(p, {4, 3})) return false;
  if (!std::ranges::contains(p, 4, &Pt::y) || std::ranges::contains(p, 4, &Pt::x)) return false;
  InputRange<int> in{a, a + 5};
  if (!std::ranges::contains(in, 1)) return false;

  int s1[] = {1, 4, 1};
  int s2[] = {1, 5, 9};
  if (!std::ranges::contains_subrange(a, s1) || std::ranges::contains_subrange(a, s2)) return false;
  if (!std::ranges::contains_subrange(a, a + 5, s1, s1)) return false;  // empty needle
  if (!std::ranges::contains_subrange(a, a, s1, s1)) return false;     // both empty
  if (std::ranges::contains_subrange(a, a, s1, s1 + 1)) return false;
  int tens[] = {40, 10};
  if (!std::ranges::contains_subrange(a, tens, {}, {}, [](int v) { return v / 10; })) return false;
  if (!std::ranges::contains_subrange(a, tens, std::ranges::equal_to{}, [](int v) { return v * 10; })) return false;
  ForwardRange<int> fr{a, a + 5};
  if (!std::ranges::contains_subrange(fr, s1)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
