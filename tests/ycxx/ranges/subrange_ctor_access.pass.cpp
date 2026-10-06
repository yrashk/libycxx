// [range.subrange.general]: subrange<I, S, K>, K defaulting to sized iff
// sized_sentinel_for<S, I>; StoreSize when K is sized but S - I is not available;
// convertible-to-non-slicing rejects a derived-to-base pointer conversion (but not a
// qualification conversion); the conversion to a pair-like type requires
// pair-like-convertible-from (not a range, not a reference, non-slicing first element);
// the deduction guides.
// [range.subrange.ctor]/1-/7: (i, s) only without StoreSize; (i, s, n) only for sized subranges,
// storing n; from a borrowed range R (and with StoreSize only from a sized one, storing
// ranges::size(r)); (r, n).
// [range.subrange.access]/4-/9: empty, size (the stored size, or end_ - begin_), next, prev,
// advance (negative n for bidirectional iterators; the stored size follows).
// [range.subrange] tuple interface: get<0>, get<1>, structured bindings.
#include <ranges>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using LI = std::list<int>::iterator;
using FI = std::forward_list<int>::iterator;

struct Base { int x; };
struct Derived : Base { int y; };

// defaults
static_assert(std::is_same_v<rg::subrange<int*>, rg::subrange<int*, int*, rg::subrange_kind::sized>>);
static_assert(std::is_same_v<rg::subrange<LI>, rg::subrange<LI, LI, rg::subrange_kind::unsized>>);
template <class I, class S, rg::subrange_kind K>
concept valid_subrange = requires { typename rg::subrange<I, S, K>; };
static_assert(!valid_subrange<int*, int*, rg::subrange_kind::unsized>);  // sized_sentinel_for holds
static_assert(valid_subrange<LI, LI, rg::subrange_kind::sized>);

using SL = rg::subrange<LI, LI, rg::subrange_kind::sized>;  // StoreSize
static_assert(rg::sized_range<SL> && !rg::sized_range<rg::subrange<LI>>);
static_assert(!std::is_constructible_v<SL, LI, LI>);
static_assert(std::is_constructible_v<SL, LI, LI, std::size_t>);
static_assert(!std::is_constructible_v<rg::subrange<LI>, LI, LI, std::size_t>);
static_assert(std::is_constructible_v<rg::subrange<int*>, int*, int*, std::size_t>);

// non-slicing
static_assert(std::is_constructible_v<rg::subrange<const int*>, int*, int*>);
static_assert(!std::is_constructible_v<rg::subrange<Base*>, Derived*, Derived*>);
static_assert(!std::is_constructible_v<rg::subrange<Base*>, std::span<Derived>>);
static_assert(std::is_constructible_v<rg::subrange<const int*>, std::span<int>>);

// from ranges: borrowed only; StoreSize needs a sized range
static_assert(std::is_constructible_v<rg::subrange<std::vector<int>::iterator>, std::vector<int>&>);
static_assert(!std::is_constructible_v<rg::subrange<std::vector<int>::iterator>, std::vector<int>>);
static_assert(std::is_constructible_v<SL, std::list<int>&>);
using SF = rg::subrange<FI, FI, rg::subrange_kind::sized>;
static_assert(!std::is_constructible_v<SF, std::forward_list<int>&>);
static_assert(std::is_constructible_v<SF, std::forward_list<int>&, std::size_t>);
static_assert(std::is_convertible_v<std::span<int>, rg::subrange<int*>>);

// pair-like conversion
static_assert(std::is_convertible_v<rg::subrange<int*>, std::pair<int*, int*>>);
static_assert(std::is_convertible_v<rg::subrange<int*>, std::tuple<const int*, const int*>>);
static_assert(!std::is_convertible_v<rg::subrange<Derived*>, std::pair<Base*, Derived*>>);
static_assert(std::is_convertible_v<rg::subrange<Derived*>, std::pair<Derived*, const Base*>>);  // only the first must not slice
static_assert(!std::is_convertible_v<rg::subrange<int*>, std::tuple<int*, int*, int*>>);

// deduction guides
static_assert(std::is_same_v<decltype(rg::subrange(std::declval<LI>(), std::declval<LI>())), rg::subrange<LI>>);
static_assert(std::is_same_v<decltype(rg::subrange(std::declval<LI>(), std::declval<LI>(), 3u)), SL>);
static_assert(std::is_same_v<decltype(rg::subrange(std::declval<std::list<int>&>())), SL>);
static_assert(std::is_same_v<decltype(rg::subrange(std::declval<std::forward_list<int>&>())), rg::subrange<FI>>);
static_assert(std::is_same_v<decltype(rg::subrange(std::declval<std::forward_list<int>&>(), 2u)), SF>);

int main() {
  std::list<int> l{1, 2, 3, 4, 5};
  SL s(l.begin(), l.end(), 5);
  CHECK(s.size() == 5 && !s.empty() && *s.begin() == 1);
  // advance keeps the stored size in step, forwards and backwards
  s.advance(2);
  CHECK(s.size() == 3 && *s.begin() == 3);
  s.advance(-1);
  CHECK(s.size() == 4 && *s.begin() == 2);
  s.advance(10);  // stops at end: the size is reduced by the distance actually moved
  CHECK(s.size() == 0 && s.empty() && s.begin() == l.end());
  SL whole(l);
  CHECK(whole.size() == 5);
  auto n = whole.next(2);
  CHECK(n.size() == 3 && whole.size() == 5 && *n.begin() == 3);
  auto p = n.prev();
  CHECK(p.size() == 4 && *p.begin() == 2);
  auto m = std::move(n).next();
  CHECK(m.size() == 2 && *m.begin() == 4);

  std::forward_list<int> fl{1, 2, 3};
  SF sf(fl, 3);
  CHECK(sf.size() == 3);
  sf.advance(1);
  CHECK(sf.size() == 2 && *sf.begin() == 2);

  int a[4] = {1, 2, 3, 4};
  rg::subrange<int*> sa(a, a + 4);
  CHECK(sa.size() == 4);
  CHECK(sa.next(3).size() == 1 && sa.next(3).prev(2).size() == 3);
  auto [b, e] = sa;
  CHECK(b == a && e == a + 4);
  CHECK(rg::get<0>(sa) == a && std::get<1>(sa) == a + 4);
  std::pair<const int*, const int*> pr = sa;
  CHECK(pr.first == a && pr.second == a + 4);
  rg::subrange<const int*> cs = std::span<int>(a, 2);
  CHECK(cs.size() == 2);
  rg::subrange<int*> empty_sr;
  CHECK(empty_sr.empty() && empty_sr.size() == 0);
  return 0;
}
