// Generic requirement checks extracted from tests/ycxx/containers/allocator_aware.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_allocators.hpp"

namespace reqs::allocator_aware {

template <template <class> class Rebind, bool CP, bool MP, bool SP>
constexpr bool test() {
  using A0 = IdAlloc<int, CP, MP, SP>;
  using X = Rebind<A0>;
  using A = typename X::allocator_type;
  using T = typename X::value_type;
  static_assert(std::is_same_v<decltype(std::declval<const X&>().get_allocator()), A>);
  auto fill = [](X& x, int first, int n) {
    for (int i = 0; i < n; ++i) x.insert(x.end(), val<T>(first + i));
  };
  {
    X u;
    if (!u.empty() || !(u.get_allocator() == A())) return false;
    X u2 = X();
    if (!u2.empty() || !(u2.get_allocator() == A())) return false;
  }
  X t{A(1)};
  if (!t.empty() || t.get_allocator().id != 1) return false;
  fill(t, 0, 25);
  {
    X u(t, A(2));
    if (!(u == t) || u.get_allocator().id != 2) return false;
    X c(t);  // select_on_container_copy_construction: a copy
    if (!(c == t) || c.get_allocator().id != 1) return false;
  }
  {
    X src(t);
    X u(std::move(src));
    if (!(u == t) || u.get_allocator().id != 1) return false;
  }
  {
    X same(t);
    X u(std::move(same), A(1));
    if (!(u == t) || u.get_allocator().id != 1) return false;
    X other(t);
    X w(std::move(other), A(3));
    if (!(w == t) || w.get_allocator().id != 3) return false;
  }
  {
    X a{A(4)};
    fill(a, 40, 3);
    a = t;
    if (!(a == t) || a.get_allocator().id != (CP ? 1 : 4)) return false;
  }
  {
    X a{A(5)};
    fill(a, 40, 30);
    X rv(t);
    a = std::move(rv);
    if (!(a == t) || a.get_allocator().id != (MP ? 1 : 5)) return false;
    X small{A(6)};
    X rv2(t);
    small = std::move(rv2);
    if (!(small == t) || small.get_allocator().id != (MP ? 1 : 6)) return false;
  }
  {
    X a{A(7)};
    fill(a, 50, 2);
    X b{SP ? A(8) : A(7)};
    fill(b, 60, 3);
    a.swap(b);
    if (!holds(a, {60, 61, 62}) || !holds(b, {50, 51})) return false;
    if (SP && (a.get_allocator().id != 8 || b.get_allocator().id != 7)) return false;
    if (!SP && (a.get_allocator().id != 7 || b.get_allocator().id != 7)) return false;
  }
  return true;
}

}  // namespace reqs::allocator_aware
