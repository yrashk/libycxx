// [numeric.ops.midpoint]: midpoint(a, b) for arithmetic T other than bool returns half the
// sum without overflow; for integers an odd sum is rounded towards a; for floating point at
// most one inexact operation occurs. midpoint(T* a, T* b) returns a pointer to element
// i + (j - i) / 2 (division truncated towards zero) of the same array.
#include <numeric>
#include <climits>
#include <cfloat>
#include <limits>
#include <type_traits>
#include "check.hpp"

template <class T>
concept has_midpoint = requires(T a) { std::midpoint(a, a); };
static_assert(has_midpoint<int> && has_midpoint<unsigned char> && has_midpoint<double> && has_midpoint<int*>);
static_assert(has_midpoint<char> && has_midpoint<long double>);
static_assert(!has_midpoint<bool>);
static_assert(!has_midpoint<void (*)()>);  // T must be an object type
static_assert(noexcept(std::midpoint(1, 2)));
static_assert(std::is_same_v<decltype(std::midpoint(short(1), short(2))), short>);

constexpr bool test() {
  if (std::midpoint(2, 6) != 4 || std::midpoint(6, 2) != 4) return false;
  if (std::midpoint(2, 5) != 3 || std::midpoint(5, 2) != 4) return false;  // rounds towards a
  if (std::midpoint(-3, 0) != -2 || std::midpoint(0, -3) != -1) return false;
  if (std::midpoint(INT_MAX, INT_MAX - 2) != INT_MAX - 1) return false;  // no overflow
  if (std::midpoint(INT_MIN, INT_MAX) != -1 || std::midpoint(INT_MAX, INT_MIN) != 0) return false;
  if (std::midpoint(UINT_MAX, 0u) != UINT_MAX / 2 + 1 || std::midpoint(0u, UINT_MAX) != UINT_MAX / 2) return false;
  if (std::midpoint((unsigned char)255, (unsigned char)0) != 128) return false;
  if (std::midpoint(LLONG_MIN, LLONG_MIN) != LLONG_MIN) return false;
  if (std::midpoint(1.0, 2.0) != 1.5 || std::midpoint(-1.0f, 1.0f) != 0.0f) return false;
  if (std::midpoint(DBL_MAX, DBL_MAX) != DBL_MAX) return false;  // no overflow to infinity
  if (std::midpoint(-DBL_MAX, DBL_MAX) != 0.0) return false;
  if (std::midpoint(DBL_MIN, DBL_MIN) != DBL_MIN) return false;
  double tiny = std::numeric_limits<double>::denorm_min();
  if (std::midpoint(tiny, 3 * tiny) != 2 * tiny) return false;

  int arr[10] = {};
  if (std::midpoint(arr, arr + 10) != arr + 5) return false;
  if (std::midpoint(arr, arr + 9) != arr + 4) return false;      // (9 - 0) / 2 = 4
  if (std::midpoint(arr + 9, arr) != arr + 5) return false;      // 9 + (0 - 9) / 2 = 9 - 4
  if (std::midpoint(arr + 3, arr + 3) != arr + 3) return false;
  const int* c = arr;
  if (std::midpoint(c, c + 2) != c + 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
