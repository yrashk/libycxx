// [flat.set.cons.alloc]/1: the constructors taking an Alloc "shall not participate in overload
// resolution unless uses_allocator_v<container_type, Alloc> is true"; /2, /4, /6: each is the
// corresponding constructor "except that c is constructed with uses-allocator construction"
// (so c.get_allocator() is the given allocator, converted to the container's allocator type);
// sorted and deduplicated as flat_set(cont) is ([flat.set.cons]/1). [flat.multiset.cons.alloc]:
// the same for flat_multiset, keeping equivalent elements. [flat.set.syn],
// [flat.multiset.syn]: uses_allocator<flat_set<Key, Compare, KeyContainer>, Alloc> is
// uses_allocator_v<KeyContainer, Alloc>. The deduction guides from (KeyContainer, Allocator).
#include <flat_set>
#include <functional>
#include <inplace_vector>
#include <memory>
#include <type_traits>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"

using A = IdAlloc<int>;
using C = std::vector<int, A>;
using S = std::flat_set<int, std::less<int>, C>;
using MS = std::flat_multiset<int, std::greater<int>, C>;
using IS = std::flat_set<int, std::less<int>, std::inplace_vector<int, 8>>;

static_assert(std::uses_allocator_v<S, A> && std::uses_allocator_v<MS, A>);
static_assert(std::uses_allocator_v<S, IdAlloc<char>>);  // convertible to A
static_assert(!std::uses_allocator_v<S, std::allocator<int>>);
static_assert(!std::is_constructible_v<S, std::allocator<int>>);
static_assert(!std::is_constructible_v<S, const C&, std::allocator<int>>);
static_assert(std::is_constructible_v<S, A> && !std::is_convertible_v<A, S>);  // explicit
static_assert(!std::uses_allocator_v<IS, std::allocator<int>>);
static_assert(!std::is_constructible_v<IS, std::allocator<int>>);
static_assert(!std::is_constructible_v<IS, const std::inplace_vector<int, 8>&, std::allocator<int>>);

template <class X>
constexpr bool alloc_is(const X& s, int id) {
  // extract() gives the container with its allocator
  X copy = s;
  return std::move(copy).extract().get_allocator().id == id;
}

template <class X>
constexpr bool test(bool multi) {
  int arr[] = {3, 1, 3, 2};
  const C cont{3, 1, 3, 2};
  const C sorted = multi ? C{3, 3, 2, 1} : C{1, 2, 3};
  X a(A(1));
  X b(typename X::key_compare(), A(2));
  X c(cont, A(3));
  X d(cont, typename X::key_compare(), A(4));
  X e(arr, arr + 4, A(5));
  X f(std::from_range, arr, A(6));
  X g({3, 1, 3, 2}, A(7));
  X h(c, A(8));
  X i(X(c), A(9));
  X j(cont, IdAlloc<char>(10));
  if (!a.empty() || !b.empty()) return false;
  if (!alloc_is(a, 1) || !alloc_is(b, 2) || !alloc_is(c, 3) || !alloc_is(d, 4) || !alloc_is(e, 5)) return false;
  if (!alloc_is(f, 6) || !alloc_is(g, 7) || !alloc_is(h, 8) || !alloc_is(i, 9) || !alloc_is(j, 10)) return false;
  for (const X* x : {&c, &d, &e, &f, &g, &h, &i, &j}) {
    X tmp = *x;
    if (!(std::move(tmp).extract() == sorted)) return false;
  }
  if (c.size() != (multi ? 4u : 3u)) return false;
  return true;
}

constexpr bool sorted_forms() {
  const C asc{1, 2, 3};
  S a(std::sorted_unique, asc, A(1));
  S b(std::sorted_unique, asc, std::less<int>(), A(2));
  S c(std::sorted_unique, asc.begin(), asc.end(), A(3));
  S d(std::sorted_unique, {1, 2, 3}, A(4));
  if (!alloc_is(a, 1) || !alloc_is(b, 2) || !alloc_is(c, 3) || !alloc_is(d, 4)) return false;
  if (a.size() != 3 || *a.begin() != 1 || !(a == d)) return false;
  const C desc{3, 3, 1};
  MS m(std::sorted_equivalent, desc, A(5));
  MS n(std::sorted_equivalent, {3, 3, 1}, std::greater<int>(), A(6));
  if (!alloc_is(m, 5) || !alloc_is(n, 6) || m.count(3) != 2 || !(m == n)) return false;
  return true;
}

int main() {
  CHECK(test<S>(false));
  CHECK(test<MS>(true));
  CHECK(sorted_forms());
  static_assert(test<S>(false) && test<MS>(true) && sorted_forms());
  // deduction from (KeyContainer, Allocator)
  std::flat_set ds(C{2, 1}, A(3));
  static_assert(std::is_same_v<decltype(ds), S>);
  std::flat_multiset dm(C{2, 2}, std::greater<int>(), A(3));
  static_assert(std::is_same_v<decltype(dm), MS>);
  CHECK(alloc_is(ds, 3) && dm.size() == 2);
  return 0;
}
