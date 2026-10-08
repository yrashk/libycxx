// libycxx core: <bit>
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// Unsigned integer types, plus unsigned _BitInt(N) where the compiler has it (extension).
template <class _Tp>
concept __bit_unsigned =
    __is_standard_unsigned_integer<_Tp> || (__bitint_width<_Tp> != 0 && !__bitint_info<__remove_cv(_Tp)>::is_signed);
template <class _Tp>
concept __bit_integer = __is_signed_or_unsigned_integer<_Tp> || __bitint_width<_Tp> != 0;
template <class _Tp>
inline constexpr int __bit_digits = __bitint_width<_Tp> != 0 ? __bitint_width<_Tp> : static_cast<int>(sizeof(_Tp) * __CHAR_BIT__);
template <class _Tp>
constexpr _Tp __byteswap_std(_Tp value) noexcept;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

enum class endian { little = __ORDER_LITTLE_ENDIAN__, big = __ORDER_BIG_ENDIAN__, native = __BYTE_ORDER__ };

template <class _To, class _From>
  requires(sizeof(_To) == sizeof(_From) && is_trivially_copyable_v<_To> && is_trivially_copyable_v<_From>)
[[nodiscard]] constexpr _To bit_cast(const _From& from) noexcept {
  return __builtin_bit_cast(_To, from);
}

template <class _Tp>
  requires is_integral_v<_Tp> || (__ycxx::__detail::__bitint_width<_Tp> != 0 && __ycxx::__detail::__bitint_width<_Tp> % 8 == 0)
