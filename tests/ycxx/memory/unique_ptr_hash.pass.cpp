// [util.smartptr.hash]/1: hash<unique_ptr<T, D>> is enabled iff hash<pointer> is enabled,
// and then hash<UP>()(p) == hash<UP::pointer>()(p.get()). /2: hash<shared_ptr<T>>()(p) ==
// hash<shared_ptr<T>::element_type*>()(p.get()).
#include <memory>
#include <cstddef>
#include <functional>
#include <type_traits>
#include "check.hpp"

// A pointer type for which std::hash is not enabled.
struct Handle {
  int v = 0;
  Handle() = default;
  Handle(std::nullptr_t) {}
  friend bool operator==(Handle, Handle) = default;
};
struct HandleDeleter {
  using pointer = Handle;
  void operator()(Handle) const {}
};

static_assert(!std::is_default_constructible_v<std::hash<std::unique_ptr<int, HandleDeleter>>>);
static_assert(std::is_default_constructible_v<std::hash<std::unique_ptr<int>>>);
static_assert(std::is_same_v<decltype(std::hash<std::unique_ptr<int>>{}(std::declval<const std::unique_ptr<int>&>())),
                             std::size_t>);

int main() {
  std::unique_ptr<int> u(new int(1));
  CHECK(std::hash<std::unique_ptr<int>>{}(u) == std::hash<int*>{}(u.get()));
  std::unique_ptr<int> n;
  CHECK(std::hash<std::unique_ptr<int>>{}(n) == std::hash<int*>{}(nullptr));
  std::unique_ptr<int[]> a(new int[2]);
  CHECK(std::hash<std::unique_ptr<int[]>>{}(a) == std::hash<int*>{}(a.get()));
  std::shared_ptr<int> s = std::make_shared<int>(3);
  CHECK(std::hash<std::shared_ptr<int>>{}(s) == std::hash<int*>{}(s.get()));
  std::shared_ptr<int[]> sa(new int[3]);
  CHECK(std::hash<std::shared_ptr<int[]>>{}(sa) == std::hash<int*>{}(sa.get()));
  return 0;
}
