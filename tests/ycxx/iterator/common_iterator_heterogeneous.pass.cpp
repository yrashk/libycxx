// [common.iter.const]/3-4: common_iterator<I, S>(const common_iterator<I2, S2>& x) keeps x's
// alternative (iterator or sentinel); /5-7: the converting assignment assigns when the
// alternatives match and emplaces otherwise. [common.iter.cmp]/1-2: for I and I2 not
// equality_comparable_with, x == y is "true if i == j, and otherwise get<i>(x.v_) ==
// get<j>(y.v_)" (two iterator states always compare equal); /3-4: when they are, "true if i
// and j are each 1, and otherwise get<i>(x.v_) == get<j>(y.v_)"; /5-6: x - y is "0 if i and j
// are each 1, and otherwise get<i>(x.v_) - get<j>(y.v_)" with type iter_difference_t<I2>.
// [common.iter.cust]/3-4: iter_swap(x, y) for common_iterators of different iterator types
// swaps the pointed-to elements; /1-2: iter_move is ranges::iter_move of the iterator.
#include <iterator>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct End {
  const int* e;
  friend constexpr bool operator==(const int* p, End s) { return p == s.e; }
  friend constexpr std::ptrdiff_t operator-(End s, const int* p) { return s.e - p; }
  friend constexpr std::ptrdiff_t operator-(const int* p, End s) { return p - s.e; }
};

using C = std::common_iterator<const int*, End>;
using M = std::common_iterator<int*, End>;
static_assert(std::is_constructible_v<C, const M&> && !std::is_constructible_v<M, const C&>);
static_assert(std::is_assignable_v<C&, const M&>);

// Input iterators that are not equality comparable with each other.
struct In {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  const int* p;
  constexpr const int& operator*() const { return *p; }
  constexpr In& operator++() { ++p; return *this; }
  constexpr void operator++(int) { ++p; }
};
struct In2 {  // a different type, not ==-comparable with In
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  const int* p;
  constexpr const int& operator*() const { return *p; }
  constexpr In2& operator++() { ++p; return *this; }
  constexpr void operator++(int) { ++p; }
};
struct InEnd {
  const int* e;
  friend constexpr bool operator==(const In& i, InEnd s) { return i.p == s.e; }
  friend constexpr bool operator==(const In2& i, InEnd s) { return i.p == s.e; }
};
static_assert(!std::equality_comparable_with<In, In2>);
static_assert(std::sentinel_for<InEnd, In2>);

constexpr bool run() {
  int a[5] = {1, 2, 3, 4, 5};
  M mi(a + 1);
  M ms(End{a + 5});
  C ci = mi;
  C cs = ms;
  if (ci == cs || !(cs == C(End{a + 5})) || *ci != 2) return false;
  // Heterogeneous == and -.
  if (!(ci == mi) || !(cs == ms) || ci == ms || cs == mi) return false;
  if (!(ms == cs) || !(M(a + 5) == cs) || !(cs == M(a + 5))) return false;
  if (cs - mi != 4 || mi - cs != -4 || ci - M(a + 3) != -2) return false;
  if (cs - ms != 0 || ms - cs != 0) return false;  // two sentinels
  static_assert(std::is_same_v<decltype(ci - mi), std::iter_difference_t<int*>>);
  // Converting assignment across alternatives.
  C x(End{a + 5});
  x = mi;  // sentinel -> iterator: emplace
  if (x != ci || *x != 2) return false;
  x = M(a + 2);  // iterator -> iterator: assign
  if (*x != 3) return false;
  x = ms;  // iterator -> sentinel
  if (!(x == cs) || x - ci != 4) return false;
  // iter_swap between common_iterator<int*, End> objects.
  M p(a), q(a + 4);
  std::ranges::iter_swap(p, q);
  if (a[0] != 5 || a[4] != 1) return false;
  if (std::ranges::iter_move(ci) != 2) return false;
  // Input iterators: two iterator states compare equal whatever they point to.
  std::common_iterator<In, InEnd> u(In{a}), ue(InEnd{a + 5});
  std::common_iterator<In2, InEnd> v(In2{a + 3});
  if (!(u == v) || !(v == u)) return false;
  if (u == ue || !(std::common_iterator<In2, InEnd>(InEnd{a}) == ue)) return false;
  std::common_iterator<In2, InEnd> at_end(In2{a + 5});
  if (!(at_end == ue) || !(ue == at_end)) return false;  // iterator vs sentinel uses In == InEnd
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
