// [utility.underlying]: to_underlying(value) returns
// static_cast<underlying_type_t<T>>(value); constexpr and noexcept ([utility.syn]).
#include <climits>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "check.hpp"

enum class Byte : unsigned char { a = 200, b = 255 };
enum Signed : short { m = -5, n = SHRT_MIN };
enum class Big : unsigned long long { top = ULLONG_MAX };
enum class Default { x = 7 };   // scoped default: int
enum Plain { p0, p1 = 42 };    // unspecified underlying type
enum class Bool : bool { f, t };
enum class Ch : char32_t { c = U'z' };

static_assert(std::is_same_v<decltype(std::to_underlying(Byte::a)), unsigned char>);
static_assert(std::is_same_v<decltype(std::to_underlying(m)), short>);
static_assert(std::is_same_v<decltype(std::to_underlying(Big::top)), unsigned long long>);
static_assert(std::is_same_v<decltype(std::to_underlying(Default::x)), int>);
static_assert(std::is_same_v<decltype(std::to_underlying(p1)), std::underlying_type_t<Plain>>);
static_assert(std::is_same_v<decltype(std::to_underlying(Bool::t)), bool>);
static_assert(std::is_same_v<decltype(std::to_underlying(Ch::c)), char32_t>);
static_assert(noexcept(std::to_underlying(Byte::a)));

template <class T> concept CanToUnderlying = requires(T t) { std::to_underlying(t); };
static_assert(CanToUnderlying<Byte> && CanToUnderlying<Plain>);

constexpr bool test() {
  if (std::to_underlying(Byte::a) != 200 || std::to_underlying(Byte::b) != 255) return false;
  if (std::to_underlying(m) != -5 || std::to_underlying(n) != SHRT_MIN) return false;
  if (std::to_underlying(Big::top) != ULLONG_MAX) return false;
  if (std::to_underlying(Default::x) != 7) return false;
  if (std::to_underlying(p1) != 42) return false;
  if (std::to_underlying(Bool::t) != true) return false;
  if (std::to_underlying(Ch::c) != U'z') return false;
  if (std::to_underlying(static_cast<Byte>(17)) != 17) return false;   // value with no enumerator
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
