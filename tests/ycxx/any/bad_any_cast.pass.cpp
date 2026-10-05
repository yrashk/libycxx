// [any.bad.any.cast]: "class bad_any_cast : public bad_cast" with
// "const char* what() const noexcept override;" returning an implementation-defined ntbs.
// "Objects of type bad_any_cast are thrown by a failed any_cast." Special members per [exception].
// REQUIRES: exceptions
#include <any>
#include <typeinfo>
#include <exception>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_base_of_v<std::bad_cast, std::bad_any_cast>);
static_assert(std::is_convertible_v<std::bad_any_cast*, std::bad_cast*>);
static_assert(std::is_convertible_v<std::bad_any_cast*, std::exception*>);
static_assert(std::is_nothrow_default_constructible_v<std::bad_any_cast>);
static_assert(std::is_nothrow_copy_constructible_v<std::bad_any_cast>);
static_assert(std::is_nothrow_copy_assignable_v<std::bad_any_cast>);
static_assert(std::has_virtual_destructor_v<std::bad_any_cast>);
static_assert(noexcept(std::declval<const std::bad_any_cast&>().what()));
static_assert(std::is_same_v<decltype(std::declval<const std::bad_any_cast&>().what()), const char*>);

int main() {
  std::bad_any_cast e;
  CHECK(e.what() != nullptr);
  std::bad_any_cast copy(e);
  CHECK(copy.what() != nullptr);
  const std::exception& base = e;
  CHECK(base.what() != nullptr);
  bool caught = false;
  try {
    std::any a = 1;
    (void)std::any_cast<double>(a);
  } catch (const std::bad_any_cast& ex) {
    caught = ex.what() != nullptr;
  }
  CHECK(caught);
  return 0;
}
