// [alg.all.of], [alg.any.of], [alg.none.of]: all_of is false iff some element fails pred
// (true for an empty range), any_of is true iff some element satisfies it (false for an
// empty range), none_of is true iff no element does (true for an empty range). The
// ranges:: forms apply invoke(pred, invoke(proj, *i)); all are constexpr and accept
// single-pass input iterators and non-common sentinels.
#include <algorithm>
#include <functional>
#include "test_iterators.hpp"
#include "check.hpp"

struct Item {
  int key;
  bool flag;
  constexpr bool is_positive() const { return key > 0; }
};

constexpr bool test() {
  int a[] = {2, 4, 6, 7};
  auto even = [](int x) { return x % 2 == 0; };
  auto big = [](int x) { return x > 100; };
  if (std::all_of(a, a + 3, even) != true || std::all_of(a, a + 4, even) != false) return false;
  if (!std::any_of(a, a + 4, even) || std::any_of(a, a + 4, big)) return false;
  if (!std::none_of(a, a + 4, big) || std::none_of(a, a + 4, even)) return false;
  // empty ranges
  if (!std::all_of(a, a, big) || std::any_of(a, a, even) || !std::none_of(a, a, even)) return false;
  // input iterators
  if (!std::all_of(InputIter<int>(a), InputIter<int>(a + 3), even)) return false;
  if (!std::any_of(InputIter<int>(a), InputIter<int>(a + 4), [](int x) { return x == 7; })) return false;

  // ranges forms, iterator/sentinel and range overloads, with projections
  Item items[] = {{1, true}, {2, true}, {-3, false}};
  if (!std::ranges::all_of(items, items + 2, std::identity{}, &Item::flag)) return false;
  if (std::ranges::all_of(items, &Item::flag)) return false;
  if (!std::ranges::any_of(items, [](int k) { return k < 0; }, &Item::key)) return false;
  if (!std::ranges::none_of(items, [](int k) { return k > 5; }, &Item::key)) return false;
  if (!std::ranges::all_of(items, items + 2, &Item::is_positive)) return false;  // member fn as pred
  if (std::ranges::all_of(items, &Item::is_positive)) return false;
  InputRange<int> in{a, a + 4};
  if (std::ranges::all_of(in, even) || !std::ranges::any_of(in, even) || std::ranges::none_of(in, even)) return false;
  ForwardRange<int> fr{a, a + 3};
  if (!std::ranges::all_of(fr.begin(), fr.end(), even)) return false;
  int empty_[1] = {0};
  if (!std::ranges::all_of(empty_, empty_, big) || std::ranges::any_of(empty_, empty_, even)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // Each result is specified; evaluation order and short-circuiting are not.
  // Complexity: at most N predicate applications, including zero for an empty range.
  int a[] = {1, 2, 3, 4, 5};
  int calls = 0;
  auto pred = [&](int x) {
    ++calls;
    return x < 3;
  };
  CHECK(!std::all_of(a, a + 5, pred) && calls <= 5);
  calls = 0;
  CHECK(std::any_of(a, a + 5, [&](int x) { ++calls; return x == 2; }) && calls <= 5);
  calls = 0;
  CHECK(!std::ranges::none_of(a, [&](int x) { ++calls; return x == 1; }) && calls <= 5);
  calls = 0;
  CHECK(std::all_of(a, a, pred) && calls == 0);
  CHECK(!std::any_of(a, a, pred) && calls == 0);
  CHECK(std::none_of(a, a, pred) && calls == 0);
  return 0;
}
