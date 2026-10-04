// [array.zero]: array<T, 0> is supported; begin() == end(); swap has a non-throwing exception
// specification (even if T's swap may throw); [array.overview]/3 container requirements:
// empty() is true, size() == 0. T need not be default-constructible.
// [array.special]/1: non-member swap is constrained on "N == 0 or is_swappable_v<T>".
#include <array>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct NoDefault { constexpr NoDefault(int) {} };
struct ThrowingSwap {
  friend void swap(ThrowingSwap&, ThrowingSwap&) noexcept(false) {}
};
struct NotSwappable {
  NotSwappable& operator=(const NotSwappable&) = delete;
};

static_assert(std::is_default_constructible_v<std::array<NoDefault, 0>>);
static_assert(noexcept(std::declval<std::array<ThrowingSwap, 0>&>().swap(std::declval<std::array<ThrowingSwap, 0>&>())));
static_assert(std::is_nothrow_swappable_v<std::array<ThrowingSwap, 0>>);
static_assert(!std::is_nothrow_swappable_v<std::array<ThrowingSwap, 1>>);
static_assert(std::is_swappable_v<std::array<NotSwappable, 0>>);
static_assert(!std::is_swappable_v<std::array<NotSwappable, 1>>);
static_assert(std::is_empty_v<std::array<int, 0>> || sizeof(std::array<int, 0>) > 0);
static_assert(std::tuple_size_v<std::array<int, 0>> == 0);

constexpr bool test() {
  std::array<int, 0> a{};
  if (!a.empty() || a.size() != 0 || a.max_size() != 0) return false;
  if (a.begin() != a.end() || a.cbegin() != a.cend() || a.rbegin() != a.rend()) return false;
  std::array<int, 0> b;
  a.swap(b);
  swap(a, b);
  for (int x : a) { (void)x; return false; }
  std::array<NoDefault, 0> n;
  if (n.begin() != n.end()) return false;
  if (!(a == b) || (a <=> b) != 0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::array<int, 0> a{};
  // begin() == end() == unique value: two different arrays may still compare their own
  // iterators only; data() is unspecified but callable.
  (void)a.data();
  return 0;
}
