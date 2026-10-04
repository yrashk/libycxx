// [bitset.hash]: "template<size_t N> struct hash<bitset<N>>; The specialization is enabled
// ([unord.hash])." Equal bitsets hash equal ([unord.hash]/5).
#include <bitset>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

template <class T>
constexpr bool enabled = std::is_default_constructible_v<std::hash<T>> && std::is_copy_constructible_v<std::hash<T>> &&
                         std::is_move_assignable_v<std::hash<T>> &&
                         std::is_same_v<decltype(std::hash<T>{}(std::declval<const T&>())), std::size_t>;

static_assert(enabled<std::bitset<0>>);
static_assert(enabled<std::bitset<1>>);
static_assert(enabled<std::bitset<64>>);
static_assert(enabled<std::bitset<1000>>);

int main() {
  std::hash<std::bitset<100>> h;
  std::bitset<100> a, b;
  a.set(3).set(99);
  b.set(99).set(3);
  CHECK(h(a) == h(b));
  const std::bitset<100>& ca = a;
  CHECK(h(ca) == h(a));
  std::bitset<8> x(5), y(5);
  CHECK(std::hash<std::bitset<8>>{}(x) == std::hash<std::bitset<8>>{}(y));
  return 0;
}
