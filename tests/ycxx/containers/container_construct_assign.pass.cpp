// [container.reqmts]/10-23: "X u; X u = X();" Postconditions: u.empty(). "X u(v); X u = v;"
// Postconditions: u == v. "X u(rv); X u = rv;" Postconditions: u is equal to the value that
// rv had before this construction. "t = v" Result: X&; Postconditions: t == v. "t = rv"
// Result: X&; Postconditions: if t and rv do not refer to the same object, t is equal to the
// value rv had before. /24-25: a.~X() destroys every element and deallocates any memory
// obtained (checked by evaluating everything in a constant expression, where a leaked
// allocation is ill-formed, using an element type that owns heap memory).
// [utility.arg.requirements] Cpp17CopyAssignable: after t = v "the value of v is unchanged",
// so copy self-assignment keeps the value.
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr bool test_with(std::initializer_list<int> idx) {
  X v = make<X>(idx);
  {
    X u;
    X w = X();
    if (!u.empty() || !w.empty()) return false;
  }
  {
    X u(v);
    X w = v;
    if (!(u == v) || !(w == v) || !holds(u, idx) || !holds(v, idx)) return false;
  }
  {
    X src = v;
    X u(std::move(src));
    if (!holds(u, idx)) return false;
    X src2 = v;
    X w = std::move(src2);
    if (!holds(w, idx)) return false;
  }
  {
    X t = make<X>({70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 70, 71, 72, 73, 74, 75, 76, 77});
    static_assert(std::is_same_v<decltype(t = v), X&>);
    X& r = (t = v);
    if (&r != &t || !(t == v) || !holds(v, idx)) return false;
    X small = make<X>({60});
    small = v;  // growing
    if (!(small == v)) return false;
    X e;
    e = v;
    if (!(e == v)) return false;
    v = X();  // to empty and back
    if (!v.empty()) return false;
    v = t;
  }
  {
    X src = v;
    X t = make<X>({61, 62});
    static_assert(std::is_same_v<decltype(t = std::move(src)), X&>);
    X& r = (t = std::move(src));
    if (&r != &t || !holds(t, idx)) return false;
    X src2 = v;
    X t2;
    t2 = std::move(src2);
    if (!holds(t2, idx)) return false;
    // the moved-from object is valid: it can be assigned to and used
    src2 = v;
    if (!holds(src2, idx)) return false;
  }
  {
    X t = v;
    X& self = t;
    t = self;
    if (!holds(t, idx)) return false;
  }
  return true;
}

template <class X>
constexpr bool test() {
  return test_with<X>({}) && test_with<X>({1}) && test_with<X>({3, 1, 2}) &&
         test_with<X>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                       21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39});
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::vector<std::string>>());
static_assert(test<std::string>());
static_assert(test<std::u32string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(test<std::u32string>());
  return 0;
}
