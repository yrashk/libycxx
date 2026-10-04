// [span.cons]/3-7: template<class It> constexpr explicit(extent != dynamic_extent)
// span(It first, size_type count); "Constraints: Let U be
// remove_reference_t<iter_reference_t<It>>. It satisfies contiguous_iterator.
// is_convertible_v<U(*)[], element_type(*)[]> is true." "Effects: Initializes data_ with
// to_address(first) and size_ with count."
#include <span>
#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include "check.hpp"

struct Base {};
struct Derived : Base {};

// A bidirectional, non-contiguous iterator.
struct BidiIt {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  int& operator*() const;
  BidiIt& operator++();
  BidiIt operator++(int);
  BidiIt& operator--();
  BidiIt operator--(int);
  bool operator==(const BidiIt&) const;
};
static_assert(std::bidirectional_iterator<BidiIt>);

using sz = std::size_t;
static_assert(std::is_constructible_v<std::span<int>, int*, sz>);
static_assert(std::is_constructible_v<std::span<const int>, int*, sz>);
static_assert(std::is_constructible_v<std::span<const int>, const int*, sz>);
static_assert(!std::is_constructible_v<std::span<int>, const int*, sz>);
static_assert(!std::is_constructible_v<std::span<long>, int*, sz>);
static_assert(!std::is_constructible_v<std::span<unsigned>, int*, sz>);
static_assert(!std::is_constructible_v<std::span<Base>, Derived*, sz>);
static_assert(!std::is_constructible_v<std::span<int>, BidiIt, sz>);
static_assert(std::is_constructible_v<std::span<int>, std::array<int, 3>::iterator, sz>);
static_assert(std::is_constructible_v<std::span<const int>, std::array<int, 3>::const_iterator, sz>);
static_assert(!std::is_constructible_v<std::span<int>, std::array<int, 3>::const_iterator, sz>);
static_assert(std::is_constructible_v<std::span<int, 3>, int*, sz>);

// explicit(extent != dynamic_extent)
template <class S>
void take(S);
template <class S, class... Args>
concept implicitly = requires(Args... args) { take<S>({args...}); };
static_assert(implicitly<std::span<int>, int*, sz>);
static_assert(!implicitly<std::span<int, 3>, int*, sz>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  std::span<int> s(a + 1, 3);
  if (s.data() != a + 1 || s.size() != 3 || s[0] != 2) return false;
  std::span<const int, 2> f(a, 2);
  if (f.data() != a || f.size() != 2) return false;
  std::array<int, 4> arr{6, 7, 8, 9};
  std::span<int> t(arr.begin() + 1, 2);  // to_address of an iterator
  if (t.data() != arr.data() + 1 || t.size() != 2 || t[1] != 8) return false;
  std::span<int> e(a, 0);
  if (!e.empty() || e.data() != a) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
