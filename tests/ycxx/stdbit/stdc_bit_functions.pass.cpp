// [stdbit.h.syn]: the C23 bit utilities (ISO/IEC 9899:2024 7.18) as functions with _uc/_us/
// _ui/_ul/_ull suffixes and as function templates for unsigned integer types (/1: "Each
// function template has the same semantics as the corresponding type-generic function";
// return type an unsigned integer type for the counting functions, bool for has_single_bit,
// T for bit_floor/bit_ceil). Semantics per C23 7.18: leading/trailing zeros/ones count from the
// most/least significant bit (the width for an all-zero/all-one value); first_leading_zero/one
// and first_trailing_zero/one return the 1-based position of that bit counted from the
// most/least significant end, or 0 if there is none; count_zeros/ones; has_single_bit;
// bit_width (0 for 0); bit_floor (0 for 0); bit_ceil the smallest power of 2 not less than the
// value (1 for 0, 0 when not representable). Exhaustive for unsigned char and unsigned short,
// boundaries and random values for the wider types, against bit-by-bit oracles. Also the
// version and endianness macros (__STDC_ENDIAN_NATIVE__ equals one of the other two).
#include <stdbit.h>
#include <climits>
#include <type_traits>
#include "check.hpp"

static_assert(__STDC_VERSION_STDBIT_H__ == 202311L);
static_assert(__STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__ || __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__);
static_assert(__STDC_ENDIAN_LITTLE__ != __STDC_ENDIAN_BIG__);

template <class T>
constexpr int W = sizeof(T) * CHAR_BIT;

template <class T>
static bool bit(T v, int i) {  // i counted from the least significant bit
  return (v >> i) & 1u;
}

template <class T>
struct Oracle {
  static unsigned lead(T v, bool one) {
    unsigned n = 0;
    for (int i = W<T> - 1; i >= 0 && bit(v, i) == one; --i) ++n;
    return n;
  }
  static unsigned trail(T v, bool one) {
    unsigned n = 0;
    for (int i = 0; i < W<T> && bit(v, i) == one; ++i) ++n;
    return n;
  }
  static unsigned first_lead(T v, bool one) {
    for (int i = W<T> - 1; i >= 0; --i)
      if (bit(v, i) == one) return static_cast<unsigned>(W<T> - i);
    return 0;
  }
  static unsigned first_trail(T v, bool one) {
    for (int i = 0; i < W<T>; ++i)
      if (bit(v, i) == one) return static_cast<unsigned>(i + 1);
    return 0;
  }
  static unsigned count(T v, bool one) {
    unsigned n = 0;
    for (int i = 0; i < W<T>; ++i) n += bit(v, i) == one;
    return n;
  }
  static unsigned width(T v) { return static_cast<unsigned>(W<T>) - lead(v, false); }
  static T floor(T v) { return v == 0 ? T(0) : static_cast<T>(T(1) << (width(v) - 1)); }
  static T ceil(T v) {
    if (v <= 1) return T(1);
    const unsigned w = width(static_cast<T>(v - 1));
    return w >= static_cast<unsigned>(W<T>) ? T(0) : static_cast<T>(T(1) << w);
  }
};

template <class T, class F>
static void each_value(F check) {
  if constexpr (sizeof(T) <= 2) {
    for (unsigned long v = 0; v <= static_cast<T>(-1); ++v) check(static_cast<T>(v));
  } else {
    for (int i = 0; i < W<T>; ++i) {
      const T p = static_cast<T>(T(1) << i);
      check(p);
      check(static_cast<T>(p - 1));
      check(static_cast<T>(p + 1));
      check(static_cast<T>(~p));
      check(static_cast<T>(-p));
    }
    check(T(0));
    check(static_cast<T>(-1));
    unsigned long long x = 88172645463325252ull;
    for (int k = 0; k < 20000; ++k) {
      x ^= x << 13;
      x ^= x >> 7;
      x ^= x << 17;
      check(static_cast<T>(x >> (k % W<T>)));
    }
  }
}

