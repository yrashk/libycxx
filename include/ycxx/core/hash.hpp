// libycxx core: std::hash ([unord.hash]) and the byte-hash function used for strings.
//
// Integral hashes are the identity (cast to size_t): libycxx's unordered containers apply a
// multiplicative mixing step to every hash, so identity is both fast and safe there.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/compare.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// Byte-sequence hash. Construction: 64x64->128-bit multiply folded to 64 bits ("mum"), the
// mixing primitive popularised by wyhash/rapidhash (public-domain designs); this is libycxx's
// own arrangement of it, processing 16 bytes per step (two multiplies). Constant-evaluable.
// Note: the seed is fixed, so this is not a defence against deliberate hash flooding.
inline constexpr std::uint64_t hash_seed = 0x9e3779b97f4a7c15ull;
inline constexpr std::uint64_t hash_k1 = 0xa0761d6478bd642full;
inline constexpr std::uint64_t hash_k2 = 0xe7037ed1a0b428dbull;

constexpr std::uint64_t mum(std::uint64_t a, std::uint64_t b) noexcept {
  uint128 r = static_cast<uint128>(a) * b;
  return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
}

template <class CharT>
constexpr std::uint64_t load_le(const CharT* p, int nbytes) noexcept {
  // Reads `nbytes` (1..8) bytes worth of characters as a little-endian integer. Works on the
  // character values (not object representation) so it is usable in constant evaluation.
  std::uint64_t v = 0;
  if constexpr (sizeof(CharT) == 1) {
    for (int i = 0; i < nbytes; ++i)
      v |= static_cast<std::uint64_t>(static_cast<unsigned char>(p[i])) << (8 * i);
  } else {
    constexpr int per = static_cast<int>(sizeof(CharT));
    for (int i = 0; i < nbytes / per; ++i)
      v |= static_cast<std::uint64_t>(static_cast<std::make_unsigned_t<CharT>>(p[i])) << (8 * per * i);
  }
  return v;
}

// One 16-byte step. Each half is multiplied separately with the running state folded in, so no
// single chosen 8-byte word can zero the state: cancelling both products requires both words
// to equal a value determined by the state (one input, not 2^64).
constexpr std::uint64_t hash_step(std::uint64_t h, std::uint64_t a, std::uint64_t b) noexcept {
  return mum(a ^ hash_k1 ^ h, hash_k2) ^ mum(b ^ hash_k2 ^ h, hash_k1);
}

template <class CharT>
constexpr std::uint64_t hash_chars(const CharT* p, std::size_t n) noexcept {
  const std::size_t bytes = n * sizeof(CharT);
  std::uint64_t h = hash_seed ^ mum(bytes ^ hash_k1, hash_k2);
  constexpr std::size_t per8 = 8 / sizeof(CharT); // characters per 8 bytes
  std::size_t i = 0;
  for (; i + 2 * per8 <= n; i += 2 * per8) {
    std::uint64_t a = load_le(p + i, 8);
    std::uint64_t b = load_le(p + i + per8, 8);
    h = hash_step(h, a, b);
  }
  std::size_t rest = n - i;
  if (rest) {
    std::uint64_t a = 0, b = 0;
    if (rest > per8) {
      a = load_le(p + i, 8);
      b = load_le(p + i + per8, static_cast<int>((rest - per8) * sizeof(CharT)));
    } else {
      a = load_le(p + i, static_cast<int>(rest * sizeof(CharT)));
    }
    h = hash_step(h ^ rest, a, b);
  }
  return mum(h ^ hash_k1, h ^ hash_k2);
}

template <class T>
inline constexpr bool hash_identity_integral =
    is_integral_v<T> && !__is_const(T) && !__is_volatile(T);

struct x87_hash_layout {
  unsigned long long significand;
  unsigned short sign_exponent;
};

template <class T>
constexpr std::size_t hash_float(T v) noexcept {
  if (v == T(0))
    return 0; // +0.0 and -0.0 compare equal, so they must hash equal
  if constexpr (__is_same(T, long double) && fp_format<long double>.digits == 64) {
    auto r = __builtin_bit_cast(x87_hash_layout, v);
    return static_cast<std::size_t>(mum(r.significand ^ hash_k1, r.sign_exponent ^ hash_k2));
  } else if constexpr (sizeof(T) <= 8) {
    using U = std::conditional_t<sizeof(T) == 2, unsigned short,
                                 std::conditional_t<sizeof(T) == 4, unsigned int, unsigned long long>>;
    return static_cast<std::size_t>(__builtin_bit_cast(U, v));
  } else {
    auto u = __builtin_bit_cast(uint128, v);
    return static_cast<std::size_t>(mum(static_cast<std::uint64_t>(u) ^ hash_k1, static_cast<std::uint64_t>(u >> 64)));
  }
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// The primary template is defined but *disabled* ([unord.hash]/5): not default constructible,
// not copyable, not a function object.
template <class T>
struct hash {
  hash() = delete;
  hash(const hash&) = delete;
  hash(hash&&) = delete;
  hash& operator=(const hash&) = delete;
  hash& operator=(hash&&) = delete;
};

template <class T>
  requires ycxx::detail::hash_identity_integral<T>
struct hash<T> {
  [[nodiscard]] constexpr size_t operator()(T v) const noexcept { return static_cast<size_t>(v); }
};

template <class T>
  requires(is_enum_v<T> && !is_const_v<T> && !is_volatile_v<T>)
struct hash<T> {
  [[nodiscard]] constexpr size_t operator()(T v) const noexcept {
    return static_cast<size_t>(static_cast<underlying_type_t<T>>(v));
  }
};

template <class T>
  requires(ycxx::detail::is_floating_v<T> && !is_const_v<T> && !is_volatile_v<T>)
struct hash<T> {
  [[nodiscard]] constexpr size_t operator()(T v) const noexcept { return ycxx::detail::hash_float(v); }
};

template <class T>
struct hash<T*> {
  [[nodiscard]] size_t operator()(T* p) const noexcept { return reinterpret_cast<size_t>(p); }
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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
// "Cpp17Hash is enabled for Key": the condition every unordered container / optional / variant
// hash relies on.
template <class T>
concept hash_enabled = std::is_default_constructible_v<std::hash<T>> &&
                       requires(const std::hash<T>& h, const T& v) {
                         { h(v) } -> same_as_<std::size_t>;
                       };
}} // namespace ycxx::detail
