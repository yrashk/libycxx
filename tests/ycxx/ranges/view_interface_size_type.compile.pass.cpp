// [view.interface.general]: view_interface::size() is
//   "return to-unsigned-like(ranges::end(derived()) - ranges::begin(derived()));"
// so it returns the unsigned counterpart of the difference type.
#include <cstddef>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"

template <class It>
struct V : std::ranges::view_interface<V<It>> {
  It b{}, e{};
  constexpr It begin() const { return b; }
  constexpr It end() const { return e; }
};

static_assert(std::is_same_v<decltype(std::declval<V<int*>&>().size()), std::size_t>);
static_assert(std::is_same_v<decltype(std::declval<const V<int*>&>().size()), std::size_t>);
static_assert(std::is_same_v<decltype(std::declval<V<RandomIter<int>>&>().size()), std::size_t>);
