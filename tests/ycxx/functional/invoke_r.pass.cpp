// [func.invoke]/4-6: template<class R, class F, class... Args> constexpr R invoke_r(F&& f,
// Args&&... args) noexcept(is_nothrow_invocable_r_v<R, F, Args...>); "Constraints:
// is_invocable_r_v<R, F, Args...> is true." "Returns: INVOKE<R>(...)". [func.require]/2:
// INVOKE<R> is static_cast<void>(INVOKE(...)) if R is cv void, otherwise INVOKE(...)
// implicitly converted to R; "If reference_converts_from_temporary_v<R,
// decltype(INVOKE(f, t1, t2, ..., tN))> is true, INVOKE<R>(f, t1, t2, ..., tN) is ill-formed."
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

constexpr long get_long() { return 7; }
int global_x = 3;
constexpr int& get_ref() { return global_x; }
struct Explicit {
  explicit Explicit(int) {}
};
int throws_maybe() { return 1; }
constexpr int no_throw() noexcept { return 1; }
struct ConvThrows {
  ConvThrows(int) {}
};

template <class R, class... A>
concept can_invoke_r = requires(A&&... a) { std::invoke_r<R>(std::forward<A>(a)...); };

static_assert(can_invoke_r<long, int (*)()>);
static_assert(can_invoke_r<void, int (*)()>);
static_assert(can_invoke_r<const void, int (*)()>);
static_assert(!can_invoke_r<int*, int (*)()>);
static_assert(!can_invoke_r<Explicit, int (*)()>);  // implicit conversion only
static_assert(!can_invoke_r<const int&, long (*)()>);  // would bind to a temporary
static_assert(can_invoke_r<const int&, int& (*)()>);
static_assert(std::is_same_v<decltype(std::invoke_r<double>(get_long)), double>);
static_assert(std::is_same_v<decltype(std::invoke_r<void>(get_long)), void>);
static_assert(noexcept(std::invoke_r<long>(no_throw)));
static_assert(!noexcept(std::invoke_r<long>(throws_maybe)));
static_assert(!noexcept(std::invoke_r<ConvThrows>(no_throw)));  // the conversion may throw

constexpr bool test() {
  if (std::invoke_r<int>(get_long) != 7) return false;
  std::invoke_r<void>(get_long);
  auto plus = [](int a, int b) { return a + b; };
  if (std::invoke_r<double>(plus, 1, 2) != 3.0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int& r = std::invoke_r<int&>(get_ref);
  CHECK(&r == &global_x);
  return 0;
}
