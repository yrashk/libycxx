// [atomics.ref.ops]/37: "constexpr address-return-type address() const noexcept; Returns:
// ptr." where address-return-type is COPYCV(T, void)*.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <type_traits>
#include "check.hpp"

int main() {
  alignas(std::atomic_ref<int>::required_alignment) int x = 0;
  std::atomic_ref<int> r(x);
  static_assert(std::is_same_v<decltype(r.address()), void*>);
  static_assert(noexcept(r.address()));
  CHECK(r.address() == &x);
  std::atomic_ref<const int> cr(r);
  static_assert(std::is_same_v<decltype(cr.address()), const void*>);
  CHECK(cr.address() == &x);

  alignas(std::atomic_ref<double>::required_alignment) double d = 0;
  CHECK(std::atomic_ref<double>(d).address() == &d);
  int* p = nullptr;
  alignas(std::atomic_ref<int*>::required_alignment) int* q = p;
  CHECK(std::atomic_ref<int*>(q).address() == &q);
  struct S { int a; };
  alignas(std::atomic_ref<S>::required_alignment) S s{};
  CHECK(std::atomic_ref<S>(s).address() == &s);
  return 0;
}
