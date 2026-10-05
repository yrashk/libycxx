// libycxx core: building blocks of the constexpr ("soft") floating-point math behind <cmath>.
//
// - wide<N>: an N x 64-bit unsigned integer (little-endian limbs) with the few operations the
//   exact algorithms need.
// - fp_layout<T> / fp_decode / fp_encode: the bits of any binary floating-point format the
//   compilers offer (binary16/32/64/128, bfloat16, x87 extended), read and written with
//   __builtin_bit_cast, so they work during constant evaluation.
// - fp_round<T>: correct rounding (to nearest, ties to even) of an exact or sticky-tailed value
//   m * 2^e, with the IEEE exception flags it raises (overflow, underflow, inexact).
// - fp_raise: reports flags. It is deliberately not constexpr: reached during constant
//   evaluation, the call makes the evaluation non-constant ([library.c]/3: a C library call
//   that raises a floating-point exception other than FE_INEXACT is a non-constant library
//   call); at run time it raises the flags with volatile arithmetic.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::fpm {

using u64 = unsigned long long;

// ---- 64 x 64 -> 128-bit multiplication and 128 / 64 division ------------------------------
struct u64pair {
  u64 hi, lo;
};
template <class U = ycxx::detail::uint128>
constexpr u64pair mul64(u64 a, u64 b) noexcept {
  if constexpr (cfg::has_int128) {
    const U p = U(a) * b;
    return {static_cast<u64>(p >> 64), static_cast<u64>(p)};
  } else {
    const u64 a0 = a & 0xffffffffu, a1 = a >> 32, b0 = b & 0xffffffffu, b1 = b >> 32;
    const u64 p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    const u64 mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
    return {p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32), (mid << 32) | (p00 & 0xffffffffu)};
  }
}
// (hi * 2^64 + lo) / d for hi < d; returns the quotient, stores the remainder.
template <class U = ycxx::detail::uint128>
constexpr u64 div128(u64 hi, u64 lo, u64 d, u64& rem) noexcept {
  if constexpr (cfg::has_int128) {
    const U n = (U(hi) << 64) | lo;
    rem = static_cast<u64>(n % d);
    return static_cast<u64>(n / d);
  } else {
    u64 q = 0;
    for (int i = 63; i >= 0; --i) {
      const bool top = hi >> 63;
      hi = (hi << 1) | ((lo >> i) & 1);
      q <<= 1;
      if (top || hi >= d) {
        hi -= d;
        q |= 1;
      }
    }
    rem = hi;
    return q;
  }
}

constexpr int clz64(u64 x) noexcept { return x == 0 ? 64 : __builtin_clzll(x); }
constexpr int ctz64(u64 x) noexcept { return x == 0 ? 64 : __builtin_ctzll(x); }

// ---- wide<N> --------------------------------------------------------------------------------
template <int N>
struct wide {
  u64 w[N] = {};
};

