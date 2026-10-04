// [span.cons]/1-2: constexpr span() noexcept; "Constraints: Extent == dynamic_extent ||
// Extent == 0 is true." "Postconditions: size() == 0 && data() == nullptr."
#include <span>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::span<int>>);
static_assert(std::is_nothrow_default_constructible_v<std::span<int, 0>>);
static_assert(!std::is_default_constructible_v<std::span<int, 1>>);
static_assert(!std::is_default_constructible_v<std::span<const int, 5>>);

constexpr bool test() {
  std::span<int> a;
  std::span<const int, 0> b;
  std::span<long> c{};
  return a.size() == 0 && a.data() == nullptr && b.size() == 0 && b.data() == nullptr && c.empty() &&
         c.data() == nullptr && a.begin() == a.end();
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
