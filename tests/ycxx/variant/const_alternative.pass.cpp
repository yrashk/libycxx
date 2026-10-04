// Alternatives may be cv-qualified ([variant.variant.general]/2 requires only Cpp17Destructible,
// non-array, non-reference, non-void). Then:
// [variant.assign]/5: copy assignment "is defined as deleted unless is_copy_constructible_v<Ti>
// && is_copy_assignable_v<Ti> is true for all i"; /7: move assignment constrained on
// is_move_assignable_v<Ti>; /12.2: operator=(T&&) constrained on is_assignable_v<Tj&, T>.
// [variant.specalg]/1: swap constrained on is_move_constructible_v<Ti> && is_swappable_v<Ti>.
// [variant.helper]/3: variant_alternative<I, const T> is add_const_t<...>.
// [variant.ctor]/14-16: the converting constructor selects FUN(const int) for an int argument.
// emplace (destroy + construct), copy/move construction, get, visit, comparison and hash work.
#include <variant>
#include <string>
#include <type_traits>
#include <functional>
#include "check.hpp"

using V = std::variant<const int, std::string>;
static_assert(!std::is_copy_assignable_v<V>);
static_assert(!std::is_move_assignable_v<V>);
static_assert(std::is_copy_constructible_v<V> && std::is_move_constructible_v<V>);
static_assert(!std::is_assignable_v<V&, int>);
static_assert(std::is_assignable_v<V&, std::string>);  // Tj = string is assignable
static_assert(!std::is_swappable_v<V>);
static_assert(std::is_same_v<std::variant_alternative_t<0, V>, const int>);
static_assert(std::is_same_v<std::variant_alternative_t<0, const V>, const int>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<V&>())), const int&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<V&&>())), const int&&>);
static_assert(std::is_same_v<decltype(std::get_if<const int>(std::declval<V*>())), const int*>);
static_assert(std::is_constructible_v<V, int>);

int main() {
  V v(4);
  CHECK(v.index() == 0 && std::get<0>(v) == 4);
  CHECK(std::holds_alternative<const int>(v) && std::get<const int>(v) == 4);
  v.emplace<1>("abc");
  CHECK(std::get<1>(v) == "abc");
  v.emplace<const int>(5);
  CHECK(std::get<0>(v) == 5);
  V w = v;
  V m = std::move(w);
  CHECK(std::get<0>(m) == 5);
  CHECK(v == m && !(v < m) && (v <=> m) == 0);
  int r = std::visit([](auto& x) { return std::is_const_v<std::remove_reference_t<decltype(x)>> ? 1 : 0; }, v);
  CHECK(r == 1);
  CHECK(std::hash<V>{}(v) == std::hash<V>{}(m));
  v = std::string("xy");  // (13.2): emplace<1>, destroying the const int
  CHECK(v.index() == 1 && std::get<1>(v) == "xy");
  v = std::string("z");   // (13.1): assigns to the string
  CHECK(std::get<1>(v) == "z");
  return 0;
}
