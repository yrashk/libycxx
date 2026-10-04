// [alg.starts.with]: ranges::starts_with returns ranges::mismatch(first1, last1, first2,
// last2, pred, proj1, proj2).in2 == last2. [alg.ends.with]: ranges::ends_with returns false
// if N1 < N2, otherwise ranges::equal of the last N2 elements of the first range with the
// second. ends_with needs forward or sized ranges.
#include <algorithm>
#include <functional>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 4, 5};
  int pre[] = {1, 2};
  int suf[] = {4, 5};
  int bad[] = {2, 3};
  int longer[] = {1, 2, 3, 4, 5, 6};
  if (!std::ranges::starts_with(a, pre) || std::ranges::starts_with(a, bad)) return false;
  if (!std::ranges::ends_with(a, suf) || std::ranges::ends_with(a, bad)) return false;
  if (std::ranges::starts_with(a, longer) || std::ranges::ends_with(a, longer)) return false;
  if (!std::ranges::starts_with(a, a + 5, pre, pre) || !std::ranges::ends_with(a, a + 5, suf, suf)) return false;
  if (!std::ranges::starts_with(a, a) || !std::ranges::ends_with(a, a)) return false;
  int tens[] = {10, 20};
  if (!std::ranges::starts_with(a, tens, {}, [](int v) { return v * 10; })) return false;
  int fifties[] = {40, 50};
  if (!std::ranges::ends_with(a, fifties, {}, {}, [](int v) { return v / 10; })) return false;
  if (std::ranges::ends_with(a, tens, {}, {}, [](int v) { return v / 10; })) return false;
  if (!std::ranges::starts_with(a, a + 5, tens, tens + 2, std::ranges::less{})) return false;
  // input (non-forward, unsized) ranges for starts_with
  InputRange<int> ia{a, a + 5};
  InputRange<int> ip{pre, pre + 2};
  if (!std::ranges::starts_with(ia, ip)) return false;
  // forward, non-sized ranges for ends_with
  ForwardRange<int> fa{a, a + 5};
  ForwardRange<int> fs{suf, suf + 2};
  if (!std::ranges::ends_with(fa, fs)) return false;
  ForwardRange<int> fl{longer, longer + 6};
  if (std::ranges::ends_with(fa, fl)) return false;
  return true;
}
static_assert(test());

template <class R1, class R2>
concept can_ends_with = requires(R1 r1, R2 r2) { std::ranges::ends_with(r1, r2); };
static_assert(can_ends_with<ForwardRange<int>, ForwardRange<int>>);
static_assert(!can_ends_with<InputRange<int>, ForwardRange<int>>);  // neither forward nor sized

int main() {
  CHECK(test());
  return 0;
}
