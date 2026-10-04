// [default.allocator], [allocator.members], [allocator.traits.members], [specialized.construct],
// [specialized.destroy]: allocator<T> and allocator_traits are usable in constant
// expressions; allocate starts the lifetime of the array but not of its elements;
// construct_at / destroy_at / destroy / destroy_n manage element lifetimes.
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include "check.hpp"

struct Tracked {
  int value;
  int* live;
  constexpr Tracked(int v, int* l) : value(v), live(l) { ++*live; }
  constexpr ~Tracked() { --*live; }
};

static_assert(std::is_same_v<std::allocator<int>::value_type, int>);
static_assert(std::is_same_v<std::allocator<int>::size_type, std::size_t>);
static_assert(std::is_same_v<std::allocator<int>::difference_type, std::ptrdiff_t>);
static_assert(std::allocator<int>::propagate_on_container_move_assignment::value);
static_assert(std::allocator_traits<std::allocator<int>>::is_always_equal::value);
static_assert(std::is_nothrow_default_constructible_v<std::allocator<int>>);
static_assert(std::is_nothrow_constructible_v<std::allocator<int>, const std::allocator<long>&>);
static_assert(std::allocator<int>{} == std::allocator<long>{});

constexpr bool test() {
  int live = 0;
  std::allocator<Tracked> a;
  Tracked* p = a.allocate(4);
  for (int i = 0; i < 4; ++i) std::construct_at(p + i, i * 10, &live);
  if (live != 4 || p[2].value != 20) return false;
  std::destroy_at(p + 3);
  if (live != 3) return false;
  Tracked* end = std::destroy_n(p, 2);
  if (end != p + 2 || live != 1) return false;
  std::destroy(p + 2, p + 3);
  if (live != 0) return false;
  a.deallocate(p, 4);

  using Traits = std::allocator_traits<std::allocator<Tracked>>;
  std::allocator<Tracked> b(a);
  Tracked* q = Traits::allocate(b, 2);
  Traits::construct(b, q, 5, &live);
  Traits::construct(b, q + 1, 6, &live);
  if (live != 2 || q[1].value != 6) return false;
  Traits::destroy(b, q);
  Traits::destroy(b, q + 1);
  if (live != 0) return false;
  Traits::deallocate(b, q, 2);
  if (Traits::max_size(b) != std::numeric_limits<std::size_t>::max() / sizeof(Tracked) &&
      Traits::max_size(b) == 0)
    return false;

  // Rebinding through the converting constructor.
  std::allocator<char> c(b);
  char* s = c.allocate(3);
  std::construct_at(s, 'a');
  if (*s != 'a') return false;
  c.deallocate(s, 3);
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
