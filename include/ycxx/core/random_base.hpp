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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

using __rand_u64 = unsigned long long;

// ---- [rand.req.genl]/1: template arguments -----------------------------------------------------
// IntType: a standard signed or unsigned integer type (signed char ... long long; not char,
// bool or the character types). UIntType: an unsigned integer type at least as wide as short.
// RealType: a standard floating-point type (the implementation-defined subset of the extended
// floating-point types is empty). cv-qualified arguments are rejected by all three.
template <class _Tp>
inline constexpr bool __rand_int_type =
    __is_any_of<_Tp, signed char, short, int, long, long long, unsigned char, unsigned short, unsigned int,
              unsigned long, unsigned long long>;
template <class _Tp>
inline constexpr bool __rand_uint_type = __is_any_of<_Tp, unsigned short, unsigned int, unsigned long, unsigned long long>;
template <class _Tp>
inline constexpr bool __rand_real_type = __is_any_of<_Tp, float, double, long double>;
// generate_canonical, which needs nothing beyond numeric_limits, also accepts the extended
// floating-point types and GNU __float128.
template <class _Tp>
inline constexpr bool __rand_canonical_type =
    __rand_real_type<_Tp> || __is_any_of<_Tp, __float16, __float32, __float64, __y_float128, __bfloat16, __gnu_float128>;

