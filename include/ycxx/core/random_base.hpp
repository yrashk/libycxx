// libycxx core: the shared machinery of <random> ([rand]): the template-argument checks of
// [rand.req.genl], wide integer arithmetic, the seed-sequence qualification of
// [rand.eng.general]/7, uniform integers and generate_canonical ([rand.util.canonical]), and the
// stream helpers the inserters and extractors use.
//
// The stream operators of <random> are hosted ([rand.req.eng]/6), but they are templates whose
// every use of the stream depends on its type, so they are defined here against the declarations
// of ycxx/core/iosfwd.hpp (DECISIONS §7) and need <istream>/<ostream> only where they are used.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/urbg.hpp>

namespace ycxx::detail {

using rand_u64 = unsigned long long;

// ---- [rand.req.genl]/1: template arguments -----------------------------------------------------
// IntType: a standard signed or unsigned integer type (signed char ... long long; not char,
// bool or the character types). UIntType: an unsigned integer type at least as wide as short.
// RealType: a standard floating-point type (the implementation-defined subset of the extended
// floating-point types is empty). cv-qualified arguments are rejected by all three.
template <class T>
inline constexpr bool rand_int_type =
    is_any_of<T, signed char, short, int, long, long long, unsigned char, unsigned short, unsigned int,
              unsigned long, unsigned long long>;
template <class T>
inline constexpr bool rand_uint_type = is_any_of<T, unsigned short, unsigned int, unsigned long, unsigned long long>;
template <class T>
inline constexpr bool rand_real_type = is_any_of<T, float, double, long double>;
// generate_canonical, which needs nothing beyond numeric_limits, also accepts the extended
// floating-point types and GNU __float128.
template <class T>
inline constexpr bool rand_canonical_type =
    rand_real_type<T> || is_any_of<T, float16, float32, float64, float128, bfloat16, gnu_float128>;

// ---- wide arithmetic ------------------------------------------------------------------------------
struct rand_u128 {
  rand_u64 hi, lo;
};
// The full 128-bit product of a and b.
template <class U = uint128>
constexpr rand_u128 rand_mul_wide(rand_u64 a, rand_u64 b) noexcept {
  if constexpr (cfg::has_int128) {
    U p = static_cast<U>(a) * b;
    return {static_cast<rand_u64>(p >> 64), static_cast<rand_u64>(p)};
  } else {
    const rand_u64 a0 = a & 0xffffffffu, a1 = a >> 32, b0 = b & 0xffffffffu, b1 = b >> 32;
    const rand_u64 p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    const rand_u64 mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
    return {p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32), (mid << 32) | (p00 & 0xffffffffu)};
  }
}
// x << s and x >> s for any s (0 once s reaches the width).
constexpr rand_u64 rand_shl(rand_u64 x, size_t s) noexcept { return s >= 64 ? 0 : x << s; }
constexpr rand_u64 rand_shr(rand_u64 x, size_t s) noexcept { return s >= 64 ? 0 : x >> s; }
// 2^w - 1 for 0 < w <= 64.
constexpr rand_u64 rand_mask(size_t w) noexcept { return w >= 64 ? ~0ull : (1ull << w) - 1; }
// (a + b) mod m for a, b < m.
constexpr rand_u64 rand_addmod(rand_u64 a, rand_u64 b, rand_u64 m) noexcept { return a >= m - b ? a - (m - b) : a + b; }
// (a * b) mod m for a, b < m, m > 0.
template <class U = uint128>
constexpr rand_u64 rand_mulmod(rand_u64 a, rand_u64 b, rand_u64 m) noexcept {
  if (m <= (1ull << 32))
    return a * b % m;
  if constexpr (cfg::has_int128) {
    return static_cast<rand_u64>(static_cast<U>(a) * b % m);
  } else {
    rand_u64 r = 0;
    for (int i = 63; i >= 0; --i) {
      r = ::ycxx::detail::rand_addmod(r, r, m);
      if ((b >> i) & 1)
        r = ::ycxx::detail::rand_addmod(r, a, m);
    }
    return r;
  }
}

