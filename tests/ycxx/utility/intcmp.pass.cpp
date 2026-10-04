// [utility.intcmp]: cmp_equal, cmp_not_equal, cmp_less, cmp_greater, cmp_less_equal,
// cmp_greater_equal compare mathematical values of integers of possibly different
// signedness; in_range<R>(t) is true iff t is representable in R.
#include <climits>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>
#include "check.hpp"

using namespace std;

constexpr bool test_cmp() {
  // The classic pitfalls of the built-in operators.
  if (cmp_equal(-1, UINT_MAX)) return false;               // -1 == UINT_MAX would be true
  if (!cmp_not_equal(-1, UINT_MAX)) return false;
  if (!cmp_less(-1, 0u)) return false;                     // -1 < 0u would be false
  if (cmp_less(0u, -1)) return false;
  if (!cmp_greater(0u, -1)) return false;
  if (!cmp_greater(UINT_MAX, -1)) return false;
  if (!cmp_less_equal(-1, 0u) || cmp_greater_equal(-1, 0u)) return false;
  if (!cmp_greater_equal(0u, -1) || cmp_less_equal(0u, -1)) return false;
  // Extremes of the widest types.
  if (cmp_equal(LLONG_MIN, ULLONG_MAX)) return false;
  if (!cmp_less(LLONG_MIN, 0ull) || !cmp_less(LLONG_MAX, ULLONG_MAX)) return false;
  if (cmp_less(ULLONG_MAX, LLONG_MIN) || !cmp_greater(ULLONG_MAX, LLONG_MAX)) return false;
  if (!cmp_equal(LLONG_MAX, (unsigned long long)LLONG_MAX)) return false;
  if (!cmp_equal((unsigned long long)LLONG_MAX, LLONG_MAX)) return false;
  if (!cmp_less((unsigned long long)LLONG_MAX, (unsigned long long)LLONG_MAX + 1)) return false;
  // Narrow types.
  if (cmp_equal(std::uint8_t(255), std::int8_t(-1))) return false;
  if (!cmp_less(std::int8_t(-128), std::uint8_t(0))) return false;
  if (!cmp_greater(std::uint8_t(200), std::int8_t(100))) return false;
  if (!cmp_equal(std::uint16_t(65535), 65535)) return false;
  if (!cmp_equal(std::int16_t(-1), -1LL)) return false;
  if (!cmp_less(std::int16_t(-1), std::uint64_t(0))) return false;
  // Same signedness behaves like the built-in operators.
  if (!cmp_less(-2, -1) || !cmp_equal(5u, 5ull) || !cmp_greater(10L, (short)-3)) return false;
  if (!cmp_less_equal(3, 3) || !cmp_greater_equal(3u, 3u)) return false;
  // cmp_not_equal/less_equal/greater_equal are the stated compositions.
  for (int a : {INT_MIN, -1, 0, 1, INT_MAX})
    for (unsigned b : {0u, 1u, 2147483647u, 2147483648u, UINT_MAX}) {
      if (cmp_not_equal(a, b) != !cmp_equal(a, b)) return false;
      if (cmp_greater(a, b) != cmp_less(b, a)) return false;
      if (cmp_less_equal(a, b) != !cmp_greater(a, b)) return false;
      if (cmp_greater_equal(a, b) != !cmp_less(a, b)) return false;
      bool math_less = a < 0 || unsigned(a) < b;
      if (cmp_less(a, b) != math_less) return false;
      if (cmp_equal(a, b) != (a >= 0 && unsigned(a) == b)) return false;
    }
  return true;
}

constexpr bool test_in_range() {
  if (!in_range<std::uint8_t>(255) || in_range<std::uint8_t>(256) || in_range<std::uint8_t>(-1)) return false;
  if (!in_range<std::uint8_t>(0)) return false;
  if (!in_range<std::int8_t>(-128) || in_range<std::int8_t>(-129) || !in_range<std::int8_t>(127)) return false;
  if (in_range<std::int8_t>(128u)) return false;
  if (in_range<unsigned>(-1) || !in_range<unsigned>(UINT_MAX) || !in_range<unsigned>(0LL)) return false;
  if (in_range<int>(UINT_MAX) || !in_range<int>(unsigned(INT_MAX)) || in_range<int>(unsigned(INT_MAX) + 1)) return false;
  if (in_range<long long>(ULLONG_MAX) || !in_range<long long>((unsigned long long)LLONG_MAX)) return false;
  if (!in_range<unsigned long long>(LLONG_MAX) || in_range<unsigned long long>(LLONG_MIN)) return false;
  if (!in_range<short>(SHRT_MIN) || in_range<short>(SHRT_MIN - 1) || in_range<short>(SHRT_MAX + 1)) return false;
  if (!in_range<std::uint16_t>(std::uint64_t(65535)) || in_range<std::uint16_t>(std::uint64_t(65536))) return false;
  if (!in_range<signed char>((signed char)-5)) return false;
  return true;
}

static_assert(noexcept(cmp_equal(1, 1u)) && noexcept(cmp_less(1, 1u)) && noexcept(in_range<int>(1u)));
static_assert(is_same_v<decltype(cmp_greater_equal(1, 1u)), bool>);
static_assert(test_cmp());
static_assert(test_in_range());

int main() {
  CHECK(test_cmp());
  CHECK(test_in_range());
  return 0;
}
