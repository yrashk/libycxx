// The C++26 <bit> operations against oracles written from their formulas, exhaustively for
// 8-bit operands and on structured 16/32/64-bit values:
//   [bit.permute]/3 bit_reverse: reverse(x) = sum x_n 2^(N-n-1) (Formula 22.1)
//   [bit.permute]/7 bit_repeat: repeat(x, l) = sum x_(n mod l) 2^n (Formula 22.2), every l > 0
//     (including l >= N)
//   [bit.permute]/12 bit_compress: compress(x, m) = sum m_n x_n 2^sigma(m, n) (Formula 22.3)
//   [bit.permute]/15 bit_expand: expand(x, m) = sum m_n x_sigma(m, n) 2^n (Formula 22.4)
//     (sigma(a, n): the number of one bits among the lowest n bits of a)
//   [bit.shift]/1,3,5 shl/shr: x * 2^s (resp. 2^-s) rounded towards negative infinity, computed
//     in a type of sufficient range and converted with static_cast<T>; T and S any signed or
//     unsigned integer types, every 8-bit x with s in [-20, 20] and the extreme values of S.
#include <bit>
#include <climits>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class T>
constexpr int N = std::numeric_limits<T>::digits;
template <class T>
constexpr unsigned bitn(T x, int n) { return unsigned((x >> n) & 1); }
template <class T>
constexpr int sigma(T a, int n) {
  int c = 0;
  for (int k = 0; k < n; ++k) c += bitn(a, k);
  return c;
}

template <class T>
constexpr bool permute_ok(T x, T m) {
  T rev = 0, comp = 0, exp = 0;
  for (int n = 0; n < N<T>; ++n) {
    rev = T(rev | T(T(bitn(x, n)) << (N<T> - n - 1)));
    if (bitn(m, n)) {
      comp = T(comp | T(T(bitn(x, n)) << sigma(m, n)));
      exp = T(exp | T(T(bitn(x, sigma(m, n))) << n));
    }
  }
  if (std::bit_reverse(x) != rev || std::bit_compress(x, m) != comp || std::bit_expand(x, m) != exp)
    return false;
  for (int l : {1, 2, 3, 5, 7, 8, 9, 13, 16, 31, 32, 33, 63, 64, 65, 1000, INT_MAX}) {
    T rep = 0;
    for (int n = 0; n < N<T>; ++n) rep = T(rep | T(T(bitn(x, n % l)) << n));
    if (std::bit_repeat(x, l) != rep) return false;
  }
  return true;
}

// shl/shr oracle on 8-bit T: the exact value x * 2^s floored, reduced modulo 2^8 (static_cast).
template <class T>
constexpr T shift_ref(T x, long long s) {
  long long v = x;  // |x| < 2^8
  if (s >= 0) {
    if (s >= 16) return T(0);  // a multiple of 2^16: 0 modulo 2^8
    v = v * (1LL << s);
  } else {
    if (s <= -16) return v < 0 ? T(-1) : T(0);
    long long d = 1LL << -s;
    v = v >= 0 ? v / d : -((-v + d - 1) / d);
  }
  return static_cast<T>(v);
}

template <class T, class S>
constexpr bool shift_ok(T x) {
  using L = std::numeric_limits<S>;
  for (long long s = -20; s <= 20; ++s) {
    if (s < 0 && !std::is_signed_v<S>) continue;
    if (!std::in_range<S>(s)) continue;
    if (std::shl(x, S(s)) != shift_ref(x, s) || std::shr(x, S(s)) != shift_ref(x, -s)) return false;
  }
  for (S s : {L::min(), S(L::min() + 1), L::max(), S(L::max() - 1)}) {
    // beyond the 8-bit range either way, unless small
    long long ls = std::in_range<long long>(s) && std::in_range<signed char>(s) ? (long long)s
                   : std::cmp_less(s, 0)                                       ? -1000
                                                                               : 1000;
    if (std::shl(x, s) != shift_ref(x, ls) || std::shr(x, s) != shift_ref(x, -ls)) return false;
  }
  return true;
}

template <class T>
constexpr bool shifts_8(T x) {
  return shift_ok<T, signed char>(x) && shift_ok<T, short>(x) && shift_ok<T, int>(x) &&
         shift_ok<T, long long>(x) && shift_ok<T, unsigned char>(x) && shift_ok<T, unsigned>(x) &&
         shift_ok<T, unsigned long long>(x);
}

constexpr bool all8() {
  for (unsigned x = 0; x < 256; ++x)
    for (unsigned m = 0; m < 256; ++m)
      if (!permute_ok<unsigned char>((unsigned char)x, (unsigned char)m)) return false;
  return true;
}
constexpr bool all8_shifts() {
  for (int x = 0; x < 256; ++x)
    if (!shifts_8<unsigned char>((unsigned char)x) || !shifts_8<signed char>((signed char)(x - 128)))
      return false;
  return true;
}

template <class T>
bool wide() {
  for (int i = 0; i < N<T>; ++i)
    for (int j = 0; j < N<T>; j += 3) {
      T a = T(T(1) << i), b = T(~(T(1) << j));
      for (T x : {a, b, T(a ^ b), T(a - 1), T(0x5A5A5A5A5A5A5A5Aull)})
        for (T m : {a, b, T(a - 1), T(~T(a - 1)), T(0x0F0F0F0F0F0F0F0Full), T(0)})
          if (!permute_ok(x, m)) return false;
    }
  return true;
}

static_assert(std::bit_repeat(std::uint32_t{0xc}, 4) == 0xccccccccu);  // Example 1
static_assert(std::bit_compress(0b1011u, 0b0101u) == 0b0001u);         // Example 2 (ABCD = 1011: B = 0, D = 1)
static_assert(std::bit_expand(0b1011u, 0b0101u) == 0b0101u);           // Example 3 (C = 1, D = 1)

int main() {
  CHECK(all8());
  CHECK(all8_shifts());
  CHECK(wide<unsigned short>());
  CHECK(wide<unsigned>());
  CHECK(wide<unsigned long long>());
  return 0;
}

// Constant evaluation: every shift-count type and count as above, for a representative set of
// 8-bit values (all bit positions set and clear, sign boundaries). The exhaustive all8_shifts()
// runs at run time in main(); evaluating it here needs several million constant-evaluation steps,
// beyond Clang's default limit.
constexpr bool sample8_shifts() {
  for (int x : {0x00, 0x01, 0x02, 0x10, 0x40, 0x55, 0x7f, 0x80, 0x81, 0xaa, 0xc3, 0xfe, 0xff})
    if (!shifts_8<unsigned char>((unsigned char)x) || !shifts_8<signed char>((signed char)(x - 128)) ||
        !shifts_8<signed char>((signed char)(unsigned char)x))
      return false;
  return true;
}
static_assert(sample8_shifts());