// ---- [rand.eng.general]/7: what qualifies as a seed sequence ---------------------------------------
// Not implicitly convertible to the engine's result_type, and not one of the types whose own
// constructors the Sseq& templates would otherwise hijack (the engine itself and, for an adaptor,
// its base engine, when passed as non-const lvalues).
// It must also have the generate member the engines call (so that, e.g., a const seed_seq does not
// qualify).
template <class Q, class Result, class... Excluded>
concept rand_seed_seq = !std::is_convertible_v<Q, Result> && (!std::is_same_v<std::remove_cv_t<Q>, Excluded> && ...) &&
                        requires(Q& q, std::uint_least32_t* p) { q.generate(p, p); };

// ---- uniform integers --------------------------------------------------------------------------------
// A uniformly distributed value in [0, n], drawn from g without bias. A generator whose range is
// a power of two uses Lemire's multiply-and-reject method; any other range rejects the values
// above the largest multiple of n + 1; a range smaller than n + 1 combines several draws.
template <class G>
rand_u64 rand_uniform_upto(G& g, rand_u64 n) {
  using GT = std::remove_cvref_t<G>;
  using R = std::invoke_result_t<G&>;
  constexpr rand_u64 gmin = static_cast<rand_u64>(GT::min());
  constexpr rand_u64 grange = static_cast<rand_u64>(GT::max()) - gmin;
  auto draw = [&g] { return static_cast<rand_u64>(static_cast<R>(g())) - gmin; };
  if (n == 0)
    return 0;
  if (n == grange)
    return draw();
  if (n < grange) {
    const rand_u64 s = n + 1;
    if constexpr ((grange & (grange + 1)) == 0) {
      // Lemire: x * s / 2^b for a b-bit x, rejecting the (2^b mod s) low products that bias it.
      constexpr int b = std::bit_width(grange);
      if constexpr (b <= 32) {
        rand_u64 m = draw() * s;
        if ((m & grange) < s) {
          const rand_u64 t = (grange - n) % s;
          while ((m & grange) < t)
            m = draw() * s;
        }
        return m >> b;
      } else {
        rand_u128 m = ::ycxx::detail::rand_mul_wide(draw(), s);
        if ((m.lo & grange) < s) {
          const rand_u64 t = (grange - n) % s;
          while ((m.lo & grange) < t)
            m = ::ycxx::detail::rand_mul_wide(draw(), s);
        }
        if constexpr (b == 64)
          return m.hi;
        else
          return (m.hi << (64 - b)) | (m.lo >> b);
      }
    } else {
      // Accept v below the largest multiple of s in [0, grange + 1).
      const rand_u64 limit = grange - (grange + 1) % s; // values in [0, limit] are accepted
      for (;;) {
        rand_u64 v = draw();
        if (v <= limit)
          return v % s;
      }
    }
  }
  // n > grange: v = hi * (grange + 1) + lo with hi uniform in [0, n / (grange + 1)].
  constexpr rand_u64 base = grange + 1;
  for (;;) {
    const rand_u64 hi = ::ycxx::detail::rand_uniform_upto(g, n / base);
    const rand_u64 lo = draw();
    if (hi <= (n - lo) / base)
      return hi * base + lo;
  }
}

// ---- [rand.util.canonical] ------------------------------------------------------------------------
// A 192-bit unsigned integer for the general case of generate_canonical: S < R^k < R * 2^d
// <= 2^64 * 2^113.
struct rand_big {
  rand_u64 w[3] = {0, 0, 0}; // little-endian limbs

