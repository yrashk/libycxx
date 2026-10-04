// [sequence.reqmts]/5-7: "X u(n, t);" constructs a sequence container with n copies of t;
// distance(u.begin(), u.end()) == n. /8-10: "X u(i, j);" constructs a sequence container
// equal to the range [i, j) (input or forward iterators whose elements are implicitly
// convertible to value_type); distance == distance(i, j). /11-14: "X(from_range, rg)"
// constructs a container equal to rg for any container-compatible-range<T>. /15: "X(il)" is
// X(il.begin(), il.end()).
#include <vector>
#include <string>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  using S = typename X::size_type;
  {
    const T t = val<T>(7);
    X u(S(5), t);
    if (std::distance(u.begin(), u.end()) != 5 || !holds(u, {7, 7, 7, 7, 7})) return false;
    X z(S(0), t);
    if (!z.empty()) return false;
    X big(S(50), t);
    if (count_elems(big) != 50) return false;
    for (const auto& x : big)
      if (!(x == t)) return false;
  }
  T arr[] = {val<T>(1), val<T>(2), val<T>(3), val<T>(4)};
  {
    X u(arr, arr + 4);  // pointers are random access iterators
    if (!holds(u, {1, 2, 3, 4})) return false;
    X fw(ForwardIter<T>(arr), ForwardIter<T>(arr + 3));
    if (!holds(fw, {1, 2, 3})) return false;
    X in(InputIter<T>(arr + 1), InputIter<T>(arr + 4));
    if (!holds(in, {2, 3, 4})) return false;
    X empty_range(arr, arr);
    if (!empty_range.empty()) return false;
  }
  {
    X r(std::from_range, InputRange<T>{arr, arr + 4});
    if (!holds(r, {1, 2, 3, 4})) return false;
    X f(std::from_range, ForwardRange<T>{arr, arr + 2});
    if (!holds(f, {1, 2})) return false;
    X s(std::from_range, arr);  // sized, contiguous
    if (!holds(s, {1, 2, 3, 4})) return false;
    X other = make<X>({9, 8});
    X c(std::from_range, other);  // a container is a range
    if (!holds(c, {9, 8})) return false;
    X e(std::from_range, std::ranges::subrange(arr, arr));
    if (!e.empty()) return false;
  }
  {
    std::initializer_list<T> il = {val<T>(3), val<T>(1), val<T>(4)};
    X a(il);
    X b(il.begin(), il.end());
    if (!(a == b) || !holds(a, {3, 1, 4})) return false;
    X brace{val<T>(5), val<T>(6)};
    if (!holds(brace, {5, 6})) return false;
  }
  return true;
}

// Elements implicitly convertible to value_type (container-compatible-range only needs
// convertible_to<range_reference_t<R>, T>; the iterator forms need implicit convertibility).
constexpr bool converting() {
  int ints[] = {1, 2, 3};
  std::vector<long> a(ints, ints + 3);
  std::vector<long> b(std::from_range, ints);
  std::vector<Elem> c(ints, ints + 3);
  std::vector<Elem> d(std::from_range, InputRange<int>{ints, ints + 3});
  std::vector<double> e(InputIter<int>(ints), InputIter<int>(ints + 2));
  return a.size() == 3 && a[2] == 3 && b == a && c.size() == 3 && c[1].value() == 2 && d == c &&
         e.size() == 2 && e[1] == 2.0;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::vector<bool>>());
static_assert(test<std::string>());
static_assert(converting());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<bool>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(converting());
  return 0;
}
