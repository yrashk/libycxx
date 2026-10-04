// [numeric.sat.func]/1-10 and [numeric.sat.cast]/2 checked against exact (128-bit) arithmetic:
// every pair of 8-bit operands (and, in constant evaluation, every 8-bit x with a few y), and for the wider types every pair
// drawn from boundary values (min, min + 1, -1, 0, 1, max - 1, max, the square roots of the
// bounds, ...). "Returns: If x op y is representable as a value of type T, x op y; otherwise,
// either the largest or smallest representable value of type T, whichever is closer";
// saturating_div: x / y (truncating), max() for min() / -1; saturating_cast<R>(x): x if
// representable in R, otherwise the closer bound. Every signed and unsigned standard integer type
// is used, also as the source and target of saturating_cast.
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <type_traits>
#include "check.hpp"

using i128 = __int128;

template <class T>
constexpr T clampw(i128 v) {
  using L = std::numeric_limits<T>;
  if (v < i128(L::min())) return L::min();
  if (v > i128(L::max())) return L::max();
  return T(v);
}

template <class T>
constexpr bool pair_ok(T x, T y) {
  i128 a = x, b = y;
  if (std::saturating_add(x, y) != clampw<T>(a + b)) return false;
  if (std::saturating_sub(x, y) != clampw<T>(a - b)) return false;
  i128 p;
  if (__builtin_mul_overflow(a, b, &p))  // only for 64-bit unsigned operands: far beyond any bound
    p = (a < 0) != (b < 0) ? i128(-1) << 126 : i128(1) << 126;
  if (std::saturating_mul(x, y) != clampw<T>(p)) return false;
  if (y != 0 && std::saturating_div(x, y) != clampw<T>(a / b)) return false;
  return true;
}

template <class T>
constexpr bool exhaustive8() {
  using L = std::numeric_limits<T>;
  for (int x = L::min(); x <= L::max(); ++x)
    for (int y = L::min(); y <= L::max(); ++y)
      if (!pair_ok(T(x), T(y))) return false;
  return true;
}
template <class T>
constexpr bool rows8(int y0, int y1) {  // constant evaluation: every x with a few y
  using L = std::numeric_limits<T>;
  for (int x = L::min(); x <= L::max(); ++x)
    for (int y : {int(L::min()), y0, y1, int(L::max())})
      if (!pair_ok(T(x), T(y))) return false;
  return true;
}
static_assert(rows8<signed char>(-1, 3) && rows8<signed char>(0, -128 / 3));
static_assert(rows8<unsigned char>(0, 1) && rows8<unsigned char>(15, 16));

template <class T>
constexpr int boundary(T* out) {
  using L = std::numeric_limits<T>;
  int n = 0;
  auto add = [&](i128 v) {
    if (v >= i128(L::min()) && v <= i128(L::max())) out[n++] = T(v);
  };
  i128 mn = L::min(), mx = L::max();
  i128 lo = 0, hi = i128(1) << 32;  // r = floor(sqrt(max))
  while (lo < hi) {
    i128 mid = (lo + hi + 1) / 2;
    if (mid * mid <= mx) lo = mid; else hi = mid - 1;
  }
  const i128 r = lo;
  const i128 vals[] = {mn, mn + 1, mn + 2, mn / 2, mn / 2 - 1, -r - 1, -r, -3, -2, -1, 0, 1, 2,
                       3, r, r + 1, mx / 2, mx / 2 + 1, mx - 2, mx - 1, mx};
  for (i128 v : vals)
    add(v);
  return n;
}

template <class T>
bool boundary_pairs() {
  T v[32];
  int n = boundary(v);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (!pair_ok(v[i], v[j])) {
        CHECK(!"saturating arithmetic differs from the exact result");
        return false;
      }
  return true;
}

template <class R, class T>
bool casts_from() {
  T v[32];
  int n = boundary(v);
  for (int i = 0; i < n; ++i) {
    R got = std::saturating_cast<R>(v[i]);
    if (got != clampw<R>(i128(v[i]))) {
      CHECK(!"saturating_cast differs from the clamped value");
      return false;
    }
  }
  return true;
}

template <class R, class... Ts>
bool casts_to() {
  return (casts_from<R, Ts>() && ...);
}

#define TYPES                                                                                       \
  signed char, short, int, long, long long, unsigned char, unsigned short, unsigned, unsigned long, \
      unsigned long long

template <class... Ts>
bool all_casts() {
  return (casts_to<Ts, TYPES>() && ...);
}

int main() {
  CHECK(exhaustive8<signed char>() && exhaustive8<unsigned char>());
  CHECK(exhaustive8<std::int8_t>() && exhaustive8<std::uint8_t>());
  CHECK(boundary_pairs<short>() && boundary_pairs<unsigned short>());
  CHECK(boundary_pairs<int>() && boundary_pairs<unsigned>());
  CHECK(boundary_pairs<long>() && boundary_pairs<unsigned long>());
  CHECK(boundary_pairs<long long>() && boundary_pairs<unsigned long long>());
  CHECK(all_casts<TYPES>());
  static_assert(std::saturating_cast<unsigned long long>(-1LL) == 0);
  static_assert(std::saturating_cast<long long>(~0ULL) == std::numeric_limits<long long>::max());
  static_assert(std::saturating_mul(std::numeric_limits<long long>::min(), -1LL) ==
                std::numeric_limits<long long>::max());
  static_assert(std::saturating_div(std::numeric_limits<long long>::min(), -1LL) ==
                std::numeric_limits<long long>::max());
  return 0;
}
