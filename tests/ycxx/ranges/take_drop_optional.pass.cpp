// [range.take.overview]/2.2, [range.drop.overview]/2.2: for an optional (which models view,
// [optional.optional.general]), views::take(E, F) is
//   "(static_cast<D>(F) == D() ? ((void)E, T()) : decay-copy(E))"
// and views::drop(E, F) is
//   "(static_cast<D>(F) == D() ? decay-copy(E) : ((void)E, T()))".
#include <optional>
#include <ranges>
#include <type_traits>
#include "check.hpp"

namespace vw = std::views;

static_assert(std::ranges::view<std::optional<int>>);
static_assert(std::is_same_v<decltype(std::optional<int>(1) | vw::take(1)), std::optional<int>>);
static_assert(std::is_same_v<decltype(std::optional<int>(1) | vw::drop(1)), std::optional<int>>);

constexpr bool test() {
  std::optional<int> o(5);
  CHECK(!(o | vw::take(0)).has_value());
  CHECK((o | vw::take(1)).value() == 5);
  CHECK((o | vw::take(3)).value() == 5);
  CHECK((o | vw::drop(0)).value() == 5);
  CHECK(!(o | vw::drop(1)).has_value());
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
