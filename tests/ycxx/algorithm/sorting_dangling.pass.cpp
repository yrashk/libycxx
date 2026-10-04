// [range.dangling] applied to [alg.sorting]: the range forms return borrowed_iterator_t<R>
// / borrowed_subrange_t<R>, which is ranges::dangling for a non-borrowed rvalue range, and
// the *_result types carry dangling in that position; borrowed ranges (span, subrange)
// still yield iterators.
#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <type_traits>
#include "check.hpp"

using A = std::array<int, 4>;
using D = std::ranges::dangling;

static_assert(std::is_same_v<decltype(std::ranges::sort(A{})), D>);
static_assert(std::is_same_v<decltype(std::ranges::stable_sort(A{})), D>);
static_assert(std::is_same_v<decltype(std::ranges::make_heap(A{})), D>);
static_assert(std::is_same_v<decltype(std::ranges::is_sorted_until(A{})), D>);
static_assert(std::is_same_v<decltype(std::ranges::lower_bound(A{}, 1)), D>);
static_assert(std::is_same_v<decltype(std::ranges::equal_range(A{}, 1)), D>);
static_assert(std::is_same_v<decltype(std::ranges::partition(A{}, [](int) { return true; })), D>);
static_assert(std::is_same_v<decltype(std::ranges::partition_point(A{}, [](int) { return true; })), D>);
static_assert(std::is_same_v<decltype(std::ranges::min_element(A{})), D>);
static_assert(std::is_same_v<decltype(std::ranges::minmax_element(A{})), std::ranges::minmax_element_result<D>>);
static_assert(std::is_same_v<decltype(std::ranges::next_permutation(A{})), std::ranges::next_permutation_result<D>>);
static_assert(std::is_same_v<decltype(std::ranges::merge(A{}, A{}, (int*)nullptr)), std::ranges::merge_result<D, D, int*>>);
static_assert(std::is_same_v<decltype(std::ranges::set_difference(A{}, A{}, (int*)nullptr)), std::ranges::set_difference_result<D, int*>>);
static_assert(std::is_same_v<decltype(std::ranges::partial_sort_copy(A{}, A{})), std::ranges::partial_sort_copy_result<D, D>>);
// value-returning algorithms are unaffected
static_assert(std::is_same_v<decltype(std::ranges::min(A{})), int>);
static_assert(std::is_same_v<decltype(std::ranges::minmax(A{})), std::ranges::minmax_result<int>>);
static_assert(std::is_same_v<decltype(std::ranges::is_sorted(A{})), bool>);
// borrowed ranges
static_assert(std::is_same_v<decltype(std::ranges::sort(std::span<int>{})), std::span<int>::iterator>);
static_assert(std::is_same_v<decltype(std::ranges::equal_range(std::span<int>{}, 1)), std::ranges::subrange<std::span<int>::iterator>>);

int main() {
  int a[] = {3, 1, 2};
  auto it = std::ranges::sort(std::span<int>(a));
  CHECK(&*(it - 1) == a + 2 && a[0] == 1);
  return 0;
}