// ---- wide arithmetic ------------------------------------------------------------------------------
struct __rand_u128 {
  __rand_u64 __hi, __lo;
};
// The full 128-bit product of a and b.
template <class _Up = __uint128>
constexpr __rand_u128 __rand_mul_wide(__rand_u64 a, __rand_u64 b) noexcept {
  if constexpr (__cfg::__has_int128) {
    _Up p = static_cast<_Up>(a) * b;
    return {static_cast<__rand_u64>(p >> 64), static_cast<__rand_u64>(p)};
  } else {
    const __rand_u64 __a0 = a & 0xffffffffu, __a1 = a >> 32, __b0 = b & 0xffffffffu, __b1 = b >> 32;
    const __rand_u64 __p00 = __a0 * __b0, __p01 = __a0 * __b1, __p10 = __a1 * __b0, __p11 = __a1 * __b1;
    const __rand_u64 __mid = (__p00 >> 32) + (__p01 & 0xffffffffu) + (__p10 & 0xffffffffu);
    return {__p11 + (__p01 >> 32) + (__p10 >> 32) + (__mid >> 32), (__mid << 32) | (__p00 & 0xffffffffu)};
  }
}
// x << s and x >> s for any s (0 once s reaches the width).
constexpr __rand_u64 __rand_shl(__rand_u64 __x, size_t s) noexcept { return s >= 64 ? 0 : __x << s; }
constexpr __rand_u64 __rand_shr(__rand_u64 __x, size_t s) noexcept { return s >= 64 ? 0 : __x >> s; }
// 2^w - 1 for 0 < w <= 64.
constexpr __rand_u64 __rand_mask(size_t __w) noexcept { return __w >= 64 ? ~0ull : (1ull << __w) - 1; }
// (a + b) mod m for a, b < m.
constexpr __rand_u64 __rand_addmod(__rand_u64 a, __rand_u64 b, __rand_u64 m) noexcept { return a >= m - b ? a - (m - b) : a + b; }
// (a * b) mod m for a, b < m, m > 0.
template <class _Up = __uint128>
constexpr __rand_u64 __rand_mulmod(__rand_u64 a, __rand_u64 b, __rand_u64 m) noexcept {
  if (m <= (1ull << 32))
    return a * b % m;
  if constexpr (__cfg::__has_int128) {
    return static_cast<__rand_u64>(static_cast<_Up>(a) * b % m);
  } else {
    __rand_u64 r = 0;
    for (int i = 63; i >= 0; --i) {
      r = ::__ycxx::__detail::__rand_addmod(r, r, m);
      if ((b >> i) & 1)
        r = ::__ycxx::__detail::__rand_addmod(r, a, m);
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
template <class _Qp, class _Result, class... _Excluded>
concept __rand_seed_seq = !std::is_convertible_v<_Qp, _Result> && (!std::is_same_v<std::remove_cv_t<_Qp>, _Excluded> && ...) &&
                        requires(_Qp& __q, std::uint_least32_t* p) { __q.generate(p, p); };

// ---- uniform integers --------------------------------------------------------------------------------
// A uniformly distributed value in [0, n], drawn from g without bias. A generator whose range is
// a power of two uses Lemire's multiply-and-reject method; any other range rejects the values
// above the largest multiple of n + 1; a range smaller than n + 1 combines several draws.
template <class _Gp>
__rand_u64 __rand_uniform_upto(_Gp& __g, __rand_u64 n) {
  using _GT = std::remove_cvref_t<_Gp>;
  using _Rp = std::invoke_result_t<_Gp&>;
  // Wider generators must be reduced without bias before the 64-bit integer mapping.
  // Accept a whole number of 2^64-sized blocks, then take the low limb of each draw.
  constexpr bool __wide = static_cast<_Rp>(_GT::max() - _GT::min()) > static_cast<_Rp>(~0ull);
  constexpr __rand_u64 __gmin = __wide ? 0 : static_cast<__rand_u64>(_GT::min());
  constexpr __rand_u64 __grange = __wide ? ~0ull : static_cast<__rand_u64>(_GT::max()) - __gmin;
  auto __draw = [&__g] {
    if constexpr (__wide) {
      constexpr _Rp __wrange = static_cast<_Rp>(_GT::max() - _GT::min());
      constexpr _Rp __base = static_cast<_Rp>(~0ull) + 1;
      constexpr _Rp __limit = __wrange % __base == __base - 1 ? __wrange : __wrange / __base * __base - 1;
      for (;;) {
        const _Rp __value = static_cast<_Rp>(__g() - _GT::min());
        if (__value <= __limit)
          return static_cast<__rand_u64>(__value);
      }
    } else {
      return static_cast<__rand_u64>(static_cast<_Rp>(__g())) - __gmin;
    }
  };
  if (n == 0)
    return 0;
  if (n == __grange)
    return __draw();
  if (n < __grange) {
    const __rand_u64 s = n + 1;
    if constexpr ((__grange & (__grange + 1)) == 0) {
      // Lemire: x * s / 2^b for a b-bit x, rejecting the (2^b mod s) low products that bias it.
      constexpr int b = std::bit_width(__grange);
      if constexpr (b <= 32) {
        __rand_u64 m = __draw() * s;
        if ((m & __grange) < s) {
          const __rand_u64 t = (__grange - n) % s;
          while ((m & __grange) < t)
            m = __draw() * s;
        }
        return m >> b;
      } else {
        __rand_u128 m = ::__ycxx::__detail::__rand_mul_wide(__draw(), s);
        if ((m.__lo & __grange) < s) {
          const __rand_u64 t = (__grange - n) % s;
          while ((m.__lo & __grange) < t)
            m = ::__ycxx::__detail::__rand_mul_wide(__draw(), s);
        }
        if constexpr (b == 64)
          return m.__hi;
        else
          return (m.__hi << (64 - b)) | (m.__lo >> b);
      }
    } else {
      // Accept v below the largest multiple of s in [0, grange + 1).
      const __rand_u64 __limit = __grange - (__grange + 1) % s; // values in [0, limit] are accepted
      for (;;) {
        __rand_u64 __v = __draw();
        if (__v <= __limit)
          return __v % s;
      }
    }
  }
  // n > grange: v = hi * (grange + 1) + lo with hi uniform in [0, n / (grange + 1)].
  constexpr __rand_u64 base = __grange + 1;
  for (;;) {
    const __rand_u64 __hi = ::__ycxx::__detail::__rand_uniform_upto(__g, n / base);
    const __rand_u64 __lo = __draw();
    if (__hi <= (n - __lo) / base)
      return __hi * base + __lo;
  }
}

// ---- [rand.util.canonical] ------------------------------------------------------------------------
// A 256-bit unsigned integer for generate_canonical: S < R^k < R * 2^d,
// including 128-bit generator ranges and the supported 113-bit floating significands.
struct __rand_big {
  __rand_u64 __w[4] = {0, 0, 0, 0}; // little-endian limbs

  constexpr void __mul_add(__rand_u64 m, __rand_u64 a) noexcept { // *this = *this * m + a
    __rand_u64 __carry = a;
    for (__rand_u64& __limb : __w) {
      const __rand_u128 p = ::__ycxx::__detail::__rand_mul_wide(__limb, m);
      __limb = p.__lo + __carry;
      __carry = p.__hi + (__limb < __carry);
    }
  }
  // Multiplication/addition by a generator value wider than one limb (at most 128 bits).
  template <class _UInt>
  constexpr void __mul_add_wide(_UInt m, _UInt a) noexcept {
    __rand_big __upper = *this;
    __upper.__mul_add(static_cast<__rand_u64>(m >> 64), 0);
    __upper = __upper.shl(64);
    __mul_add(static_cast<__rand_u64>(m), static_cast<__rand_u64>(a));
    add(__upper);
    __rand_big __high_add;
    __high_add.__w[1] = static_cast<__rand_u64>(a >> 64);
    add(__high_add);
  }
  constexpr void add(const __rand_big& __o) noexcept {
    __rand_u64 __carry = 0;
    for (int i = 0; i < 4; ++i) {
      const __rand_u64 t = __w[i] + __carry;
      const __rand_u64 __c1 = t < __carry;
      __w[i] = t + __o.__w[i];
      __carry = __c1 + (__w[i] < t);
    }
  }
  constexpr void __sub(const __rand_big& __o) noexcept { // requires *this >= o
    __rand_u64 __borrow = 0;
    for (int i = 0; i < 4; ++i) {
      const __rand_u64 t = __w[i] - __o.__w[i];
      const __rand_u64 __b1 = __w[i] < __o.__w[i];
      __w[i] = t - __borrow;
      __borrow = __b1 + (t < __borrow);
    }
  }
  constexpr void __shl1() noexcept {
    for (int i = 3; i > 0; --i)
      __w[i] = (__w[i] << 1) | (__w[i - 1] >> 63);
    __w[0] <<= 1;
  }
  constexpr bool __bit(size_t i) const noexcept { return (__w[i / 64] >> (i % 64)) & 1; }
  constexpr void __set_bit(size_t i) noexcept { __w[i / 64] |= 1ull << (i % 64); }
  constexpr size_t __bit_length() const noexcept {
    for (int i = 3; i >= 0; --i)
      if (__w[i] != 0)
        return static_cast<size_t>(i) * 64 + static_cast<size_t>(std::bit_width(__w[i]));
    return 0;
  }
  constexpr __rand_big shr(size_t s) const noexcept {
    __rand_big r;
    for (size_t i = 0; i < 256; ++i)
      if (i + s < 256 && __bit(i + s))
        r.__set_bit(i);
    return r;
  }
  constexpr __rand_big shl(size_t s) const noexcept {
    __rand_big r;
    for (size_t i = s; i < 256; ++i)
      if (__bit(i - s))
        r.__set_bit(i);
    return r;
  }
  friend constexpr bool operator<(const __rand_big& a, const __rand_big& b) noexcept {
    for (int i = 3; i >= 0; --i)
      if (a.__w[i] != b.__w[i])
        return a.__w[i] < b.__w[i];
    return false;
  }
  // floor(*this / d) for d > 0, by binary long division.
  constexpr __rand_big div(const __rand_big& d) const noexcept {
    __rand_big __q, r;
    for (size_t i = __bit_length(); i-- > 0;) {
      r.__shl1();
      if (__bit(i))
        r.__w[0] |= 1;
      if (!(r < d)) {
        r.__sub(d);
        __q.__set_bit(i);
      }
    }
    return __q;
  }
};

// v * 2^-d for an integer v < 2^d given as two limbs; exact since d <= digits of Real. Computed in
// long double when that holds every value of Real (so that 2^64 does not overflow a narrow type).
template <class _Real>
_Real __rand_scale_down(__rand_u64 __hi, __rand_u64 __lo, size_t d) {
  using _Wp = std::conditional_t<(std::numeric_limits<_Real>::digits <= std::numeric_limits<long double>::digits),
                               long double, _Real>;
  _Wp __v = _Wp(__lo);
  if (__hi != 0)
    __v += _Wp(__hi) * (_Wp(4294967296.0) * _Wp(4294967296.0));
  _Wp scale = 1;
  for (; d >= 32; d -= 32)
    scale /= _Wp(4294967296.0);
  for (; d > 0; --d)
    scale /= 2;
  return static_cast<_Real>(__v * scale);
}

// The C++26 arithmetic with a generator range wider than 64 bits. Keeping the draws
// in their own unsigned type avoids narrowing either R or the high bits of S.
template <class _Real, size_t _Digits, class _Gp>
_Real __rand_canonical_wide(_Gp& __g) {
  using _UInt = std::invoke_result_t<_Gp&>;
  constexpr _UInt __min = _Gp::min();
  constexpr _UInt __rm1 = _Gp::max() - __min;
  if constexpr (_Digits == 0) {
    return _Real(0);
  } else if constexpr ((__rm1 & (__rm1 + 1)) == 0) {
    constexpr size_t b = static_cast<size_t>(std::bit_width(__rm1));
    constexpr size_t k = (_Digits + b - 1) / b;
    __rand_big s;
    for (size_t i = 0; i < k; ++i) {
      const _UInt __draw = __g() - __min;
      __rand_big t;
      t.__w[0] = static_cast<__rand_u64>(__draw);
      t.__w[1] = static_cast<__rand_u64>(__draw >> 64);
      s.add(t.shl(i * b));
    }
    const __rand_big __q = s.shr(k * b - _Digits);
    return __rand_scale_down<_Real>(__q.__w[1], __q.__w[0], _Digits);
  } else {
    constexpr _UInt __base = __rm1 + 1;
    struct __consts {
      size_t k = 0;
      __rand_big __rk, __x, __limit;
    };
    constexpr __consts c = [] {
      __consts r;
      r.__rk.__w[0] = 1;
      while (r.__rk.__bit_length() <= _Digits) {
        r.__rk.__mul_add_wide(__base, _UInt(0));
        ++r.k;
      }
      r.__x = r.__rk.shr(_Digits);
      r.__limit = r.__x.shl(_Digits);
      return r;
    }();
    for (;;) {
      __rand_big s, p;
      p.__w[0] = 1;
      for (size_t i = 0; i < c.k; ++i) {
        __rand_big t = p;
        t.__mul_add_wide(_UInt(__g() - __min), _UInt(0));
        s.add(t);
        p.__mul_add_wide(__base, _UInt(0));
      }
      if (s < c.__limit) {
        const __rand_big __q = s.div(c.__x);
        return __rand_scale_down<_Real>(__q.__w[1], __q.__w[0], _Digits);
      }
    }
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _RealType, size_t digits, class _URBG>
_RealType generate_canonical(_URBG& __g) {
  static_assert(__ycxx::__detail::__rand_canonical_type<_RealType>,
                "generate_canonical: RealType must be a floating-point type ([rand.req.genl]/1.5)");
  using __ycxx::__detail::__rand_u64;
  using _Lp = numeric_limits<_RealType>;
  static_assert(_Lp::radix == 2);
  constexpr size_t d = digits < static_cast<size_t>(_Lp::digits) ? digits : static_cast<size_t>(_Lp::digits);
  using _UInt = invoke_result_t<_URBG&>;
  constexpr _UInt __range = _URBG::max() - _URBG::min();
  if constexpr (__range > static_cast<_UInt>(~0ull)) {
    return __ycxx::__detail::__rand_canonical_wide<_RealType, d>(__g);
  } else {
    constexpr __rand_u64 __gmin = static_cast<__rand_u64>(_URBG::min());
    constexpr __rand_u64 __rm1 = static_cast<__rand_u64>(_URBG::max()) - __gmin; // R - 1
    auto __draw = [&__g] { return static_cast<__rand_u64>(__g()) - __gmin; };
    if constexpr (d == 0) {
      return _RealType(0);
    } else if constexpr ((__rm1 & (__rm1 + 1)) == 0) {
      // R = 2^b: one attempt; the result is the top d of the k*b bits of S, all the bits below
      // them coming from g_0 (k*b - d < b).
      constexpr size_t b = static_cast<size_t>(std::bit_width(__rm1));
      constexpr size_t k = (d + b - 1) / b;
      constexpr size_t drop = k * b - d;
      __rand_u64 __hi = 0, __lo = __draw() >> drop; // the d-bit result as two limbs
      for (size_t i = 1; i < k; ++i) {
        const __rand_u64 __v = __draw();
        const size_t __pos = i * b - drop;
        __lo |= __ycxx::__detail::__rand_shl(__v, __pos);
        if (__pos + b > 64)
          __hi |= __pos >= 64 ? __ycxx::__detail::__rand_shl(__v, __pos - 64) : __ycxx::__detail::__rand_shr(__v, 64 - __pos);
      }
      if constexpr (d <= 64) {
        // lo < 2^d with d <= digits converts exactly, and scaling by 2^-d is exact.
        constexpr _RealType scale = [] {
          _RealType s = 1;
          for (size_t i = 0; i < d; ++i)
            s /= 2;
          return s;
        }();
        return static_cast<_RealType>(__lo) * scale;
      } else {
        return __ycxx::__detail::__rand_scale_down<_RealType>(__hi, __lo, d);
      }
    } else {
      // General R: R^k and x = floor(R^k / 2^d) are compile-time constants; attempts are made until
      // S < x * 2^d, then the result is floor(S / x) / 2^d.
      constexpr __rand_u64 _Rp = __rm1 + 1;
      struct __consts {
        size_t k = 0;
        __ycxx::__detail::__rand_big __rk, __x, __limit;
      };
      constexpr __consts c = [] {
        __consts r;
        r.__rk.__w[0] = 1;
        while (r.__rk.__bit_length() <= d) { // R^k < 2^d
          r.__rk.__mul_add(_Rp, 0);
          ++r.k;
        }
        r.__x = r.__rk.shr(d);
        r.__limit = r.__x.shl(d);
        return r;
      }();
      if constexpr (c.__rk.__w[1] == 0 && c.__rk.__w[2] == 0 && c.__rk.__w[3] == 0) {
        // R^k fits in 64 bits.
        constexpr __rand_u64 __x = c.__x.__w[0], __limit = c.__limit.__w[0];
        for (;;) {
          __rand_u64 s = 0, p = 1;
          for (size_t i = 0; i < c.k; ++i, p *= _Rp)
            s += __draw() * p;
          if (s < __limit)
            return __ycxx::__detail::__rand_scale_down<_RealType>(0, s / __x, d);
        }
      } else {
        for (;;) {
          __ycxx::__detail::__rand_big s, p;
          p.__w[0] = 1;
          for (size_t i = 0; i < c.k; ++i) {
            __ycxx::__detail::__rand_big t = p;
            t.__mul_add(__draw(), 0);
            s.add(t);
            p.__mul_add(_Rp, 0);
          }
          if (s < c.__limit) {
            const __ycxx::__detail::__rand_big __q = s.div(c.__x);
            return __ycxx::__detail::__rand_scale_down<_RealType>(__q.__w[1], __q.__w[0], d);
          }
        }
      }
    }
  }
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// A uniform value in [0, 1) with all the digits of Real; and in (0, 1) and (0, 1].
template <class _Real, class _Gp>
_Real __rand_canonical(_Gp& __g) {
  return std::generate_canonical<_Real, static_cast<size_t>(std::numeric_limits<_Real>::digits)>(__g);
}
template <class _Real, class _Gp>
_Real __rand_open01(_Gp& __g) {
  for (;;) {
    _Real __u = ::__ycxx::__detail::__rand_canonical<_Real>(__g);
    if (__u != _Real(0))
      return __u;
  }
}
template <class _Real, class _Gp>
_Real __rand_left_open01(_Gp& __g) {
  return _Real(1) - ::__ycxx::__detail::__rand_canonical<_Real>(__g);
}

// ---- stream helpers ([rand.req.eng]/6-17, [rand.req.dist]/11-20) ----------------------------------
// Sets a stream's format flags (and, for output, the fill character) for the duration of an
// inserter or extractor and restores them, with the precision, afterwards.
template <class _Sp>
class __rand_io_state {
public:
  using __flags_type = typename _Sp::fmtflags;
  __rand_io_state(_Sp& s, __flags_type __f) : __s_(s), __flags_(s.flags(__f)), __fill_(s.fill()), __precision_(s.precision()) {
    s.fill(s.widen(' '));
  }
  __rand_io_state(const __rand_io_state&) = delete;
  __rand_io_state& operator=(const __rand_io_state&) = delete;
  ~__rand_io_state() {
    __s_.flags(__flags_);
    __s_.fill(__fill_);
    __s_.precision(__precision_);
  }

private:
  _Sp& __s_;
  __flags_type __flags_;
  typename _Sp::char_type __fill_;
  decltype(std::declval<_Sp&>().precision()) __precision_;
};

// Engines are written with dec|left and a space fill; distributions additionally with every
// floating-point value in scientific notation with max_digits10 significant digits, so that it
// reads back exactly. Extractors read with dec (and skipws, to cross the separators).
template <class _OS>
__rand_io_state<_OS> __rand_out_engine(_OS& __os) {
  return __rand_io_state<_OS>(__os, _OS::dec | _OS::left);
}
template <class _OS>
__rand_io_state<_OS> __rand_out_dist(_OS& __os) {
  return __rand_io_state<_OS>(__os, _OS::dec | _OS::left | _OS::scientific);
}
template <class _IS>
__rand_io_state<_IS> __rand_in(_IS& is) {
  return __rand_io_state<_IS>(is, _IS::dec | _IS::skipws);
}

// Writes values separated by single spaces.
template <class _OS, class _Tp>
void __rand_put1(_OS& __os, const _Tp& __v) {
  if constexpr (std::is_floating_point_v<_Tp>) {
    __os.precision(std::numeric_limits<_Tp>::max_digits10 - 1);
    __os << __v;
  } else if constexpr (std::is_same_v<_Tp, bool>) {
    __os << static_cast<int>(__v);
  } else if constexpr (std::is_signed_v<_Tp>) {
    __os << static_cast<long long>(__v);
  } else {
    __os << static_cast<unsigned long long>(__v);
  }
}
template <class _OS, class _Tp>
void __rand_put_sep(_OS& __os, const _Tp& __v) {
  __os.put(__os.widen(' '));
  ::__ycxx::__detail::__rand_put1(__os, __v);
}
template <class _OS, class _Tp, class... _Ts>
void __rand_put(_OS& __os, const _Tp& __v, const _Ts&... __vs) {
  ::__ycxx::__detail::__rand_put1(__os, __v);
  (::__ycxx::__detail::__rand_put_sep(__os, __vs), ...);
}
// Writes a sequence of values, each preceded by a space.
template <class _OS, class _Seq>
void __rand_put_seq(_OS& __os, const _Seq& s) {
  for (const auto& __v : s)
    ::__ycxx::__detail::__rand_put_sep(__os, __v);
}

// Reads one value; on a malformed or out-of-range value sets failbit and leaves v unchanged.
template <class _IS, class _Tp>
bool __rand_get1(_IS& is, _Tp& __v) {
  if constexpr (std::is_floating_point_v<_Tp>) {
    _Tp __x{};
    is >> __x;
    if (!is.fail())
      __v = __x;
  } else if constexpr (std::is_signed_v<_Tp> || std::is_same_v<_Tp, bool>) {
    long long __x = 0;
    is >> __x;
    if (!is.fail()) {
      if (__x < static_cast<long long>(std::numeric_limits<_Tp>::min()) ||
          __x > static_cast<long long>(std::numeric_limits<_Tp>::max()))
        is.setstate(_IS::failbit);
      else
        __v = static_cast<_Tp>(__x);
    }
  } else {
    unsigned long long __x = 0;
    is >> __x;
    if (!is.fail()) {
      if (__x > static_cast<unsigned long long>(std::numeric_limits<_Tp>::max()))
        is.setstate(_IS::failbit);
      else
        __v = static_cast<_Tp>(__x);
    }
  }
  return !is.fail();
}
template <class _IS, class... _Ts>
bool __rand_get(_IS& is, _Ts&... __vs) {
  return (::__ycxx::__detail::__rand_get1(is, __vs) && ...);
}

}} // namespace __ycxx::__detail
