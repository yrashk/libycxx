// [span.cons]/13-15: span(type_identity_t<element_type> (&arr)[N]) noexcept,
// span(array<T, N>&) noexcept, span(const array<T, N>&) noexcept. "Constraints: Let U be
// remove_pointer_t<decltype(std::data(arr))>. extent == dynamic_extent || N == extent is true,
// and is_convertible_v<U(*)[], element_type(*)[]> is true." "Postconditions: size() == N &&
// data() == std::data(arr) is true." These constructors are not explicit.
#include <span>
#include <array>
#include <type_traits>
#include "check.hpp"

struct Base {};
struct Derived : Base {};

static_assert(std::is_nothrow_constructible_v<std::span<int>, int (&)[3]>);
static_assert(std::is_nothrow_constructible_v<std::span<int, 3>, int (&)[3]>);
static_assert(std::is_convertible_v<int (&)[3], std::span<int, 3>>);
static_assert(std::is_convertible_v<int (&)[3], std::span<const int>>);
static_assert(!std::is_constructible_v<std::span<int, 2>, int (&)[3]>);
static_assert(!std::is_constructible_v<std::span<int, 4>, int (&)[3]>);
static_assert(!std::is_constructible_v<std::span<int>, const int (&)[3]>);
static_assert(!std::is_constructible_v<std::span<long>, int (&)[3]>);
static_assert(!std::is_constructible_v<std::span<Base>, Derived (&)[3]>);

static_assert(std::is_nothrow_constructible_v<std::span<int>, std::array<int, 3>&>);
static_assert(std::is_convertible_v<std::array<int, 3>&, std::span<int, 3>>);
static_assert(std::is_convertible_v<const std::array<int, 3>&, std::span<const int, 3>>);
static_assert(!std::is_constructible_v<std::span<int>, const std::array<int, 3>&>);
static_assert(!std::is_constructible_v<std::span<int, 2>, std::array<int, 3>&>);
static_assert(std::is_convertible_v<std::array<int, 0>&, std::span<int, 0>>);
static_assert(std::is_convertible_v<std::array<const int, 2>&, std::span<const int, 2>>);
static_assert(!std::is_constructible_v<std::span<int, 2>, std::array<const int, 2>&>);
static_assert(!std::is_constructible_v<std::span<Base>, std::array<Derived, 2>&>);
// an rvalue array binds to the const array<T, N>& overload (span of const only); the range
// constructor rejects specializations of array, so span<int> cannot view a temporary.
static_assert(std::is_constructible_v<std::span<const int>, std::array<int, 2>&&>);
static_assert(std::is_constructible_v<std::span<const int, 2>, std::array<int, 2>&&>);
static_assert(!std::is_constructible_v<std::span<int>, std::array<int, 2>&&>);

constexpr bool test() {
  int a[3] = {1, 2, 3};
  std::span<int> s = a;
  if (s.size() != 3 || s.data() != a) return false;
  std::span<int, 3> f = a;
  if (f.size() != 3 || f.data() != a) return false;
  std::span<const int> c = a;
  if (c.data() != a) return false;

  std::array<int, 2> arr{4, 5};
  std::span<int, 2> sa = arr;
  if (sa.data() != arr.data() || sa.size() != 2) return false;
  const std::array<int, 2>& carr = arr;
  std::span<const int> sc = carr;
  if (sc.data() != arr.data() || sc.size() != 2) return false;
  std::array<int, 0> zero{};
  std::span<int, 0> sz = zero;
  if (sz.size() != 0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
