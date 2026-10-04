// [expected.bad]: bad_expected_access<E>'s constructor, error() and what() are constexpr
// (what() "during constant evaluation is encoded with the ordinary literal encoding").
#include <expected>
#include "check.hpp"

constexpr bool test() {
  std::bad_expected_access<int> b(3);
  if (b.error() != 3) return false;
  return b.what() != nullptr;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