template <class T>
static void generic() {
  using O = Oracle<T>;
  each_value<T>([](T v) {
    CHECK(stdc_leading_zeros(v) == O::lead(v, false));
    CHECK(stdc_leading_ones(v) == O::lead(v, true));
    CHECK(stdc_trailing_zeros(v) == O::trail(v, false));
    CHECK(stdc_trailing_ones(v) == O::trail(v, true));
    CHECK(stdc_first_leading_zero(v) == O::first_lead(v, false));
    CHECK(stdc_first_leading_one(v) == O::first_lead(v, true));
    CHECK(stdc_first_trailing_zero(v) == O::first_trail(v, false));
    CHECK(stdc_first_trailing_one(v) == O::first_trail(v, true));
    CHECK(stdc_count_zeros(v) == O::count(v, false));
    CHECK(stdc_count_ones(v) == O::count(v, true));
    CHECK(stdc_has_single_bit(v) == (O::count(v, true) == 1));
    CHECK(stdc_bit_width(v) == O::width(v));
    CHECK(stdc_bit_floor(v) == O::floor(v));
    CHECK(stdc_bit_ceil(v) == O::ceil(v));
  });
  static_assert(std::is_same_v<decltype(stdc_bit_floor(T())), T> && std::is_same_v<decltype(stdc_bit_ceil(T())), T>);
  static_assert(std::is_same_v<decltype(stdc_has_single_bit(T())), bool>);
  static_assert(std::is_unsigned_v<decltype(stdc_count_ones(T()))>);
  static_assert(std::is_unsigned_v<decltype(stdc_leading_zeros(T()))>);
}

int main() {
  generic<unsigned char>();
  generic<unsigned short>();
  generic<unsigned int>();
  generic<unsigned long>();
  generic<unsigned long long>();

  // the suffixed functions agree with the oracle (spot values)
  using UC = Oracle<unsigned char>;
  using UL = Oracle<unsigned long long>;
  for (unsigned v = 0; v < 256; v += 7) {
    const auto c = static_cast<unsigned char>(v);
    CHECK(stdc_leading_zeros_uc(c) == UC::lead(c, false) && stdc_trailing_ones_uc(c) == UC::trail(c, true));
    CHECK(stdc_first_leading_one_uc(c) == UC::first_lead(c, true) && stdc_first_trailing_zero_uc(c) == UC::first_trail(c, false));
    CHECK(stdc_bit_ceil_uc(c) == UC::ceil(c) && stdc_bit_floor_uc(c) == UC::floor(c));
  }
  const unsigned long long big[] = {0, 1, 0x8000000000000000ull, 0x8000000000000001ull, ~0ull, 0xF0F0ull};
  for (auto v : big) {
    CHECK(stdc_leading_ones_ull(v) == UL::lead(v, true) && stdc_count_zeros_ull(v) == UL::count(v, false));
    CHECK(stdc_first_leading_zero_ull(v) == UL::first_lead(v, false) && stdc_first_trailing_one_ull(v) == UL::first_trail(v, true));
    CHECK(stdc_bit_width_ull(v) == UL::width(v) && stdc_has_single_bit_ull(v) == (UL::count(v, true) == 1));
    CHECK(stdc_bit_ceil_ull(v) == UL::ceil(v));
  }
  CHECK(stdc_count_ones_us(0xFFFF) == 16 && stdc_bit_ceil_us(0x8001) == 0 && stdc_bit_ceil_ui(0x80000000u) == 0x80000000u);
  CHECK(stdc_trailing_zeros_ul(0) == sizeof(unsigned long) * CHAR_BIT && stdc_bit_floor_ul(0) == 0);
  CHECK(stdc_first_leading_zero_ui(~0u) == 0 && stdc_first_leading_zero_ui(0x7FFFFFFFu) == 1);
  return 0;
}