  constexpr void mul_add(rand_u64 m, rand_u64 a) noexcept { // *this = *this * m + a
    rand_u64 carry = a;
    for (rand_u64& limb : w) {
      const rand_u128 p = ::ycxx::detail::rand_mul_wide(limb, m);
      limb = p.lo + carry;
      carry = p.hi + (limb < carry);
    }
  }
  constexpr void add(const rand_big& o) noexcept {
    rand_u64 carry = 0;
    for (int i = 0; i < 3; ++i) {
      const rand_u64 t = w[i] + carry;
      const rand_u64 c1 = t < carry;
      w[i] = t + o.w[i];
      carry = c1 + (w[i] < t);
    }
  }
  constexpr void sub(const rand_big& o) noexcept { // requires *this >= o
    rand_u64 borrow = 0;
    for (int i = 0; i < 3; ++i) {
      const rand_u64 t = w[i] - o.w[i];
      const rand_u64 b1 = w[i] < o.w[i];
      w[i] = t - borrow;
      borrow = b1 + (t < borrow);
    }
  }
  constexpr void shl1() noexcept {
    w[2] = (w[2] << 1) | (w[1] >> 63);
    w[1] = (w[1] << 1) | (w[0] >> 63);
    w[0] <<= 1;
  }
  constexpr bool bit(size_t i) const noexcept { return (w[i / 64] >> (i % 64)) & 1; }
  constexpr void set_bit(size_t i) noexcept { w[i / 64] |= 1ull << (i % 64); }
  constexpr size_t bit_length() const noexcept {
    for (int i = 2; i >= 0; --i)
      if (w[i] != 0)
        return static_cast<size_t>(i) * 64 + static_cast<size_t>(std::bit_width(w[i]));
    return 0;
  }
  constexpr rand_big shr(size_t s) const noexcept {
    rand_big r;
    for (size_t i = 0; i < 192; ++i)
      if (i + s < 192 && bit(i + s))
        r.set_bit(i);
    return r;
  }
  constexpr rand_big shl(size_t s) const noexcept {
    rand_big r;
    for (size_t i = s; i < 192; ++i)
      if (bit(i - s))
        r.set_bit(i);
    return r;
  }
  friend constexpr bool operator<(const rand_big& a, const rand_big& b) noexcept {
    for (int i = 2; i >= 0; --i)
      if (a.w[i] != b.w[i])
        return a.w[i] < b.w[i];
    return false;
  }
  // floor(*this / d) for d > 0, by binary long division.
  constexpr rand_big div(const rand_big& d) const noexcept {
    rand_big q, r;
    for (size_t i = bit_length(); i-- > 0;) {
      r.shl1();
      if (bit(i))
        r.w[0] |= 1;
      if (!(r < d)) {
        r.sub(d);
        q.set_bit(i);
      }
    }
    return q;
  }
};

// v * 2^-d for an integer v < 2^d given as two limbs; exact since d <= digits of Real. Computed in
// long double when that holds every value of Real (so that 2^64 does not overflow a narrow type).
template <class Real>
Real rand_scale_down(rand_u64 hi, rand_u64 lo, size_t d) {
  using W = std::conditional_t<(std::numeric_limits<Real>::digits <= std::numeric_limits<long double>::digits),
                               long double, Real>;
  W v = W(lo);
  if (hi != 0)
    v += W(hi) * (W(4294967296.0) * W(4294967296.0));
  W scale = 1;
  for (; d >= 32; d -= 32)
    scale /= W(4294967296.0);
  for (; d > 0; --d)
    scale /= 2;
  return static_cast<Real>(v * scale);
}

} // namespace ycxx::detail

