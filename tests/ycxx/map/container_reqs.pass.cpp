// [map.overview]/2, [multimap.overview]/2: container, reversible container and
// allocator-aware container requirements. Here: begin/end/cbegin/cend, rbegin/rend
// ([container.rev.reqmts]) visiting the elements in descending key order; size / max_size /
// empty; copy and move construction and assignment ([container.reqmts]/10-23); swap and
// move construction / assignment that keep the elements in place
// ([container.reqmts]/15-16, /50, /65 via support/reqs/move_swap_stability.hpp); the
// allocator-aware constructors and propagation ([container.alloc.reqmts]); clear() and
// noexcept members per the synopsis.
#include <map>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "reqs/move_swap_stability.hpp"
#include "check.hpp"

template <class M>
constexpr M make_n(int n) {
  M m;
  for (int i = 0; i < n; ++i) m.emplace((i * 7) % n, i);
  return m;
}

template <class M>
constexpr bool basics() {
  M m = make_n<M>(20);
  const M& cm = m;
  if (m.size() != 20 || m.empty() || cm.max_size() < 20) return false;
  if (std::distance(m.begin(), m.end()) != 20 || cm.cbegin() != cm.begin() || m.cend() != cm.end()) return false;
  int k = 19;
  for (auto it = m.rbegin(); it != m.rend(); ++it, --k)
    if (it->first != k) return false;
  if (m.crbegin()->first != 19 || std::prev(cm.crend())->first != 0) return false;
  M copy(m);
  if (!(copy == m)) return false;
  M moved(std::move(copy));
  if (!(moved == m)) return false;
  M assigned;
  assigned = m;
  assigned = std::move(moved);
  if (!(assigned == m)) return false;
  M& self = assigned;
  assigned = self;
  if (!(assigned == m)) return false;
  assigned.clear();
  if (!assigned.empty() || assigned.size() != 0) return false;
  M e;
  return e.empty() && e.begin() == e.end() && e.rbegin() == e.rend();
}

template <class T, bool CP, bool MP, bool SP>
constexpr bool alloc() {
  using A = IdAlloc<std::pair<const int, T>, CP, MP, SP>;
  using M = std::map<int, T, std::less<int>, A>;
  M t(A(1));
  for (int i = 0; i < 10; ++i) t.emplace(i, val<T>(i));
  if (t.get_allocator().id != 1) return false;
  M u(t, A(2));
  if (!(u == t) || u.get_allocator().id != 2) return false;
  M src(t);
  M v(std::move(src), A(3));
  if (!(v == t) || v.get_allocator().id != 3) return false;
  M a(A(4));
  a = t;
  if (!(a == t) || a.get_allocator().id != (CP ? 1 : 4)) return false;
  M b(A(5));
  b.emplace(99, val<T>(1));
  M rv(t);
  b = std::move(rv);
  if (!(b == t) || b.get_allocator().id != (MP ? 1 : 5)) return false;
  M s1(A(6)), s2(SP ? A(7) : A(6));
  s1.emplace(1, val<T>(1));
  s1.swap(s2);
  if (!s1.empty() || s2.size() != 1) return false;
  if (SP && (s1.get_allocator().id != 7 || s2.get_allocator().id != 6)) return false;
  M c(std::less<int>(), A(8));
  M d({{1, val<T>(1)}}, A(9));
  return c.get_allocator().id == 8 && d.get_allocator().id == 9 && d.size() == 1;
}

template <class T>
constexpr bool allocs() {
  return alloc<T, false, false, false>() && alloc<T, true, false, false>() && alloc<T, false, true, false>() &&
         alloc<T, false, false, true>() && alloc<T, true, true, true>();
}

using M = std::map<int, int>;
M& m() noexcept;
const M& cm() noexcept;
static_assert(noexcept(m().begin()) && noexcept(cm().end()) && noexcept(m().rbegin()) && noexcept(cm().crend()));
static_assert(noexcept(cm().size()) && noexcept(cm().empty()) && noexcept(cm().max_size()) && noexcept(m().clear()));
static_assert(noexcept(cm().get_allocator()));
static_assert(std::is_nothrow_move_assignable_v<M> && noexcept(m().swap(m())) && noexcept(swap(m(), m())));
static_assert(std::is_nothrow_move_assignable_v<std::multimap<int, int>>);

static_assert(basics<std::map<int, int>>() && basics<std::multimap<int, int>>());
static_assert(allocs<int>() && allocs<Elem>());
static_assert(reqs::move_swap_stability::test<std::map<int, int>>(make_n<std::map<int, int>>));
static_assert(reqs::move_swap_stability::test<std::multimap<int, Elem>>(make_n<std::multimap<int, Elem>>));

int main() {
  CHECK(basics<std::map<int, int>>());
  CHECK(basics<std::multimap<int, int>>());
  CHECK(allocs<int>());
  CHECK(allocs<Elem>());
  CHECK(reqs::move_swap_stability::test<std::map<int, int>>(make_n<std::map<int, int>>));
  CHECK(reqs::move_swap_stability::test<std::multimap<int, Elem>>(make_n<std::multimap<int, Elem>>));
  std::map<int, int, std::less<int>, MinimalAlloc<std::pair<const int, int>>> mm{{2, 2}, {1, 1}};
  CHECK(mm.begin()->first == 1 && mm.size() == 2);
  return 0;
}
