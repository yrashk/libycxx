// [span.obs]: size(), size_bytes() == size() * sizeof(element_type), empty() (all noexcept).
// [span.elem]: operator[] returns *(data() + idx); at(idx) (C++26) "Throws: out_of_range if
// idx >= size() is true."; front(), back(), data() noexcept. All return reference, i.e.
// element_type&, even on a const span.
// REQUIRES: exceptions
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

using S = std::span<int>;
static_assert(noexcept(std::declval<const S&>().size()));
static_assert(noexcept(std::declval<const S&>().size_bytes()));
static_assert(noexcept(std::declval<const S&>().empty()));
static_assert(noexcept(std::declval<const S&>().data()));
static_assert(std::is_same_v<decltype(std::declval<const S&>()[0]), int&>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().at(0)), int&>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().front()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().back()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().data()), int*>);
static_assert(std::is_same_v<decltype(std::declval<std::span<const int, 2>&>()[0]), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().size()), std::size_t>);
static_assert(std::is_same_v<decltype(std::declval<const S&>().empty()), bool>);

constexpr bool test() {
  long a[3] = {10, 20, 30};
  const std::span<long> s(a);
  if (s.size() != 3 || s.size_bytes() != 3 * sizeof(long) || s.empty()) return false;
  if (s[1] != 20 || &s[2] != a + 2) return false;
  s[0] = 11;  // a const span still gives mutable access
  if (a[0] != 11) return false;
  if (&s.front() != a || &s.back() != a + 2) return false;
  if (&s.at(1) != a + 1) return false;
  std::span<long, 0> z;
  if (!z.empty() || z.size_bytes() != 0) return false;
  std::span<const char, 2> c("ab", 2);
  if (c.size_bytes() != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[2] = {1, 2};
  std::span<int> s(a);
  bool caught = false;
  try {
    (void)s.at(2);
  } catch (const std::out_of_range&) {
    caught = true;
  }
  CHECK(caught);
  caught = false;
  try {
    (void)std::span<int>().at(0);
  } catch (const std::out_of_range&) {
    caught = true;
  }
  CHECK(caught);
  CHECK(s.at(1) == 2);
  return 0;
}
