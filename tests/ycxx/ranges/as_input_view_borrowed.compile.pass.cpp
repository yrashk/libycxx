// [ranges.syn]: "template<class V> constexpr bool enable_borrowed_range<as_input_view<V>> =
// enable_borrowed_range<V>;"
#include <ranges>
#include "range_support.hpp"

namespace rg = std::ranges;

static_assert(rg::borrowed_range<rg::as_input_view<rg::ref_view<int[3]>>>);
static_assert(rg::borrowed_range<rg::as_input_view<BorrowedView<int>>>);
static_assert(!rg::borrowed_range<rg::as_input_view<rg::owning_view<rg::single_view<int>>>>);
