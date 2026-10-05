// [variant.get]/7 + [variant.bad.access]: get throws bad_variant_access; get is constexpr and
// bad_variant_access has a constexpr what(), so throwing and catching it works during
// constant evaluation (P3068 constexpr exceptions, P3378 constexpr exception types).
// XFAIL-COMPILER: clang  no constexpr exception support (P3068) in clang yet
// REQUIRES: exceptions
#include <variant>
#include "check.hpp"

constexpr bool test() {
  std::variant<int, long> v(1);
  try {
    (void)std::get<1>(v);
  } catch (const std::bad_variant_access& e) {
    return e.what() != nullptr;
  }
  return false;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
