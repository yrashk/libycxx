// [container.reqmts]/42-47: c == b has type bool and returns equal(c.begin(), c.end(),
// b.begin(), b.end()) — so it compares sizes and then elements using T's ==; == is an
// equivalence relation; c != b is equivalent to !(c == b). Checked with an element type
// whose == is a non-identity equivalence (equal modulo 10), which a container must honour
// rather than compare object representations.
#include <vector>
#include <string>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

struct Mod10 {
  int v;
  constexpr Mod10(int x) : v(x) {}
  friend constexpr bool operator==(const Mod10& a, const Mod10& b) { return a.v % 10 == b.v % 10; }
};

template <class X>
constexpr bool generic() {
  static_assert(std::is_same_v<decltype(std::declval<const X&>() == std::declval<const X&>()), bool>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>() != std::declval<const X&>()), bool>);
  X a = make<X>({1, 2, 3});
  X b = make<X>({1, 2, 3});
  X c = make<X>({1, 2, 3});
  X shorter = make<X>({1, 2});
  X longer = make<X>({1, 2, 3, 4});
  X diff = make<X>({1, 2, 4});
  X e1, e2;
  // reflexive, symmetric, transitive
  if (!(a == a) || !(a == b) || !(b == a) || !(b == c) || !(a == c)) return false;
  if (a == shorter || shorter == a || a == longer || longer == a || e1 == a || a == e1) return false;
  if (!(e1 == e2)) return false;
  if (!(a != shorter) || !(a != longer) || a != b || e1 != e2) return false;
  if constexpr (!std::is_same_v<typename X::value_type, bool>) {
    if (a == diff || !(a != diff)) return false;
  }
  return true;
}

constexpr bool custom_eq() {
  std::vector<Mod10> a{1, 22, 333}, b{11, 2, 3}, c{1, 22, 334};
  if (!(a == b) || !(b == a) || a != b) return false;
  if (a == c || !(a != c)) return false;
  std::vector<Mod10> d{1, 22};
  if (a == d) return false;
  return true;
}

static_assert(generic<std::vector<int>>());
static_assert(generic<std::vector<Elem>>());
static_assert(generic<std::vector<bool>>());
static_assert(generic<std::string>());
static_assert(custom_eq());

int main() {
  CHECK(generic<std::vector<int>>());
  CHECK(generic<std::vector<Elem>>());
  CHECK(generic<std::vector<bool>>());
  CHECK(generic<std::vector<std::string>>());
  CHECK(generic<std::string>());
  CHECK(generic<std::u8string>());
  CHECK(custom_eq());
  return 0;
}
