// [expected.object.swap]: Table 72 -- value/value: swap(val); error/error: swap(unex);
// value/error: exchange via moves (both branches of the "see below" algorithm); error/value:
// rhs.swap(*this). Constraints: swappable, move-constructible, and at least one of T, E
// nothrow move-constructible. noexcept per /4. Friend swap.
#include <expected>
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
  constexpr friend void swap(Swappy& a, Swappy& b) noexcept { std::swap(a.v, b.v); ++a.swaps; ++b.swaps; }
};
struct ThrowingMove {
  int v;
  constexpr ThrowingMove(int x) : v(x) {}
  constexpr ThrowingMove(ThrowingMove&& o) noexcept(false) : v(o.v) {}
  constexpr ThrowingMove& operator=(ThrowingMove&& o) noexcept(false) { v = o.v; return *this; }
};
}  // namespace ns
using ns::Swappy;
using ns::ThrowingMove;

static_assert(std::is_nothrow_swappable_v<std::expected<int, long>>);
static_assert(std::is_swappable_v<std::expected<ThrowingMove, int>>);
static_assert(!std::is_nothrow_swappable_v<std::expected<ThrowingMove, int>>);
static_assert(!std::is_nothrow_swappable_v<std::expected<int, ThrowingMove>>);
template <class X> concept member_swappable = requires(X& a, X& b) { a.swap(b); };
static_assert(member_swappable<std::expected<ThrowingMove, int>>);
static_assert(!member_swappable<std::expected<ThrowingMove, ThrowingMove>>);  // (1.4)

constexpr bool test() {
  {
    std::expected<Swappy, int> a(std::in_place, 1), b(std::in_place, 2);
    a.swap(b);
    if (a->v != 2 || b->v != 1 || a->swaps != 1) return false;
  }
  {
    std::expected<int, Swappy> a(std::unexpect, 1), b(std::unexpect, 2);
    swap(a, b);
    if (a.error().v != 2 || a.error().swaps != 1) return false;
  }
  // value / error with nothrow-move E (first branch)
  {
    std::expected<ThrowingMove, int> a(std::in_place, 1), b(std::unexpect, 2);
    a.swap(b);
    if (a.has_value() || a.error() != 2 || !b.has_value() || b->v != 1) return false;
    a.swap(b);  // error / value -> rhs.swap(*this)
    if (!a.has_value() || a->v != 1 || b.error() != 2) return false;
  }
  // value / error with throwing-move E, nothrow-move T (second branch)
  {
    std::expected<int, ThrowingMove> a(1), b(std::unexpect, 2);
    a.swap(b);
    if (a.has_value() || a.error().v != 2 || !b.has_value() || *b != 1) return false;
    swap(a, b);
    if (!a.has_value() || *a != 1 || b.has_value() || b.error().v != 2) return false;
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
