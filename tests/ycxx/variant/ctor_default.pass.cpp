// [variant.ctor]/2-6: default constructor -- Constraints is_default_constructible_v<T0>,
// value-initializes T0, index() == 0, noexcept(is_nothrow_default_constructible_v<T0>),
// constexpr when value-initialization of T0 is constexpr-suitable.
// [variant.monostate] note: monostate makes a variant default-constructible.
#include <variant>
#include <type_traits>
#include "check.hpp"

struct NoDefault { NoDefault(int) {} };
struct ThrowingDefault { ThrowingDefault() noexcept(false) {} };
struct Counter { int v; constexpr Counter() : v(42) {} };

static_assert(std::is_default_constructible_v<std::variant<int, NoDefault>>);
static_assert(!std::is_default_constructible_v<std::variant<NoDefault, int>>);
static_assert(std::is_default_constructible_v<std::variant<std::monostate, NoDefault>>);
static_assert(std::is_nothrow_default_constructible_v<std::variant<int, ThrowingDefault>>);
static_assert(!std::is_nothrow_default_constructible_v<std::variant<ThrowingDefault, int>>);

constexpr bool test() {
  std::variant<int, double> v;
  if (v.index() != 0 || v.valueless_by_exception()) return false;
  if (std::get<0>(v) != 0) return false;  // value-initialized
  std::variant<Counter, int> c;
  if (std::get<0>(c).v != 42) return false;
  std::variant<std::monostate, NoDefault> m;
  if (!std::holds_alternative<std::monostate>(m)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::variant<ThrowingDefault, int> t;
  CHECK(t.index() == 0);
  return 0;
}
