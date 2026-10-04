// [any.nonmembers]/9-10: const T* any_cast(const any*) noexcept; T* any_cast(any*) noexcept.
// "Returns: If operand != nullptr && operand->type() == typeid(T) is true, a pointer to the
// object contained by operand; otherwise, nullptr."
#include <any>
#include <type_traits>
#include "check.hpp"

struct S { int v; };

static_assert(std::is_same_v<decltype(std::any_cast<int>(static_cast<std::any*>(nullptr))), int*>);
static_assert(std::is_same_v<decltype(std::any_cast<int>(static_cast<const std::any*>(nullptr))), const int*>);
static_assert(std::is_same_v<decltype(std::any_cast<const int>(static_cast<std::any*>(nullptr))), const int*>);
static_assert(noexcept(std::any_cast<int>(static_cast<std::any*>(nullptr))));
static_assert(noexcept(std::any_cast<int>(static_cast<const std::any*>(nullptr))));

int main() {
  std::any* np = nullptr;
  const std::any* cnp = nullptr;
  CHECK(std::any_cast<int>(np) == nullptr);
  CHECK(std::any_cast<int>(cnp) == nullptr);

  std::any a = S{4};
  S* p = std::any_cast<S>(&a);
  CHECK(p != nullptr);
  CHECK(p->v == 4);
  p->v = 5;
  CHECK(std::any_cast<S&>(a).v == 5);
  CHECK(std::any_cast<S>(&a) == p);  // stable address of the contained object

  const std::any& ca = a;
  const S* cp = std::any_cast<S>(&ca);
  CHECK(cp == p);

  CHECK(std::any_cast<int>(&a) == nullptr);
  CHECK(std::any_cast<const S>(&a) == p);  // typeid(const S) == typeid(S)

  std::any empty;
  CHECK(std::any_cast<int>(&empty) == nullptr);

  std::any i = 3;
  CHECK(std::any_cast<long>(&i) == nullptr);
  CHECK(std::any_cast<unsigned>(&i) == nullptr);
  CHECK(*std::any_cast<int>(&i) == 3);
  return 0;
}
