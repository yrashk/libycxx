// [scoped.adaptor.operators]/1:
//   template<class OuterA1, class OuterA2, class... InnerAllocs>
//     bool operator==(const scoped_allocator_adaptor<OuterA1, InnerAllocs...>& a,
//                     const scoped_allocator_adaptor<OuterA2, InnerAllocs...>& b) noexcept;
// "Returns: If sizeof...(InnerAllocs) is zero, a.outer_allocator() == b.outer_allocator()
// otherwise a.outer_allocator() == b.outer_allocator() && a.inner_allocator() ==
// b.inner_allocator()". So adaptors whose outer allocators differ in value type (a rebound
// copy, as containers hold) compare with ==, and != is its rewritten negation, for one, two
// and three allocators.
#include <scoped_allocator>
#include <cstddef>
#include <memory>
#include <type_traits>
#include "check.hpp"

template <class T, int Tag>
struct A {
  using value_type = T;
  template <class U>
  struct rebind { using other = A<U, Tag>; };
  int id = 0;
  A() = default;
  explicit A(int i) : id(i) {}
  template <class U>
  A(const A<U, Tag>& o) : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U>
  friend bool operator==(const A& a, const A<U, Tag>& b) { return a.id == b.id; }
};

using std::scoped_allocator_adaptor;

int main() {
  scoped_allocator_adaptor<A<int, 1>> one(A<int, 1>(1));
  scoped_allocator_adaptor<A<long, 1>> one_l(A<long, 1>(1)), one_l2(A<long, 1>(2));
  CHECK(one == one_l && !(one != one_l) && one != one_l2);
  static_assert(noexcept(one == one_l));

  scoped_allocator_adaptor<A<int, 1>, A<int, 2>> two(A<int, 1>(1), A<int, 2>(2));
  scoped_allocator_adaptor<A<long, 1>, A<int, 2>> two_l(A<long, 1>(1), A<int, 2>(2));
  scoped_allocator_adaptor<A<long, 1>, A<int, 2>> two_l2(A<long, 1>(1), A<int, 2>(3));
  CHECK(two == two_l && two != two_l2 && two_l2 != two);

  scoped_allocator_adaptor<A<int, 1>, A<int, 2>, A<int, 3>> three(A<int, 1>(1), A<int, 2>(2), A<int, 3>(3));
  scoped_allocator_adaptor<A<char, 1>, A<int, 2>, A<int, 3>> three_c(A<char, 1>(1), A<int, 2>(2), A<int, 3>(3));
  scoped_allocator_adaptor<A<char, 1>, A<int, 2>, A<int, 3>> three_c2(A<char, 1>(1), A<int, 2>(2), A<int, 3>(4));
  CHECK(three == three_c && three_c == three && three != three_c2);
  // The rebound copy a container would hold compares equal to the original.
  std::allocator_traits<decltype(three)>::rebind_alloc<double> reb(three);
  CHECK(reb == three && three == reb);
  return 0;
}
