// [any.class.general], [any.cons]/1, [any.observers]: constexpr any() noexcept;
// "Postconditions: has_value() is false." type() returns typeid(void) when there is no
// contained value. The default constructor is constexpr, so constinit works.
#include <any>
#include <typeinfo>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::any>);
static_assert(noexcept(std::any()));
static_assert(noexcept(std::declval<const std::any&>().has_value()));
static_assert(noexcept(std::declval<const std::any&>().type()));
static_assert(std::is_same_v<decltype(std::declval<const std::any&>().type()), const std::type_info&>);
static_assert(std::is_same_v<decltype(std::declval<const std::any&>().has_value()), bool>);

constinit std::any global;

int main() {
  CHECK(!global.has_value());
  CHECK(global.type() == typeid(void));
  std::any a;
  CHECK(!a.has_value());
  CHECK(a.type() == typeid(void));
  std::any b{};
  CHECK(!b.has_value());
  return 0;
}