[[nodiscard]] constexpr _Tp byteswap(_Tp value) noexcept {
  if constexpr (__ycxx::__detail::__bitint_width<_Tp> != 0) {
    // _BitInt(N) extension: reverse whole bytes one at a time.
    if constexpr (__ycxx::__detail::__bitint_width<_Tp> == 8) {
      return value;
    } else {
      _Tp r = 0;
      for (int i = 0; i < __ycxx::__detail::__bitint_width<_Tp>; i += 8)
        r = static_cast<_Tp>((r << 8) | ((value >> i) & 0xff));
      return r;
    }
  } else {
    return __ycxx::__detail::__byteswap_std(value);
  }
}
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
constexpr _Tp __byteswap_std(_Tp value) noexcept {
  using _Up = std::make_unsigned_t<std::conditional_t<__is_same(__remove_cv(_Tp), bool), unsigned char, _Tp>>;
  if constexpr (sizeof(_Tp) == 1)
    return value;
  else if constexpr (sizeof(_Tp) == 2)
    return static_cast<_Tp>(__builtin_bswap16(static_cast<_Up>(value)));
  else if constexpr (sizeof(_Tp) == 4)
    return static_cast<_Tp>(__builtin_bswap32(static_cast<_Up>(value)));
  else if constexpr (sizeof(_Tp) == 8)
    return static_cast<_Tp>(__builtin_bswap64(static_cast<_Up>(value)));
  else {
    auto __u = static_cast<__ycxx::__detail::__uint128>(static_cast<_Up>(value));
    auto __lo = static_cast<unsigned long long>(__u);
    auto __hi = static_cast<unsigned long long>(__u >> 64);
    return static_cast<_Tp>((static_cast<__ycxx::__detail::__uint128>(__builtin_bswap64(__lo)) << 64) | __builtin_bswap64(__hi));
  }
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
// [bit.count]
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr int countl_zero(_Tp __x) noexcept {
  return __builtin_clzg(__x, __ycxx::__detail::__bit_digits<_Tp>);
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr int countl_one(_Tp __x) noexcept {
  return __builtin_clzg(static_cast<_Tp>(~__x), __ycxx::__detail::__bit_digits<_Tp>);
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr int countr_zero(_Tp __x) noexcept {
  return __builtin_ctzg(__x, __ycxx::__detail::__bit_digits<_Tp>);
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr int countr_one(_Tp __x) noexcept {
  return __builtin_ctzg(static_cast<_Tp>(~__x), __ycxx::__detail::__bit_digits<_Tp>);
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr int popcount(_Tp __x) noexcept {
  return __builtin_popcountg(__x);
}

// [bit.pow.two]
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr bool has_single_bit(_Tp __x) noexcept {
  return __x != 0 && (__x & (__x - 1)) == 0;
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr int bit_width(_Tp __x) noexcept {
  return __ycxx::__detail::__bit_digits<_Tp> - countl_zero(__x);
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp bit_floor(_Tp __x) noexcept {
  return __x == 0 ? _Tp(0) : static_cast<_Tp>(_Tp(1) << (bit_width(__x) - 1));
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp bit_ceil(_Tp __x) {
  if (__x <= 1)
    return _Tp(1);
  int __w = bit_width(static_cast<_Tp>(__x - 1));
  __ycxx::__detail::__precondition(__w < __ycxx::__detail::__bit_digits<_Tp>, "std::bit_ceil: result not representable");
  if constexpr (sizeof(_Tp) < sizeof(unsigned)) {
    // Integral promotion: shift in unsigned int, then truncate (makes overflow a constant-eval error above).
    return static_cast<_Tp>(1u << __w);
  } else {
    return static_cast<_Tp>(_Tp(1) << __w);
  }
}

// [bit.rotate]
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp rotl(_Tp __x, int s) noexcept {
  constexpr int n = __ycxx::__detail::__bit_digits<_Tp>;
  int r = s % n;
  if (r == 0)
    return __x;
  if (r < 0)
    r += n;
  return static_cast<_Tp>((__x << r) | (__x >> (n - r)));
}
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp rotr(_Tp __x, int s) noexcept {
  constexpr int n = __ycxx::__detail::__bit_digits<_Tp>;
  int r = s % n;
  if (r == 0)
    return __x;
  if (r < 0)
    r += n;
  return static_cast<_Tp>((__x >> r) | (__x << (n - r)));
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// x * 2^s and x * 2^-s rounded toward negative infinity, modulo 2^N, for any shift amount.
// One return statement each: constant evaluation counts statements, and shl/shr are cheap enough
// to be called in long constant-evaluated loops.
template <class _Tp>
constexpr _Tp shift_left(_Tp __x, unsigned long long s) noexcept {
  return s >= static_cast<unsigned long long>(__bit_digits<_Tp>)
           ? _Tp(0)
           : static_cast<_Tp>(static_cast<std::make_unsigned_t<_Tp>>(static_cast<std::make_unsigned_t<_Tp>>(__x) << s));
}
template <class _Tp>
constexpr _Tp shift_right(_Tp __x, unsigned long long s) noexcept {
  // An arithmetic shift for signed T: rounds toward -infinity.
  return s >= static_cast<unsigned long long>(__bit_digits<_Tp>) ? ((is_signed_v<_Tp> && __x < 0) ? _Tp(-1) : _Tp(0))
                                                             : static_cast<_Tp>(__x >> s);
}
// |s|, computed in unsigned long long (a narrow type would promote to int).
template <class _Sp>
  requires(sizeof(_Sp) <= sizeof(unsigned long long))
constexpr unsigned long long __magnitude(_Sp s) noexcept {
  return s < 0 ? 0ull - static_cast<unsigned long long>(s) : static_cast<unsigned long long>(s);
}
// A wider S: a magnitude beyond unsigned long long saturates (it shifts every bit out anyway).
template <class _Sp>
  requires(sizeof(_Sp) > sizeof(unsigned long long))
constexpr unsigned long long __magnitude(_Sp s) noexcept {
  using _Up = std::make_unsigned_t<_Sp>;
  const _Up m = s < 0 ? static_cast<_Up>(_Up(0) - static_cast<_Up>(s)) : static_cast<_Up>(s);
  return m > static_cast<_Up>(~0ull) ? ~0ull : static_cast<unsigned long long>(m);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [bit.shift]
template <__ycxx::__detail::__bit_integer _Tp, __ycxx::__detail::__bit_integer _Sp>
[[nodiscard]] constexpr _Tp shl(_Tp __x, _Sp s) noexcept {
  return s < 0 ? __ycxx::__detail::shift_right(__x, __ycxx::__detail::__magnitude(s))
               : __ycxx::__detail::shift_left(__x, __ycxx::__detail::__magnitude(s));
}
template <__ycxx::__detail::__bit_integer _Tp, __ycxx::__detail::__bit_integer _Sp>
[[nodiscard]] constexpr _Tp shr(_Tp __x, _Sp s) noexcept {
  return s < 0 ? __ycxx::__detail::shift_left(__x, __ycxx::__detail::__magnitude(s))
               : __ycxx::__detail::shift_right(__x, __ycxx::__detail::__magnitude(s));
}

// [bit.permute]
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp bit_reverse(_Tp __x) noexcept {
  constexpr int n = __ycxx::__detail::__bit_digits<_Tp>;
  // Swap progressively larger groups: 1, 2, 4 bits, then bytes via byteswap.
  constexpr _Tp __ones = static_cast<_Tp>(~_Tp(0));
  constexpr _Tp __m1 = static_cast<_Tp>(__ones / 3);  // 0x55..
  constexpr _Tp __m2 = static_cast<_Tp>(__ones / 5);  // 0x33..
  constexpr _Tp __m4 = static_cast<_Tp>(__ones / 17); // 0x0f..
  __x = static_cast<_Tp>(((__x >> 1) & __m1) | ((__x & __m1) << 1));
  __x = static_cast<_Tp>(((__x >> 2) & __m2) | ((__x & __m2) << 2));
  __x = static_cast<_Tp>(((__x >> 4) & __m4) | ((__x & __m4) << 4));
  if constexpr (n > 8)
    __x = byteswap(__x);
  return __x;
}

template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp bit_repeat(_Tp __x, int __l) {
  __ycxx::__detail::__precondition(__l > 0, "std::bit_repeat: l must be positive");
  constexpr int n = __ycxx::__detail::__bit_digits<_Tp>;
  if (__l >= n)
    return __x;
  _Tp pattern = static_cast<_Tp>(__x & static_cast<_Tp>((_Tp(1) << __l) - 1));
  _Tp r = 0;
  for (int i = 0; i < n; i += __l)
    r = static_cast<_Tp>(r | static_cast<_Tp>(pattern << i));
  return r;
}

// Hacker's Delight (2nd ed.), 7-4 "Compress, or Generalized Extract": parallel-suffix method.
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp bit_compress(_Tp __x, _Tp m) noexcept {
  constexpr int n = __ycxx::__detail::__bit_digits<_Tp>;
  __x = static_cast<_Tp>(__x & m);
  _Tp __mk = static_cast<_Tp>(~m << 1); // count 0's to the right
  for (int i = 1; i < n; i <<= 1) {
    _Tp __mp = static_cast<_Tp>(__mk ^ (__mk << 1)); // parallel suffix
    for (int __j = 2; __j < n; __j <<= 1)
      __mp = static_cast<_Tp>(__mp ^ (__mp << __j));
    _Tp __mv = static_cast<_Tp>(__mp & m); // bits to move
    m = static_cast<_Tp>((m ^ __mv) | (__mv >> i));
    _Tp t = static_cast<_Tp>(__x & __mv);
    __x = static_cast<_Tp>((__x ^ t) | (t >> i));
    __mk = static_cast<_Tp>(__mk & ~__mp);
  }
  return __x;
}

// Hacker's Delight (2nd ed.), 7-5 "Expand, or Generalized Insert" (inverse of compress).
template <__ycxx::__detail::__bit_unsigned _Tp>
[[nodiscard]] constexpr _Tp bit_expand(_Tp __x, _Tp m) noexcept {
  constexpr int n = __ycxx::__detail::__bit_digits<_Tp>;
  constexpr int __steps = __builtin_ctzg(static_cast<unsigned>(n)); // log2(n)
  _Tp array[__steps > 0 ? __steps : 1]{};
  _Tp __m0 = m;
  _Tp __mk = static_cast<_Tp>(~m << 1);
  for (int i = 0; i < __steps; ++i) {
    _Tp __mp = static_cast<_Tp>(__mk ^ (__mk << 1));
    for (int __j = 2; __j < n; __j <<= 1)
      __mp = static_cast<_Tp>(__mp ^ (__mp << __j));
    _Tp __mv = static_cast<_Tp>(__mp & m);
    array[i] = __mv;
    m = static_cast<_Tp>((m ^ __mv) | (__mv >> (1 << i)));
    __mk = static_cast<_Tp>(__mk & ~__mp);
  }
  for (int i = __steps - 1; i >= 0; --i) {
    _Tp __mv = array[i];
    _Tp t = static_cast<_Tp>(__x << (1 << i));
    __x = static_cast<_Tp>((__x & ~__mv) | (t & __mv));
  }
  return static_cast<_Tp>(__x & __m0);
}

}} // namespace std
