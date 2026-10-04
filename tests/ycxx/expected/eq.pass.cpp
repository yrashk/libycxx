// [expected.object.eq]: expected == expected<T2,E2> (T2 not void), expected == T2 (T2 not an
// expected), expected == unexpected<E2>; each constrained on the underlying == being valid
// (C++26: constraints rather than mandates). != is rewritten from ==.
// [expected.void.eq]: expected<void,E> == expected<void,E2> and == unexpected<E2>.
#include <expected>
#include <type_traits>
#include "check.hpp"

struct NoEq {};
template <class A, class B> concept eq = requires(const A& a, const B& b) { a == b; };

static_assert(eq<std::expected<int, long>, std::expected<long, int>>);
static_assert(!eq<std::expected<NoEq, long>, std::expected<NoEq, long>>);
static_assert(!eq<std::expected<int, NoEq>, std::expected<int, NoEq>>);
static_assert(eq<std::expected<int, long>, int>);
static_assert(!eq<std::expected<NoEq, long>, int>);
static_assert(!eq<std::expected<int, long>, NoEq>);
static_assert(eq<std::expected<int, long>, std::unexpected<int>>);
static_assert(!eq<std::expected<int, NoEq>, std::unexpected<NoEq>>);
static_assert(!eq<std::expected<int, long>, std::expected<void, long>>);
static_assert(eq<std::expected<void, long>, std::expected<void, int>>);
static_assert(!eq<std::expected<void, NoEq>, std::expected<void, NoEq>>);
static_assert(eq<std::expected<void, long>, std::unexpected<long>>);

constexpr bool test() {
  using E = std::expected<int, long>;
  E v1(1), v2(2), e1(std::unexpect, 1L), e2(std::unexpect, 2L);
  if (!(v1 == E(1)) || v1 == v2 || !(v1 != v2)) return false;
  if (v1 == e1 || e1 == v1) return false;  // same payload, different state
  if (!(e1 == E(std::unexpect, 1L)) || e1 == e2) return false;
  if (!(v1 == std::expected<long, int>(1L))) return false;
  if (!(v1 == 1) || v1 == 2 || !(1 == v1) || e1 == 1 || !(e1 != 1)) return false;
  if (!(e1 == std::unexpected(1)) || e1 == std::unexpected(2) || v1 == std::unexpected(1)) return false;
  if (!(std::unexpected(1L) == e1)) return false;

  using V = std::expected<void, long>;
  V a, b, c(std::unexpect, 3L), d(std::unexpect, 3L);
  if (!(a == b) || a == c || c == a || !(c == d) || c != d) return false;
  if (!(c == std::unexpected(3)) || a == std::unexpected(3)) return false;
  if (!(a == std::expected<const void, int>())) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
