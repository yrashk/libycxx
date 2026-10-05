// [stdckdint.h.syn]: ckd_add, ckd_sub and ckd_mul with the semantics of ISO/IEC 9899:2024 7.20:
// the operation is performed on the mathematical values of a and b (of any two signed or
// unsigned integer types), the result converted to type1 "as if" wrapped modulo 2^N is
// stored in *result, and true is returned exactly when that stored value differs from the
// mathematical result. Every combination of the ten standard signed/unsigned integer types for
// result, a and b, over boundary operands, against an oracle computed with 128-bit arithmetic.
// __STDC_VERSION_STDCKDINT_H__ is 202311L.
#include <stdckdint.h>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(__STDC_VERSION_STDCKDINT_H__ == 202311L);

using I128 = __int128;
using U128 = unsigned __int128;

template <class T>
static constexpr I128 vals[] = {
    I128(std::numeric_limits<T>::min()), I128(std::numeric_limits<T>::max()), I128(0), I128(1),
    I128(std::numeric_limits<T>::max()) - 1, I128(std::numeric_limits<T>::min()) + (std::is_signed_v<T> ? 1 : 0),
    I128(std::numeric_limits<T>::max()) / 2 + 1, std::is_signed_v<T> ? I128(-1) : I128(2), I128(100)};

// exact value (sign, magnitude) wrapped to R; returns whether it fits
template <class R>
static bool wrap(bool neg, U128 mag, R& out) {
  using UR = std::make_unsigned_t<R>;
  const U128 bits = neg ? U128(0) - mag : mag;
  out = static_cast<R>(static_cast<UR>(bits));
  if (neg) return mag <= U128(0) - static_cast<U128>(I128(std::numeric_limits<R>::min()));
  return mag <= static_cast<U128>(std::numeric_limits<R>::max());
}

static long checks = 0;

template <class R, class A, class B>
static void combo() {
  for (I128 av : vals<A>)
    for (I128 bv : vals<B>) {
      const A a = static_cast<A>(av);
      const B b = static_cast<B>(bv);
      R got{}, exp{};
      // add / sub: exact in 128 bits
      I128 s = av + bv;
      bool fits = wrap<R>(s < 0, s < 0 ? U128(0) - U128(s) : U128(s), exp);
      CHECK(ckd_add(&got, a, b) == !fits && got == exp);
      s = av - bv;
      fits = wrap<R>(s < 0, s < 0 ? U128(0) - U128(s) : U128(s), exp);
      CHECK(ckd_sub(&got, a, b) == !fits && got == exp);
      // mul: magnitudes below 2^64 each, product below 2^128
      const U128 ma = av < 0 ? U128(0) - U128(av) : U128(av), mb = bv < 0 ? U128(0) - U128(bv) : U128(bv);
      const U128 mp = ma * mb;
      fits = wrap<R>(mp != 0 && ((av < 0) != (bv < 0)), mp, exp);
      CHECK(ckd_mul(&got, a, b) == !fits && got == exp);
      checks += 3;
    }
}

template <class R, class A, class... Bs>
static void over_b() {
  (combo<R, A, Bs>(), ...);
}
template <class R, class... As>
static void over_a() {
  (over_b<R, As, signed char, unsigned char, short, unsigned short, int, unsigned, long, unsigned long, long long,
          unsigned long long>(),
   ...);
}
template <class... Rs>
static void over_r() {
  (over_a<Rs, signed char, unsigned char, short, unsigned short, int, unsigned, long, unsigned long, long long,
          unsigned long long>(),
   ...);
}

int main() {
  over_r<signed char, unsigned char, short, unsigned short, int, unsigned, long, unsigned long, long long,
         unsigned long long>();
  CHECK(checks == 1000L * 81 * 3);
  // a few by hand
  int i = 0;
  CHECK(!ckd_add(&i, 2147483647LL, -1) && i == 2147483646);
  CHECK(ckd_add(&i, 2147483647, 1) && i == -2147483647 - 1);
  unsigned u = 0;
  CHECK(ckd_sub(&u, 0, 1) && u == 4294967295u);
  unsigned char uc = 0;
  CHECK(ckd_mul(&uc, 16, 16) && uc == 0);
  long long ll = 0;
  CHECK(ckd_add(&ll, 18446744073709551615ull, -1LL) && ll == -2);
  CHECK(!ckd_sub(&ll, 9223372036854775807ull, 9223372036854775808ull) && ll == -1);
  CHECK(!ckd_mul(&ll, -4294967296LL, 2147483648u) && ll == -9223372036854775807LL - 1);
  return 0;
}
