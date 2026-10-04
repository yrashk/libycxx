// [variant.ctor]/20-39: in_place_type_t / in_place_index_t constructors, with and without
// initializer_list. Constraints: exactly one occurrence of T for in_place_type;
// I < sizeof...(Types) and is_constructible for in_place_index. Constructors are explicit.
#include <variant>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

struct IL {
  int sum = 0;
  int extra = 0;
  constexpr IL(std::initializer_list<int> il, int e = 0) : extra(e) { for (int x : il) sum += x; }
};
struct Two { int a, b; constexpr Two(int x, int y) : a(x), b(y) {} };

constexpr bool test() {
  { std::variant<int, Two> v(std::in_place_type<Two>, 1, 2);
    if (v.index() != 1 || std::get<1>(v).a != 1 || std::get<1>(v).b != 2) return false; }
  { std::variant<int, Two> v(std::in_place_index<1>, 3, 4);
    if (std::get<Two>(v).b != 4) return false; }
  { std::variant<int, int> v(std::in_place_index<1>, 9);  // duplicates are fine by index
    if (v.index() != 1 || std::get<1>(v) != 9) return false; }
  { std::variant<int, IL> v(std::in_place_type<IL>, {1, 2, 3});
    if (std::get<1>(v).sum != 6) return false; }
  { std::variant<int, IL> v(std::in_place_index<1>, {1, 2, 3}, 7);
    if (std::get<1>(v).sum != 6 || std::get<1>(v).extra != 7) return false; }
  { std::variant<int, long> v(std::in_place_type<long>);  // value-init
    if (v.index() != 1 || std::get<1>(v) != 0) return false; }
  return true;
}
static_assert(test());

using V = std::variant<int, int, Two>;
static_assert(!std::is_constructible_v<V, std::in_place_type_t<int>, int>);   // T occurs twice
static_assert(std::is_constructible_v<V, std::in_place_index_t<0>, int>);
static_assert(std::is_constructible_v<V, std::in_place_type_t<Two>, int, int>);
static_assert(!std::is_constructible_v<V, std::in_place_type_t<Two>, int>);   // not constructible
static_assert(!std::is_constructible_v<V, std::in_place_type_t<long>, int>);  // not an alternative
static_assert(!std::is_constructible_v<V, std::in_place_index_t<3>>);         // out of range
static_assert(!std::is_constructible_v<V, std::in_place_index_t<2>, int>);
// explicit
static_assert(!std::is_convertible_v<std::in_place_index_t<0>, std::variant<int>>);

int main() {
  CHECK(test());
  return 0;
}
