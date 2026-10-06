// libycxx core: std::hash ([unord.hash]) and the byte-hash function used for strings.
//
// Integral hashes are the identity (cast to size_t): libycxx's unordered containers apply a
// multiplicative mixing step to every hash, so identity is both fast and safe there.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/compare.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Byte-sequence hash. Construction: 64x64->128-bit multiply folded to 64 bits ("mum"), the
// mixing primitive popularised by wyhash/rapidhash (public-domain designs); this is libycxx's
// own arrangement of it, processing 16 bytes per step (two multiplies). Constant-evaluable.
// Note: the seed is fixed, so this is not a defence against deliberate hash flooding.
inline constexpr std::uint64_t __hash_seed = 0x9e3779b97f4a7c15ull;
inline constexpr std::uint64_t __hash_k1 = 0xa0761d6478bd642full;
inline constexpr std::uint64_t __hash_k2 = 0xe7037ed1a0b428dbull;

constexpr std::uint64_t __mum(std::uint64_t a, std::uint64_t b) noexcept {
  __uint128 r = static_cast<__uint128>(a) * b;
  return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
}

template <class _CharT>
constexpr std::uint64_t __load_le(const _CharT* p, int __nbytes) noexcept {
  // Reads `__nbytes` (1..8) bytes worth of characters as a little-endian integer. Works on the
  // character values (not object representation) so it is usable in constant evaluation.
  std::uint64_t __v = 0;
  if constexpr (sizeof(_CharT) == 1) {
    for (int i = 0; i < __nbytes; ++i)
      __v |= static_cast<std::uint64_t>(static_cast<unsigned char>(p[i])) << (8 * i);
  } else {
    constexpr int __per = static_cast<int>(sizeof(_CharT));
    for (int i = 0; i < __nbytes / __per; ++i)
      __v |= static_cast<std::uint64_t>(static_cast<std::make_unsigned_t<_CharT>>(p[i])) << (8 * __per * i);
  }
  return __v;
}

// One 16-byte step. Each half is multiplied separately with the running state folded in, so no
// single chosen 8-byte word can zero the state: cancelling both products requires both words
// to equal a value determined by the state (one input, not 2^64).
constexpr std::uint64_t __hash_step(std::uint64_t h, std::uint64_t a, std::uint64_t b) noexcept {
  return __mum(a ^ __hash_k1 ^ h, __hash_k2) ^ __mum(b ^ __hash_k2 ^ h, __hash_k1);
}

template <class _CharT>
constexpr std::uint64_t __hash_chars(const _CharT* p, std::size_t n) noexcept {
  const std::size_t __bytes = n * sizeof(_CharT);
  std::uint64_t h = __hash_seed ^ __mum(__bytes ^ __hash_k1, __hash_k2);
  constexpr std::size_t __per8 = 8 / sizeof(_CharT); // characters per 8 bytes
  std::size_t i = 0;
  for (; i + 2 * __per8 <= n; i += 2 * __per8) {
    std::uint64_t a = __load_le(p + i, 8);
    std::uint64_t b = __load_le(p + i + __per8, 8);
    h = __hash_step(h, a, b);
  }
  std::size_t __rest = n - i;
  if (__rest) {
    std::uint64_t a = 0, b = 0;
    if (__rest > __per8) {
      a = __load_le(p + i, 8);
      b = __load_le(p + i + __per8, static_cast<int>((__rest - __per8) * sizeof(_CharT)));
    } else {
      a = __load_le(p + i, static_cast<int>(__rest * sizeof(_CharT)));
    }
    h = __hash_step(h ^ __rest, a, b);
  }
  return __mum(h ^ __hash_k1, h ^ __hash_k2);
}

template <class _Tp>
inline constexpr bool __hash_identity_integral =
    is_integral_v<_Tp> && !__is_const(_Tp) && !__is_volatile(_Tp);

struct __x87_hash_layout {
  unsigned long long __significand;
  unsigned short __sign_exponent;
};

template <class _Tp>
constexpr std::size_t __hash_float(_Tp __v) noexcept {
  if (__v == _Tp(0))
    return 0; // +0.0 and -0.0 compare equal, so they must hash equal
  if constexpr (__is_same(_Tp, long double) && __fp_format<long double>.digits == 64) {
    auto r = __builtin_bit_cast(__x87_hash_layout, __v);
    return static_cast<std::size_t>(__mum(r.__significand ^ __hash_k1, r.__sign_exponent ^ __hash_k2));
  } else if constexpr (sizeof(_Tp) <= 8) {
    using _Up = std::conditional_t<sizeof(_Tp) == 2, unsigned short,
                                 std::conditional_t<sizeof(_Tp) == 4, unsigned int, unsigned long long>>;
    return static_cast<std::size_t>(__builtin_bit_cast(_Up, __v));
  } else {
    auto __u = __builtin_bit_cast(__uint128, __v);
    return static_cast<std::size_t>(__mum(static_cast<std::uint64_t>(__u) ^ __hash_k1, static_cast<std::uint64_t>(__u >> 64)));
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// The primary template is defined but *disabled* ([unord.hash]/5): not default constructible,
// not copyable, not a function object.
template <class _Tp>
struct hash {
  hash() = delete;
  hash(const hash&) = delete;
  hash(hash&&) = delete;
  hash& operator=(const hash&) = delete;
  hash& operator=(hash&&) = delete;
};

template <class _Tp>
  requires __ycxx::__detail::__hash_identity_integral<_Tp>
struct hash<_Tp> {
  [[nodiscard]] constexpr size_t operator()(_Tp __v) const noexcept { return static_cast<size_t>(__v); }
};

template <class _Tp>
  requires(is_enum_v<_Tp> && !is_const_v<_Tp> && !is_volatile_v<_Tp>)
struct hash<_Tp> {
  [[nodiscard]] constexpr size_t operator()(_Tp __v) const noexcept {
    return static_cast<size_t>(static_cast<underlying_type_t<_Tp>>(__v));
  }
};

template <class _Tp>
  requires(__ycxx::__detail::__is_floating_v<_Tp> && !is_const_v<_Tp> && !is_volatile_v<_Tp>)
struct hash<_Tp> {
  [[nodiscard]] constexpr size_t operator()(_Tp __v) const noexcept { return __ycxx::__detail::__hash_float(__v); }
};

template <class _Tp>
struct hash<_Tp*> {
  [[nodiscard]] size_t operator()(_Tp* p) const noexcept { return reinterpret_cast<size_t>(p); }
};

template <>
struct hash<nullptr_t> {
  [[nodiscard]] constexpr size_t operator()(nullptr_t) const noexcept { return 0; }
};

struct monostate;
template <>
struct hash<monostate> {
  [[nodiscard]] constexpr size_t operator()(const monostate&) const noexcept { return 0x6d6f6e6fu; }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// "Cpp17Hash is enabled for Key": the condition every unordered container / optional / variant
// hash relies on.
template <class _Tp>
concept __hash_enabled = std::is_default_constructible_v<std::hash<_Tp>> &&
                       requires(const std::hash<_Tp>& h, const _Tp& __v) {
                         { h(__v) } -> __same_as_<std::size_t>;
                       };
}} // namespace __ycxx::__detail