namespace std {

template <class RealType, size_t digits, class URBG>
RealType generate_canonical(URBG& g) {
  static_assert(ycxx::detail::rand_canonical_type<RealType>,
                "generate_canonical: RealType must be a floating-point type ([rand.req.genl]/1.5)");
  using ycxx::detail::rand_u64;
  using L = numeric_limits<RealType>;
  static_assert(L::radix == 2);
  constexpr size_t d = digits < static_cast<size_t>(L::digits) ? digits : static_cast<size_t>(L::digits);
  constexpr rand_u64 gmin = static_cast<rand_u64>(URBG::min());
  constexpr rand_u64 rm1 = static_cast<rand_u64>(URBG::max()) - gmin; // R - 1
  auto draw = [&g] { return static_cast<rand_u64>(g()) - gmin; };
  if constexpr (d == 0) {
    return RealType(0);
  } else if constexpr ((rm1 & (rm1 + 1)) == 0) {
    // R = 2^b: one attempt; the result is the top d of the k*b bits of S, all the bits below
    // them coming from g_0 (k*b - d < b).
    constexpr size_t b = static_cast<size_t>(std::bit_width(rm1));
    constexpr size_t k = (d + b - 1) / b;
    constexpr size_t drop = k * b - d;
    rand_u64 hi = 0, lo = draw() >> drop; // the d-bit result as two limbs
    for (size_t i = 1; i < k; ++i) {
      const rand_u64 v = draw();
      const size_t pos = i * b - drop;
      lo |= ycxx::detail::rand_shl(v, pos);
      if (pos + b > 64)
        hi |= pos >= 64 ? ycxx::detail::rand_shl(v, pos - 64) : ycxx::detail::rand_shr(v, 64 - pos);
    }
    return ycxx::detail::rand_scale_down<RealType>(hi, lo, d);
  } else {
    // General R: R^k and x = floor(R^k / 2^d) are compile-time constants; attempts are made until
    // S < x * 2^d, then the result is floor(S / x) / 2^d.
    constexpr rand_u64 R = rm1 + 1;
    struct consts {
      size_t k = 0;
      ycxx::detail::rand_big rk, x, limit;
    };
    constexpr consts c = [] {
      consts r;
      r.rk.w[0] = 1;
      while (r.rk.bit_length() <= d) { // R^k < 2^d
        r.rk.mul_add(R, 0);
        ++r.k;
      }
      r.x = r.rk.shr(d);
      r.limit = r.x.shl(d);
      return r;
    }();
    if constexpr (c.rk.w[1] == 0 && c.rk.w[2] == 0) {
      // R^k fits in 64 bits.
      constexpr rand_u64 x = c.x.w[0], limit = c.limit.w[0];
      for (;;) {
        rand_u64 s = 0, p = 1;
        for (size_t i = 0; i < c.k; ++i, p *= R)
          s += draw() * p;
        if (s < limit)
          return ycxx::detail::rand_scale_down<RealType>(0, s / x, d);
      }
    } else {
      for (;;) {
        ycxx::detail::rand_big s, p;
        p.w[0] = 1;
        for (size_t i = 0; i < c.k; ++i) {
          ycxx::detail::rand_big t = p;
          t.mul_add(draw(), 0);
          s.add(t);
          p.mul_add(R, 0);
        }
        if (s < c.limit) {
          const ycxx::detail::rand_big q = s.div(c.x);
          return ycxx::detail::rand_scale_down<RealType>(q.w[1], q.w[0], d);
        }
      }
    }
  }
}

} // namespace std

