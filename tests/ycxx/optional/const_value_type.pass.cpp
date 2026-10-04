// optional<const T> is valid ([optional.optional.general]/3: T is a complete non-array object
// type other than in_place_t or nullopt_t, cv-qualification allowed), but:
// [optional.assign]/7: copy assignment "is defined as deleted unless
// is_copy_constructible_v<T> is true and is_copy_assignable_v<T> is true";
// /13: move assignment is constrained on is_move_assignable_v<T>;
// /18: operator=(U&&) is constrained on "is_assignable_v<T&, U> is true";
// [optional.specalg]/1: swap is constrained on is_swappable_v<T>.
// emplace and reset destroy/construct, so they work ([optional.assign]/29-32), as does
// operator=(nullopt_t). value_or returns remove_cv_t<T> ([optional.observe]/15).
#include <optional>
#include <type_traits>
#include "check.hpp"

using O = std::optional<const int>;
static_assert(std::is_copy_constructible_v<O> && std::is_move_constructible_v<O>);
static_assert(!std::is_copy_assignable_v<O>);
static_assert(!std::is_move_assignable_v<O>);
static_assert(!std::is_assignable_v<O&, int>);
static_assert(!std::is_assignable_v<O&, std::optional<int>>);
static_assert(std::is_nothrow_assignable_v<O&, std::nullopt_t>);
static_assert(!std::is_swappable_v<O>);
static_assert(std::is_same_v<decltype(std::declval<O&>().value_or(0)), int>);
static_assert(std::is_same_v<decltype(*std::declval<O&>()), const int&>);

int main() {
  O o;
  CHECK(o.emplace(3) == 3 && *o == 3);
  O c = o;
  CHECK(*c == 3);
  o = std::nullopt;
  CHECK(!o);
  CHECK(o.value_or(9) == 9 && c.value_or(9) == 3);
  auto t = c.transform([](const int& x) -> const int { return x * 2; });
  static_assert(std::is_same_v<decltype(t), std::optional<int>>);
  CHECK(*t == 6);
  c.reset();
  CHECK(!c);
  return 0;
}
