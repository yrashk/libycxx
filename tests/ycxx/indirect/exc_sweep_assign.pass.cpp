// Exception-injection sweep over indirect and polymorphic with exh::alloc (non-propagating,
// equal iff same id): the owned object's constructors/assignments and the allocator throw at
// their k-th call, for every k. After every run all owned objects are destroyed exactly once
// and every block deallocated exactly once with its size.
//   [indirect.assign]/4: copy assignment: "If any exception is thrown, the result of the
//     expression this->valueless_after_move() remains unchanged. If an exception is thrown
//     during the call to T's selected copy constructor, no effect. If an exception is thrown
//     during the call to T's copy assignment, the state of its owned object is as defined by
//     the exception safety guarantee of T's copy assignment." (exh::T's assignment is strong.)
//     An allocation failure in (2.4) happens before anything changes: no effect either.
//   [indirect.assign]/9: move assignment: "If any exception is thrown, there are no effects on
//     *this or other."
//   [polymorphic.assign]/4: copy assignment: "If any exception is thrown, there are no effects
//     on *this." /8: move assignment: "no effects on *this or other."
//   [indirect.ctor]/2, [polymorphic.ctor]/2: constructors throw only what allocate/construct
//     throw; nothing leaks.
// (indirect and polymorphic are declared in <memory>, [memory.syn].)
// REQUIRES: exceptions
#include <memory>
#include <utility>
#include "exc_harness.hpp"

using namespace exh;
using A = alloc<T, false>;
using Ind = std::indirect<T, A>;

struct Base {
  virtual ~Base() = default;
  virtual int get() const = 0;
};
struct Derived : Base {
  T t;
  explicit Derived(int v) : t(v) {}
  int get() const override { return t.v; }
};
using AB = alloc<Base, false>;
using Poly = std::polymorphic<Base, AB>;
static const T seven_g(7); // created before any sweep

int main() {
  const auto K = {copy_ctor, move_ctor, copy_assign, move_assign, allocation};

  // ---- indirect
  for (int same : {1, 0})
    for (int lhs_valueless : {0, 1})
      for (Kind k : K)
        sweep(same ? "indirect copy assignment, equal allocators" : "indirect copy assignment, unequal allocators", k,
              [=] {
                Ind a(std::allocator_arg, A(1), 1);
                Ind b(std::allocator_arg, A(same ? 1 : 2), 2);
                if (lhs_valueless) {
                  Ind sink(std::move(a));
                }
                bool threw = attempt([&] { a = b; });
                if (threw) {
                  EXH_EXPECT(a.valueless_after_move() == bool(lhs_valueless), "valueless_after_move() changed");
                  if (!lhs_valueless) EXH_EXPECT(a->v == 1, "indirect copy assignment changed the owned value");
                  EXH_EXPECT(!b.valueless_after_move() && b->v == 2, "source changed");
                }
                return threw;
              });
  for (int lhs_valueless : {0, 1})
    for (Kind k : K)
      sweep("indirect move assignment, unequal non-propagating allocators", k, [=] {
        Ind a(std::allocator_arg, A(1), 1);
        Ind b(std::allocator_arg, A(2), 2);
        if (lhs_valueless) {
          Ind sink(std::move(a));
        }
        bool threw = attempt([&] { a = std::move(b); });
        if (threw) {
          EXH_EXPECT(a.valueless_after_move() == bool(lhs_valueless), "[indirect.assign]/9: effect on *this");
          if (!lhs_valueless) EXH_EXPECT(a->v == 1, "[indirect.assign]/9: effect on *this");
          EXH_EXPECT(!b.valueless_after_move() && b->v == 2, "[indirect.assign]/9: effect on other");
        }
        return threw;
      });
  for (Kind k : K) {
    sweep("indirect(allocator_arg, a, int)", k, [] { return attempt([] { Ind a(std::allocator_arg, A(1), 5); }); });
    sweep("indirect copy constructor", k, [] {
      Ind b(std::allocator_arg, A(1), 2);
      return attempt([&] { Ind a(b); });
    });
    sweep("indirect(allocator_arg, a, const indirect&)", k, [] {
      Ind b(std::allocator_arg, A(1), 2);
      return attempt([&] { Ind a(std::allocator_arg, A(3), b); });
    });
    sweep("indirect(allocator_arg, a, indirect&&) unequal", k, [] {
      Ind b(std::allocator_arg, A(1), 2);
      bool threw = attempt([&] { Ind a(std::allocator_arg, A(3), std::move(b)); });
      return threw;
    });
    sweep("indirect = T (converting assignment)", k, [] {
      Ind a(std::allocator_arg, A(1), 1);
      return attempt([&] { a = seven_g; });
    });
  }

  // ---- polymorphic
  for (int same : {1, 0})
    for (int lhs_valueless : {0, 1})
      for (Kind k : K)
        sweep(same ? "polymorphic copy assignment, equal allocators" : "polymorphic copy assignment, unequal allocators",
              k, [=] {
                Poly a(std::allocator_arg, AB(1), std::in_place_type<Derived>, 1);
                Poly b(std::allocator_arg, AB(same ? 1 : 2), std::in_place_type<Derived>, 2);
                if (lhs_valueless) {
                  Poly sink(std::move(a));
                }
                bool threw = attempt([&] { a = b; });
                if (threw) {
                  EXH_EXPECT(a.valueless_after_move() == bool(lhs_valueless), "[polymorphic.assign]/4: effect on *this");
                  if (!lhs_valueless) EXH_EXPECT(a->get() == 1, "[polymorphic.assign]/4: effect on *this");
                }
                return threw;
              });
  for (int lhs_valueless : {0, 1})
    for (Kind k : K)
      sweep("polymorphic move assignment, unequal non-propagating allocators", k, [=] {
        Poly a(std::allocator_arg, AB(1), std::in_place_type<Derived>, 1);
        Poly b(std::allocator_arg, AB(2), std::in_place_type<Derived>, 2);
        if (lhs_valueless) {
          Poly sink(std::move(a));
        }
        bool threw = attempt([&] { a = std::move(b); });
        if (threw) {
          EXH_EXPECT(a.valueless_after_move() == bool(lhs_valueless), "[polymorphic.assign]/8: effect on *this");
          if (!lhs_valueless) EXH_EXPECT(a->get() == 1, "[polymorphic.assign]/8: effect on *this");
          EXH_EXPECT(!b.valueless_after_move() && b->get() == 2, "[polymorphic.assign]/8: effect on other");
        }
        return threw;
      });
  for (Kind k : {value_ctor, copy_ctor, move_ctor, allocation}) {
    sweep("polymorphic(allocator_arg, a, in_place_type<Derived>, int)", k, [] {
      return attempt([] { Poly a(std::allocator_arg, AB(1), std::in_place_type<Derived>, 5); });
    });
    sweep("polymorphic copy constructor", k, [] {
      Poly b(std::allocator_arg, AB(1), std::in_place_type<Derived>, 2);
      return attempt([&] { Poly a(b); });
    });
    sweep("polymorphic(allocator_arg, a, polymorphic&&) unequal", k, [] {
      Poly b(std::allocator_arg, AB(1), std::in_place_type<Derived>, 2);
      return attempt([&] { Poly a(std::allocator_arg, AB(3), std::move(b)); });
    });
  }
  return finish();
}
