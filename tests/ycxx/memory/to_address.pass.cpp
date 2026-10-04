// [pointer.conversion]: template<class T> constexpr T* to_address(T* p) noexcept; "Mandates: T
// is not a function type." "Returns: p." template<class Ptr> constexpr auto to_address(const
// Ptr& p) noexcept; "Returns: pointer_traits<Ptr>::to_address(p) if that expression is
// well-formed, otherwise to_address(p.operator->())."
#include <memory>
#include <type_traits>
#include "check.hpp"

template <class T>
struct Arrow {
  T* p;
  constexpr T* operator->() const { return p; }
};
// operator-> returns another fancy pointer: recursion
template <class T>
struct Outer {
  Arrow<T> a;
  constexpr Arrow<T> operator->() const { return a; }
};
// customised through pointer_traits::to_address (no operator-> at all)
struct Custom {
  int* p;
  using element_type = int;
};
template <>
struct std::pointer_traits<Custom> {
  using pointer = Custom;
  using element_type = int;
  using difference_type = std::ptrdiff_t;
  static constexpr int* to_address(Custom c) noexcept { return c.p + 1; }
};
// both: pointer_traits::to_address wins
struct Both {
  int* p;
  using element_type = int;
  constexpr int* operator->() const { return p; }
};
template <>
struct std::pointer_traits<Both> {
  using pointer = Both;
  using element_type = int;
  using difference_type = std::ptrdiff_t;
  static constexpr int* to_address(Both b) noexcept { return b.p + 2; }
};

static_assert(noexcept(std::to_address(static_cast<int*>(nullptr))));
static_assert(noexcept(std::to_address(Arrow<int>{})));
static_assert(std::is_same_v<decltype(std::to_address(Outer<const int>{})), const int*>);
static_assert(std::is_same_v<decltype(std::to_address(static_cast<void*>(nullptr))), void*>);

constexpr bool test() {
  int a[4] = {};
  if (std::to_address(a + 1) != a + 1) return false;
  if (std::to_address(Arrow<int>{a}) != a) return false;
  if (std::to_address(Outer<int>{{a + 2}}) != a + 2) return false;
  if (std::to_address(Custom{a}) != a + 1) return false;
  if (std::to_address(Both{a}) != a + 2) return false;
  int* null = nullptr;
  if (std::to_address(null) != nullptr) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
