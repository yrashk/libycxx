// [optional.specalg]/3-6: make_optional(T&&) returns optional<decay_t<T>> and is constrained to
// calls that do *not* use an explicit template-argument-list beginning with a type argument;
// make_optional<T>(args...) is optional<T>(in_place, args...), make_optional<T>(il, args...).
// COUNTERPART: libcxx:utilities/optional/optional.specalg/make_optional(_explicit|_explicit_initializer_list)?.pass.cpp
#include <optional>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

struct IL {
  int sum = 0, extra = 0;
  constexpr IL(std::initializer_list<int> il, int e) : extra(e) { for (int x : il) sum += x; }
};
struct Two { int a, b; constexpr Two(int x, int y) : a(x), b(y) {} };
struct ExplicitCopy {
  int v;
  constexpr explicit ExplicitCopy(int x) : v(x) {}
};

constexpr bool test() {
  int i = 3;
  const int ci = 4;
  static_assert(std::is_same_v<decltype(std::make_optional(i)), std::optional<int>>);
  static_assert(std::is_same_v<decltype(std::make_optional(ci)), std::optional<int>>);
  static_assert(std::is_same_v<decltype(std::make_optional("abc")), std::optional<const char*>>);
  if (*std::make_optional(i) != 3) return false;

  // Explicit template argument: always the in_place form.
  static_assert(std::is_same_v<decltype(std::make_optional<int&>(i)), std::optional<int&>>);
  auto r = std::make_optional<int&>(i);
  if (&*r != &i) return false;
  static_assert(std::is_same_v<decltype(std::make_optional<const int>(1)), std::optional<const int>>);
  static_assert(std::is_same_v<decltype(std::make_optional<long>(i)), std::optional<long>>);
  if (*std::make_optional<long>(i) != 3) return false;
  auto t = std::make_optional<Two>(1, 2);
  if (t->a != 1 || t->b != 2) return false;
  auto il = std::make_optional<IL>({1, 2, 3}, 4);
  if (il->sum != 6 || il->extra != 4) return false;
  // in_place form direct-initializes, so an explicit constructor is fine
  auto ex = std::make_optional<ExplicitCopy>(5);
  if (ex->v != 5) return false;
  auto z = std::make_optional<int>();
  if (!z || *z != 0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
