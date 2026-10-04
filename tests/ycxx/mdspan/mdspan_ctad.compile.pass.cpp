// [mdspan.mdspan.overview]: deduction guides: from a one-dimensional C array (static extent),
// a pointer (rank 0), a pointer and integrals (extents<size_t, maybe-static-ext<I>...>), a
// pointer and span/array (dextents<size_t, N>), a pointer and extents, a pointer and a mapping
// (its extents and layout), and a data handle, mapping and accessor.
#include <mdspan>
#include <array>
#include <span>
#include <type_traits>
#include <utility>

using std::dynamic_extent;
using std::is_same_v;
using std::size_t;

int arr[6];
int* p = arr;
const int* cp = arr;

static_assert(is_same_v<decltype(std::mdspan(arr)), std::mdspan<int, std::extents<size_t, 6>>>);
static_assert(is_same_v<decltype(std::mdspan(p)), std::mdspan<int, std::extents<size_t>>>);
static_assert(is_same_v<decltype(std::mdspan(cp, 2, 3)), std::mdspan<const int, std::dextents<size_t, 2>>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::cw<2>, 3)), std::mdspan<int, std::extents<size_t, 2, dynamic_extent>>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::integral_constant<size_t, 2>{}, std::integral_constant<int, 3>{})),
                        std::mdspan<int, std::extents<size_t, 2, 3>>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::array<int, 3>{1, 2, 3})), std::mdspan<int, std::dextents<size_t, 3>>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::declval<std::span<long, 2>>())), std::mdspan<int, std::dextents<size_t, 2>>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::extents<short, 2, dynamic_extent>(3))),
                        std::mdspan<int, std::extents<short, 2, dynamic_extent>>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::layout_left::mapping<std::extents<int, 2, 3>>())),
                        std::mdspan<int, std::extents<int, 2, 3>, std::layout_left>>);
static_assert(is_same_v<decltype(std::mdspan(p, std::layout_stride::mapping<std::dextents<int, 1>>(), std::default_accessor<int>())),
                        std::mdspan<int, std::dextents<int, 1>, std::layout_stride, std::default_accessor<int>>>);
