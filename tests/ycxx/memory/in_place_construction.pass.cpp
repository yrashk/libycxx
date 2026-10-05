// The <memory> creation functions construct the object in place from the forwarded arguments:
// no temporary, no copy or move of the new object, and each argument keeps its value category.
//   [util.smartptr.shared.create]/7.4: make_shared initializes the object "via the expression
//     ::new(pv) U(l...)"; /7.5.2: allocate_shared "via the expression
//     allocator_traits<A2>::construct(a2, pu, l...)", where a2 is a rebound copy of the
//     allocator; /7.12: destroyed via allocator_traits<A2>::destroy(a2, pu); /9-10:
//     make_shared<T>(args...) gives "an object of type T with initial value
//     T(std::forward<Args>(args)...)".
//   [allocator.uses.construction]/23: make_obj_using_allocator is "return
//     make_from_tuple<T>(uses_allocator_construction_args<T>(alloc,
//     std::forward<Args>(args)...));" (a prvalue all the way, so T need not be movable);
//     /24 uninitialized_construct_using_allocator is "return apply([&]<class... U>(U&&... xs)
//     { return construct_at(p, std::forward<U>(xs)...); }, uses_allocator_construction_args<T>(
//     alloc, std::forward<Args>(args)...));"; /5: the leading-allocator convention
//     (allocator_arg, alloc, args...) is preferred over the trailing one (args..., alloc); for
//     a pair, the piecewise form forwards the tuple elements to each member (/13-14).
//   [specialized.construct]/3: construct_at and ranges::construct_at are "return ::new
//     (voidify(*location)) T(std::forward<Args>(args)...);" and are constexpr.
// The probes (support/inplace_probe.hpp) count copies and moves; Pinned can be neither copied
// nor moved, so every form that compiles with it constructed the object where it lives.
// REQUIRES: exceptions
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"
#include "test_allocators.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

template <class A>
constexpr int cat_of() {
  if constexpr (std::is_lvalue_reference_v<A>)
    return std::is_const_v<std::remove_reference_t<A>> ? probe::clref : probe::lref;
  else
    return std::is_const_v<std::remove_reference_t<A>> ? probe::crref : probe::rref;
}

