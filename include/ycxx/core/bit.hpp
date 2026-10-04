// libycxx core: <bit>
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/error.hpp>

namespace ycxx::detail {
// Unsigned integer types, plus unsigned _BitInt(N) where the compiler has it (extension).
template <class T>
concept bit_unsigned =
    is_standard_unsigned_integer<T> || (bitint_width<T> != 0 && !bitint_info<__remove_cv(T)>::is_signed);
template <class T>
concept bit_integer = is_signed_or_unsigned_integer<T> || bitint_width<T> != 0;
template <class T>
inline constexpr int bit_digits = bitint_width<T> != 0 ? bitint_width<T> : static_cast<int>(sizeof(T) * __CHAR_BIT__);
template <class T>
constexpr T byteswap_std(T value) noexcept;
} // namespace ycxx::detail

namespace std {

enum class endian { little = __ORDER_LITTLE_ENDIAN__, big = __ORDER_BIG_ENDIAN__, native = __BYTE_ORDER__ };

template <class To, class From>
  requires(sizeof(To) == sizeof(From) && is_trivially_copyable_v<To> && is_trivially_copyable_v<From>)
[[nodiscard]] constexpr To bit_cast(const From& from) noexcept {
  return __builtin_bit_cast(To, from);
}

template <class T>
  requires is_integral_v<T> || (ycxx::detail::bitint_width<T> != 0 && ycxx::detail::bitint_width<T> % 8 == 0)
[[nodiscard]] constexpr T byteswap(T value) noexcept {
  if constexpr (ycxx::detail::bitint_width<T> != 0) {
    // _BitInt(N) extension: reverse whole bytes one at a time.
    if constexpr (ycxx::detail::bitint_width<T> == 8) {
      return value;
    } else {
      T r = 0;
      for (int i = 0; i < ycxx::detail::bitint_width<T>; i += 8)
        r = static_cast<T>((r << 8) | ((value >> i) & 0xff));
      return r;
    }
  } else {
    return ycxx::detail::byteswap_std(value);
  }
}
} // namespace std

namespace ycxx::detail {
template <class T>
constexpr T byteswap_std(T value) noexcept {
  using U = std::make_unsigned_t<std::conditional_t<__is_same(__remove_cv(T), bool), unsigned char, T>>;
  if constexpr (sizeof(T) == 1)
    return value;
  else if constexpr (sizeof(T) == 2)
    return static_cast<T>(__builtin_bswap16(static_cast<U>(value)));
  else if constexpr (sizeof(T) == 4)
    return static_cast<T>(__builtin_bswap32(static_cast<U>(value)));
  else if constexpr (sizeof(T) == 8)
    return static_cast<T>(__builtin_bswap64(static_cast<U>(value)));
  else {
    auto u = static_cast<ycxx::detail::uint128>(static_cast<U>(value));
    auto lo = static_cast<unsigned long long>(u);
    auto hi = static_cast<unsigned long long>(u >> 64);
    return static_cast<T>((static_cast<ycxx::detail::uint128>(__builtin_bswap64(lo)) << 64) | __builtin_bswap64(hi));
  }
}
} // namespace ycxx::detail

