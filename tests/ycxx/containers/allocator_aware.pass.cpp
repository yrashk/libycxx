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

#include "reqs/allocator_aware.hpp"

using namespace reqs::allocator_aware;

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
