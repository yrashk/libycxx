// [expr.typeid]/6: "If the type of the type-id is a reference to a possibly cv-qualified
// type, the result ... refers to a std::type_info object representing the cv-unqualified
// referenced type." /7: "If the type of the expression or type-id is a cv-qualified type, the
// result ... represents the cv-unqualified type." Example 1: typeid(d1) == typeid(d2),
// typeid(D) == typeid(const D), typeid(D) == typeid(d2), typeid(D) == typeid(const D&).
// Only top-level cv is removed: int* and const int* are different types. /5: array-to-pointer
// and function-to-pointer conversions are not applied to the expression.
#include <typeinfo>
#include "check.hpp"

class D {};
struct P {
  virtual ~P() = default;
};
int fn(int) { return 0; }

int main() {
  D d1;
  const D d2;
  CHECK(typeid(d1) == typeid(d2));
  CHECK(typeid(D) == typeid(const D));
  CHECK(typeid(D) == typeid(d2));
  CHECK(typeid(D) == typeid(const D&));
  CHECK(typeid(D) == typeid(volatile D&&));

  const P cp;
  CHECK(typeid(cp) == typeid(P));
  const volatile P& cvr = cp;
  CHECK(typeid(cvr) == typeid(P));

  CHECK(typeid(int* const) == typeid(int*));
  CHECK(typeid(const int*) != typeid(int*));
  CHECK(typeid(const int* const) == typeid(const int*));
  CHECK(typeid(int**) != typeid(int* const*));
  CHECK(typeid(int D::*) != typeid(const int D::*));

  int arr[3] = {};
  CHECK(typeid(arr) == typeid(int[3]));
  CHECK(typeid(arr) != typeid(int*));
  CHECK(typeid(int[3]) != typeid(int[4]));
  CHECK(typeid(int[]) != typeid(int[3]));
  CHECK(typeid(fn) == typeid(int(int)));
  CHECK(typeid(fn) != typeid(int (*)(int)));
  CHECK(typeid(int(int)) != typeid(int(int) noexcept));
  CHECK(typeid(int (*)(int)) != typeid(int (*)(int) noexcept));

  // fundamental types are pairwise distinct
  CHECK(typeid(char) != typeid(signed char));
  CHECK(typeid(char) != typeid(unsigned char));
  CHECK(typeid(int) != typeid(long));
  CHECK(typeid(long) != typeid(long long));
  CHECK(typeid(char8_t) != typeid(unsigned char));
  CHECK(typeid(wchar_t) != typeid(int));
  CHECK(typeid(decltype(nullptr)) != typeid(void*));
  CHECK(typeid(void) == typeid(const void));
  return 0;
}
