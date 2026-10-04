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

#include "reqs/container_equality.hpp"

using namespace reqs::container_equality;

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
