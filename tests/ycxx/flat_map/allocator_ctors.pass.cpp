// [flat.map.cons.alloc]: the constructors taking an allocator construct c.keys and c.values
// with uses-allocator construction, and take part only when both containers use that
// allocator type. [flat.map.syn]: uses_allocator<flat_map<...>, Alloc> is true iff it is
// true for both containers.
// REQUIRES: exceptions
#include <flat_map>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"

using KA = IdAlloc<int>;
using VA = IdAlloc<long>;
using KC = std::vector<int, KA>;
using VC = std::vector<long, VA>;
using M = std::flat_map<int, long, std::less<int>, KC, VC>;
using MM = std::flat_multimap<int, long, std::less<int>, KC, VC>;

static_assert(std::uses_allocator_v<M, KA> && std::uses_allocator_v<M, VA>);
static_assert(!std::uses_allocator_v<M, std::allocator<int>>);
static_assert(!std::uses_allocator_v<std::flat_map<int, long, std::less<int>, KC, std::vector<long>>, KA>);
static_assert(!std::is_constructible_v<M, std::allocator<int>>);
static_assert(std::is_constructible_v<M, KA>);

template <class X>
constexpr bool alloc_is(const X& m, int id) {
  return m.keys().get_allocator().id == id && m.values().get_allocator().id == id;
}

template <class X>
constexpr bool test() {
  std::pair<int, long> arr[] = {{2, 20}, {1, 10}};
  X a(KA(1));
  X b(std::less<int>(), KA(2));
  X c(KC{3, 1}, VC{30, 10}, KA(3));
  X d(arr, arr + 2, KA(4));
  X e(std::from_range, arr, KA(5));
  X f({{1, 1L}, {2, 2L}}, KA(6));
  X g(c, KA(7));
  X h(std::move(g), KA(8));
  if (!alloc_is(a, 1) || !alloc_is(b, 2) || !alloc_is(c, 3) || !alloc_is(d, 4) || !alloc_is(e, 5) || !alloc_is(f, 6))
    return false;
  if (!alloc_is(h, 8) || !(h == c) || c.keys()[0] != 1 || d.size() != 2 || e.begin()->second != 10) return false;
  return true;
}

constexpr bool sorted_forms() {
  M s(std::sorted_unique, KC{1, 2}, VC{10, 20}, KA(9));
  MM t(std::sorted_equivalent, KC{1, 1}, VC{10, 11}, KA(10));
  std::pair<int, long> arr[] = {{1, 10}, {2, 20}};
  M u(std::sorted_unique, arr, arr + 2, KA(11));
  return alloc_is(s, 9) && alloc_is(t, 10) && alloc_is(u, 11) && t.size() == 2 && u.at(2) == 20;
}

static_assert(test<M>() && test<MM>());
static_assert(sorted_forms());

int main() {
  CHECK(test<M>());
  CHECK(test<MM>());
  CHECK(sorted_forms());
  return 0;
}
