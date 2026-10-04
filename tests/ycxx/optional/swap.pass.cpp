// [optional.swap]: Table 71 -- both engaged: ADL swap; one engaged: move-construct into the
// other and destroy the source; neither: no effect. noexcept = nothrow move-constructible &&
// nothrow swappable. [optional.specalg]/1: non-member swap constrained on
// is_reference_v<T> || (is_move_constructible_v<T> && is_swappable_v<T>).
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ns {
struct Swappy {
  int v;
  int swaps = 0;
  constexpr Swappy(int x) : v(x) {}
  constexpr Swappy(Swappy&& o) noexcept : v(o.v) {}
  constexpr Swappy& operator=(Swappy&& o) noexcept { v = o.v; return *this; }
  constexpr friend void swap(Swappy& a, Swappy& b) noexcept {
    int t = a.v; a.v = b.v; b.v = t; ++a.swaps; ++b.swaps;
  }
};
struct ThrowingSwap {
  ThrowingSwap() = default;
  friend void swap(ThrowingSwap&, ThrowingSwap&) noexcept(false) {}
};
struct NotSwappable {
  NotSwappable() = default;
  NotSwappable(NotSwappable&&) = default;
  NotSwappable& operator=(NotSwappable&&) = delete;  // not move-assignable -> std::swap not viable
};
}  // namespace ns

static_assert(std::is_nothrow_swappable_v<std::optional<int>>);
static_assert(std::is_nothrow_swappable_v<std::optional<ns::Swappy>>);
static_assert(!std::is_nothrow_swappable_v<std::optional<ns::ThrowingSwap>>);
static_assert(std::is_swappable_v<std::optional<ns::ThrowingSwap>>);
static_assert(!std::is_swappable_v<std::optional<ns::NotSwappable>>);
static_assert(std::is_nothrow_swappable_v<std::optional<int&>>);

constexpr bool test() {
  std::optional<ns::Swappy> a(std::in_place, 1), b(std::in_place, 2), e;
  a.swap(b);
  if (a->v != 2 || b->v != 1 || a->swaps != 1) return false;
  swap(a, b);
  if (a->v != 1 || a->swaps != 2) return false;
  a.swap(e);  // only *this engaged
  if (a.has_value() || !e.has_value() || e->v != 1 || e->swaps != 0) return false;
  a.swap(e);  // only rhs engaged
  if (!a.has_value() || e.has_value() || a->v != 1) return false;
  std::optional<int> x, y;
  x.swap(y);
  if (x || y) return false;
  std::swap(x, y);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
