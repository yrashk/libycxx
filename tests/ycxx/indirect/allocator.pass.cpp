// [indirect.general]/3-4, [indirect.ctor], [indirect.assign], [indirect.swap]: all allocation
// and construction goes through allocator_traits with alloc; the allocator_arg_t constructors
// initialize alloc with a; the copy constructor uses select_on_container_copy_construction;
// the extended move constructor takes ownership only when the allocators are equal (otherwise
// it constructs a new owned object, other still becomes valueless); copy and move assignment
// replace the allocator only if POCCA / POCMA; swap swaps allocators only if POCS. The
// deduction guide indirect(allocator_arg_t, Allocator, Value) rebinds the allocator.
// REQUIRES: exceptions
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "test_allocators.hpp"

using A = IdAlloc<int>;
using I = std::indirect<int, A>;
static_assert(!std::is_nothrow_constructible_v<I, std::allocator_arg_t, const A&, I&&>);  // is_always_equal false
static_assert(std::is_nothrow_constructible_v<std::indirect<int>, std::allocator_arg_t, const std::allocator<int>&,
                                              std::indirect<int>&&>);
static_assert(!std::is_nothrow_move_assignable_v<I>);
static_assert(std::is_nothrow_move_assignable_v<std::indirect<int, IdAlloc<int, false, true>>>);
static_assert(!noexcept(std::declval<I&>().swap(std::declval<I&>())));
static_assert(std::is_same_v<decltype(std::indirect(std::allocator_arg, IdAlloc<char>(1), 5)), std::indirect<int, IdAlloc<int>>>);

int main() {
  I a(std::allocator_arg, A(1), 7);
  CHECK(a.get_allocator().id == 1 && *a == 7);
  I b(std::allocator_arg, A(2));
  CHECK(b.get_allocator().id == 2 && *b == 0);
  I c(std::allocator_arg, A(3), std::in_place, 9);
  CHECK(*c == 9 && c.get_allocator().id == 3);
  I copy(a);
  CHECK(copy.get_allocator().id == 1 && *copy == 7);  // IdAlloc's select_on... is the default (copy)
  I copy2(std::allocator_arg, A(4), a);
  CHECK(copy2.get_allocator().id == 4 && *copy2 == 7);

  // Extended move constructor.
  I src1(std::allocator_arg, A(5), 11);
  int* p1 = &*src1;
  I same(std::allocator_arg, A(5), std::move(src1));
  CHECK(&*same == p1 && src1.valueless_after_move());
  I src2(std::allocator_arg, A(5), 12);
  int* p2 = &*src2;
  I diff(std::allocator_arg, A(6), std::move(src2));
  CHECK(*diff == 12 && &*diff != p2 && diff.get_allocator().id == 6);
  CHECK(src2.valueless_after_move());  // [indirect.ctor]/16: Postconditions: other is valueless.

  // Copy assignment without POCCA keeps the allocator.
  I x(std::allocator_arg, A(10), 1), y(std::allocator_arg, A(20), 2);
  x = y;
  CHECK(*x == 2 && x.get_allocator().id == 10);
  // With POCCA the allocator is replaced.
  using IC = std::indirect<int, IdAlloc<int, true>>;
  IC xc(std::allocator_arg, IdAlloc<int, true>(10), 1), yc(std::allocator_arg, IdAlloc<int, true>(20), 2);
  xc = yc;
  CHECK(*xc == 2 && xc.get_allocator().id == 20);

  // Move assignment with unequal allocators and no POCMA: a new owned object.
  I m1(std::allocator_arg, A(1), 3), m2(std::allocator_arg, A(2), 4);
  int* pm2 = &*m2;
  m1 = std::move(m2);
  CHECK(*m1 == 4 && &*m1 != pm2 && m1.get_allocator().id == 1 && m2.valueless_after_move());
  // With POCMA: ownership and the allocator move over.
  using IM = std::indirect<int, IdAlloc<int, false, true>>;
  IM n1(std::allocator_arg, IdAlloc<int, false, true>(1), 3), n2(std::allocator_arg, IdAlloc<int, false, true>(2), 4);
  int* pn2 = &*n2;
  n1 = std::move(n2);
  CHECK(&*n1 == pn2 && n1.get_allocator().id == 2 && n2.valueless_after_move());

  // swap with POCS exchanges the allocators.
  using IS = std::indirect<int, IdAlloc<int, false, false, true>>;
  IS s1(std::allocator_arg, IdAlloc<int, false, false, true>(1), 1), s2(std::allocator_arg, IdAlloc<int, false, false, true>(2), 2);
  s1.swap(s2);
  CHECK(*s1 == 2 && s1.get_allocator().id == 2 && *s2 == 1 && s2.get_allocator().id == 1);
  // Without POCS, equal allocators required; they stay.
  I e1(std::allocator_arg, A(7), 1), e2(std::allocator_arg, A(7), 2);
  swap(e1, e2);
  CHECK(*e1 == 2 && *e2 == 1 && e1.get_allocator().id == 7);

  // Every allocation and construction goes through the allocator.
  alloc_counters = {};
  {
    std::indirect<int, CountingAlloc<int>> k(5);
    std::indirect<int, CountingAlloc<int>> k2(k);
    CHECK(alloc_counters.allocations >= 2 && alloc_counters.constructs >= 2);  // each owned object via construct
  }
  CHECK(alloc_counters.deallocations >= 2 && alloc_counters.destroys >= 2 && alloc_counters.outstanding == 0);
  return 0;
}
