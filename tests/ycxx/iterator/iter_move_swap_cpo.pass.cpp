// [iterator.cust.move]: ranges::iter_move(E) is iter_move(E) found by ADL if valid; otherwise
// if *E is an lvalue, std::move(*E); otherwise *E. iter_rvalue_reference_t is
// decltype(ranges::iter_move(declval<I&>())). [iterator.cust.swap]: ranges::iter_swap(E1, E2)
// is (void)iter_swap(E1, E2) via ADL; otherwise ranges::swap(*E1, *E2) when both are
// indirectly_readable with swappable references; otherwise an exchange through
// iter-exchange-move. Both are customization point objects (constexpr, SFINAE-friendly).
// COUNTERPART: libcxx:iterators/iterator.requirements/iterator.cust/iterator.cust.swap/iter_swap.pass.cpp
#include <iterator>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace N {
struct It {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  int* moves;
  constexpr int& operator*() const { return *p; }
  constexpr It& operator++() { ++p; return *this; }
  constexpr It operator++(int) { auto t = *this; ++p; return t; }
  friend constexpr int&& iter_move(const It& i) {
    ++*i.moves;
    return std::move(*i.p);
  }
  friend constexpr void iter_swap(const It& a, const It& b) {
    ++*a.moves;
    int t = *a.p;
    *a.p = *b.p;
    *b.p = t;
  }
};
}  // namespace N

struct PrvalueIt {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int v;
  constexpr int operator*() const { return v; }
  constexpr PrvalueIt& operator++() { return *this; }
  constexpr PrvalueIt operator++(int) { return *this; }
};

static_assert(std::is_same_v<decltype(std::ranges::iter_move(std::declval<int*&>())), int&&>);
static_assert(std::is_same_v<decltype(std::ranges::iter_move(std::declval<PrvalueIt&>())), int>);
static_assert(std::is_same_v<std::iter_rvalue_reference_t<const int*>, const int&&>);
static_assert(noexcept(std::ranges::iter_move(std::declval<int*&>())));
static_assert(noexcept(std::ranges::iter_swap(std::declval<int*&>(), std::declval<int*&>())));
template <class A, class B>
concept can_iter_swap = requires(A a, B b) { std::ranges::iter_swap(a, b); };
static_assert(can_iter_swap<int*, int*>);
static_assert(!can_iter_swap<const int*, int*>);
static_assert(can_iter_swap<int*, long*>);  // through the exchange-move fallback

constexpr bool test() {
  int a[2] = {1, 2};
  int moves = 0;
  N::It i{a, &moves}, j{a + 1, &moves};
  int&& r = std::ranges::iter_move(i);
  if (&r != a || moves != 1) return false;
  std::ranges::iter_swap(i, j);
  if (moves != 2 || a[0] != 2 || a[1] != 1) return false;
  int* p = a;
  int* q = a + 1;
  std::ranges::iter_swap(p, q);
  if (a[0] != 1 || a[1] != 2) return false;
  long l = 7;
  long* pl = &l;
  std::ranges::iter_swap(p, pl);
  if (a[0] != 7 || l != 1) return false;
  if (std::ranges::iter_move(PrvalueIt{5}) != 5) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
