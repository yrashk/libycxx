// [expected.object.assign]/18-21: emplace is noexcept, constrained on
// is_nothrow_constructible_v<T, Args...>; destroys the value or error, constructs the value,
// returns T&.
#include <expected>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

struct IL {
  int sum = 0;
  constexpr IL(std::initializer_list<int> il) noexcept { for (int x : il) sum += x; }
};
struct Throwing { Throwing(int) noexcept(false) {} };

template <class E, class... A> concept can_emplace = requires(E e, A... a) { e.emplace(a...); };
static_assert(can_emplace<std::expected<int, long>, int>);
static_assert(!can_emplace<std::expected<Throwing, long>, int>);
static_assert(noexcept(std::declval<std::expected<int, long>&>().emplace(1)));
static_assert(std::is_same_v<decltype(std::declval<std::expected<int, long>&>().emplace(1)), int&>);

constexpr bool test() {
  std::expected<int, long> e(std::unexpect, 3L);
  int& r = e.emplace(4);
  if (!e.has_value() || &r != &*e || *e != 4) return false;
  e.emplace(5);
  if (*e != 5) return false;
  std::expected<IL, int> il(std::unexpect, 1);
  if (il.emplace({1, 2, 3}).sum != 6 || !il) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
