// [array.overview]: array is an aggregate (list-initialization with up to N elements), member
// types, iterators/reverse iterators, size/max_size/empty, element access (operator[], at,
// front, back, data), all usable in constant expressions; deduction guide.
// [array.members]: size() == N; data() == addressof(front()); fill; swap.
// [sequence.reqmts]: at(n) throws out_of_range when n >= size().
#include <array>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

using A = std::array<int, 3>;
static_assert(std::is_aggregate_v<A>);
static_assert(std::is_same_v<A::value_type, int>);
static_assert(std::is_same_v<A::pointer, int*>);
static_assert(std::is_same_v<A::const_pointer, const int*>);
static_assert(std::is_same_v<A::reference, int&>);
static_assert(std::is_same_v<A::const_reference, const int&>);
static_assert(std::is_same_v<A::size_type, std::size_t>);
static_assert(std::is_same_v<A::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<A::reverse_iterator, std::reverse_iterator<A::iterator>>);
static_assert(std::is_same_v<A::const_reverse_iterator, std::reverse_iterator<A::const_iterator>>);
static_assert(std::contiguous_iterator<A::iterator>);
static_assert(std::contiguous_iterator<A::const_iterator>);
static_assert(std::is_same_v<std::iter_reference_t<A::const_iterator>, const int&>);
static_assert(std::is_trivially_copyable_v<A>);
static_assert(noexcept(std::declval<A&>().begin()) && noexcept(std::declval<const A&>().cend()));
static_assert(noexcept(std::declval<A&>().rbegin()) && noexcept(std::declval<const A&>().crend()));
static_assert(noexcept(std::declval<A&>().size()) && noexcept(std::declval<A&>().empty()));
static_assert(noexcept(std::declval<A&>().max_size()) && noexcept(std::declval<A&>().data()));
static_assert(std::is_same_v<decltype(std::declval<const A&>().begin()), A::const_iterator>);
static_assert(std::is_same_v<decltype(std::declval<const A&>().data()), const int*>);
static_assert(std::is_same_v<decltype(std::declval<const A&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const A&>().front()), const int&>);
static_assert(std::is_convertible_v<A::iterator, A::const_iterator>);

// deduction guide
static_assert(std::is_same_v<decltype(std::array{1, 2, 3}), std::array<int, 3>>);
static_assert(std::is_same_v<decltype(std::array{1.0}), std::array<double, 1>>);

constexpr bool test() {
  A a{1, 2, 3};
  if (a.size() != 3 || a.max_size() != 3 || a.empty()) return false;
  if (a[0] != 1 || a.at(2) != 3 || a.front() != 1 || a.back() != 3) return false;
  if (a.data() != &a.front() || a.data() + 2 != &a.back()) return false;
  if (*a.rbegin() != 3 || *(a.rend() - 1) != 1 || *a.crbegin() != 3) return false;
  if (a.end() - a.begin() != 3 || a.cend() - a.cbegin() != 3) return false;
  int sum = 0;
  for (int x : a) sum += x;
  if (sum != 6) return false;
  A partial{7};  // remaining elements value-initialized
  if (partial[0] != 7 || partial[1] != 0 || partial[2] != 0) return false;
  A brace = {{4, 5, 6}};  // brace elision is not required
  if (brace[2] != 6) return false;
  a.fill(9);
  if (a[0] != 9 || a[2] != 9) return false;
  a.swap(brace);
  if (a[0] != 4 || brace[0] != 9) return false;
  std::swap(a, brace);
  if (a[0] != 9) return false;
  a[1] = 42;
  if (a.at(1) != 42) return false;
  const A c{1, 2, 3};
  if (c.at(0) != 1 || *c.begin() != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  A a{};
  bool threw = false;
  try { (void)a.at(3); } catch (const std::out_of_range&) { threw = true; }
  CHECK(threw);
  threw = false;
  const A& ca = a;
  try { (void)ca.at(100); } catch (const std::out_of_range&) { threw = true; }
  CHECK(threw);
  return 0;
}
