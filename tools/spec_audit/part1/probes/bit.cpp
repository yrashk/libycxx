// [bit]: <bit> (all freestanding): signatures, constexpr, noexcept and constraints.
// FREESTANDING
#include <bit>
#include <cstdint>
#include <type_traits>

using u8 = unsigned char;
using u32 = std::uint32_t;

// [bit.cast]
static_assert(std::bit_cast<u32>(1.0f) == 0x3f800000u);
static_assert(noexcept(std::bit_cast<u32>(1.0f)));
template <class To, class From>
concept can_bit_cast = requires(const From& f) { std::bit_cast<To>(f); };
static_assert(!can_bit_cast<u32, double>);      // sizeof differs: Constraints
// [bit.byteswap]
static_assert(std::byteswap(u32(0x01020304)) == 0x04030201u && noexcept(std::byteswap(u32())));
static_assert(std::byteswap(std::int16_t(0x0102)) == 0x0201);
template <class T>
concept can_byteswap = requires(T t) { std::byteswap(t); };
static_assert(!can_byteswap<float> && can_byteswap<bool>);   // Constraints: integral (bool is)
// [bit.pow.two]
static_assert(std::has_single_bit(8u) && noexcept(std::has_single_bit(8u)));
static_assert(std::bit_ceil(5u) == 8u);
static_assert(std::bit_floor(5u) == 4u && noexcept(std::bit_floor(5u)));
static_assert(std::is_same_v<decltype(std::bit_width(5u)), int> && std::bit_width(5u) == 3);
template <class T>
concept can_pow2 = requires(T t) { std::has_single_bit(t); };
static_assert(!can_pow2<int> && !can_pow2<bool> && !can_pow2<char> && can_pow2<unsigned long long>);
// [bit.shift]: shifts by any count
static_assert(std::shl(u32(1), 3) == 8u && noexcept(std::shl(u32(1), 3)));
static_assert(std::shr(u32(8), 3) == 1u && noexcept(std::shr(u32(8), 3)));
static_assert(std::shl(u32(1), 32) == 0u && std::shr(u32(0x80000000u), 40) == 0u);
static_assert(std::is_same_v<decltype(std::shl(u8(1), 3)), u8>);
// signed operands, negative counts: x * 2^s rounded towards negative infinity ([bit.shift]/3, /5)
static_assert(std::shl(-1, 1) == -2 && std::shr(-1, 100) == -1 && std::shr(-7, 1) == -4);
static_assert(std::shl(u32(8), -3) == 1u && std::shr(u32(1), -3) == 8u && std::shl(1, 31) == (-2147483647 - 1));
static_assert(std::shl(u32(1), 1000000LL) == 0u && std::shr(-8LL, (unsigned char)2) == -2);
template <class T, class S>
concept can_shl = requires(T t, S s) { std::shl(t, s); };
static_assert(!can_shl<bool, int> && !can_shl<int, bool> && !can_shl<char, int> && !can_shl<float, int>);
// [bit.rotate], [bit.count]
static_assert(std::rotl(u8(0x81), 1) == 0x03 && std::rotr(u8(0x81), 1) == 0xc0 && noexcept(std::rotl(u8(), 1)));
static_assert(std::countl_zero(u8(1)) == 7 && std::countl_one(u8(0xf0)) == 4 && std::countr_zero(u8(8)) == 3 &&
              std::countr_one(u8(7)) == 3 && std::popcount(u8(0xff)) == 8);
static_assert(noexcept(std::popcount(0u)) && std::is_same_v<decltype(std::popcount(0u)), int>);
// [bit.permute] (bitops 202607L)
static_assert(std::bit_reverse(u8(0x01)) == 0x80 && noexcept(std::bit_reverse(u8())));
static_assert(std::bit_repeat(u8(0b10), 2) == 0b10101010);
static_assert(std::bit_compress(u8(0b10110010), u8(0b11110000)) == 0b1011 && noexcept(std::bit_compress(u8(), u8())));
static_assert(std::bit_expand(u8(0b1011), u8(0b11110000)) == 0b10110000 && noexcept(std::bit_expand(u8(), u8())));
// [bit.endian]
static_assert(std::is_enum_v<std::endian> && !std::is_convertible_v<std::endian, int>);
static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);
