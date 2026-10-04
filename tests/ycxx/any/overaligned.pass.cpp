// [any.class.general], [any.cons]: the contained value is an object of type VT, so it is
// suitably aligned for VT even for over-aligned types; any_cast<cv T> to a pointer finds the
// value since typeid ignores top-level cv-qualifiers (operand->type() == typeid(T)), and the
// result has type cv T*.
#include <any>
#include <cstdint>
#include <type_traits>
#include "check.hpp"

struct alignas(64) Big {
  int v;
};
struct alignas(32) Small {
  char c;
};

int main() {
  std::any a(Big{7});
  Big* p = std::any_cast<Big>(&a);
  CHECK(p != nullptr && p->v == 7 && reinterpret_cast<std::uintptr_t>(p) % 64 == 0);
  std::any b(Small{'x'});
  Small* q = std::any_cast<Small>(&b);
  CHECK(q != nullptr && q->c == 'x' && reinterpret_cast<std::uintptr_t>(q) % 32 == 0);
  std::any c = b;
  CHECK(reinterpret_cast<std::uintptr_t>(std::any_cast<Small>(&c)) % 32 == 0);
  std::any d = std::move(a);
  CHECK(reinterpret_cast<std::uintptr_t>(std::any_cast<Big>(&d)) % 64 == 0 && std::any_cast<Big&>(d).v == 7);

  std::any i = 5;
  const int* ci = std::any_cast<const int>(&i);
  static_assert(std::is_same_v<decltype(std::any_cast<const int>(&i)), const int*>);
  CHECK(ci != nullptr && *ci == 5);
  CHECK(std::any_cast<const volatile int>(&i) != nullptr);
  return 0;
}
