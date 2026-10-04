// [bit.syn]: the declared return types and exception specifications, for every standard
// unsigned integer type (and the <cstdint> aliases): bit_cast/byteswap/bit_floor/
// has_single_bit/shl/shr/rotl/rotr/count*/popcount/bit_reverse/bit_compress/bit_expand are
// noexcept; bit_ceil and bit_repeat are not declared noexcept (they have preconditions).
// has_single_bit returns bool; bit_width and the counting functions return int; the others
// return T. Every function is constexpr (checked by use in constant expressions).
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

template <class T>
constexpr bool check() {
  T x{};
  static_assert(std::is_same_v<decltype(std::byteswap(x)), T>);
  static_assert(std::is_same_v<decltype(std::has_single_bit(x)), bool>);
  static_assert(std::is_same_v<decltype(std::bit_ceil(x)), T>);
  static_assert(std::is_same_v<decltype(std::bit_floor(x)), T>);
  static_assert(std::is_same_v<decltype(std::bit_width(x)), int>);
  static_assert(std::is_same_v<decltype(std::shl(x, 1)), T>);
  static_assert(std::is_same_v<decltype(std::shr(x, 1)), T>);
  static_assert(std::is_same_v<decltype(std::rotl(x, 1)), T>);
  static_assert(std::is_same_v<decltype(std::rotr(x, 1)), T>);
  static_assert(std::is_same_v<decltype(std::countl_zero(x)), int>);
  static_assert(std::is_same_v<decltype(std::countl_one(x)), int>);
  static_assert(std::is_same_v<decltype(std::countr_zero(x)), int>);
  static_assert(std::is_same_v<decltype(std::countr_one(x)), int>);
  static_assert(std::is_same_v<decltype(std::popcount(x)), int>);
  static_assert(std::is_same_v<decltype(std::bit_reverse(x)), T>);
  static_assert(std::is_same_v<decltype(std::bit_repeat(x, 1)), T>);
  static_assert(std::is_same_v<decltype(std::bit_compress(x, x)), T>);
  static_assert(std::is_same_v<decltype(std::bit_expand(x, x)), T>);
  static_assert(std::is_same_v<decltype(std::bit_cast<T>(x)), T>);

  static_assert(noexcept(std::byteswap(x)));
  static_assert(noexcept(std::has_single_bit(x)));
  static_assert(noexcept(std::bit_floor(x)));
  static_assert(noexcept(std::bit_width(x)));
  static_assert(noexcept(std::shl(x, 1)));
  static_assert(noexcept(std::shr(x, 1)));
  static_assert(noexcept(std::rotl(x, 1)));
  static_assert(noexcept(std::rotr(x, 1)));
  static_assert(noexcept(std::countl_zero(x)));
  static_assert(noexcept(std::countl_one(x)));
  static_assert(noexcept(std::countr_zero(x)));
  static_assert(noexcept(std::countr_one(x)));
  static_assert(noexcept(std::popcount(x)));
  static_assert(noexcept(std::bit_reverse(x)));
  static_assert(noexcept(std::bit_compress(x, x)));
  static_assert(noexcept(std::bit_expand(x, x)));
  static_assert(noexcept(std::bit_cast<T>(x)));

  // all usable in constant evaluation, with exact results at small values
  T one = 1;
  return std::byteswap(T(0)) == 0 && std::has_single_bit(one) && std::bit_ceil(T(3)) == 4 &&
         std::bit_floor(T(3)) == 2 && std::bit_width(T(3)) == 2 && std::shl(one, 2) == 4 &&
         std::shr(T(4), 2) == 1 && std::rotl(one, 1) == 2 && std::rotr(T(2), 1) == 1 &&
         std::countl_zero(one) == std::numeric_limits<T>::digits - 1 && std::countl_one(T(0)) == 0 &&
         std::countr_zero(T(4)) == 2 && std::countr_one(T(3)) == 2 && std::popcount(T(7)) == 3 &&
         std::bit_reverse(one) == T(T(1) << (std::numeric_limits<T>::digits - 1)) &&
         std::bit_repeat(one, 2) == T(T(~T(0)) / 3) && std::bit_compress(T(0b1010), T(0b1010)) == 3 &&
         std::bit_expand(T(3), T(0b1010)) == 0b1010 && std::bit_cast<T>(one) == 1;
}

static_assert(check<unsigned char>());
static_assert(check<unsigned short>());
static_assert(check<unsigned int>());
static_assert(check<unsigned long>());
static_assert(check<unsigned long long>());
static_assert(check<std::uint8_t>());
static_assert(check<std::uint16_t>());
static_assert(check<std::uint32_t>());
static_assert(check<std::uint64_t>());
static_assert(check<std::size_t>());
static_assert(check<std::uintmax_t>());
static_assert(check<std::uintptr_t>());
