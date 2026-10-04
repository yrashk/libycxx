// [expected.object.general]/2: "A type T is a valid value type for expected, if remove_cv_t<T>
// is void or a complete non-array object type that is not in_place_t, unexpect_t, or a
// specialization of unexpected" -- so expected<const int, E> is valid.
// [expected.object.assign]/4: copy assignment "is defined as deleted unless:
// is_copy_assignable_v<T> is true and ..."; /6: move assignment is constrained on
// is_move_assignable_v<T>; /11.4: operator=(U&&) is constrained on is_assignable_v<T&, U>.
// All false for T = const int. operator=(const unexpected<G>&) (/14-16) only needs E to be
// constructible and assignable: when *this holds a value it reinit-expected(unex, val, ...),
// which destroys and re-constructs, so it is available. emplace (/22) constructs.
// [expected.object.swap]/1: constrained on is_swappable_v<T>.
#include <expected>
#include <type_traits>
#include <utility>
#include "check.hpp"

using E = std::expected<const int, int>;
static_assert(std::is_copy_constructible_v<E> && std::is_move_constructible_v<E>);
static_assert(!std::is_copy_assignable_v<E>);
static_assert(!std::is_move_assignable_v<E>);
static_assert(!std::is_assignable_v<E&, int>);
static_assert(!std::is_assignable_v<E&, const int&>);
static_assert(std::is_assignable_v<E&, std::unexpected<int>>);
static_assert(std::is_assignable_v<E&, const std::unexpected<int>&>);
static_assert(!std::is_swappable_v<E>);
static_assert(std::is_same_v<E::value_type, const int>);
static_assert(std::is_same_v<decltype(*std::declval<E&>()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<E&>().value_or(0)), int>);  // remove_cv_t<T>

int main() {
  E e(3);
  CHECK(e.has_value() && *e == 3);
  CHECK(e.emplace(7) == 7 && *e == 7);
  E copy = e;
  CHECK(*copy == 7);
  e = std::unexpected(5);  // reinit-expected(unex, val, ...)
  CHECK(!e.has_value() && e.error() == 5);
  e = std::unexpected(6);  // unex = ...
  CHECK(e.error() == 6);
  CHECK(e.emplace(8) == 8 && *e == 8);  // destroys unex, constructs the value
  CHECK(e.value_or(0) == 8);
  auto t = e.transform([](const int& x) -> const int { return x + 1; });
  static_assert(std::is_same_v<decltype(t), std::expected<int, int>>);  // remove_cv_t
  CHECK(*t == 9);
  CHECK(e == 8 && e != 9);
  E u(std::unexpect, 1);
  CHECK(u.value_or(42) == 42 && u == std::unexpected(1));
  return 0;
}
