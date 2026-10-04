// [uninitialized.construct.default]/1: uninitialized_default_construct(first, last) is
// "for (; first != last; ++first) ::new (voidify(*first)) iterator_traits<I>::value_type;"
// /3: uninitialized_default_construct_n(first, n) does the same "for (; n > 0; (void)++first,
// --n)" and returns first. Both are constexpr. [specialized.destroy]: destroy / destroy_n.
#include <memory>
#include <cstddef>
#include "check.hpp"

struct D {
  int v = 7;  // default-initialization runs the default member initializer
  constexpr D() = default;
};
struct Counted {
  static inline int ctor = 0, dtor = 0;
  Counted() { ++ctor; }
  ~Counted() { ++dtor; }
};

constexpr bool test() {
  std::allocator<D> a;
  D* p = a.allocate(4);
  std::uninitialized_default_construct(p, p + 4);
  for (int i = 0; i < 4; ++i)
    if (p[i].v != 7) return false;
  std::destroy(p, p + 4);
  D* r = std::uninitialized_default_construct_n(p, 3);
  if (r != p + 3) return false;
  if (p[0].v != 7 || p[2].v != 7) return false;
  if (std::destroy_n(p, 3) != p + 3) return false;
  // n <= 0 constructs nothing
  if (std::uninitialized_default_construct_n(p, 0) != p) return false;
  if (std::uninitialized_default_construct_n(p, -2) != p) return false;
  // an empty range constructs nothing
  std::uninitialized_default_construct(p, p);
  a.deallocate(p, 4);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::allocator<Counted> a;
  Counted* p = a.allocate(5);
  std::uninitialized_default_construct(p, p + 5);
  CHECK(Counted::ctor == 5);
  std::destroy(p, p + 5);
  CHECK(Counted::dtor == 5);
  CHECK(std::uninitialized_default_construct_n(p, 2u) == p + 2);  // any integer Size
  CHECK(Counted::ctor == 7);
  std::destroy_n(p, 2);
  a.deallocate(p, 5);
  return 0;
}
