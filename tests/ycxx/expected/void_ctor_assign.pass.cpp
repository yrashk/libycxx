// [expected.void.general], [expected.void.cons], [expected.void.assign], [expected.void.dtor]:
// expected<cv void, E> -- default and in_place constructors noexcept (has_value), unexpect
// (+il), from unexpected<G> (explicit(bool)), converting from expected<U,G> with void U,
// copy/move (deleted/trivial/noexcept rules), assignments in all states, emplace() noexcept.
#include <expected>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Explicit { int v; constexpr explicit Explicit(int x) : v(x) {} };
struct IL {
  int sum = 0;
  constexpr IL(std::initializer_list<int> il, int) { for (int x : il) sum += x; }
};
struct MoveOnly {
  int v;
  constexpr MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
  constexpr MoveOnly& operator=(MoveOnly&& o) noexcept { v = o.v; o.v = -1; return *this; }
};
struct NTDtor { ~NTDtor() {} };
struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
  ThrowingMove& operator=(ThrowingMove&&) noexcept(false) { return *this; }
};

using V = std::expected<void, long>;
static_assert(std::is_same_v<V::value_type, void>);
static_assert(std::is_same_v<V::rebind<int>, std::expected<int, long>>);
static_assert(std::is_same_v<std::expected<const void, long>::value_type, const void>);
static_assert(std::is_nothrow_default_constructible_v<V>);
static_assert(std::is_nothrow_constructible_v<V, std::in_place_t>);
static_assert(!std::is_convertible_v<std::in_place_t, V>);
static_assert(!std::is_convertible_v<std::unexpect_t, V>);
static_assert(std::is_convertible_v<std::unexpected<int>, V>);
static_assert(!std::is_convertible_v<std::unexpected<int>, std::expected<void, Explicit>>);
static_assert(std::is_constructible_v<std::expected<void, Explicit>, std::unexpected<int>>);
static_assert(std::is_convertible_v<std::expected<void, int>, V>);
static_assert(std::is_constructible_v<V, std::expected<const void, int>>);
static_assert(!std::is_constructible_v<V, std::expected<int, long>>);  // U must be void
static_assert(!std::is_convertible_v<std::expected<void, int>, std::expected<void, Explicit>>);
// triviality / deletion
static_assert(std::is_trivially_copyable_v<V>);
static_assert(std::is_trivially_copy_assignable_v<V> && std::is_trivially_move_assignable_v<V>);
static_assert(!std::is_trivially_destructible_v<std::expected<void, NTDtor>>);
static_assert(!std::is_copy_constructible_v<std::expected<void, MoveOnly>>);
static_assert(std::is_move_constructible_v<std::expected<void, MoveOnly>>);
static_assert(!std::is_copy_assignable_v<std::expected<void, MoveOnly>>);
static_assert(std::is_nothrow_move_assignable_v<std::expected<void, MoveOnly>>);
static_assert(!std::is_nothrow_move_constructible_v<std::expected<void, ThrowingMove>>);
static_assert(!std::is_nothrow_move_assignable_v<std::expected<void, ThrowingMove>>);
static_assert(noexcept(std::declval<V&>().emplace()));

constexpr bool test() {
  { V v; if (!v.has_value() || v.has_error() || !v) return false; }
  { V v(std::in_place); if (!v) return false; }
  { V v(std::unexpect, 3L); if (v || v.error() != 3) return false; }
  { V v(std::unexpect); if (v || v.error() != 0) return false; }
  { std::expected<void, IL> v(std::unexpect, {1, 2}, 0); if (v.error().sum != 3) return false; }
  { V v = std::unexpected(4); if (v.error() != 4) return false; }
  { std::expected<void, Explicit> v(std::unexpected(5)); if (v.error().v != 5) return false; }
  { std::expected<void, int> s(std::unexpect, 6); V v(s); if (v || v.error() != 6) return false; }
  { std::expected<void, int> s; V v(std::move(s)); if (!v) return false; }
  { std::expected<void, MoveOnly> a(std::unexpect, 7), b(std::move(a));
    if (b.error().v != 7 || a.has_value() || a.error().v != -1) return false; }
  // assignment, all states
  V a, b(std::unexpect, 1L);
  a = b;  // value <- error
  if (a || a.error() != 1) return false;
  a = V();  // error <- value
  if (!a) return false;
  a = V();  // value <- value
  if (!a) return false;
  a = b;
  a = V(std::unexpect, 2L);  // error <- error
  if (a.error() != 2) return false;
  a = std::unexpected(3);  // unexpected<G> on error
  if (a.error() != 3) return false;
  a.emplace();
  if (!a) return false;
  a = std::unexpected(4L);  // unexpected<G> on value
  if (a || a.error() != 4) return false;
  a.emplace();
  a.emplace();  // emplace on value: no effect
  if (!a) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
