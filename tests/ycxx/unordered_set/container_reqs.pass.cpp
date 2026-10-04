// [unord.req.general]/2, /11: an unordered container meets the container and
// allocator-aware container requirements. Here: size / max_size / empty and begin / end
// over all elements; copy / move construction and assignment; the allocator-extended
// constructors and propagate_on_container_* behaviour ([container.alloc.reqmts],
// [container.reqmts]/64) with a stateful allocator; move construction / move assignment /
// swap keeping the elements in place ([container.reqmts]/15-16, /50, /65); and the noexcept
// members of the synopsis.
#include <unordered_set>
#include <iterator>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_allocators.hpp"
#include "reqs/unordered.hpp"
#include "reqs/move_swap_stability.hpp"
#include "check.hpp"

using reqs::unordered::Hash;
using reqs::unordered::Eq;

template <class X>
constexpr X make_n(int n) {
  X x;
  for (int i = 0; i < n; ++i) x.insert(val<typename X::value_type>(i % 80));
  return x;
}

template <class X>
constexpr bool basics() {
  X a = make_n<X>(30);
  const X& ca = a;
  if (a.size() != 30 || a.empty() || ca.max_size() < 30 || std::distance(ca.begin(), ca.end()) != 30) return false;
  if (a.cbegin() != ca.begin() || a.cend() != ca.end()) return false;
  X b(a);
  X c(std::move(b));
  X d;
  d = c;
  X e;
  e = std::move(d);
  if (!(c == a) || !(e == a)) return false;
  e.clear();
  return e.empty() && e.begin() == e.end();
}

template <class T, bool CP, bool MP, bool SP>
constexpr bool alloc() {
  using A = IdAlloc<T, CP, MP, SP>;
  using X = std::unordered_set<T, Hash, Eq, A>;
  X t(8, Hash(), Eq(), A(1));
  for (int i = 0; i < 10; ++i) t.insert(val<T>(i));
  X u(t, A(2));
  if (!(u == t) || u.get_allocator().id != 2 || t.get_allocator().id != 1) return false;
  X src(t);
  if (src.get_allocator().id != 1) return false;
  X v(std::move(src), A(3));
  if (!(v == t) || v.get_allocator().id != 3) return false;
  X a(8, Hash(), Eq(), A(4));
  a = t;
  if (!(a == t) || a.get_allocator().id != (CP ? 1 : 4)) return false;
  X b(8, Hash(), Eq(), A(5));
  X rv(t);
  b = std::move(rv);
  if (!(b == t) || b.get_allocator().id != (MP ? 1 : 5)) return false;
  X s1(8, Hash(), Eq(), A(6)), s2(8, Hash(), Eq(), SP ? A(7) : A(6));
  s1.insert(val<T>(1));
  s1.swap(s2);
  if (!s1.empty() || s2.size() != 1) return false;
  if (SP && (s1.get_allocator().id != 7 || s2.get_allocator().id != 6)) return false;
  X c(A(8));
  X d(4, A(9));
  X g({val<T>(1)}, 4, Hash(), A(10));
  return c.get_allocator().id == 8 && d.get_allocator().id == 9 && d.bucket_count() >= 4 && g.get_allocator().id == 10;
}

template <class T>
constexpr bool allocs() {
  return alloc<T, false, false, false>() && alloc<T, true, false, false>() && alloc<T, false, true, false>() &&
         alloc<T, false, false, true>() && alloc<T, true, true, true>();
}

using S = std::unordered_set<int>;
S& s() noexcept;
const S& cs() noexcept;
static_assert(noexcept(s().begin()) && noexcept(cs().end()) && noexcept(cs().cbegin()) && noexcept(cs().size()));
static_assert(noexcept(cs().empty()) && noexcept(cs().max_size()) && noexcept(s().clear()));
static_assert(noexcept(cs().bucket_count()) && noexcept(cs().load_factor()) && noexcept(cs().max_load_factor()));
static_assert(std::is_nothrow_move_assignable_v<S> && noexcept(s().swap(s())) && noexcept(swap(s(), s())));

static_assert(basics<std::unordered_set<int, Hash, Eq>>() && basics<std::unordered_multiset<Elem, Hash, Eq>>());
static_assert(allocs<int>());
static_assert(reqs::move_swap_stability::test<std::unordered_set<int, Hash, Eq>>(make_n<std::unordered_set<int, Hash, Eq>>));

int main() {
  CHECK(basics<std::unordered_set<int, Hash, Eq>>());
  CHECK(basics<std::unordered_multiset<Elem, Hash, Eq>>());
  CHECK(allocs<int>());
  CHECK(allocs<Elem>());
  CHECK(reqs::move_swap_stability::test<std::unordered_set<int, Hash, Eq>>(make_n<std::unordered_set<int, Hash, Eq>>));
  CHECK(reqs::move_swap_stability::test<std::unordered_multiset<Elem, Hash, Eq>>(make_n<std::unordered_multiset<Elem, Hash, Eq>>));
  return 0;
}
