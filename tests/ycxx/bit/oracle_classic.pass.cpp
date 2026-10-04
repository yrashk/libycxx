// Every <bit> function of C++20/23 checked against a bit-by-bit oracle written from its Returns:
// element, for every 8-bit and 16-bit value (rotations: every count in [-40, 40] and the extreme
// int counts) and for 32/64-bit values built from all single bits and their complements:
//   [bit.count] countl_zero/countl_one/countr_zero/countr_one: "the number of consecutive 0 (1)
//     bits ... starting from the most (least) significant bit"; popcount: "the number of 1 bits"
//   [bit.pow.two] has_single_bit: x is an integral power of two; bit_ceil: the minimal power of
//     two not less than x (only where representable, /5 Preconditions); bit_floor: 0 for 0,
//     otherwise the maximal power of two not greater than x; bit_width: 0 for 0, otherwise
//     1 + floor(log2(x))
//   [bit.rotate] rotl/rotr: r = s % N; r == 0: x; r > 0: (x << r) | (x >> (N - r)); r < 0: the
//     other rotation by -r
//   [bit.byteswap] byteswap: the bytes of the value representation reversed
// Constant evaluation covers the 8-bit types.
#include <bit>
#include <climits>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include "check.hpp"

template <class T>
constexpr int N = std::numeric_limits<T>::digits;
template <class T>
constexpr bool bitn(T x, int n) { return (x >> n) & 1; }

template <class T>
constexpr bool check(T x) {
  int clz = 0, clo = 0, ctz = 0, cto = 0, pop = 0;
  while (clz < N<T> && !bitn(x, N<T> - 1 - clz)) ++clz;
  while (clo < N<T> && bitn(x, N<T> - 1 - clo)) ++clo;
  while (ctz < N<T> && !bitn(x, ctz)) ++ctz;
  while (cto < N<T> && bitn(x, cto)) ++cto;
  for (int n = 0; n < N<T>; ++n) pop += bitn(x, n);
  if (std::countl_zero(x) != clz || std::countl_one(x) != clo || std::countr_zero(x) != ctz ||
      std::countr_one(x) != cto || std::popcount(x) != pop)
    return false;
  if (std::has_single_bit(x) != (pop == 1)) return false;
  int width = x == 0 ? 0 : N<T> - clz;
  if (std::bit_width(x) != T(width)) return false;
  T floor2 = x == 0 ? T(0) : T(T(1) << (width - 1));
  if (std::bit_floor(x) != floor2) return false;
  if (x <= T(T(1) << (N<T> - 1))) {
    T ceil2 = x <= 1 ? T(1) : (pop == 1 ? x : T(floor2 << 1));
    if (std::bit_ceil(x) != ceil2) return false;
  }
  for (int s = -40; s <= 40; ++s) {
    int r = s % N<T>;
    auto rotl_ref = [&](int k) { return T(T(x << k) | T(x >> (N<T> - k))); };
    auto rotr_ref = [&](int k) { return T(T(x >> k) | T(x << (N<T> - k))); };
    T l = r == 0 ? x : r > 0 ? rotl_ref(r) : rotr_ref(-r);
    T rr = r == 0 ? x : r > 0 ? rotr_ref(r) : rotl_ref(-r);
    if (std::rotl(x, s) != l || std::rotr(x, s) != rr) return false;
  }
  for (int s : {INT_MIN, INT_MIN + 1, INT_MAX, INT_MAX - 1}) {
    int r = s % N<T>;
    T l = x, rr = x;
    for (int k = 0; k < (r < 0 ? -r : r); ++k) {
      T hi = T(l >> (N<T> - 1)), lo = T(rr & 1);
      if (r > 0) {
        l = T(T(l << 1) | hi);
        rr = T(T(rr >> 1) | T(lo << (N<T> - 1)));
      } else {
        l = T(T(l >> 1) | T(T(l & 1) << (N<T> - 1)));
        rr = T(T(rr << 1) | T(rr >> (N<T> - 1)));
      }
    }
    if (std::rotl(x, s) != l || std::rotr(x, s) != rr) return false;
  }
  T swapped = 0;
  for (unsigned b = 0; b < sizeof(T); ++b)
    swapped = T(swapped | T(T((x >> (8 * b)) & 0xFF) << (8 * (sizeof(T) - 1 - b))));
  if (std::byteswap(x) != swapped) return false;
  return true;
}

template <class T>
constexpr bool all_values() {
  for (unsigned v = 0; v <= std::numeric_limits<T>::max(); ++v)
    if (!check(T(v))) return false;
  return true;
}
template <class T>
bool wide_values() {
  for (int i = 0; i < N<T>; ++i) {
    T one = T(T(1) << i);
    for (T x : {one, T(~one), T(one - 1), T(~T(one - 1)), T(one | 1), T(one + (one >> 1))})
      if (!check(x)) return false;
  }
  return check(T(0)) && check(std::numeric_limits<T>::max());
}

static_assert(all_values<unsigned char>());
static_assert(all_values<std::uint8_t>());

int main() {
  CHECK(all_values<unsigned char>());
  CHECK(all_values<unsigned short>());
  CHECK(wide_values<unsigned>());
  CHECK(wide_values<unsigned long>());
  CHECK(wide_values<unsigned long long>());
  return 0;
}
