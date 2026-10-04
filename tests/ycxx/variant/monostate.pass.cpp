// [variant.monostate]: monostate is a trivially copyable empty class (a unit type).
// [variant.monostate.relops]: == returns true, <=> returns strong_ordering::equal, both
// constexpr and noexcept. [variant.hash]/2: hash<monostate> is enabled.
#include <variant>
#include <compare>
#include <functional>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_empty_v<std::monostate>);
static_assert(std::is_trivially_copyable_v<std::monostate>);
static_assert(std::is_trivially_default_constructible_v<std::monostate>);
static_assert(std::is_nothrow_default_constructible_v<std::monostate>);
static_assert(std::is_same_v<decltype(std::monostate{} <=> std::monostate{}), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::monostate{} == std::monostate{}), bool>);
static_assert(noexcept(std::monostate{} == std::monostate{}));
static_assert(noexcept(std::monostate{} <=> std::monostate{}));
static_assert(std::monostate{} == std::monostate{});
static_assert(!(std::monostate{} != std::monostate{}));
static_assert(!(std::monostate{} < std::monostate{}));
static_assert(std::monostate{} <= std::monostate{});
static_assert(std::monostate{} >= std::monostate{});
static_assert((std::monostate{} <=> std::monostate{}) == std::strong_ordering::equal);
static_assert(std::three_way_comparable<std::monostate, std::strong_ordering>);

// hash<monostate> enabled
static_assert(std::is_default_constructible_v<std::hash<std::monostate>>);
static_assert(std::is_same_v<decltype(std::hash<std::monostate>{}(std::monostate{})), std::size_t>);

int main() {
  std::hash<std::monostate> h;
  CHECK(h(std::monostate{}) == h(std::monostate{}));
  std::variant<std::monostate, int> v;
  CHECK(v.index() == 0);
  CHECK(v == std::variant<std::monostate, int>{});
  return 0;
}