// An allocator that is not interchangeable with std::allocator, carrying an id.
template <class T>
struct TagAlloc {
  using value_type = T;
  int id = 0;
  TagAlloc() = default;
  explicit TagAlloc(int i) : id(i) {}
  template <class U>
  TagAlloc(const TagAlloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(const TagAlloc&, const TagAlloc&) = default;
};

// Uses-allocator types that can be neither copied nor moved.
struct Leading {
  using allocator_type = TagAlloc<char>;
  int key = 0, cat = 0, alloc_id = -1;
  template <class A>
  Leading(std::allocator_arg_t, const allocator_type& al, int k, A&&)
      : key(k), cat(cat_of<A&&>()), alloc_id(al.id) {
    ++counts.made;
  }
  template <class A>
  Leading(int k, A&&) : key(k), cat(cat_of<A&&>()) {  // must not be chosen with an allocator
    ++counts.made;
  }
  Leading(const Leading&) = delete;
};
struct Trailing {
  using allocator_type = TagAlloc<char>;
  int key = 0, cat = 0, alloc_id = -1;
  template <class A>
  Trailing(int k, A&&, const allocator_type& al) : key(k), cat(cat_of<A&&>()), alloc_id(al.id) {
    ++counts.made;
  }
  Trailing(const Trailing&) = delete;
};

void make_shared_cases() {
  Arg a{7};
  const Arg ca{8};
  probe::reset();
  {
    auto p1 = std::make_shared<Pinned>(1, a);
    auto p2 = std::make_shared<Pinned>(2, ca);
    auto p3 = std::make_shared<Pinned>(3, std::move(ca));
    auto p4 = std::make_shared<const Pinned>(4, Arg{9});
    CHECK(p1->cat == probe::lref && p2->cat == probe::clref && p3->cat == probe::crref);
    CHECK(p4->cat == probe::rref && p4->extra == 9);
    CHECK(!a.moved_from);
    auto p5 = std::make_shared<Probe>(5, std::move(a));
    CHECK(p5->cat == probe::rref && a.moved_from);
    CHECK(counts.made == 5 && counts.extra() == 0 && counts.destroyed == 0);
  }
  CHECK(counts.destroyed == 5);

  // allocate_shared: through the (rebound) allocator's construct, exactly once per object.
  alloc_counters = AllocCounters{};
  probe::reset();
  {
    Arg b{3};
    auto q1 = std::allocate_shared<Pinned>(CountingAlloc<int>{}, 1, b);
    auto q2 = std::allocate_shared<Pinned>(CountingAlloc<Pinned>{}, 2, std::move(b));
    auto q3 = std::allocate_shared<const Pinned>(CountingAlloc<char>{}, 3, std::as_const(b));
    CHECK(q1->cat == probe::lref && q2->cat == probe::rref && q3->cat == probe::clref);
    CHECK(alloc_counters.constructs == 3 && alloc_counters.destroys == 0);
    CHECK(counts.made == 3 && counts.extra() == 0 && counts.destroyed == 0);
  }
  CHECK(alloc_counters.destroys == 3 && counts.destroyed == 3);
  CHECK(alloc_counters.outstanding == 0);

  // Default initial value ([util.smartptr.shared.create]/7.6-7.7): U(), constructed in place.
  probe::reset();
  {
    auto d = std::make_shared<Pinned>();
    auto e = std::allocate_shared<Pinned>(CountingAlloc<int>{});
    CHECK(d->key == 0 && e->key == 0);
    CHECK(counts.made == 2 && counts.extra() == 0);
  }
}

void uses_allocator_cases() {
  const TagAlloc<int> al(42);
  Arg a{1};
  const Arg ca{2};

  // A type that does not use the allocator: it is ignored, the arguments are forwarded.
  probe::reset();
  {
    Pinned p = std::make_obj_using_allocator<Pinned>(al, 1, a);
    CHECK(p.cat == probe::lref);
    Pinned q = std::make_obj_using_allocator<Pinned>(al, 2, std::move(ca));
    CHECK(q.cat == probe::crref);
    CHECK(counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);
  }

  // Leading-allocator convention, preferred to the plain constructor.
  {
    Leading l = std::make_obj_using_allocator<Leading>(al, 1, a);
    CHECK(l.alloc_id == 42 && l.cat == probe::lref);
    Leading m = std::make_obj_using_allocator<Leading>(al, 2, Arg{});
    CHECK(m.alloc_id == 42 && m.cat == probe::rref);
    Trailing t = std::make_obj_using_allocator<Trailing>(al, 3, ca);
    CHECK(t.alloc_id == 42 && t.cat == probe::clref);
  }

  // uninitialized_construct_using_allocator constructs at p and returns p.
  {
    alignas(Leading) unsigned char buf[sizeof(Leading)];
    Leading* where = reinterpret_cast<Leading*>(buf);
    Leading* r = std::uninitialized_construct_using_allocator(where, al, 4, std::move(a));
    CHECK(r == where && r->key == 4 && r->alloc_id == 42 && r->cat == probe::rref);
    std::destroy_at(r);

    alignas(Trailing) unsigned char buf2[sizeof(Trailing)];
    Trailing* w2 = reinterpret_cast<Trailing*>(buf2);
    Trailing* r2 = std::uninitialized_construct_using_allocator(w2, al, 5, a);
    CHECK(r2 == w2 && r2->alloc_id == 42 && r2->cat == probe::lref);
    std::destroy_at(r2);

    probe::reset();
    alignas(Pinned) unsigned char buf3[sizeof(Pinned)];
    Pinned* w3 = reinterpret_cast<Pinned*>(buf3);
    Pinned* r3 = std::uninitialized_construct_using_allocator(w3, al, 6, ca);
    CHECK(r3 == w3 && r3->cat == probe::clref && counts.made == 1 && counts.extra() == 0);
    std::destroy_at(r3);
  }

  // pair: piecewise, each member is uses-allocator constructed from its own forwarded tuple.
  probe::reset();
  {
    using P = std::pair<Leading, Probe>;
    Arg b{5};
    alignas(P) unsigned char buf[sizeof(P)];
    P* w = reinterpret_cast<P*>(buf);
    P* r = std::uninitialized_construct_using_allocator(
        w, al, std::piecewise_construct, std::forward_as_tuple(7, b),
        std::forward_as_tuple(8, std::move(b)));
    CHECK(r == w && r->first.alloc_id == 42 && r->first.cat == probe::lref);
    CHECK(r->second.key == 8 && r->second.cat == probe::rref && b.moved_from);
    CHECK(counts.made == 2 && counts.extra() == 0);
    std::destroy_at(r);
  }
  // pair from two values, and from a pair rvalue: the members are constructed from the
  // forwarded values or elements ([allocator.uses.construction]/15, /19), Probe never copied
  // or moved as a whole.
  probe::reset();
  {
    using P = std::pair<Probe, Probe>;
    alignas(P) unsigned char buf[sizeof(P)];
    P* w = reinterpret_cast<P*>(buf);
    P* r = std::uninitialized_construct_using_allocator(w, al, 3, 4);
    CHECK(r == w && r->first.key == 3 && r->second.key == 4);
    std::destroy_at(r);
    P q = std::make_obj_using_allocator<P>(al, std::pair<int, int>(5, 6));
    CHECK(q.first.key == 5 && q.second.key == 6);
    CHECK(counts.made == 4 && counts.extra() == 0);
  }
}

// construct_at / ranges::construct_at: at the location, arguments forwarded, constexpr.
struct Lit {
  int v;
  int cat;
  constexpr Lit(int& x) : v(x), cat(probe::lref) {}
  constexpr Lit(const int& x) : v(x), cat(probe::clref) {}
  constexpr Lit(int&& x) : v(x), cat(probe::rref) {}
  Lit(const Lit&) = delete;
};

constexpr bool construct_at_constexpr() {
  std::allocator<Lit> alloc;
  Lit* p = alloc.allocate(3);
  int x = 4;
  const int cx = 5;
  Lit* a = std::construct_at(p, x);
  Lit* b = std::ranges::construct_at(p + 1, cx);
  Lit* c = std::construct_at(p + 2, 6);
  bool ok = a == p && b == p + 1 && c == p + 2 && a->cat == probe::lref && b->cat == probe::clref &&
            c->cat == probe::rref && a->v + b->v + c->v == 15;
  std::destroy(p, p + 3);
  alloc.deallocate(p, 3);
  return ok;
}
static_assert(construct_at_constexpr());

void construct_at_cases() {
  Arg a{1};
  const Arg ca{2};
  probe::reset();
  alignas(Pinned) unsigned char buf[4][sizeof(Pinned)];
  Pinned* p0 = reinterpret_cast<Pinned*>(buf[0]);
  Pinned* p1 = reinterpret_cast<Pinned*>(buf[1]);
  Pinned* p2 = reinterpret_cast<Pinned*>(buf[2]);
  Pinned* p3 = reinterpret_cast<Pinned*>(buf[3]);
  CHECK(std::construct_at(p0, 1, a) == p0 && p0->cat == probe::lref);
  CHECK(std::ranges::construct_at(p1, 2, ca) == p1 && p1->cat == probe::clref);
  CHECK(std::ranges::construct_at(p2, 3, std::move(ca)) == p2 && p2->cat == probe::crref);
  CHECK(std::construct_at(p3, 4, std::move(a)) == p3 && p3->cat == probe::rref && a.moved_from);
  CHECK(counts.made == 4 && counts.extra() == 0 && counts.destroyed == 0);
  std::destroy_at(p0);
  std::ranges::destroy_at(p1);
  std::destroy_at(p2);
  std::destroy_at(p3);
  CHECK(counts.destroyed == 4);
}

int main() {
  make_shared_cases();
  uses_allocator_cases();
  construct_at_cases();
  return 0;
}
