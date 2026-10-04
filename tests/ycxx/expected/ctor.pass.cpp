// [expected.object.cons]: default (value-initializes, constrained), copy/move
// (rhs.has_value() unchanged, noexcept), converting from expected<U,G> (explicit(bool)),
// U&& (explicit(!is_convertible_v<U,T>), default U = remove_cv_t<T>), from unexpected<G>
// (explicit(bool)), in_place / unexpect (+ initializer_list), all constexpr.
// [expected.object.general]: member types value_type, error_type, unexpected_type, rebind.
#include <expected>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct NoDefault { NoDefault(int) {} };
struct Explicit { int v; constexpr explicit Explicit(int x) : v(x) {} };
struct IL {
  int sum = 0, extra = 0;
  constexpr IL(std::initializer_list<int> il, int e) : extra(e) { for (int x : il) sum += x; }
};
struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
};
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
};
struct Agg { int a, b; };

using E = std::expected<int, long>;
static_assert(std::is_same_v<E::value_type, int>);
static_assert(std::is_same_v<E::error_type, long>);
static_assert(std::is_same_v<E::unexpected_type, std::unexpected<long>>);
static_assert(std::is_same_v<E::rebind<double>, std::expected<double, long>>);

static_assert(std::is_default_constructible_v<E>);
static_assert(!std::is_default_constructible_v<std::expected<NoDefault, int>>);
static_assert(std::is_default_constructible_v<std::expected<int, NoDefault>>);
static_assert(std::is_nothrow_move_constructible_v<std::expected<MoveOnly, int>>);
static_assert(!std::is_nothrow_move_constructible_v<std::expected<ThrowingMove, int>>);
static_assert(!std::is_nothrow_move_constructible_v<std::expected<int, ThrowingMove>>);
static_assert(!std::is_copy_constructible_v<std::expected<MoveOnly, int>>);
static_assert(!std::is_copy_constructible_v<std::expected<int, MoveOnly>>);
// explicit(bool)
static_assert(std::is_convertible_v<int, E>);
static_assert(!std::is_convertible_v<int, std::expected<Explicit, int>>);
static_assert(std::is_constructible_v<std::expected<Explicit, int>, int>);
static_assert(std::is_convertible_v<std::unexpected<int>, E>);
static_assert(!std::is_convertible_v<std::unexpected<int>, std::expected<int, Explicit>>);
static_assert(std::is_constructible_v<std::expected<int, Explicit>, std::unexpected<int>>);
static_assert(!std::is_convertible_v<const std::unexpected<int>&, std::expected<int, Explicit>>);
static_assert(std::is_convertible_v<std::expected<short, int>, E>);
static_assert(!std::is_convertible_v<std::expected<int, int>, std::expected<Explicit, int>>);
static_assert(!std::is_convertible_v<std::expected<int, int>, std::expected<int, Explicit>>);
static_assert(std::is_constructible_v<std::expected<Explicit, Explicit>, const std::expected<int, int>&>);
static_assert(!std::is_constructible_v<E, std::expected<int*, long>>);
// tags are explicit
static_assert(!std::is_convertible_v<std::in_place_t, E>);
static_assert(!std::is_convertible_v<std::unexpect_t, E>);
// U&& constraint (23.4): an unexpected<G> argument never initializes the value, even if T
// could be constructed from it.
struct FromUnex { constexpr FromUnex(std::unexpected<int>) {} };
static_assert(std::is_constructible_v<std::expected<FromUnex, int>, std::unexpected<int>>);

constexpr bool test() {
  { E e; if (!e.has_value() || *e != 0) return false; }
  { E e(5); if (!e || *e != 5) return false; }
  { E e = 6; if (*e != 6) return false; }
  { std::expected<Agg, int> e({1, 2}); if (e->b != 2) return false; }  // U defaults to T
  { E e(std::in_place, 7); if (*e != 7) return false; }
  { E e(std::in_place); if (*e != 0) return false; }
  { std::expected<Agg, int> e(std::in_place, 1, 2); if (e->a != 1) return false; }
  { std::expected<IL, int> e(std::in_place, {1, 2, 3}, 4); if (e->sum != 6 || e->extra != 4) return false; }
  { E e(std::unexpect, 8L); if (e.has_value() || e.error() != 8) return false; }
  { E e(std::unexpect); if (e || e.error() != 0) return false; }
  { std::expected<int, IL> e(std::unexpect, {4, 5}, 1); if (e.error().sum != 9) return false; }
  { E e = std::unexpected(9); if (e || e.error() != 9) return false; }
  { const std::unexpected<int> u(10); E e(u); if (e.error() != 10) return false; }
  { std::expected<int, Explicit> e(std::unexpected<int>(11)); if (e.error().v != 11) return false; }
  // copy / move preserve state
  { E a(1), b(a); if (*b != 1) return false; }
  { E a(std::unexpect, 2), b(a); if (b || b.error() != 2) return false; }
  { std::expected<MoveOnly, int> a(std::in_place, 3), b(std::move(a));
    if (!a.has_value() || b->v != 3 || a->v != -1) return false; }
  { std::expected<int, MoveOnly> a(std::unexpect, 4), b(std::move(a));
    if (a.has_value() || b.error().v != 4 || a.error().v != -1) return false; }
  { std::expected<FromUnex, int> e(std::unexpected<int>(1)); if (e.has_value() || e.error() != 1) return false; }
  // converting from expected<U, G>
  { std::expected<short, int> s(12); E e(s); if (*e != 12) return false; }
  { std::expected<short, int> s(std::unexpect, 13); E e(s); if (e || e.error() != 13) return false; }
  { std::expected<int, int> s(14); std::expected<Explicit, Explicit> e(std::move(s)); if (e->v != 14) return false; }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
