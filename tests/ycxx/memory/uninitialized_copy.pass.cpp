// [uninitialized.copy]/2-3: uninitialized_copy(first, last, result) is
// "for (; first != last; ++result, (void)++first) ::new (voidify(*result))
// iterator_traits<NoThrowForwardIterator>::value_type(*first);" and returns result.
// /7-8: uninitialized_copy_n(first, n, result) does it for n elements and returns result.
// Both are constexpr.
#include <memory>
#include <iterator>
#include <cstddef>
#include "check.hpp"

struct Explicit {  // direct-initialization: explicit constructors are usable
  long v;
  constexpr explicit Explicit(int x) : v(x) {}
};
struct Copies {
  static inline int copies = 0;
  int v;
  Copies(int x) : v(x) {}
  Copies(const Copies& o) : v(o.v) { ++copies; }
};

// a minimal single-pass input iterator
struct In {
  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using pointer = const int*;
  using reference = const int&;
  const int* p;
  constexpr reference operator*() const { return *p; }
  constexpr In& operator++() {
    ++p;
    return *this;
  }
  constexpr In operator++(int) {
    In t = *this;
    ++p;
    return t;
  }
  constexpr bool operator==(const In&) const = default;
};

constexpr bool test() {
  const int src[4] = {1, 2, 3, 4};
  std::allocator<long> a;
  long* p = a.allocate(4);
  long* e = std::uninitialized_copy(src, src + 4, p);
  if (e != p + 4) return false;
  if (p[0] != 1 || p[3] != 4) return false;
  std::destroy(p, p + 4);
  e = std::uninitialized_copy(In{src}, In{src + 3}, p);
  if (e != p + 3 || p[2] != 3) return false;
  std::destroy(p, p + 3);
  e = std::uninitialized_copy_n(src + 1, 2, p);
  if (e != p + 2 || p[0] != 2 || p[1] != 3) return false;
  std::destroy(p, p + 2);
  if (std::uninitialized_copy_n(src, 0, p) != p) return false;
  if (std::uninitialized_copy(src, src, p) != p) return false;
  a.deallocate(p, 4);

  std::allocator<Explicit> ae;
  Explicit* q = ae.allocate(2);
  std::uninitialized_copy(src, src + 2, q);
  if (q[1].v != 2) return false;
  std::destroy_n(q, 2);
  std::uninitialized_copy_n(In{src + 2}, 2, q);
  if (q[0].v != 3 || q[1].v != 4) return false;
  std::destroy_n(q, 2);
  ae.deallocate(q, 2);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  Copies src[3] = {1, 2, 3};
  std::allocator<Copies> a;
  Copies* p = a.allocate(3);
  Copies* e = std::uninitialized_copy(src, src + 3, p);
  CHECK(e == p + 3 && Copies::copies == 3);
  CHECK(p[1].v == 2 && src[1].v == 2);
  std::destroy(p, p + 3);
  e = std::uninitialized_copy_n(src, 2, p);
  CHECK(e == p + 2 && Copies::copies == 5);
  std::destroy(p, p + 2);
  a.deallocate(p, 3);
  return 0;
}
