// [container.reqmts]/10-23: "X u; X u = X();" Postconditions: u.empty(). "X u(v); X u = v;"
// Postconditions: u == v. "X u(rv); X u = rv;" Postconditions: u is equal to the value that
// rv had before this construction. "t = v" Result: X&; Postconditions: t == v. "t = rv"
// Result: X&; Postconditions: if t and rv do not refer to the same object, t is equal to the
// value rv had before. /24-25: a.~X() destroys every element and deallocates any memory
// obtained (checked by evaluating everything in a constant expression, where a leaked
// allocation is ill-formed, using an element type that owns heap memory).
// [utility.arg.requirements] Cpp17CopyAssignable: after t = v "the value of v is unchanged",
// so copy self-assignment keeps the value.
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

#include "reqs/container_construct_assign.hpp"

using namespace reqs::container_construct_assign;

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::vector<std::string>>());
static_assert(test<std::string>());
static_assert(test<std::u32string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(test<std::u32string>());
  return 0;
}
