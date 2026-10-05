// [expr.dynamic.cast]/8-/10 and [except.handle]/3 give results that depend only on the
// operands and the thrown type, not on what other threads are doing: dynamic_cast and handler
// matching evaluated by several threads at once (no data race: they modify no objects of
// the program, [intro.races]), first in a single-threaded warm-up and then concurrently in
// different orders, over hierarchies with non-public, virtual and multiple bases.
// dynamic_cast: 9.1 (a public base subobject of a C object derived from it), 9.2 (a public
// base subobject of the most derived object, C an unambiguous public base of it), else null /
// bad_cast. Handlers: T an unambiguous public base of E (3.2), i.e. is_convertible_v<E*, T*>.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct B {
  int b = 1;
  virtual ~B() = default;
};
struct X {
  int x = 2;
  virtual ~X() = default;
};
struct In : B {};
struct Out : private In {
  B* b_of_out() { return this; }
  In* in_of_out() { return this; }
};
struct Top : Out {};
struct Y : private X, public B {
  X* x_of_y() { return this; }
};
struct Z : Y {};
struct VQ : virtual B {};
struct VP : private virtual B {};
struct VJ : VP, VQ {};
struct P : B {};
struct Q2 : P {};

static Top top;
static Z z;
static VJ vj;
static Q2 q2;

static std::atomic<long> failures{0};
#define EXPECT(...)                    \
  do {                                 \
    if (!(__VA_ARGS__)) ++failures;    \
  } while (0)

template <class To, class From>
static bool ref_fails(From& f) {
  try {
    (void)dynamic_cast<To&>(f);
  } catch (const std::bad_cast&) {
    return true;
  }
  return false;
}

static void casts_top() {
  B* b = top.b_of_out();
  EXPECT(dynamic_cast<In*>(b) == top.in_of_out());
  EXPECT(dynamic_cast<Out*>(b) == nullptr);
  EXPECT(dynamic_cast<Top*>(b) == nullptr);
  EXPECT(dynamic_cast<Z*>(b) == nullptr);
  EXPECT(dynamic_cast<VJ*>(b) == nullptr);
  EXPECT(dynamic_cast<X*>(b) == nullptr);
  EXPECT(dynamic_cast<void*>(b) == static_cast<void*>(&top));
  EXPECT(ref_fails<Top>(*b));
}
static void casts_z() {
  B* b = &z;
  EXPECT(dynamic_cast<Z*>(b) == &z);
  EXPECT(dynamic_cast<Y*>(b) == &z);
  EXPECT(dynamic_cast<X*>(b) == nullptr);
  EXPECT(dynamic_cast<In*>(b) == nullptr);
  EXPECT(dynamic_cast<Q2*>(b) == nullptr);
  X* x = z.x_of_y();
  EXPECT(dynamic_cast<Z*>(x) == nullptr);
  EXPECT(dynamic_cast<Y*>(x) == nullptr);
  EXPECT(dynamic_cast<B*>(x) == nullptr);
  EXPECT(dynamic_cast<void*>(x) == static_cast<void*>(&z));
  EXPECT(ref_fails<Z>(*x));
}
static void casts_vj() {
  B* b = static_cast<VQ*>(&vj);
  EXPECT(dynamic_cast<VJ*>(b) == &vj);
  EXPECT(dynamic_cast<VQ*>(b) == static_cast<VQ*>(&vj));
  EXPECT(dynamic_cast<VP*>(b) == static_cast<VP*>(&vj));  // 9.2
  EXPECT(dynamic_cast<Top*>(b) == nullptr);
  EXPECT(dynamic_cast<P*>(b) == nullptr);
  EXPECT(dynamic_cast<void*>(b) == static_cast<void*>(&vj));
}
static void casts_q2() {
  B* b = &q2;
  EXPECT(dynamic_cast<Q2*>(b) == &q2);
  EXPECT(dynamic_cast<P*>(b) == &q2);
  EXPECT(dynamic_cast<In*>(b) == nullptr);
  EXPECT(dynamic_cast<VJ*>(b) == nullptr);
  EXPECT(dynamic_cast<X*>(b) == nullptr);
  EXPECT(&dynamic_cast<Q2&>(*b) == &q2);
}

template <class... Ts>
struct list {};
using Handlers = list<B, X, In, Out, Top, Y, Z, VQ, VP, VJ, P, Q2>;

template <class E, class H>
static void handler() {
  constexpr bool expect = std::is_convertible_v<E*, H*>;
  bool caught = false;
  try {
    throw E();
  } catch (const H& h) {
    caught = true;
    if constexpr (expect) EXPECT(typeid(h) == typeid(E));
  } catch (...) {
  }
  EXPECT(caught == expect);
  static E obj;
  caught = false;
  try {
    throw &obj;
  } catch (H* p) {
    caught = true;
    if constexpr (expect) EXPECT(p == static_cast<H*>(&obj));
  } catch (...) {
  }
  EXPECT(caught == expect);
}
template <class E, class... Hs>
static void handlers(list<Hs...>) {
  (handler<E, Hs>(), ...);
}
static void throws_top() { handlers<Top>(Handlers{}); }
static void throws_z() { handlers<Z>(Handlers{}); }
static void throws_vj() { handlers<VJ>(Handlers{}); }
static void throws_q2() { handlers<Q2>(Handlers{}); }

static void (*const work[])() = {casts_top, throws_vj, casts_z, throws_q2, casts_vj, throws_top, casts_q2, throws_z};
constexpr int NW = sizeof work / sizeof work[0];

int main() {
  watchdog(50);
  // single-threaded warm-up
  for (auto* w : work) w();
  CHECK(failures.load() == 0);
  std::atomic<bool> go{false};
  std::vector<std::thread> ts;
  for (int k = 0; k < 4; ++k)
    ts.emplace_back([k, &go] {
      while (!go.load()) std::this_thread::yield();
      for (int i = 0; i < 6000; ++i) work[(i * (2 * k + 1) + k) % NW]();
    });
  go = true;
  for (int i = 0; i < 6000; ++i) work[(NW - 1 - i % NW)]();
  for (auto& t : ts) t.join();
  CHECK(failures.load() == 0);
}
