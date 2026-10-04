// [concept.swappable]/2: ranges::swap(E1, E2) is (void)swap(E1, E2) found by ADL (with a
// deleted template<class T> void swap(T&, T&) in the lookup set); otherwise for lvalue arrays
// of equal extent swaps elementwise; otherwise for lvalues of a move_constructible and
// assignable_from<T&, T> type exchanges the values; otherwise ill-formed (SFINAE-friendly).
// noexcept and constant-expression properties per /2.3. swappable / swappable_with.
#include <concepts>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace N {
struct Custom {
  int v;
  int swaps = 0;
};
constexpr void swap(Custom& a, Custom& b) noexcept(false) {
  int t = a.v;
  a.v = b.v;
  b.v = t;
  ++a.swaps;
  ++b.swaps;
}
struct Unconstrained {
  int v;
};
// unconstrained template: not more specialised than the poison pill, so ambiguous ->
// ranges::swap falls back to the move-based exchange
template <class T>
void swap(T&, T&) = delete;

struct A {
  int m;
};
struct Proxy {
  A* a;
  constexpr Proxy(A& x) : a{&x} {}
  friend constexpr void swap(Proxy x, Proxy y) { std::ranges::swap(*x.a, *y.a); }
};
}  // namespace N

struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
  ThrowingMove& operator=(ThrowingMove&&) noexcept(false) { return *this; }
};
struct Immovable {
  Immovable() = default;
  Immovable(Immovable&&) = delete;
};

template <class A, class B>
concept can_swap = requires(A&& a, B&& b) { std::ranges::swap(std::forward<A>(a), std::forward<B>(b)); };

static_assert(std::is_same_v<decltype(std::ranges::swap(std::declval<int&>(), std::declval<int&>())), void>);
static_assert(noexcept(std::ranges::swap(std::declval<int&>(), std::declval<int&>())));
static_assert(!noexcept(std::ranges::swap(std::declval<ThrowingMove&>(), std::declval<ThrowingMove&>())));
static_assert(!noexcept(std::ranges::swap(std::declval<N::Custom&>(), std::declval<N::Custom&>())));
static_assert(noexcept(std::ranges::swap(std::declval<int (&)[3]>(), std::declval<int (&)[3]>())));
static_assert(can_swap<int&, int&>);
static_assert(!can_swap<int&, long&>);
static_assert(!can_swap<int, int>);
static_assert(!can_swap<int (&)[2], int (&)[3]>);
static_assert(!can_swap<Immovable&, Immovable&>);
static_assert(!can_swap<const int&, const int&>);
static_assert(std::swappable<int>);
static_assert(std::swappable<int[2][3]>);
static_assert(!std::swappable<Immovable>);
static_assert(std::swappable<N::Custom>);
static_assert(std::swappable<N::Unconstrained>);
static_assert(std::swappable_with<int&, int&>);
static_assert(!std::swappable_with<int, int>);
static_assert(!std::swappable_with<int&, long&>);

constexpr bool test() {
  int a = 1, b = 2;
  std::ranges::swap(a, b);
  if (a != 2 || b != 1) return false;
  int x[2][2] = {{1, 2}, {3, 4}}, y[2][2] = {{5, 6}, {7, 8}};
  std::ranges::swap(x, y);
  if (x[1][1] != 8 || y[0][0] != 1) return false;
  N::Custom c1{1}, c2{2};
  std::ranges::swap(c1, c2);
  if (c1.v != 2 || c1.swaps != 1) return false;  // the ADL swap was used
  N::Unconstrained u1{1}, u2{2};
  std::ranges::swap(u1, u2);
  if (u1.v != 2 || u2.v != 1) return false;
  // the example of [concept.swappable]: swapping through a proxy rvalue
  N::A a1{5}, a2{-5};
  std::ranges::swap(N::Proxy(a1), N::Proxy(a2));
  if (a1.m != -5 || a2.m != 5) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
