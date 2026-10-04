// [variant.swap]: member swap -- same index calls swap(GET<i>(*this), GET<i>(rhs)) (found by
// ADL); different index exchanges values. noexcept = AND of nothrow move-constructible and
// nothrow swappable. [variant.specalg]: non-member swap, Constraints
// is_move_constructible_v<Ti> && is_swappable_v<Ti> for all i; noexcept(v.swap(w)).
#include <variant>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ns {
struct Swappy {
  int v;
  int swaps = 0;
  constexpr Swappy(int x) : v(x) {}
  constexpr friend void swap(Swappy& a, Swappy& b) noexcept {
    int t = a.v; a.v = b.v; b.v = t;
    ++a.swaps; ++b.swaps;
  }
};
struct ThrowingSwap {
  ThrowingSwap() = default;
  friend void swap(ThrowingSwap&, ThrowingSwap&) noexcept(false) {}
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
  ThrowingMove& operator=(ThrowingMove&&) noexcept { return *this; }
};
}  // namespace ns

static_assert(std::is_nothrow_swappable_v<std::variant<int, double>>);
static_assert(std::is_nothrow_swappable_v<std::variant<int, ns::Swappy>>);
static_assert(!std::is_nothrow_swappable_v<std::variant<int, ns::ThrowingSwap>>);
static_assert(!std::is_nothrow_swappable_v<std::variant<int, ns::ThrowingMove>>);
static_assert(std::is_swappable_v<std::variant<int, ns::ThrowingSwap>>);
static_assert(noexcept(std::declval<std::variant<int>&>().swap(std::declval<std::variant<int>&>())));
static_assert(!noexcept(std::declval<std::variant<ns::ThrowingMove>&>().swap(
    std::declval<std::variant<ns::ThrowingMove>&>())));

constexpr bool test() {
  // same index -> ADL swap used
  {
    std::variant<int, ns::Swappy> a(std::in_place_index<1>, 1), b(std::in_place_index<1>, 2);
    a.swap(b);
    if (std::get<1>(a).v != 2 || std::get<1>(b).v != 1) return false;
    if (std::get<1>(a).swaps != 1) return false;
    swap(a, b);
    if (std::get<1>(a).v != 1 || std::get<1>(a).swaps != 2) return false;
  }
  // different index -> exchange
  {
    std::variant<int, double> a(1), b(2.5);
    a.swap(b);
    if (a.index() != 1 || std::get<1>(a) != 2.5 || b.index() != 0 || std::get<0>(b) != 1) return false;
    std::swap(a, b);
    if (a.index() != 0 || std::get<0>(a) != 1) return false;
  }
  // same index, plain types
  {
    std::variant<int, double> a(1), b(2);
    std::swap(a, b);
    if (std::get<0>(a) != 2 || std::get<0>(b) != 1) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
