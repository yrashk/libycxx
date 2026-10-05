// [variant.hash]/1: hash<variant<Types...>> is enabled iff every hash<remove_const_t<Types>>
// is enabled. [unord.hash]/4: a disabled specialization is not default/copy/move
// constructible or assignable. [unord.hash]/2: <variant> declares hash, so hash of arithmetic
// types is enabled through it. [unord.hash]/5.2: equal keys hash equal.
// COUNTERPART: libstdcxx:20_util/variant/hash.cc
#include <variant>
#include <functional>
#include <type_traits>
#include "check.hpp"

struct NoHash {};
struct Hashed { int v; bool operator==(const Hashed&) const = default; };
template <> struct std::hash<Hashed> {
  std::size_t operator()(const Hashed& h) const noexcept { return static_cast<std::size_t>(h.v); }
};

template <class T>
constexpr bool enabled = std::is_default_constructible_v<std::hash<T>> &&
                         std::is_copy_constructible_v<std::hash<T>> &&
                         std::is_move_constructible_v<std::hash<T>> &&
                         std::is_copy_assignable_v<std::hash<T>> &&
                         std::is_move_assignable_v<std::hash<T>>;
template <class T>
constexpr bool disabled = !std::is_default_constructible_v<std::hash<T>> &&
                          !std::is_copy_constructible_v<std::hash<T>> &&
                          !std::is_move_constructible_v<std::hash<T>> &&
                          !std::is_copy_assignable_v<std::hash<T>> &&
                          !std::is_move_assignable_v<std::hash<T>>;

static_assert(enabled<std::variant<int, double>>);
static_assert(enabled<std::variant<int, Hashed>>);
static_assert(enabled<std::variant<const int, std::monostate>>);  // remove_const_t
static_assert(enabled<std::variant<const Hashed>>);
static_assert(disabled<std::variant<int, NoHash>>);
static_assert(disabled<std::variant<NoHash>>);
static_assert(std::is_invocable_r_v<std::size_t, std::hash<std::variant<int, Hashed>>, const std::variant<int, Hashed>&>);
static_assert(!std::is_invocable_v<std::hash<std::variant<int, NoHash>>, const std::variant<int, NoHash>&>);

int main() {
  using V = std::variant<int, Hashed>;
  std::hash<V> h;
  V a(1), b(1), c(std::in_place_index<1>, Hashed{3}), d(std::in_place_index<1>, Hashed{3});
  CHECK(h(a) == h(b));
  CHECK(h(c) == h(d));
  std::variant<int, int> e(std::in_place_index<0>, 5), f(std::in_place_index<0>, 5);
  CHECK(std::hash<std::variant<int, int>>{}(e) == std::hash<std::variant<int, int>>{}(f));
  std::variant<const int> ci(4);
  (void)std::hash<std::variant<const int>>{}(ci);
  return 0;
}
