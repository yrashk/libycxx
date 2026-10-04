// [range.iota.overview]/4: "views::indices(E) is expression-equivalent to
// views::iota(T(0), E)" with T = remove_cvref_t<decltype((E))>.
#include <cstddef>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;

static_assert(std::is_same_v<decltype(std::views::indices(5)), rg::iota_view<int, int>>);
static_assert(std::is_same_v<decltype(std::views::indices(std::size_t(5))),
                             rg::iota_view<std::size_t, std::size_t>>);
const short cs = 3;
static_assert(std::is_same_v<decltype(std::views::indices(cs)), rg::iota_view<short, short>>);

constexpr bool test() {
  CHECK(range_equals(std::views::indices(4), {0, 1, 2, 3}));
  CHECK(std::views::indices(0).empty());
  unsigned n = 3;
  CHECK(range_equals(std::views::indices(n), {0u, 1u, 2u}));
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