namespace std {
// [bit.count]
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr int countl_zero(T x) noexcept {
  return __builtin_clzg(x, ycxx::detail::bit_digits<T>);
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr int countl_one(T x) noexcept {
  return __builtin_clzg(static_cast<T>(~x), ycxx::detail::bit_digits<T>);
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr int countr_zero(T x) noexcept {
  return __builtin_ctzg(x, ycxx::detail::bit_digits<T>);
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr int countr_one(T x) noexcept {
  return __builtin_ctzg(static_cast<T>(~x), ycxx::detail::bit_digits<T>);
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr int popcount(T x) noexcept {
  return __builtin_popcountg(x);
}

// [bit.pow.two]
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr bool has_single_bit(T x) noexcept {
  return x != 0 && (x & (x - 1)) == 0;
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr int bit_width(T x) noexcept {
  return ycxx::detail::bit_digits<T> - countl_zero(x);
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T bit_floor(T x) noexcept {
  return x == 0 ? T(0) : static_cast<T>(T(1) << (bit_width(x) - 1));
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T bit_ceil(T x) {
  if (x <= 1)
    return T(1);
  int w = bit_width(static_cast<T>(x - 1));
  ycxx::detail::precondition(w < ycxx::detail::bit_digits<T>, "std::bit_ceil: result not representable");
  if constexpr (sizeof(T) < sizeof(unsigned)) {
    // Integral promotion: shift in unsigned int, then truncate (makes overflow a constant-eval error above).
    return static_cast<T>(1u << w);
  } else {
    return static_cast<T>(T(1) << w);
  }
}

// [bit.rotate]
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T rotl(T x, int s) noexcept {
  constexpr int n = ycxx::detail::bit_digits<T>;
  int r = s % n;
  if (r == 0)
    return x;
  if (r < 0)
    r += n;
  return static_cast<T>((x << r) | (x >> (n - r)));
}
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T rotr(T x, int s) noexcept {
  constexpr int n = ycxx::detail::bit_digits<T>;
  int r = s % n;
  if (r == 0)
    return x;
  if (r < 0)
    r += n;
  return static_cast<T>((x >> r) | (x << (n - r)));
}

} // namespace std

namespace ycxx::detail {
// x * 2^s and x * 2^-s rounded toward negative infinity, modulo 2^N, for any shift amount.
// One return statement each: constant evaluation counts statements, and shl/shr are cheap enough
// to be called in long constant-evaluated loops.
template <class T>
constexpr T shift_left(T x, unsigned long long s) noexcept {
  return s >= static_cast<unsigned long long>(bit_digits<T>)
           ? T(0)
           : static_cast<T>(static_cast<std::make_unsigned_t<T>>(static_cast<std::make_unsigned_t<T>>(x) << s));
}
template <class T>
constexpr T shift_right(T x, unsigned long long s) noexcept {
  // An arithmetic shift for signed T: rounds toward -infinity.
  return s >= static_cast<unsigned long long>(bit_digits<T>) ? ((is_signed_v<T> && x < 0) ? T(-1) : T(0))
                                                             : static_cast<T>(x >> s);
}
// |s|, computed in unsigned long long (a narrow type would promote to int).
template <class S>
  requires(sizeof(S) <= sizeof(unsigned long long))
constexpr unsigned long long magnitude(S s) noexcept {
  return s < 0 ? 0ull - static_cast<unsigned long long>(s) : static_cast<unsigned long long>(s);
}
// A wider S: a magnitude beyond unsigned long long saturates (it shifts every bit out anyway).
template <class S>
  requires(sizeof(S) > sizeof(unsigned long long))
constexpr unsigned long long magnitude(S s) noexcept {
  using U = std::make_unsigned_t<S>;
  const U m = s < 0 ? static_cast<U>(U(0) - static_cast<U>(s)) : static_cast<U>(s);
  return m > static_cast<U>(~0ull) ? ~0ull : static_cast<unsigned long long>(m);
}
} // namespace ycxx::detail

namespace std {

// [bit.shift]
template <ycxx::detail::bit_integer T, ycxx::detail::bit_integer S>
[[nodiscard]] constexpr T shl(T x, S s) noexcept {
  return s < 0 ? ycxx::detail::shift_right(x, ycxx::detail::magnitude(s))
               : ycxx::detail::shift_left(x, ycxx::detail::magnitude(s));
}
template <ycxx::detail::bit_integer T, ycxx::detail::bit_integer S>
[[nodiscard]] constexpr T shr(T x, S s) noexcept {
  return s < 0 ? ycxx::detail::shift_left(x, ycxx::detail::magnitude(s))
               : ycxx::detail::shift_right(x, ycxx::detail::magnitude(s));
}

// [bit.permute]
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T bit_reverse(T x) noexcept {
  constexpr int n = ycxx::detail::bit_digits<T>;
  // Swap progressively larger groups: 1, 2, 4 bits, then bytes via byteswap.
  constexpr T ones = static_cast<T>(~T(0));
  constexpr T m1 = static_cast<T>(ones / 3);  // 0x55..
  constexpr T m2 = static_cast<T>(ones / 5);  // 0x33..
  constexpr T m4 = static_cast<T>(ones / 17); // 0x0f..
  x = static_cast<T>(((x >> 1) & m1) | ((x & m1) << 1));
  x = static_cast<T>(((x >> 2) & m2) | ((x & m2) << 2));
  x = static_cast<T>(((x >> 4) & m4) | ((x & m4) << 4));
  if constexpr (n > 8)
    x = byteswap(x);
  return x;
}

template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T bit_repeat(T x, int l) {
  ycxx::detail::precondition(l > 0, "std::bit_repeat: l must be positive");
  constexpr int n = ycxx::detail::bit_digits<T>;
  if (l >= n)
    return x;
  T pattern = static_cast<T>(x & static_cast<T>((T(1) << l) - 1));
  T r = 0;
  for (int i = 0; i < n; i += l)
    r = static_cast<T>(r | static_cast<T>(pattern << i));
  return r;
}

// Hacker's Delight (2nd ed.), 7-4 "Compress, or Generalized Extract": parallel-suffix method.
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T bit_compress(T x, T m) noexcept {
  constexpr int n = ycxx::detail::bit_digits<T>;
  x = static_cast<T>(x & m);
  T mk = static_cast<T>(~m << 1); // count 0's to the right
  for (int i = 1; i < n; i <<= 1) {
    T mp = static_cast<T>(mk ^ (mk << 1)); // parallel suffix
    for (int j = 2; j < n; j <<= 1)
      mp = static_cast<T>(mp ^ (mp << j));
    T mv = static_cast<T>(mp & m); // bits to move
    m = static_cast<T>((m ^ mv) | (mv >> i));
    T t = static_cast<T>(x & mv);
    x = static_cast<T>((x ^ t) | (t >> i));
    mk = static_cast<T>(mk & ~mp);
  }
  return x;
}

// Hacker's Delight (2nd ed.), 7-5 "Expand, or Generalized Insert" (inverse of compress).
template <ycxx::detail::bit_unsigned T>
[[nodiscard]] constexpr T bit_expand(T x, T m) noexcept {
  constexpr int n = ycxx::detail::bit_digits<T>;
  constexpr int steps = __builtin_ctzg(static_cast<unsigned>(n)); // log2(n)
  T array[steps > 0 ? steps : 1]{};
  T m0 = m;
  T mk = static_cast<T>(~m << 1);
  for (int i = 0; i < steps; ++i) {
    T mp = static_cast<T>(mk ^ (mk << 1));
    for (int j = 2; j < n; j <<= 1)
      mp = static_cast<T>(mp ^ (mp << j));
    T mv = static_cast<T>(mp & m);
    array[i] = mv;
    m = static_cast<T>((m ^ mv) | (mv >> (1 << i)));
    mk = static_cast<T>(mk & ~mp);
  }
  for (int i = steps - 1; i >= 0; --i) {
    T mv = array[i];
    T t = static_cast<T>(x << (1 << i));
    x = static_cast<T>((x & ~mv) | (t & mv));
  }
  return static_cast<T>(x & m0);
}

} // namespace std
