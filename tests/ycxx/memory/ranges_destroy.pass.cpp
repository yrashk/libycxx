// [specialized.destroy]/3: ranges::destroy(first, last) noexcept: "for (; first != last;
// ++first) destroy_at(addressof(*first)); return first;"; the range overload returns
// borrowed_iterator_t<R>. /5: ranges::destroy_n(first, n) noexcept is destroy(counted_iterator(
// std::move(first), n), default_sentinel).base(). All constexpr.
#include <memory>
#include <span>
#include <type_traits>
#include "check.hpp"

struct T {
  int* dtors;
  constexpr ~T() { ++*dtors; }
};
struct Elem {
  static inline int dtors = 0;
  ~Elem() { ++dtors; }
};

static_assert(noexcept(std::ranges::destroy(std::declval<T*>(), std::declval<T*>())));
static_assert(noexcept(std::ranges::destroy(std::declval<std::span<T>>())));
static_assert(noexcept(std::ranges::destroy_n(std::declval<T*>(), 1)));
static_assert(std::is_same_v<decltype(std::ranges::destroy(std::declval<T*>(), std::declval<T*>())), T*>);

constexpr bool test() {
  int dtors = 0;
  std::allocator<T> a;
  T* p = a.allocate(4);
  for (int i = 0; i < 4; ++i) std::construct_at(p + i, &dtors);
  if (std::ranges::destroy(p, p + 4) != p + 4 || dtors != 4) return false;
  for (int i = 0; i < 3; ++i) std::construct_at(p + i, &dtors);
  std::span<T> s(p, 3);
  if (std::ranges::destroy(s) != s.end() || dtors != 7) return false;
  for (int i = 0; i < 2; ++i) std::construct_at(p + i, &dtors);
  if (std::ranges::destroy_n(p, 2) != p + 2 || dtors != 9) return false;
  if (std::ranges::destroy_n(p, 0) != p || dtors != 9) return false;
  a.deallocate(p, 4);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // elements of array type are destroyed element by element (destroy_at on an array type)
  union U {
    Elem a[2][2];
    U() {}
    ~U() {}
  } u;
  for (auto& row : u.a)
    for (auto& e : row) std::construct_at(&e);
  CHECK(std::ranges::destroy(u.a, u.a + 2) == u.a + 2);
  CHECK(Elem::dtors == 4);
  return 0;
}
