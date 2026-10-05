// indirect's in_place constructors and polymorphic's in_place_type constructors construct the
// owned object through the allocator, directly from the forwarded arguments: no temporary, no
// copy or move of the owned object, the arguments' categories kept.
//   [indirect.general]/3: "Constructing an owned object with args... using the allocator a
//     means calling allocator_traits<Allocator>::construct(a, p, args...)"; [indirect.ctor]/21-
//     22: indirect(in_place_t, Us&&... us): "Constructs an owned object of type T with
//     std::forward<Us>(us)..., using the allocator alloc"; /23-24 the allocator_arg_t form;
//     /25-28 the initializer_list forms ("with ilist, std::forward<Us>(us)..."). Only the copy
//     operations mandate is_copy_constructible_v<T> ([indirect.ctor]/9), so T need not be
//     copyable or movable here.
//   [polymorphic.general]/3: the same definition; [polymorphic.ctor]/16-19:
//     polymorphic(in_place_type_t<U>, Ts&&... ts): "Constructs an owned object of type U with
//     std::forward<Ts>(ts)... using the allocator alloc" (U must be copy constructible).
//   Moving an indirect or polymorphic transfers ownership ([indirect.ctor]/14, [polymorphic.
//   ctor]): the owned object is not moved.
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"
#include "test_allocators.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

struct ListPinned : Pinned {
  ListPinned(std::initializer_list<int> il, Arg& a) : Pinned(static_cast<int>(il.size()), a) {}
  ListPinned(std::initializer_list<int> il, Arg&& a) : Pinned(static_cast<int>(il.size()), std::move(a)) {}
};

struct Base {
  virtual ~Base() = default;
  virtual int key() const { return -1; }
  virtual int cat() const { return -1; }
};
struct Derived : Base {
  Probe p;
  template <class... A>
  Derived(A&&... a) : p(std::forward<A>(a)...) {}
  Derived(std::initializer_list<int> il, const Arg& a) : p(static_cast<int>(il.size()), a) {}
  int key() const override { return p.key; }
  int cat() const override { return p.cat; }
};

void indirect_cases() {
  Arg a{1};
  const Arg ca{2};
  probe::reset();
  {
    std::indirect<Pinned> i1(std::in_place, 1, a);
    std::indirect<Pinned> i2(std::in_place, 2, std::move(ca));
    std::indirect<ListPinned> i3(std::in_place, {1, 2, 3}, a);
    CHECK(i1->cat == probe::lref && i2->cat == probe::crref && i3->key == 3 && i3->cat == probe::lref);
    CHECK(counts.made == 3 && counts.extra() == 0 && counts.destroyed == 0);
    const Pinned* where = &*i1;
    std::indirect<Pinned> moved(std::move(i1));
    CHECK(&*moved == where && i1.valueless_after_move());
    CHECK(counts.made == 3 && counts.extra() == 0 && counts.destroyed == 0);
  }
  CHECK(counts.destroyed == 3);

  // Through the allocator's construct, once per owned object.
  alloc_counters = AllocCounters{};
  probe::reset();
  {
    CountingAlloc<Pinned> al;
    std::indirect<Pinned, CountingAlloc<Pinned>> i(std::allocator_arg, al, std::in_place, 3, std::move(a));
    CHECK(i->cat == probe::rref && a.moved_from);
    std::indirect<ListPinned, CountingAlloc<ListPinned>> l(std::allocator_arg, CountingAlloc<ListPinned>{},
                                                            std::in_place, {1, 2}, Arg{});
    CHECK(l->key == 2 && l->cat == probe::rref);
    std::indirect<Pinned, CountingAlloc<Pinned>> d(std::in_place, 4, ca);
    CHECK(d->cat == probe::clref);
    CHECK(alloc_counters.constructs == 3 && counts.made == 3 && counts.extra() == 0);
  }
  CHECK(alloc_counters.destroys == 3 && alloc_counters.outstanding == 0 && counts.destroyed == 3);
}

// Counts construct/destroy calls for objects of type Derived only (an implementation may use
// the allocator for its own bookkeeping objects too).
int derived_constructs = 0, derived_destroys = 0;
template <class T>
struct DerivedCountingAlloc {
  using value_type = T;
  DerivedCountingAlloc() = default;
  template <class U>
  DerivedCountingAlloc(const DerivedCountingAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U, class... A>
  void construct(U* p, A&&... a) {
    if constexpr (std::is_same_v<U, Derived>) ++derived_constructs;
    ::new (static_cast<void*>(p)) U(std::forward<A>(a)...);
  }
  template <class U>
  void destroy(U* p) {
    if constexpr (std::is_same_v<U, Derived>) ++derived_destroys;
    p->~U();
  }
  friend bool operator==(const DerivedCountingAlloc&, const DerivedCountingAlloc&) = default;
};

void polymorphic_cases() {
  Arg a{1};
  const Arg ca{2};
  probe::reset();
  {
    std::polymorphic<Base> p1(std::in_place_type<Derived>, 1, a);
    std::polymorphic<Base> p2(std::in_place_type<Derived>, 2, std::move(ca));
    std::polymorphic<Base> p3(std::in_place_type<Derived>, {1, 2, 3, 4}, ca);
    CHECK(p1->cat() == probe::lref && p2->cat() == probe::crref);
    CHECK(p3->key() == 4 && p3->cat() == probe::clref);
    CHECK(counts.made == 3 && counts.extra() == 0 && counts.destroyed == 0);
    const Base* where = &*p1;
    std::polymorphic<Base> moved(std::move(p1));
    CHECK(&*moved == where && p1.valueless_after_move() && counts.extra() == 0);
  }
  CHECK(counts.destroyed == 3);

  probe::reset();
  {
    std::polymorphic<Base, DerivedCountingAlloc<Base>> p(std::allocator_arg, DerivedCountingAlloc<Base>{},
                                                         std::in_place_type<Derived>, 5, std::move(a));
    CHECK(p->key() == 5 && p->cat() == probe::rref && a.moved_from);
    CHECK(derived_constructs == 1 && counts.made == 1 && counts.extra() == 0);
  }
  CHECK(derived_destroys == 1 && counts.destroyed == 1);
}

int main() {
  indirect_cases();
  polymorphic_cases();
  return 0;
}
