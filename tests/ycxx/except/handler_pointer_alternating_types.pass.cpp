// [except.handle]/3.3: a handler of type cv T or const T&, T a pointer type, matches an
// exception object of pointer type E convertible to T by "a standard pointer conversion not
// involving conversions to pointers to private or protected or ambiguous classes", "a function
// pointer conversion", or "a qualification conversion"; /3.4: a pointer handler also matches
// std::nullptr_t; Note 1: a thrown literal 0 (an int) matches no pointer handler.
// The same handler list sees thrown pointers of many types in many orders, so that remembered
// match results must depend on the thrown type; the caught pointer must be adjusted to the
// base subobject for each dynamic type separately.
// REQUIRES: exceptions
#include <cstddef>
#include "check.hpp"

struct A {
  int a = 1;
  virtual ~A() = default;
};
struct B {
  int b = 2;
  virtual ~B() = default;
};
struct D1 : A, B {};
struct D2 : B, A {
  long pad[2] = {};
};
struct Amb : D1, D2 {};
struct PrivA : private A, public B {};

static int ints[4];
static int* iptrs[2] = {ints, ints + 1};
static double dbl;
static A a_obj;
static D1 d1;
static D2 d2;
static Amb amb;
static PrivA priv;
static void fn() {}
static void fn_noexcept() noexcept {}

// returns the index of the handler entered and the pointer value it received
struct Hit {
  int h;
  const volatile void* p;
};

static Hit run(void (*f)()) {
  try {
    f();
  } catch (int* const* p) {  // int**, int* const*, nullptr
    return {0, p};
  } catch (const int* p) {  // int*, const int*
    return {1, p};
  } catch (const int* const* p) {  // const int**, const int* const*
    return {2, p};
  } catch (A* p) {  // A*, D1*, D2*  (not Amb*, not PrivA*)
    return {3, p};
  } catch (const B* const& p) {  // B*, const B*, D1*, D2*, PrivA*
    return {4, p};
  } catch (void (*p)()) {  // void(*)(), void(*)() noexcept
    return {5, reinterpret_cast<const void*>(p)};
  } catch (void* p) {  // double*, Amb*
    return {6, p};
  } catch (const volatile void* p) {  // const double*, volatile int*, const int* volatile*
    return {7, p};
  } catch (...) {
    return {8, nullptr};
  }
}

struct Case {
  void (*f)();
  int h;
  const volatile void* p;
};

int main() {
  const Case cases[] = {
      {[] { throw iptrs; }, 0, iptrs},
      {[] { throw static_cast<int* const*>(iptrs); }, 0, iptrs},
      {[] { throw nullptr; }, 0, nullptr},
      {[] { throw ints + 2; }, 1, ints + 2},
      {[] { throw static_cast<const int*>(ints + 3); }, 1, ints + 3},
      {[] { throw const_cast<const int**>(iptrs); }, 2, iptrs},
      {[] { throw static_cast<const int* const*>(iptrs + 1); }, 2, iptrs + 1},
      {[] { throw &a_obj; }, 3, &a_obj},
      {[] { throw &d1; }, 3, static_cast<A*>(&d1)},
      {[] { throw &d2; }, 3, static_cast<A*>(&d2)},
      {[] { throw static_cast<B*>(&d1); }, 4, static_cast<B*>(&d1)},
      {[] { throw static_cast<const B*>(&d2); }, 4, static_cast<B*>(&d2)},
      {[] { throw &priv; }, 4, static_cast<B*>(&priv)},
      {[] { throw &fn; }, 5, reinterpret_cast<const void*>(&fn)},
      {[] { throw &fn_noexcept; }, 5, reinterpret_cast<const void*>(&fn_noexcept)},
      {[] { throw &dbl; }, 6, &dbl},
      {[] { throw &amb; }, 6, &amb},
      {[] { throw static_cast<const double*>(&dbl); }, 7, &dbl},
      {[] { throw static_cast<volatile int*>(ints); }, 7, ints},
      {[] { throw 0; }, 8, nullptr},
      {[] { throw static_cast<const int* volatile*>(nullptr); }, 7, nullptr},
      {[] { throw 'c'; }, 8, nullptr},
  };
  constexpr unsigned n = sizeof(cases) / sizeof(cases[0]);
  auto check = [&](unsigned i) {
    Hit r = run(cases[i].f);
    CHECK(r.h == cases[i].h);
    CHECK(r.p == cases[i].p);
  };
  for (int round = 0; round < 3; ++round) {
    for (unsigned i = 0; i < n; ++i) check(i);
    for (unsigned i = n; i-- > 0;) check(i);
  }
  unsigned x = 777;
  for (int k = 0; k < 3000; ++k) {
    x = x * 1664525u + 1013904223u;
    check((x >> 12) % n);
  }
  return 0;
}
