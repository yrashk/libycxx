// [func.identity]: struct identity { template<class T> constexpr T&& operator()(T&& t) const
// noexcept; using is_transparent = unspecified; }; "Effects: Equivalent to: return
// std::forward<T>(t);"
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Big {
  int v;
  Big(const Big&) = delete;
};

template <class F>
concept transparent = requires { typename F::is_transparent; };
static_assert(transparent<std::identity>);
static_assert(std::is_same_v<decltype(std::identity{}(1)), int&&>);
static_assert(std::is_same_v<decltype(std::identity{}(std::declval<int&>())), int&>);
static_assert(std::is_same_v<decltype(std::identity{}(std::declval<const int&>())), const int&>);
static_assert(std::is_same_v<decltype(std::identity{}(std::declval<Big&&>())), Big&&>);
static_assert(noexcept(std::identity{}(1)));
static_assert(std::is_trivially_default_constructible_v<std::identity>);
static_assert(std::is_empty_v<std::identity>);

constexpr bool test() {
  int x = 4;
  int& r = std::identity{}(x);
  if (&r != &x) return false;
  const std::identity id;
  if (id(5) != 5) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
