// [utility.undefined]/3: void observable_checkpoint() noexcept; "Establishes an observable
// checkpoint". [utility.syn]: monostate, its operator== / operator<=> and hash<monostate>
// are declared in <utility>. [variant.monostate.relops]: monostate objects always compare
// equal; operator<=> returns strong_ordering::equal; both noexcept.
#include <compare>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::observable_checkpoint()), void>);
static_assert(noexcept(std::observable_checkpoint()));

static_assert(std::is_empty_v<std::monostate>);
static_assert(std::is_trivially_copyable_v<std::monostate>);
static_assert(std::is_nothrow_default_constructible_v<std::monostate>);
static_assert(std::monostate{} == std::monostate{});
static_assert(!(std::monostate{} != std::monostate{}));
static_assert((std::monostate{} <=> std::monostate{}) == std::strong_ordering::equal);
static_assert(std::is_same_v<decltype(std::monostate{} <=> std::monostate{}), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::monostate{} == std::monostate{}), bool>);
static_assert(!(std::monostate{} < std::monostate{}) && std::monostate{} <= std::monostate{});
static_assert(noexcept(std::monostate{} == std::monostate{}));
static_assert(noexcept(std::monostate{} <=> std::monostate{}));
static_assert(std::is_default_constructible_v<std::hash<std::monostate>>);

int main() {
  int x = 1;
  x += 1;
  std::observable_checkpoint();
  CHECK(x == 2);
  std::monostate a, b;
  CHECK(a == b);
  std::hash<std::monostate> h;
  CHECK(h(a) == h(b));
  return 0;
}