template <int N>
constexpr wide<N> wide_from(u64 x) noexcept {
  wide<N> r;
  r.w[0] = x;
  return r;
}
template <int M, int N>
constexpr wide<M> wide_resize(const wide<N>& x) noexcept { // zero-extends or truncates
  wide<M> r;
  for (int i = 0; i < M && i < N; ++i) r.w[i] = x.w[i];
  return r;
}
template <int N>
constexpr bool wide_is_zero(const wide<N>& x) noexcept {
  for (int i = 0; i < N; ++i)
    if (x.w[i] != 0) return false;
  return true;
}
template <int N>
constexpr int wide_bitlen(const wide<N>& x) noexcept {
  for (int i = N - 1; i >= 0; --i)
    if (x.w[i] != 0) return 64 * i + 64 - ycxx::detail::fpm::clz64(x.w[i]);
  return 0;
}
template <int N>
constexpr int wide_ctz(const wide<N>& x) noexcept {
  for (int i = 0; i < N; ++i)
    if (x.w[i] != 0) return 64 * i + ycxx::detail::fpm::ctz64(x.w[i]);
  return 64 * N;
}
template <int N>
constexpr bool wide_bit(const wide<N>& x, int i) noexcept {
  return i >= 0 && i < 64 * N && ((x.w[i / 64] >> (i % 64)) & 1);
}
template <int N>
constexpr void wide_set_bit(wide<N>& x, int i) noexcept {
  x.w[i / 64] |= u64(1) << (i % 64);
}
// Keeps the low `n` bits.
template <int N>
constexpr wide<N> wide_low_bits(wide<N> x, int n) noexcept {
  for (int i = 0; i < N; ++i) {
    const int lo = 64 * i;
    if (n <= lo)
      x.w[i] = 0;
    else if (n < lo + 64)
      x.w[i] &= (u64(1) << (n - lo)) - 1;
  }
  return x;
}
template <int N>
constexpr int wide_cmp(const wide<N>& a, const wide<N>& b) noexcept {
  for (int i = N - 1; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}
template <int N>
constexpr wide<N> wide_shl(const wide<N>& x, int s) noexcept {
  wide<N> r;
  if (s >= 64 * N) return r;
  const int q = s / 64, b = s % 64;
  for (int i = N - 1; i >= q; --i) {
    r.w[i] = x.w[i - q] << b;
    if (b != 0 && i - q - 1 >= 0) r.w[i] |= x.w[i - q - 1] >> (64 - b);
  }
  return r;
}
// Shift right; `sticky` is or-ed with "a nonzero bit was shifted out".
template <int N>
constexpr wide<N> wide_shr(const wide<N>& x, int s, bool& sticky) noexcept {
  wide<N> r;
  if (s <= 0) return s == 0 ? x : ycxx::detail::fpm::wide_shl(x, -s);
  if (s >= 64 * N) {
    sticky = sticky || !ycxx::detail::fpm::wide_is_zero(x);
    return r;
  }
  const int q = s / 64, b = s % 64;
  for (int i = 0; i < q; ++i) sticky = sticky || x.w[i] != 0;
  if (b != 0) sticky = sticky || (x.w[q] << (64 - b)) != 0;
  for (int i = 0; i + q < N; ++i) {
    r.w[i] = x.w[i + q] >> b;
    if (b != 0 && i + q + 1 < N) r.w[i] |= x.w[i + q + 1] << (64 - b);
  }
  return r;
}
template <int N>
constexpr wide<N> wide_shr(const wide<N>& x, int s) noexcept {
  bool ignored = false;
  return ycxx::detail::fpm::wide_shr(x, s, ignored);
}
template <int N>
constexpr bool wide_add(wide<N>& a, const wide<N>& b) noexcept { // a += b; returns the carry
  bool carry = false;
  for (int i = 0; i < N; ++i) {
    const u64 s = a.w[i] + b.w[i];
    const bool c1 = s < a.w[i];
    a.w[i] = s + carry;
    carry = c1 || (carry && a.w[i] == 0);
  }
  return carry;
}
template <int N>
constexpr bool wide_sub(wide<N>& a, const wide<N>& b) noexcept { // a -= b; returns the borrow
  bool borrow = false;
  for (int i = 0; i < N; ++i) {
    const u64 d = a.w[i] - b.w[i];
    const bool b1 = a.w[i] < b.w[i];
    a.w[i] = d - borrow;
    borrow = b1 || (borrow && d == 0);
  }
  return borrow;
}
template <int N>
constexpr void wide_add_small(wide<N>& a, u64 v) noexcept {
  for (int i = 0; i < N && v != 0; ++i) {
    a.w[i] += v;
    v = a.w[i] < v ? 1 : 0;
  }
}
template <int N, int M>
constexpr wide<N + M> wide_mul(const wide<N>& a, const wide<M>& b) noexcept {
  wide<N + M> r;
  for (int i = 0; i < N; ++i) {
    if (a.w[i] == 0) continue;
    u64 carry = 0;
    for (int j = 0; j < M; ++j) {
      const u64pair p = ycxx::detail::fpm::mul64(a.w[i], b.w[j]);
      u64 lo = p.lo + carry;
      u64 hi = p.hi + (lo < carry);
      const u64 cur = r.w[i + j];
      lo += cur;
      hi += lo < cur;
      r.w[i + j] = lo;
      carry = hi;
    }
    r.w[i + M] = carry;
  }
  return r;
}
template <int N>
constexpr u64 wide_mul_small(wide<N>& a, u64 m) noexcept { // a *= m; returns the overflow limb
  u64 carry = 0;
  for (int i = 0; i < N; ++i) {
    const u64pair p = ycxx::detail::fpm::mul64(a.w[i], m);
    const u64 lo = p.lo + carry;
    carry = p.hi + (lo < carry);
    a.w[i] = lo;
  }
  return carry;
}
template <int N>
constexpr u64 wide_div_small(wide<N>& a, u64 d) noexcept { // a /= d; returns the remainder
  u64 rem = 0;
  for (int i = N - 1; i >= 0; --i) a.w[i] = ycxx::detail::fpm::div128(rem, a.w[i], d, rem);
  return rem;
}

// ---- floating-point formats -----------------------------------------------------------------
enum : int { fe_inexact = 1, fe_underflow = 2, fe_overflow = 4, fe_divbyzero = 8, fe_invalid = 16 };

// Not constexpr on purpose (see the file comment).
[[gnu::noinline, gnu::cold]] inline void fp_raise(int flags) noexcept {
  volatile double big = 0x1p1000, small = 0x1p-1000, zero = 0.0, one = 1.0;
  if (flags & fe_overflow) big = big * big;
  if (flags & fe_underflow) small = small * small;
  if (flags & fe_divbyzero) one = one / zero;
  if (flags & fe_invalid) zero = zero / zero;
  (void)big;
  (void)small;
  (void)one;
  (void)zero;
}
constexpr void fp_report(int flags) noexcept {
  if ((flags & ~fe_inexact) != 0) ycxx::detail::fpm::fp_raise(flags);
}

template <class T>
struct fp_layout {
  static constexpr int p = ycxx::detail::fp_format<T>.digits;       // significand bits
  static constexpr int emax = ycxx::detail::fp_format<T>.max_exp - 1; // largest normal: 2^emax
  static constexpr int emin = ycxx::detail::fp_format<T>.min_exp - 1; // smallest normal: 2^emin
  static constexpr bool explicit_bit = p == 64 && emax == 16383;    // x87 extended
  static constexpr int ebits = ycxx::detail::fpm::ctz64(u64(emax) + 1) + 1;
  static constexpr int fbits = explicit_bit ? p : p - 1;            // fraction field width
  static constexpr int bits = 1 + ebits + fbits;
  static constexpr int bias = emax;
  static constexpr int qmin = emin - (p - 1); // exponent of the least significant bit of denorm_min
  static_assert(p > 1 && p <= 113 && bits <= 128 && bits <= 8 * int(sizeof(T)), "libycxx: unsupported floating-point format");
};

template <class T>
struct fp_bytes {
  unsigned char b[sizeof(T)];
};

template <class T>
constexpr wide<2> fp_to_bits(T x) noexcept {
  const fp_bytes<T> by = __builtin_bit_cast(fp_bytes<T>, x);
  constexpr int n = (fp_layout<T>::bits + 7) / 8;
  constexpr bool le = std::endian::native == std::endian::little;
  wide<2> r;
  for (int i = 0; i < n; ++i) r.w[i / 8] |= u64(by.b[le ? i : int(sizeof(T)) - 1 - i]) << (8 * (i % 8));
  return r;
}
template <class T>
constexpr T fp_from_bits(const wide<2>& v) noexcept {
  fp_bytes<T> by{};
  constexpr int n = (fp_layout<T>::bits + 7) / 8;
  constexpr bool le = std::endian::native == std::endian::little;
  for (int i = 0; i < n; ++i)
    by.b[le ? i : int(sizeof(T)) - 1 - i] = static_cast<unsigned char>(v.w[i / 8] >> (8 * (i % 8)));
  return __builtin_bit_cast(T, by);
}

enum class fp_kind : unsigned char { zero, finite, inf, nan };

// A decoded value. For finite nonzero values: |x| = sig * 2^exp, 1 <= sig < 2^p (subnormals
// have fewer significant bits).
struct fp_value {
  bool neg = false;
  fp_kind kind = fp_kind::zero;
  bool signaling = false;
  int exp = 0;
  wide<2> sig;
};

template <class T>
constexpr fp_value fp_decode(T x) noexcept {
  using L = fp_layout<T>;
  const wide<2> bits = ycxx::detail::fpm::fp_to_bits(x);
  fp_value v;
  v.neg = ycxx::detail::fpm::wide_bit(bits, L::bits - 1);
  const int e = static_cast<int>(ycxx::detail::fpm::wide_shr(bits, L::fbits).w[0] & ((u64(1) << L::ebits) - 1));
  wide<2> frac = ycxx::detail::fpm::wide_low_bits(bits, L::fbits);
  if (e == (1 << L::ebits) - 1) {
    // Infinity or NaN. x87: the explicit bit is set in both; the fraction below it decides.
    const int top = L::explicit_bit ? L::fbits - 2 : L::fbits - 1; // the quiet bit
    wide<2> payload = frac;
    if (L::explicit_bit) payload.w[0] &= ~(u64(1) << 63);
    v.kind = ycxx::detail::fpm::wide_is_zero(payload) ? fp_kind::inf : fp_kind::nan;
    v.signaling = v.kind == fp_kind::nan && !ycxx::detail::fpm::wide_bit(frac, top);
    return v;
  }
  if (e == 0 && ycxx::detail::fpm::wide_is_zero(frac)) return v;
  v.kind = fp_kind::finite;
  v.sig = frac;
  if (e == 0) {
    v.exp = L::qmin;
  } else {
    if (!L::explicit_bit) ycxx::detail::fpm::wide_set_bit(v.sig, L::p - 1);
    v.exp = e - L::bias - (L::p - 1);
  }
  return v;
}

// Encodes a significand q < 2^p whose least significant bit has weight 2^lsb_exp, with
// lsb_exp >= qmin and (q < 2^(p-1) only if lsb_exp == qmin), or infinity / NaN.
template <class T>
constexpr T fp_encode_finite(bool neg, wide<2> q, int lsb_exp) noexcept {
  using L = fp_layout<T>;
  int e = 0;
  if (ycxx::detail::fpm::wide_bit(q, L::p - 1)) {
    e = lsb_exp + (L::p - 1) + L::bias;
    if (!L::explicit_bit) q.w[(L::p - 1) / 64] &= ~(u64(1) << ((L::p - 1) % 64));
  }
  wide<2> bits = q;
  wide<2> ef = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_from<2>(static_cast<u64>(e)), L::fbits);
  ycxx::detail::fpm::wide_add(bits, ef);
  if (neg) ycxx::detail::fpm::wide_set_bit(bits, L::bits - 1);
  return ycxx::detail::fpm::fp_from_bits<T>(bits);
}
template <class T>
constexpr T fp_make_special(bool nan, bool neg) noexcept {
  using L = fp_layout<T>;
  wide<2> bits = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_from<2>((u64(1) << L::ebits) - 1), L::fbits);
  if (nan) ycxx::detail::fpm::wide_set_bit(bits, L::explicit_bit ? L::fbits - 2 : L::fbits - 1);
  if (L::explicit_bit) ycxx::detail::fpm::wide_set_bit(bits, 63);
  if (neg) ycxx::detail::fpm::wide_set_bit(bits, L::bits - 1);
  return ycxx::detail::fpm::fp_from_bits<T>(bits);
}
// Built once per type (cheap during constant evaluation).
template <class T>
inline constexpr T fp_inf_v[2] = {ycxx::detail::fpm::fp_make_special<T>(false, false), ycxx::detail::fpm::fp_make_special<T>(false, true)};
template <class T>
inline constexpr T fp_qnan_v[2] = {ycxx::detail::fpm::fp_make_special<T>(true, false), ycxx::detail::fpm::fp_make_special<T>(true, true)};
template <class T>
constexpr T fp_infinity(bool neg) noexcept {
  return ycxx::detail::fpm::fp_inf_v<T>[neg];
}
template <class T>
constexpr T fp_quiet_nan(bool neg = false) noexcept {
  return ycxx::detail::fpm::fp_qnan_v<T>[neg];
}
template <class T>
constexpr T fp_zero(bool neg) noexcept {
  return neg ? -T(0) : T(0);
}

