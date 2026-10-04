// Generic requirement checks extracted from tests/ycxx/containers/container_construct_assign.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <type_traits>
#include <utility>
#include "container_values.hpp"

namespace reqs::container_construct_assign {

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

}  // namespace reqs::container_construct_assign
