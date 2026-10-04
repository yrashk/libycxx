// [const.iterators.iterator], [const.iterators.types], [const.iterators.ops]:
// basic_const_iterator adapts an iterator so that operator* yields
// iter_const_reference_t<Iterator>; member types, operations and comparisons forward to the
// underlying iterator.
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

using CI = std::basic_const_iterator<int*>;

// Member types.
static_assert(std::is_same_v<CI::iterator_type, int*>);
static_assert(std::is_same_v<CI::iterator_concept, std::contiguous_iterator_tag>);
static_assert(std::is_same_v<CI::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<CI::value_type, int>);
static_assert(std::is_same_v<CI::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<std::iter_reference_t<CI>, const int&>);
static_assert(std::is_same_v<std::iter_rvalue_reference_t<CI>, const int&&>);
static_assert(std::contiguous_iterator<CI>);
static_assert(std::is_same_v<decltype(std::declval<const CI&>().operator->()), const int*>);
static_assert(std::is_same_v<decltype(std::declval<const CI&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const CI&>().base()), const int* const&> == false);
static_assert(std::is_same_v<decltype(std::declval<const CI&>().base()), int* const&>);
static_assert(std::is_same_v<decltype(std::declval<CI&&>().base()), int*>);
static_assert(noexcept(std::declval<const CI&>().base()));
static_assert(std::is_same_v<decltype(std::declval<CI&>() <=> std::declval<CI&>()), std::strong_ordering>);
static_assert(std::default_initializable<CI>);

// Input-only underlying iterator.
struct In {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  explicit In(int* q) : p(q) {}             // not default-constructible
  int& operator*() const { return *p; }
  In& operator++() { ++p; return *this; }
  void operator++(int) { ++p; }
  friend bool operator==(const In& i, int* s) { return i.p == s; }
};
static_assert(std::input_iterator<In> && !std::forward_iterator<In>);
using CIn = std::basic_const_iterator<In>;
template <class T> concept HasCategory = requires { typename T::iterator_category; };
static_assert(!HasCategory<CIn>);                  // only present for forward iterators
static_assert(std::is_same_v<CIn::iterator_concept, std::input_iterator_tag>);
static_assert(std::is_same_v<decltype(std::declval<CIn&>()++), void>);
static_assert(!std::default_initializable<CIn>);
static_assert(std::sentinel_for<int*, CIn>);

constexpr bool test_random_access() {
  int a[5] = {10, 20, 30, 40, 50};
  CI it(a);
  CI end = std::make_const_iterator(a + 5);
  if (*it != 10 || it[3] != 40 || it.operator->() != a) return false;
  if (end - it != 5 || it - end != -5) return false;
  ++it;
  if (*it != 20 || it.base() != a + 1) return false;
  CI old = it++;
  if (*old != 20 || *it != 30) return false;
  --it;
  it--;
  if (it.base() != a) return false;
  it += 4;
  if (*it != 50) return false;
  it -= 2;
  if (*it != 30) return false;
  if (*(it + 1) != 40 || *(1 + it) != 40 || *(it - 2) != 10) return false;
  // Comparisons between basic_const_iterators.
  CI b(a + 1);
  if (!(b < it) || !(it > b) || !(b <= b) || !(it >= b) || b == it || !(b != it)) return false;
  if ((b <=> it) != std::strong_ordering::less) return false;
  // Comparisons and differences with the underlying iterator, in both orders.
  int* p = a + 2;
  if (!(it == p) || !(p == it) || it != p) return false;
  if (!(b < p) || !(p > b) || !(p >= b) || !(b <= p)) return false;
  if ((b <=> p) != std::strong_ordering::less) return false;
  if (p - b != 1 || b - p != -1) return false;
  // Conversion to a constant iterator type (operator CI).
  const int* cp = it;
  if (cp != a + 2) return false;
  const int* moved = CI(a + 3);
  if (moved != a + 3) return false;
  // Conversion from basic_const_iterator<int*> to basic_const_iterator<const int*>.
  std::basic_const_iterator<const int*> cc = it;
  if (cc.base() != a + 2) return false;
  // iter_move yields const int&&.
  static_assert(std::is_same_v<decltype(std::ranges::iter_move(it)), const int&&>);
  if (std::ranges::iter_move(it) != 30) return false;
  // Writing through the underlying iterator is visible through the adaptor.
  *it.base() = 99;
  if (*it != 99) return false;
  // Rvalue base() moves out.
  if (std::move(it).base() != a + 2) return false;
  return true;
}
static_assert(test_random_access());

int main() {
  CHECK(test_random_access());
  int a[3] = {1, 2, 3};
  CIn i{In{a}};
  int sum = 0;
  for (; i != a + 3; ++i) sum += *i;                // sentinel comparison
  CHECK(sum == 6);
  static_assert(std::is_same_v<decltype(*i), const int&>);
  CIn j{In{a}};
  j++;
  CHECK(*j == 2);
  return 0;
}
