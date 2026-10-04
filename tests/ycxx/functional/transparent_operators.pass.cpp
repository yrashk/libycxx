// [arithmetic.operations], [comparisons], [logical.operations], [bitwise.operations]: the
// T = void specializations are transparent: "template<class T, class U> constexpr auto
// operator()(T&& t, U&& u) const -> decltype(std::forward<T>(t) + std::forward<U>(u));" and
// "using is_transparent = unspecified;". The primary templates take const T& and return T
// (arithmetic/bitwise) or bool (comparisons/logical). T defaults to void.
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Meters {
  int v;
  constexpr Meters operator+(int x) const { return {v + x}; }
};
struct NoPlus {};

template <class F>
concept transparent = requires { typename F::is_transparent; };

template <class F, class... A>
concept callable = requires(F f, A&&... a) { f(std::forward<A>(a)...); };

static_assert(std::is_same_v<std::plus<>, std::plus<void>>);
static_assert(std::is_same_v<std::less<>, std::less<void>>);
static_assert(transparent<std::plus<>>);
static_assert(transparent<std::minus<>>);
static_assert(transparent<std::multiplies<>>);
static_assert(transparent<std::divides<>>);
static_assert(transparent<std::modulus<>>);
static_assert(transparent<std::negate<>>);
static_assert(transparent<std::equal_to<>>);
static_assert(transparent<std::not_equal_to<>>);
static_assert(transparent<std::less<>>);
static_assert(transparent<std::greater<>>);
static_assert(transparent<std::less_equal<>>);
static_assert(transparent<std::greater_equal<>>);
static_assert(transparent<std::logical_and<>>);
static_assert(transparent<std::logical_or<>>);
static_assert(transparent<std::logical_not<>>);
static_assert(transparent<std::bit_and<>>);
static_assert(transparent<std::bit_or<>>);
static_assert(transparent<std::bit_xor<>>);
static_assert(transparent<std::bit_not<>>);
static_assert(!transparent<std::plus<int>>);
static_assert(!transparent<std::less<int>>);

// return types: the transparent forms deduce, the typed forms return T / bool
static_assert(std::is_same_v<decltype(std::plus<>{}(1, 2.0)), double>);
static_assert(std::is_same_v<decltype(std::plus<>{}(Meters{1}, 2)), Meters>);
static_assert(std::is_same_v<decltype(std::plus<int>{}(1, 2)), int>);
static_assert(std::is_same_v<decltype(std::less<int>{}(1, 2)), bool>);
static_assert(std::is_same_v<decltype(std::bit_not<unsigned char>{}(1)), unsigned char>);
static_assert(std::is_same_v<decltype(std::bit_not<>{}(static_cast<unsigned char>(1))), int>);
static_assert(std::is_same_v<decltype(std::negate<>{}(1u)), unsigned>);
static_assert(std::is_same_v<decltype(std::logical_not<int>{}(1)), bool>);

// SFINAE-friendly
static_assert(callable<std::plus<>, int, int>);
static_assert(!callable<std::plus<>, NoPlus, int>);
static_assert(!callable<std::less<>, NoPlus, NoPlus>);
static_assert(!callable<std::negate<>, NoPlus>);

// perfect forwarding: lvalue result types are preserved
struct Shift {
  int v;
  constexpr int& operator&(int) { return v; }
};
static_assert(std::is_same_v<decltype(std::bit_and<>{}(std::declval<Shift&>(), 1)), int&>);

constexpr bool test() {
  if (std::plus<>{}(1, 2) != 3 || std::minus<>{}(5, 7) != -2 || std::multiplies<>{}(3, 4) != 12) return false;
  if (std::divides<>{}(7, 2) != 3 || std::modulus<>{}(7, 3) != 1 || std::negate<>{}(4) != -4) return false;
  if (!std::equal_to<>{}(1, 1L) || std::not_equal_to<>{}(2, 2) || !std::less<>{}(1, 2.5)) return false;
  if (!std::greater<>{}(3, 2) || !std::less_equal<>{}(2, 2) || !std::greater_equal<>{}(2, 2)) return false;
  if (!std::logical_and<>{}(1, true) || std::logical_or<>{}(0, false) || !std::logical_not<>{}(0)) return false;
  if (std::bit_and<>{}(6, 3) != 2 || std::bit_or<>{}(6, 3) != 7 || std::bit_xor<>{}(6, 3) != 5) return false;
  if (std::bit_not<>{}(0) != -1 || std::bit_not<unsigned char>{}(0) != 0xFF) return false;
  if (std::plus<long>{}(1, 2) != 3 || std::modulus<int>{}(9, 4) != 1) return false;
  // comparison of unsigned and negative uses the built-in conversion (no magic)
  if (std::less<>{}(-1, 0u)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
