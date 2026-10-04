// [default.sentinel]: "struct default_sentinel_t { }; inline constexpr default_sentinel_t
// default_sentinel{};" [unreachable.sentinel]: unreachable_sentinel_t has "template<
// weakly_incrementable I> friend constexpr bool operator==(unreachable_sentinel_t, const I&)
// noexcept { return false; }" and inline constexpr unreachable_sentinel.
#include <iterator>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_empty_v<std::default_sentinel_t> && std::is_empty_v<std::unreachable_sentinel_t>);
static_assert(std::is_same_v<decltype(std::default_sentinel), const std::default_sentinel_t>);
static_assert(std::is_same_v<decltype(std::unreachable_sentinel), const std::unreachable_sentinel_t>);
static_assert(std::semiregular<std::default_sentinel_t> && std::semiregular<std::unreachable_sentinel_t>);
static_assert(std::sentinel_for<std::unreachable_sentinel_t, int*>);
static_assert(!std::sized_sentinel_for<std::unreachable_sentinel_t, int*>);
static_assert(noexcept(std::unreachable_sentinel == static_cast<int*>(nullptr)));
static_assert(noexcept(static_cast<int*>(nullptr) != std::unreachable_sentinel));

struct NotIncrementable {};
template <class T>
concept cmp_unreachable = requires(T t) { std::unreachable_sentinel == t; };
static_assert(cmp_unreachable<int*>);
static_assert(cmp_unreachable<long>);  // integers are weakly_incrementable
static_assert(!cmp_unreachable<NotIncrementable>);

constexpr bool test() {
  int a[3] = {4, 5, 6};
  int* p = a;
  int n = 0;
  while (p != std::unreachable_sentinel && *p != 6) {
    ++p;
    ++n;
  }
  if (n != 2) return false;
  if (std::unreachable_sentinel == a + 0 || a + 0 == std::unreachable_sentinel) return false;
  if (std::unreachable_sentinel == 0) return false;
  std::counted_iterator<int*> c(a, 0);
  return c == std::default_sentinel;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
