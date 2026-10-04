// [default.allocator.general]: allocator<T> has value_type T, size_type size_t,
// difference_type ptrdiff_t, propagate_on_container_move_assignment true_type; its default,
// copy and converting constructors are constexpr and noexcept; allocator_traits<allocator<T>>
// ::is_always_equal::value is true for any T; all specializations meet the allocator
// completeness requirements. [allocator.globals]: operator== is constexpr, noexcept and
// returns true for allocator<T> and allocator<U>. [allocator.members]: allocate(n) returns
// storage for n T (the array's lifetime starts, not the elements'), deallocate(p, n) frees
// it; allocator_traits::rebind_alloc<U> is allocator<U>.
#include <memory>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct Incomplete;
struct Over {
  alignas(64) char c[3];
};

static_assert(std::is_same_v<std::allocator<int>::value_type, int>);
static_assert(std::is_same_v<std::allocator<int>::size_type, std::size_t>);
static_assert(std::is_same_v<std::allocator<int>::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<std::allocator<int>::propagate_on_container_move_assignment, std::true_type>);
static_assert(std::is_nothrow_default_constructible_v<std::allocator<int>>);
static_assert(std::is_nothrow_copy_constructible_v<std::allocator<int>>);
static_assert(std::is_nothrow_constructible_v<std::allocator<int>, const std::allocator<long>&>);
static_assert(std::is_convertible_v<std::allocator<long>, std::allocator<int>>);
static_assert(std::is_copy_assignable_v<std::allocator<int>>);
static_assert(std::allocator_traits<std::allocator<int>>::is_always_equal::value);
static_assert(std::allocator_traits<std::allocator<Incomplete>>::is_always_equal::value);
static_assert(std::is_same_v<std::allocator_traits<std::allocator<int>>::rebind_alloc<char>, std::allocator<char>>);
static_assert(noexcept(std::allocator<int>() == std::allocator<char>()));
static_assert(std::allocator<int>() == std::allocator<char>() && !(std::allocator<int>() != std::allocator<char>()));
static_assert(std::is_same_v<decltype(std::allocator<int>() == std::allocator<int>()), bool>);

constexpr bool in_constant_expression() {
  std::allocator<int> a;
  std::allocator<long> b(a);
  std::allocator<int> c(b);
  int* p = c.allocate(3);
  for (int i = 0; i < 3; ++i) std::construct_at(p + i, i * 2);
  int sum = p[0] + p[1] + p[2];
  std::destroy(p, p + 3);
  c.deallocate(p, 3);
  return sum == 6 && a == c;
}
static_assert(in_constant_expression());

int main() {
  CHECK(in_constant_expression());
  // over-aligned types get suitably aligned storage
  std::allocator<Over> ao;
  Over* o = ao.allocate(5);
  CHECK(reinterpret_cast<std::size_t>(o) % alignof(Over) == 0);
  ao.deallocate(o, 5);
  // distinct live allocations do not overlap
  std::allocator<double> ad;
  double* x = ad.allocate(10);
  double* y = ad.allocate(10);
  CHECK(x + 10 <= y || y + 10 <= x);
  ad.deallocate(y, 10);
  ad.deallocate(x, 10);
  return 0;
}
