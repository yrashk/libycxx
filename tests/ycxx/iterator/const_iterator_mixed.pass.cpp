// [const.iterators.ops]/17-26: basic_const_iterator compares (<, >, <=, >=, <=>) and
// subtracts with a different iterator type I in both orders when totally_ordered_with /
// sized_sentinel_for: members /19-20 "return current_ op y", hidden friends /21-22 "return x
// op y.current_", /25 "current_ - y", /26 "x - y.current_". Exercised with
// vector<int>::const_iterator against basic_const_iterator<vector<int>::iterator>, and with
// reverse_iterator<int*> against basic_const_iterator<reverse_iterator<int*>> (where the
// ordering is reversed, [reverse.iter.cmp]).
// /15-16: operator CI() converts only to constant iterators ([const.iterators.alias]:
// constant-iterator<CI>), so basic_const_iterator<int*> converts to const int* but not int*.
// /7: operator-> requires an lvalue reference whose type is value_type; for a non-contiguous
// iterator it is addressof(*current_). [const.iterators.iterator]: reference is
// iter_const_reference_t<Iterator> and iter_move yields
// common_reference_t<const iter_value_t<I>&&, iter_rvalue_reference_t<I>>, also for a proxy
// reference such as zip's tuple<int&, int&>.
// [reverse.iter.cmp]/1-12: reverse_iterator<I1> and reverse_iterator<I2> compare by their
// bases with the order reversed (x < y is x.base() > y.base()); [reverse.iter.nonmember]/1:
// x - y is y.base() - x.base(); [reverse.iter.cons]/3: reverse_iterator<const_iterator> is
// constructible from reverse_iterator<iterator>.
#include <iterator>
#include <compare>
#include <deque>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

template <class From, class To>
concept convertible = std::is_convertible_v<From, To>;

static_assert(convertible<std::basic_const_iterator<int*>, const int*>);
static_assert(!convertible<std::basic_const_iterator<int*>, int*>);
static_assert(!std::is_constructible_v<int*, std::basic_const_iterator<int*>>);

template <class T>
concept has_arrow = requires(const T& t) { t.operator->(); };

constexpr bool vec() {
  std::vector<int> v{1, 2, 3, 4, 5};
  using It = std::vector<int>::iterator;
  using CIt = std::vector<int>::const_iterator;
  std::basic_const_iterator<It> b(v.begin() + 1);
  CIt c = v.cbegin() + 3;
  if (!(b < c) || !(c > b) || !(b <= c) || !(c >= b) || b > c || c < b) return false;
  if ((b <=> c) != std::strong_ordering::less || (c <=> b) != std::strong_ordering::greater) return false;
  if (b - c != -2 || c - b != 2) return false;
  if (b == c || !(b == v.cbegin() + 1) || !(v.cbegin() + 1 == b)) return false;
  if (!(b == v.begin() + 1) || b - v.begin() != 1 || v.end() - b != 4) return false;
  CIt conv = b;  // operator CI: vector's const_iterator is a constant iterator
  if (conv != v.cbegin() + 1) return false;
  static_assert(std::is_same_v<std::const_iterator<It>, std::basic_const_iterator<It>>);
  static_assert(std::is_same_v<std::const_iterator<CIt>, CIt>);
  return true;
}
static_assert(vec());

constexpr bool rev() {
  int a[5] = {1, 2, 3, 4, 5};
  using R = std::reverse_iterator<int*>;
  R r0(a + 5);                                  // *r0 == 5
  std::basic_const_iterator<R> c(R(a + 3));     // *c == 3, two steps after r0
  if (*c != 3 || *r0 != 5) return false;
  if (!(r0 < c) || !(c > r0) || (c <=> r0) != std::strong_ordering::greater) return false;
  if (c - r0 != 2 || r0 - c != -2) return false;
  using CR = std::reverse_iterator<const int*>;
  CR cr = r0;  // converting construction
  if (!(cr == r0) || !(r0 < cr + 1) || (cr + 2) - r0 != 2 || r0 - (cr + 2) != -2) return false;
  if ((r0 <=> cr + 1) != std::strong_ordering::less) return false;
  // reverse_iterator of a basic_const_iterator.
  std::reverse_iterator<std::basic_const_iterator<int*>> rc(std::basic_const_iterator<int*>(a + 2));
  static_assert(std::is_same_v<decltype(*rc), const int&>);
  if (*rc != 2 || rc[1] != 1 || rc.operator->() != a + 1) return false;
  return true;
}
static_assert(rev());

int main() {
  CHECK(vec() && rev());
  // Non-contiguous operator->.
  std::deque<int> d{4, 5, 6};
  std::basic_const_iterator<std::deque<int>::iterator> di(d.begin() + 1);
  static_assert(std::is_same_v<decltype(di.operator->()), const int*>);
  CHECK(di.operator->() == &d[1]);

  // Proxy references (zip).
  std::vector<int> x{1, 2}, y{3, 4};
  auto z = std::views::zip(x, y);
  using ZI = std::ranges::iterator_t<decltype(z)>;
  std::basic_const_iterator<ZI> zi(z.begin());
  static_assert(std::is_same_v<std::iter_reference_t<decltype(zi)>, std::tuple<const int&, const int&>>);
  static_assert(std::is_same_v<std::iter_rvalue_reference_t<decltype(zi)>, std::tuple<const int&&, const int&&>>);
  static_assert(!has_arrow<decltype(zi)>);
  auto [p, q] = *zi;
  CHECK(p == 1 && q == 3 && &p == &x[0]);
  auto&& [mp, mq] = std::ranges::iter_move(zi);
  CHECK(mp == 1 && mq == 3);
  ++zi;
  CHECK(zi == z.begin() + 1 && z.begin() + 1 == zi && zi - z.begin() == 1 && z.end() - zi == 1);
  CHECK(z.begin() < zi && zi > z.begin());
  return 0;
}
