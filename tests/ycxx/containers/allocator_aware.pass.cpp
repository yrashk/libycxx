// [container.alloc.reqmts]/4-32 for an allocator type A that compares by identity:
// c.get_allocator() has type A; "X u;" gives u.empty() and u.get_allocator() == A();
// "X u(m);" gives u.empty() and u.get_allocator() == m; "X u(t, m);" gives u == t and
// u.get_allocator() == m; "X u(rv);" has rv's elements and rv's former allocator;
// "X u(rv, m);" has the elements rv had and u.get_allocator() == m (whether or not m ==
// rv.get_allocator()); "a = t" gives a == t; "a = rv" gives a the value rv had (/28), also
// when the allocators differ and propagate_on_container_move_assignment is false; a.swap(b)
// exchanges the contents. [container.reqmts]/64: copy construction uses
// select_on_container_copy_construction (here: a copy), the allocator is replaced by copy
// assignment / move assignment / swap only if the matching propagate_on_container_* is true,
// and get_allocator() returns the most recent replacement.
#include <vector>
#include <string>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "check.hpp"

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

template <class A>
using VecInt = std::vector<int, typename std::allocator_traits<A>::template rebind_alloc<int>>;
template <class A>
using VecElem = std::vector<Elem, typename std::allocator_traits<A>::template rebind_alloc<Elem>>;
template <class A>
using VecBool = std::vector<bool, typename std::allocator_traits<A>::template rebind_alloc<bool>>;
template <class A>
using Str = std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<A>::template rebind_alloc<char>>;

template <template <class> class R>
constexpr bool all() {
  return test<R, false, false, false>() && test<R, true, false, false>() &&
         test<R, false, true, false>() && test<R, false, false, true>() && test<R, true, true, true>();
}

static_assert(all<VecInt>());
static_assert(all<VecElem>());
static_assert(all<VecBool>());
static_assert(all<Str>());

int main() {
  CHECK(all<VecInt>());
  CHECK(all<VecElem>());
  CHECK(all<VecBool>());
  CHECK(all<Str>());
  return 0;
}
