// [except.handle]/3: a handler of type cv T or cv T& matches an exception object of type E if
// E and T are the same type (3.1) or "T is an unambiguous public base class of E" (3.2); a
// handler of type cv T or const T& with T a pointer type matches a thrown pointer E convertible
// to T by "a standard pointer conversion not involving conversions to pointers to private or
// protected or ambiguous classes" or a qualification conversion (3.3). For class types both
// conditions are exactly is_convertible_v<E*, T*> ([conv.ptr]/3: the conversion is ill-formed
// for an inaccessible or ambiguous base), used here as the oracle. Public single-inheritance
// chains sit on top of classes with private, protected, virtual and multiple bases; the caught
// reference/pointer must denote the T subobject of the exception object. Every exception
// type meets the same handlers in several orders so that remembered results cannot leak.
// REQUIRES: exceptions
#include <type_traits>
#include <typeinfo>
#include "check.hpp"

struct B {
  int b = 1;
  virtual ~B() = default;
};
struct X {
  int x = 2;
  virtual ~X() = default;
};
struct In : B {};
struct In2 : In {};
struct Out : private In2 {};
struct Top : Out {};
struct Top2a : Top {};
struct Out2 : private In2 {};
struct Both : Out, Out2 {};
struct Y : private X, public B {};
struct Z : Y {};
struct Z2 : Z {};
struct Yp : protected X {};
struct Zp : Yp {};
struct Zp2 : Zp {};
struct Q : protected B {};
struct W : B {};
struct T : Q, W {};
struct T2 : T {};
struct VP : private virtual B {};
struct VQ : virtual B {};
struct VJ : VP, VQ {};
struct VK : VJ {};
struct VL : VK {};
struct VP1 : protected virtual B {};
struct VP2 : private virtual B {};
struct VM : VP1, VP2 {};
struct VN : VM {};
struct M : X, private B {};
struct N : M {};
struct N2 : N {};

template <class... Ts>
struct list {};
using Handlers = list<B, X, In, In2, Out, Top, Top2a, Out2, Both, Y, Z, Z2, Yp, Zp, Zp2, Q, W, T,
                      T2, VP, VQ, VJ, VK, VL, VP1, VP2, VM, VN, M, N, N2>;

static_assert(std::is_convertible_v<VL*, B*>);   // public via VQ ([class.paths]/1)
static_assert(!std::is_convertible_v<VN*, B*>);  // non-public on every path
static_assert(!std::is_convertible_v<T2*, B*>);  // ambiguous (and one path protected)
static_assert(std::is_convertible_v<Z2*, B*> && !std::is_convertible_v<Z2*, X*>);
static_assert(!std::is_convertible_v<Top2a*, In2*> && std::is_convertible_v<Top2a*, Out*>);

template <class E, class H>
void by_reference() {
  constexpr bool expect = std::is_convertible_v<E*, H*>;
  bool caught = false;
  try {
    throw E();
  } catch (H& h) {
    caught = true;
    if constexpr (expect) {
      CHECK(typeid(h) == typeid(E));
      E* e = static_cast<E*>(dynamic_cast<void*>(&h));
      CHECK(&h == static_cast<H*>(e));
    }
  } catch (...) {
  }
  CHECK(caught == expect);
  caught = false;
  try {
    throw E();
  } catch (const volatile H&) {
    caught = true;
  } catch (...) {
  }
  CHECK(caught == expect);
}

template <class E, class H>
void by_pointer() {
  constexpr bool expect = std::is_convertible_v<E*, H*>;
  static E obj;
  bool caught = false;
  try {
    throw &obj;
  } catch (const H* p) {
    caught = true;
    if constexpr (expect) CHECK(p == static_cast<const H*>(&obj));
  } catch (...) {
  }
  CHECK(caught == expect);
  caught = false;
  try {
    E* const cp = &obj;
    throw cp;  // the exception object has type E* ([expr.throw]: top-level cv dropped)
  } catch (H* const& p) {
    caught = true;
    if constexpr (expect) CHECK(p == static_cast<H*>(&obj));
  } catch (...) {
  }
  CHECK(caught == expect);
}

template <class E, class... Hs>
void all_handlers(list<Hs...>, bool reverse) {
  if (!reverse) {
    (by_reference<E, Hs>(), ...);
    (by_pointer<E, Hs>(), ...);
  } else {
    int dummy = 0;
    // right to left, through an assignment fold
    (dummy = ... = (by_pointer<E, Hs>(), by_reference<E, Hs>(), 0));
    (void)dummy;
  }
}

template <class E>
void one(bool reverse) {
  all_handlers<E>(Handlers{}, reverse);
  // An exact-type handler after a non-matching one.
  int which = 0;
  try {
    throw E();
  } catch (const std::bad_cast&) {
    which = 1;
  } catch (const E&) {
    which = 2;
  }
  CHECK(which == 2);
}

template <class... Es>
void run(list<Es...>, int round) {
  if (round % 2 == 0) {
    (one<Es>(round % 4 == 2), ...);
  } else {
    int dummy = 0;
    (dummy = ... = (one<Es>(round % 4 == 3), 0));
    (void)dummy;
  }
}

int main() {
  using Thrown = list<Top2a, Both, Z2, Zp2, T2, VL, VJ, VN, N2, Top, Z, N, In2, VP1>;
  for (int round = 0; round < 4; ++round) run(Thrown{}, round);
}
