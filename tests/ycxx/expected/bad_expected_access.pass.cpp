// [expected.bad]: bad_expected_access<E> derives from bad_expected_access<void>, explicit
// constructor from E, error() for all value categories (noexcept), what() noexcept.
// [expected.bad.void]: bad_expected_access<void> derives from exception; its special members
// are protected.
#include <expected>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"

using B = std::bad_expected_access<int>;
static_assert(std::is_base_of_v<std::bad_expected_access<void>, B>);
static_assert(std::is_base_of_v<std::exception, std::bad_expected_access<void>>);
static_assert(std::is_convertible_v<B*, std::exception*>);
static_assert(std::is_constructible_v<B, int>);
static_assert(!std::is_convertible_v<int, B>);
static_assert(!std::is_default_constructible_v<B>);
static_assert(std::is_same_v<decltype(std::declval<B&>().error()), int&>);
static_assert(std::is_same_v<decltype(std::declval<B&&>().error()), int&&>);
static_assert(std::is_same_v<decltype(std::declval<const B&>().error()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const B&&>().error()), const int&&>);
static_assert(noexcept(std::declval<B&>().error()));
static_assert(noexcept(std::declval<const B&>().what()));
// protected special members of bad_expected_access<void>
static_assert(!std::is_default_constructible_v<std::bad_expected_access<void>>);
static_assert(!std::is_copy_constructible_v<std::bad_expected_access<void>>);
static_assert(!std::is_move_constructible_v<std::bad_expected_access<void>>);
static_assert(!std::is_copy_assignable_v<std::bad_expected_access<void>>);
static_assert(std::is_copy_constructible_v<B>);
struct Derived : std::bad_expected_access<void> {
  Derived() = default;  // accessible from a derived class
};
static_assert(std::is_default_constructible_v<Derived>);
static_assert(std::is_nothrow_default_constructible_v<Derived>);

int main() {
  B b(5);
  CHECK(b.error() == 5);
  b.error() = 6;
  CHECK(std::as_const(b).error() == 6);
  CHECK(b.what() != nullptr);
  const std::exception& e = b;
  CHECK(e.what() != nullptr);
  bool caught = false;
  try { throw B(7); } catch (const std::bad_expected_access<void>& base) { caught = base.what() != nullptr; }
  CHECK(caught);
  return 0;
}