namespace ycxx::detail {

// A uniform value in [0, 1) with all the digits of Real; and in (0, 1) and (0, 1].
template <class Real, class G>
Real rand_canonical(G& g) {
  return std::generate_canonical<Real, static_cast<size_t>(std::numeric_limits<Real>::digits)>(g);
}
template <class Real, class G>
Real rand_open01(G& g) {
  for (;;) {
    Real u = ::ycxx::detail::rand_canonical<Real>(g);
    if (u != Real(0))
      return u;
  }
}
template <class Real, class G>
Real rand_left_open01(G& g) {
  return Real(1) - ::ycxx::detail::rand_canonical<Real>(g);
}

// ---- stream helpers ([rand.req.eng]/6-17, [rand.req.dist]/11-20) ----------------------------------
// Sets a stream's format flags (and, for output, the fill character) for the duration of an
// inserter or extractor and restores them, with the precision, afterwards.
template <class S>
class rand_io_state {
public:
  using flags_type = typename S::fmtflags;
  rand_io_state(S& s, flags_type f) : s_(s), flags_(s.flags(f)), fill_(s.fill()), precision_(s.precision()) {
    s.fill(s.widen(' '));
  }
  rand_io_state(const rand_io_state&) = delete;
  rand_io_state& operator=(const rand_io_state&) = delete;
  ~rand_io_state() {
    s_.flags(flags_);
    s_.fill(fill_);
    s_.precision(precision_);
  }

private:
  S& s_;
  flags_type flags_;
  typename S::char_type fill_;
  decltype(std::declval<S&>().precision()) precision_;
};

// Engines are written with dec|left and a space fill; distributions additionally with every
// floating-point value in scientific notation with max_digits10 significant digits, so that it
// reads back exactly. Extractors read with dec (and skipws, to cross the separators).
template <class OS>
rand_io_state<OS> rand_out_engine(OS& os) {
  return rand_io_state<OS>(os, OS::dec | OS::left);
}
template <class OS>
rand_io_state<OS> rand_out_dist(OS& os) {
  return rand_io_state<OS>(os, OS::dec | OS::left | OS::scientific);
}
template <class IS>
rand_io_state<IS> rand_in(IS& is) {
  return rand_io_state<IS>(is, IS::dec | IS::skipws);
}

// Writes values separated by single spaces.
template <class OS, class T>
void rand_put1(OS& os, const T& v) {
  if constexpr (std::is_floating_point_v<T>) {
    os.precision(std::numeric_limits<T>::max_digits10 - 1);
    os << v;
  } else if constexpr (std::is_same_v<T, bool>) {
    os << static_cast<int>(v);
  } else if constexpr (std::is_signed_v<T>) {
    os << static_cast<long long>(v);
  } else {
    os << static_cast<unsigned long long>(v);
  }
}
template <class OS, class T>
void rand_put_sep(OS& os, const T& v) {
  os.put(os.widen(' '));
  ::ycxx::detail::rand_put1(os, v);
}
template <class OS, class T, class... Ts>
void rand_put(OS& os, const T& v, const Ts&... vs) {
  ::ycxx::detail::rand_put1(os, v);
  (::ycxx::detail::rand_put_sep(os, vs), ...);
}
// Writes a sequence of values, each preceded by a space.
template <class OS, class Seq>
void rand_put_seq(OS& os, const Seq& s) {
  for (const auto& v : s)
    ::ycxx::detail::rand_put_sep(os, v);
}

// Reads one value; on a malformed or out-of-range value sets failbit and leaves v unchanged.
template <class IS, class T>
bool rand_get1(IS& is, T& v) {
  if constexpr (std::is_floating_point_v<T>) {
    T x{};
    is >> x;
    if (!is.fail())
      v = x;
  } else if constexpr (std::is_signed_v<T> || std::is_same_v<T, bool>) {
    long long x = 0;
    is >> x;
    if (!is.fail()) {
      if (x < static_cast<long long>(std::numeric_limits<T>::min()) ||
          x > static_cast<long long>(std::numeric_limits<T>::max()))
        is.setstate(IS::failbit);
      else
        v = static_cast<T>(x);
    }
  } else {
    unsigned long long x = 0;
    is >> x;
    if (!is.fail()) {
      if (x > static_cast<unsigned long long>(std::numeric_limits<T>::max()))
        is.setstate(IS::failbit);
      else
        v = static_cast<T>(x);
    }
  }
  return !is.fail();
}
template <class IS, class... Ts>
bool rand_get(IS& is, Ts&... vs) {
  return (::ycxx::detail::rand_get1(is, vs) && ...);
}

} // namespace ycxx::detail
