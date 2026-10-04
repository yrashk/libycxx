// [polymorphic.ctor]: the allocator_arg_t constructors initialize alloc with a; the extended
// copy constructor copies the owned object (with its dynamic type) using a; the copy
// constructor uses select_on_container_copy_construction; [polymorphic.assign]: the
// allocator is replaced by copy/move assignment only with POCCA/POCMA; move assignment is
// noexcept iff POCMA or is_always_equal. All allocation goes through the allocator
// (allocator_traits rebinding to the owned object's type, [polymorphic.general]/3).
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "test_allocators.hpp"

struct B {
  int v = 1;
  B() = default;
  explicit B(int x) : v(x) {}
  B(const B&) = default;
  virtual ~B() = default;
  virtual int id() const { return 0; }
};
struct D : B {
  using B::B;
  int id() const override { return 1; }
};

using A = IdAlloc<B>;
using P = std::polymorphic<B, A>;
static_assert(!std::is_nothrow_move_assignable_v<P>);
static_assert(std::is_nothrow_move_assignable_v<std::polymorphic<B>>);
static_assert(std::is_nothrow_move_assignable_v<std::polymorphic<B, IdAlloc<B, false, true>>>);

int main() {
  P a(std::allocator_arg, A(1), std::in_place_type<D>, 5);
  CHECK(a.get_allocator().id == 1 && a->id() == 1 && a->v == 5);
  P b(std::allocator_arg, A(2));
  CHECK(b.get_allocator().id == 2 && b->id() == 0);
  P c(std::allocator_arg, A(3), D(7));
  CHECK(c->id() == 1 && c->v == 7 && c.get_allocator().id == 3);
  P d(std::allocator_arg, A(4), a);
  CHECK(d.get_allocator().id == 4 && d->id() == 1 && d->v == 5);
  P e(a);
  CHECK(e.get_allocator().id == 1 && e->id() == 1);
  P f(std::allocator_arg, A(9), std::move(e));
  CHECK(f.get_allocator().id == 9 && f->id() == 1 && f->v == 5);

  b = a;  // no POCCA: allocator kept
  CHECK(b.get_allocator().id == 2 && b->id() == 1);
  using PC = std::polymorphic<B, IdAlloc<B, true>>;
  PC x(std::allocator_arg, IdAlloc<B, true>(1)), y(std::allocator_arg, IdAlloc<B, true>(2), std::in_place_type<D>);
  x = y;
  CHECK(x.get_allocator().id == 2 && x->id() == 1);

  alloc_counters = {};
  {
    std::polymorphic<B, CountingAlloc<B>> k(std::in_place_type<D>, 3);
    std::polymorphic<B, CountingAlloc<B>> k2(k);
    CHECK(alloc_counters.allocations >= 2 && alloc_counters.constructs >= 2);  // each owned object via construct
    CHECK(k2->id() == 1 && k2->v == 3);
  }
  CHECK(alloc_counters.outstanding == 0 && alloc_counters.destroys >= 2 && alloc_counters.deallocations >= 2);
  return 0;
}
