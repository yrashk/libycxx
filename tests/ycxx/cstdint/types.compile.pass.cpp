// [cstdint.syn]/2: <cstdint> "defines all types and macros the same as the C standard library
// header <stdint.h>, except that the types intptr_t and uintptr_t and the macros INTPTR_MIN,
// INTPTR_MAX, and UINTPTR_MAX are always defined." /3: "if an implementation defines integer
// types with the corresponding width and no padding bits, it declares the corresponding
// typedef-names. Each of the macros listed in this subclause is defined if and only if the
// implementation declares the corresponding typedef-name."
// C 7.22.1: intN_t is a signed integer type with width N and no padding bits; int_leastN_t
// has width at least N; int_fastN_t has width at least N; intmax_t can represent any value of
// any signed integer type (this document: of the standard ones); intptr_t can hold a converted
// void*. The implementation targets have 8/16/32/64-bit types without padding, so the
// exact-width types exist.
#include <cstdint>
#include <climits>
#include <limits>
#include <type_traits>

template <class T, bool Signed>
constexpr bool integer_type() {
  return std::is_integral_v<T> && std::is_signed_v<T> == Signed && !std::is_same_v<T, bool> &&
         !std::is_same_v<T, char> && !std::is_same_v<T, wchar_t> && !std::is_same_v<T, char8_t> &&
         !std::is_same_v<T, char16_t> && !std::is_same_v<T, char32_t>;
}
template <class T>
constexpr int width = std::numeric_limits<T>::digits + std::numeric_limits<T>::is_signed;

static_assert(integer_type<std::int8_t, true> && integer_type<std::uint8_t, false>);
static_assert(width<std::int8_t> == 8 && width<std::uint8_t> == 8);
static_assert(sizeof(std::int8_t) * CHAR_BIT == 8 && sizeof(std::uint8_t) * CHAR_BIT == 8);  // no padding
static_assert(std::is_same_v<std::make_unsigned_t<std::int8_t>, std::uint8_t>);
static_assert(integer_type<std::int_least8_t, true> && integer_type<std::uint_least8_t, false>);
static_assert(width<std::int_least8_t> >= 8 && width<std::uint_least8_t> >= 8);
static_assert(integer_type<std::int_fast8_t, true> && integer_type<std::uint_fast8_t, false>);
static_assert(width<std::int_fast8_t> >= 8 && width<std::uint_fast8_t> >= 8);
static_assert(integer_type<std::int16_t, true> && integer_type<std::uint16_t, false>);
static_assert(width<std::int16_t> == 16 && width<std::uint16_t> == 16);
static_assert(sizeof(std::int16_t) * CHAR_BIT == 16 && sizeof(std::uint16_t) * CHAR_BIT == 16);  // no padding
static_assert(std::is_same_v<std::make_unsigned_t<std::int16_t>, std::uint16_t>);
static_assert(integer_type<std::int_least16_t, true> && integer_type<std::uint_least16_t, false>);
static_assert(width<std::int_least16_t> >= 16 && width<std::uint_least16_t> >= 16);
static_assert(integer_type<std::int_fast16_t, true> && integer_type<std::uint_fast16_t, false>);
static_assert(width<std::int_fast16_t> >= 16 && width<std::uint_fast16_t> >= 16);
static_assert(integer_type<std::int32_t, true> && integer_type<std::uint32_t, false>);
static_assert(width<std::int32_t> == 32 && width<std::uint32_t> == 32);
static_assert(sizeof(std::int32_t) * CHAR_BIT == 32 && sizeof(std::uint32_t) * CHAR_BIT == 32);  // no padding
static_assert(std::is_same_v<std::make_unsigned_t<std::int32_t>, std::uint32_t>);
static_assert(integer_type<std::int_least32_t, true> && integer_type<std::uint_least32_t, false>);
static_assert(width<std::int_least32_t> >= 32 && width<std::uint_least32_t> >= 32);
static_assert(integer_type<std::int_fast32_t, true> && integer_type<std::uint_fast32_t, false>);
static_assert(width<std::int_fast32_t> >= 32 && width<std::uint_fast32_t> >= 32);
static_assert(integer_type<std::int64_t, true> && integer_type<std::uint64_t, false>);
static_assert(width<std::int64_t> == 64 && width<std::uint64_t> == 64);
static_assert(sizeof(std::int64_t) * CHAR_BIT == 64 && sizeof(std::uint64_t) * CHAR_BIT == 64);  // no padding
static_assert(std::is_same_v<std::make_unsigned_t<std::int64_t>, std::uint64_t>);
static_assert(integer_type<std::int_least64_t, true> && integer_type<std::uint_least64_t, false>);
static_assert(width<std::int_least64_t> >= 64 && width<std::uint_least64_t> >= 64);
static_assert(integer_type<std::int_fast64_t, true> && integer_type<std::uint_fast64_t, false>);
static_assert(width<std::int_fast64_t> >= 64 && width<std::uint_fast64_t> >= 64);
// C 7.22.1.2: int_leastN_t is "the smallest signed integer type with a width of at least N";
// with an exact-width type present, that has width N.
static_assert(width<std::int_least8_t> == 8 && width<std::uint_least16_t> == 16);
static_assert(width<std::int_least32_t> == 32 && width<std::uint_least64_t> == 64);
static_assert(integer_type<std::intmax_t, true> && integer_type<std::uintmax_t, false>);
static_assert(width<std::intmax_t> >= width<long long> && width<std::uintmax_t> >= width<unsigned long long>);
static_assert(integer_type<std::intptr_t, true> && integer_type<std::uintptr_t, false>);
static_assert(sizeof(std::intptr_t) >= sizeof(void*) && sizeof(std::uintptr_t) >= sizeof(void*));
static_assert(std::is_same_v<std::make_unsigned_t<std::intmax_t>, std::uintmax_t>);
static_assert(std::is_same_v<std::make_unsigned_t<std::intptr_t>, std::uintptr_t>);
