// [optional.ctor]: default/nullopt (noexcept, constexpr, empty), copy/move, in_place (+il),
// converting U&& (explicit(!is_convertible_v<U,T>), default U = remove_cv_t<T>), converting
// from optional<U> (explicit(bool), converts-from-any-cvref exclusion, rhs.has_value unchanged).
// [optional.optional.general]: deduction guide optional(T) -> optional<T>.
#include <optional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct ExplicitFromInt { int v; constexpr explicit ExplicitFromInt(int x) : v(x) {} };
struct ImplicitFromInt { int v; constexpr ImplicitFromInt(int x) : v(x) {} };
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

static_assert(std::is_nothrow_default_constructible_v<std::optional<ThrowingMove>>);
static_assert(std::is_nothrow_constructible_v<std::optional<ThrowingMove>, std::nullopt_t>);
static_assert(std::is_convertible_v<std::nullopt_t, std::optional<int>>);
static_assert(std::is_nothrow_move_constructible_v<std::optional<MoveOnly>>);
static_assert(!std::is_nothrow_move_constructible_v<std::optional<ThrowingMove>>);
static_assert(!std::is_copy_constructible_v<std::optional<MoveOnly>>);
// in_place ctors are explicit
static_assert(!std::is_convertible_v<std::in_place_t, std::optional<int>>);
static_assert(std::is_constructible_v<std::optional<int>, std::in_place_t>);
// explicit(bool) on the U&& constructor
static_assert(std::is_constructible_v<std::optional<ExplicitFromInt>, int>);
static_assert(!std::is_convertible_v<int, std::optional<ExplicitFromInt>>);
static_assert(std::is_convertible_v<int, std::optional<ImplicitFromInt>>);
// explicit(bool) on optional<U> conversions
static_assert(std::is_constructible_v<std::optional<ExplicitFromInt>, const std::optional<int>&>);
static_assert(!std::is_convertible_v<const std::optional<int>&, std::optional<ExplicitFromInt>>);
static_assert(std::is_convertible_v<const std::optional<int>&, std::optional<ImplicitFromInt>>);
static_assert(std::is_convertible_v<std::optional<int>&&, std::optional<long>>);
static_assert(!std::is_constructible_v<std::optional<int*>, std::optional<long>>);
// U&& constraints
static_assert(!std::is_constructible_v<std::optional<int>, int*>);
// deduction guide
static_assert(std::is_same_v<decltype(std::optional(1)), std::optional<int>>);
static_assert(std::is_same_v<decltype(std::optional(std::optional(1))), std::optional<int>>);  // copy deduction

constexpr bool test() {
  { std::optional<int> o; if (o.has_value() || o) return false; }
  { std::optional<int> o(std::nullopt); if (o.has_value()) return false; }
  { std::optional<int> o = std::nullopt; if (o.has_value()) return false; }
  { std::optional<int> o(std::in_place); if (!o || *o != 0) return false; }
  { std::optional<Agg> o(std::in_place, 1, 2); if (o->a != 1 || o->b != 2) return false; }  // paren-agg-init
  { std::optional<IL> o(std::in_place, {1, 2, 3}, 4); if (o->sum != 6 || o->extra != 4) return false; }
  { std::optional<int> o(5); if (*o != 5) return false; }
  { std::optional<ExplicitFromInt> o(5); if (o->v != 5) return false; }
  { std::optional<ImplicitFromInt> o = 6; if (o->v != 6) return false; }
  // braced default argument: U defaults to remove_cv_t<T>
  { std::optional<Agg> o({1, 2}); if (!o || o->b != 2) return false; }
  { std::optional<const int> o({}); if (!o || *o != 0) return false; }
  // copy / move
  { std::optional<int> a(3), b(a); if (*b != 3 || !a) return false; }
  { std::optional<int> a, b(a); if (b) return false; }
  { std::optional<MoveOnly> a(std::in_place, 4), b(std::move(a));
    if (!a.has_value() || b->v != 4 || a->v != -1) return false; }  // rhs.has_value() unchanged
  // converting from optional<U>
  { std::optional<int> a(7); std::optional<long> b(a); if (*b != 7) return false; }
  { std::optional<int> a; std::optional<long> b(a); if (b) return false; }
  { std::optional<int> a(8); std::optional<ExplicitFromInt> b(std::move(a)); if (b->v != 8 || !a) return false; }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
