// [numeric.sat.func]: saturating_add / saturating_sub / saturating_mul return the
// mathematical result if it is representable in T, otherwise the largest or smallest
// value of T, whichever is closer. saturating_div returns x / y, except numeric_limits<T>::
// min() / -1 gives max() for signed T. [numeric.sat.cast]: saturating_cast<R>(x) returns x
// if representable in R, otherwise the closest bound. All are constexpr and noexcept, and
// constrained to signed or unsigned integer types (not bool, not character types).
#include <numeric>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "check.hpp"

template <class T>
concept has_sat = requires(T x) {
  std::saturating_add(x, x);
  std::saturating_sub(x, x);
  std::saturating_mul(x, x);
  std::saturating_div(x, x);
};
static_assert(has_sat<int> && has_sat<unsigned> && has_sat<signed char> && has_sat<unsigned long long>);
static_assert(!has_sat<bool>);
static_assert(!has_sat<char>);
static_assert(!has_sat<char8_t> && !has_sat<char16_t> && !has_sat<char32_t> && !has_sat<wchar_t>);
static_assert(!has_sat<double>);
template <class R, class T>
concept has_cast = requires(T x) { std::saturating_cast<R>(x); };
static_assert(has_cast<int, long> && has_cast<unsigned char, int>);
static_assert(!has_cast<bool, int> && !has_cast<int, bool> && !has_cast<char, int> && !has_cast<int, double>);
static_assert(noexcept(std::saturating_add(1, 2)) && noexcept(std::saturating_cast<short>(1)));
static_assert(std::is_same_v<decltype(std::saturating_add(short(1), short(2))), short>);  // no promotion
static_assert(std::is_same_v<decltype(std::saturating_cast<std::int8_t>(1000)), std::int8_t>);

template <class T>
using L = std::numeric_limits<T>;

constexpr bool test() {
  // add
  if (std::saturating_add(1, 2) != 3) return false;
  if (std::saturating_add(L<int>::max(), 1) != L<int>::max()) return false;
  if (std::saturating_add(L<int>::min(), -1) != L<int>::min()) return false;
  if (std::saturating_add(L<int>::max(), L<int>::min()) != -1) return false;
  if (std::saturating_add<unsigned>(L<unsigned>::max(), 1u) != L<unsigned>::max()) return false;
  if (std::saturating_add<std::int8_t>(100, 100) != 127) return false;
  if (std::saturating_add<std::int8_t>(-100, -100) != -128) return false;
  if (std::saturating_add<std::uint8_t>(200, 100) != 255) return false;
  // sub
  if (std::saturating_sub(5, 7) != -2) return false;
  if (std::saturating_sub(0u, 1u) != 0u) return false;
  if (std::saturating_sub(L<int>::min(), 1) != L<int>::min()) return false;
  if (std::saturating_sub(L<int>::max(), -1) != L<int>::max()) return false;
  if (std::saturating_sub(0, L<int>::min()) != L<int>::max()) return false;
  if (std::saturating_sub(-1, L<int>::min()) != L<int>::max()) return false;  // exactly max
  if (std::saturating_sub<std::uint8_t>(3, 200) != 0) return false;
  // mul
  if (std::saturating_mul(6, 7) != 42) return false;
  if (std::saturating_mul(L<int>::max(), 2) != L<int>::max()) return false;
  if (std::saturating_mul(L<int>::max(), -2) != L<int>::min()) return false;
  if (std::saturating_mul(L<int>::min(), -1) != L<int>::max()) return false;
  if (std::saturating_mul(L<int>::min(), 1) != L<int>::min()) return false;
  if (std::saturating_mul(L<long long>::min(), L<long long>::min()) != L<long long>::max()) return false;
  if (std::saturating_mul(0, L<int>::min()) != 0) return false;
  if (std::saturating_mul<std::uint8_t>(16, 16) != 255) return false;
  if (std::saturating_mul<std::uint8_t>(15, 17) != 255) return false;  // 255 exactly
  if (std::saturating_mul<std::int8_t>(-16, 8) != -128) return false;   // -128 exactly
  if (std::saturating_mul<std::int8_t>(-16, 9) != -128) return false;
  if (std::saturating_mul(L<unsigned long long>::max(), 2ULL) != L<unsigned long long>::max()) return false;
  // div
  if (std::saturating_div(7, 2) != 3 || std::saturating_div(-7, 2) != -3) return false;  // truncation
  if (std::saturating_div(L<int>::min(), -1) != L<int>::max()) return false;
  if (std::saturating_div<std::int8_t>(-128, -1) != 127) return false;
  if (std::saturating_div(L<unsigned>::max(), 1u) != L<unsigned>::max()) return false;
  // cast
  if (std::saturating_cast<std::int8_t>(1000) != 127) return false;
  if (std::saturating_cast<std::int8_t>(-1000) != -128) return false;
  if (std::saturating_cast<std::int8_t>(-5) != -5) return false;
  if (std::saturating_cast<std::uint8_t>(-1) != 0) return false;
  if (std::saturating_cast<std::uint8_t>(300u) != 255) return false;
  if (std::saturating_cast<unsigned>(-1LL) != 0u) return false;
  if (std::saturating_cast<int>(L<unsigned>::max()) != L<int>::max()) return false;
  if (std::saturating_cast<long long>(L<unsigned long long>::max()) != L<long long>::max()) return false;
  if (std::saturating_cast<unsigned long long>(L<long long>::min()) != 0) return false;
  if (std::saturating_cast<unsigned long long>(L<long long>::max()) != static_cast<unsigned long long>(L<long long>::max())) return false;
  if (std::saturating_cast<short>(short(-3)) != -3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  volatile int big = L<int>::max();
  CHECK(std::saturating_add(big, 5) == L<int>::max());
  return 0;
}
