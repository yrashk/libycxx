// [variant.bad.access]: bad_variant_access publicly derives from exception; what() is
// constexpr, noexcept, overrides exception::what, returns an ntbs. Special members per
// [exception] (default-constructible, copyable, nothrow copy).
#include <variant>
#include <exception>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_base_of_v<std::exception, std::bad_variant_access>);
static_assert(std::is_convertible_v<std::bad_variant_access*, std::exception*>);
static_assert(std::is_nothrow_default_constructible_v<std::bad_variant_access>);
static_assert(std::is_nothrow_copy_constructible_v<std::bad_variant_access>);
static_assert(std::is_nothrow_copy_assignable_v<std::bad_variant_access>);
static_assert(std::has_virtual_destructor_v<std::bad_variant_access>);
static_assert(noexcept(std::declval<const std::bad_variant_access&>().what()));
static_assert(std::is_same_v<decltype(std::declval<const std::bad_variant_access&>().what()), const char*>);

constexpr bool constexpr_what() {
  std::bad_variant_access e;
  const char* w = e.what();
  return w != nullptr;
}
static_assert(constexpr_what());

int main() {
  std::bad_variant_access e;
  const std::exception& base = e;
  CHECK(base.what() != nullptr);
  CHECK(base.what() == e.what() || base.what()[0] == e.what()[0]);
  return 0;
}
