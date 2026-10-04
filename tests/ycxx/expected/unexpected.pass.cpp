// [expected.un.general], [expected.un.cons], [expected.un.obs], [expected.un.swap],
// [expected.un.eq]: unexpected<E> -- explicit converting constructor (Err defaults to E, so
// braced init works), in_place constructors, error() for all value categories (noexcept),
// member/friend swap (noexcept(is_nothrow_swappable_v<E>)), heterogeneous ==, CTAD.
#include <expected>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct IL {
  int sum = 0, extra = 0;
  constexpr IL(std::initializer_list<int> il, int e) : extra(e) { for (int x : il) sum += x; }
};
struct Pt { int x, y; constexpr bool operator==(const Pt&) const = default; };
namespace ns {
struct ThrowingSwap {
  friend void swap(ThrowingSwap&, ThrowingSwap&) noexcept(false) {}
};
}

using U = std::unexpected<int>;
static_assert(std::is_constructible_v<U, int>);
static_assert(!std::is_convertible_v<int, U>);  // explicit
static_assert(std::is_constructible_v<U, long>);
static_assert(!std::is_constructible_v<U, int*>);
static_assert(std::is_constructible_v<U, std::in_place_t, int>);
static_assert(!std::is_convertible_v<std::in_place_t, U>);
static_assert(std::is_trivially_copyable_v<U>);
static_assert(std::is_same_v<decltype(std::declval<U&>().error()), int&>);
static_assert(std::is_same_v<decltype(std::declval<U&&>().error()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const U&>().error()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const U&&>().error()), const int&&>);
static_assert(noexcept(std::declval<U&>().error()) && noexcept(std::declval<const U&&>().error()));
static_assert(noexcept(std::declval<U&>().swap(std::declval<U&>())));
static_assert(!noexcept(std::declval<std::unexpected<ns::ThrowingSwap>&>().swap(
    std::declval<std::unexpected<ns::ThrowingSwap>&>())));
static_assert(std::is_nothrow_swappable_v<U>);
static_assert(!std::is_nothrow_swappable_v<std::unexpected<ns::ThrowingSwap>>);
static_assert(std::is_same_v<decltype(std::unexpected(1)), std::unexpected<int>>);
static_assert(std::is_same_v<decltype(std::unexpected(1.5)), std::unexpected<double>>);

constexpr bool test() {
  U u(5);
  if (u.error() != 5) return false;
  u.error() = 6;
  if (std::as_const(u).error() != 6 || std::move(u).error() != 6) return false;
  std::unexpected<Pt> p({1, 2});  // Err = E by default
  if (p.error().y != 2) return false;
  std::unexpected<Pt> q(std::in_place, 3, 4);
  if (q.error().x != 3) return false;
  std::unexpected<IL> il(std::in_place, {1, 2, 3}, 9);
  if (il.error().sum != 6 || il.error().extra != 9) return false;
  // copy / move / assignment
  U c(u);
  U d(7);
  d = c;
  if (d.error() != 6) return false;
  // swap
  U a(1), b(2);
  a.swap(b);
  if (a.error() != 2 || b.error() != 1) return false;
  swap(a, b);
  if (a.error() != 1) return false;
  // ==, heterogeneous
  if (!(a == U(1)) || a == U(2) || !(a != U(3))) return false;
  if (!(a == std::unexpected<long>(1L))) return false;
  if (!(p == std::unexpected<Pt>(Pt{1, 2}))) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