template <class T>
struct fp_result {
  T value;
  int flags;
};

// Rounds +-(mag * 2^exp + tail) to T, where the tail is in [0, 2^exp) and nonzero iff `sticky`.
// With a tail, mag must have at least two bits more than the result keeps (callers pass p + 2
// or more significant bits), so that the tail only ever acts as a sticky bit.
template <class T, int N>
constexpr fp_result<T> fp_round(bool neg, wide<N> mag, int exp, bool sticky = false) noexcept {
  using L = fp_layout<T>;
  const int len = ycxx::detail::fpm::wide_bitlen(mag);
  if (len == 0) return {ycxx::detail::fpm::fp_zero<T>(neg), 0};
  const long top = long(exp) + len - 1;
  if (top > L::emax) return {ycxx::detail::fpm::fp_infinity<T>(neg), fe_overflow | fe_inexact};
  long lsb = top - (L::p - 1);
  if (lsb < L::qmin) lsb = L::qmin;
  const long shift = lsb - exp;
  wide<2> q;
  bool inexact = sticky;
  if (shift <= 0) {
    q = ycxx::detail::fpm::wide_resize<2>(ycxx::detail::fpm::wide_shl(mag, static_cast<int>(-shift)));
  } else {
    const bool round = ycxx::detail::fpm::wide_bit(mag, static_cast<int>(shift - 1));
    bool rest = sticky;
    wide<N> m = ycxx::detail::fpm::wide_shr(mag, static_cast<int>(shift - 1), rest);
    q = ycxx::detail::fpm::wide_resize<2>(ycxx::detail::fpm::wide_shr(m, 1));
    inexact = round || rest;
    if (round && (rest || (q.w[0] & 1))) {
      ycxx::detail::fpm::wide_add_small(q, 1);
      if (ycxx::detail::fpm::wide_bit(q, L::p)) {
        q = ycxx::detail::fpm::wide_shr(q, 1);
        ++lsb;
        if (lsb + (L::p - 1) > L::emax) return {ycxx::detail::fpm::fp_infinity<T>(neg), fe_overflow | fe_inexact};
      }
    }
  }
  int flags = inexact ? fe_inexact : 0;
  if (inexact && !ycxx::detail::fpm::wide_bit(q, L::p - 1)) flags |= fe_underflow;
  return {ycxx::detail::fpm::fp_encode_finite<T>(neg, q, static_cast<int>(lsb)), flags};
}

// The exact value of a finite T as sig * 2^exp, normalised so that the top bit of sig is bit
// p - 1 (subnormals get a smaller exponent).
template <class T>
constexpr fp_value fp_normalize(fp_value v) noexcept {
  using L = fp_layout<T>;
  if (v.kind != fp_kind::finite) return v;
  const int len = ycxx::detail::fpm::wide_bitlen(v.sig);
  v.sig = ycxx::detail::fpm::wide_shl(v.sig, L::p - len);
  v.exp -= L::p - len;
  return v;
}

}} // namespace ycxx::detail::fpm
