// [span.cons]/22-26: template<class OtherElementType, size_t OtherExtent> constexpr
// explicit(see below) span(const span<OtherElementType, OtherExtent>& s) noexcept;
// "Constraints: extent == dynamic_extent || OtherExtent == dynamic_extent ||
// extent == OtherExtent is true, and is_convertible_v<OtherElementType(*)[],
// element_type(*)[]> is true." "Remarks: The expression inside explicit is equivalent to:
// extent != dynamic_extent && OtherExtent == dynamic_extent".
#include <span>
#include <type_traits>
#include "check.hpp"

constexpr auto dyn = std::dynamic_extent;
struct Base {};
struct Derived : Base {};

static_assert(std::is_nothrow_constructible_v<std::span<const int>, std::span<int>>);
static_assert(std::is_convertible_v<std::span<int>, std::span<const int>>);
static_assert(std::is_convertible_v<std::span<int, 3>, std::span<const int>>);
static_assert(std::is_convertible_v<std::span<int, 3>, std::span<const int, 3>>);
static_assert(std::is_convertible_v<std::span<int, 3>, std::span<volatile int, 3>>);
static_assert(std::is_constructible_v<std::span<int, 3>, std::span<int>>);
static_assert(!std::is_convertible_v<std::span<int>, std::span<int, 3>>);  // explicit
static_assert(!std::is_constructible_v<std::span<int, 3>, std::span<int, 4>>);
static_assert(!std::is_constructible_v<std::span<int>, std::span<const int>>);
static_assert(!std::is_constructible_v<std::span<long>, std::span<int>>);
static_assert(!std::is_constructible_v<std::span<Base>, std::span<Derived>>);
static_assert(!std::is_constructible_v<std::span<int, 0>, std::span<int, 1>>);
static_assert(std::is_convertible_v<std::span<int, 0>, std::span<const int, 0>>);
static_assert(dyn == std::span<int>::extent);

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  std::span<int, 4> s4(a);
  std::span<const int> d = s4;
  if (d.data() != a || d.size() != 4) return false;
  std::span<const int, 4> back(d);
  if (back.data() != a || back.size() != 4) return false;
  std::span<int> dd(a, 2);
  std::span<int, 2> ss(dd);
  if (ss.data() != a || ss.size() != 2) return false;
  // copy and assignment
  std::span<int> x(a + 1, 2);
  std::span<int> y;
  y = x;
  if (y.data() != a + 1 || y.size() != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
