// [forward.list.overview]/2: forward_list meets the allocator-aware container requirements
// ([container.alloc.reqmts]): get_allocator(); X(m); X(t, m) == t with allocator m;
// X(rv) takes rv's allocator; X(rv, m) has rv's former elements and allocator m;
// [container.reqmts]/64: copy construction uses select_on_container_copy_construction, and
// copy assignment / move assignment / swap replace the allocator only when the matching
// propagate_on_container_* trait is true; a = rv gives a the value rv had even when the
// allocators differ and do not propagate. Every combination of the traits, with a stateful
// allocator, also in constant expressions.
#include <forward_list>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "check.hpp"

template <class T, bool CP, bool MP, bool SP>
constexpr bool test() {
  using A = IdAlloc<T, CP, MP, SP>;
  using X = std::forward_list<T, A>;
  static_assert(std::is_same_v<decltype(std::declval<const X&>().get_allocator()), A>);
  X t(A(1));
  if (!t.empty() || t.get_allocator().id != 1) return false;
  for (int i = 9; i >= 0; --i) t.push_front(val<T>(i));
  X u(t, A(2));
  if (!(u == t) || u.get_allocator().id != 2) return false;
  X c(t);
  if (!(c == t) || c.get_allocator().id != 1) return false;
  X src(t);
  X m(std::move(src));
  if (!(m == t) || m.get_allocator().id != 1) return false;
  X src2(t);
  X m2(std::move(src2), A(3));
  if (!(m2 == t) || m2.get_allocator().id != 3) return false;
  X a(A(4));
  a.push_front(val<T>(50));
  a = t;
  if (!(a == t) || a.get_allocator().id != (CP ? 1 : 4)) return false;
  X b(A(5));
  b.push_front(val<T>(51));
  X rv(t);
  b = std::move(rv);
  if (!(b == t) || b.get_allocator().id != (MP ? 1 : 5)) return false;
  X s1(A(7)), s2(SP ? A(8) : A(7));
  s1.push_front(val<T>(1));
  s2.push_front(val<T>(2));
  s2.push_front(val<T>(3));
  s1.swap(s2);
  if (!holds(s1, {3, 2}) || !holds(s2, {1})) return false;
  if (SP && (s1.get_allocator().id != 8 || s2.get_allocator().id != 7)) return false;
  return true;
}

template <class T>
constexpr bool all() {
  return test<T, false, false, false>() && test<T, true, false, false>() && test<T, false, true, false>() &&
         test<T, false, false, true>() && test<T, true, true, true>();
}

static_assert(all<int>());
static_assert(all<Elem>());

int main() {
  CHECK(all<int>());
  CHECK(all<Elem>());
  std::forward_list<int, MinimalAlloc<int>> l{3, 1, 2};
  l.sort();
  l.insert_after(l.before_begin(), 0);
  CHECK(l == (std::forward_list<int, MinimalAlloc<int>>{0, 1, 2, 3}));
  return 0;
}
