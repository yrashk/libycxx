// [cstdint.syn]/2: <cstdint> "defines all types and macros the same as the C standard library
// header <stdint.h>"...
// [support.c.headers.other]/1: with <stdint.h> also included, the global names are the same
// types as those in namespace std.
#include <cstdint>
#include <stdint.h>
#include <type_traits>

static_assert(std::is_same_v<std::int8_t, ::int8_t>);
static_assert(std::is_same_v<std::uint8_t, ::uint8_t>);
static_assert(std::is_same_v<std::int_least8_t, ::int_least8_t>);
static_assert(std::is_same_v<std::uint_least8_t, ::uint_least8_t>);
static_assert(std::is_same_v<std::int_fast8_t, ::int_fast8_t>);
static_assert(std::is_same_v<std::uint_fast8_t, ::uint_fast8_t>);
static_assert(std::is_same_v<std::int16_t, ::int16_t>);
static_assert(std::is_same_v<std::uint16_t, ::uint16_t>);
static_assert(std::is_same_v<std::int_least16_t, ::int_least16_t>);
static_assert(std::is_same_v<std::uint_least16_t, ::uint_least16_t>);
static_assert(std::is_same_v<std::int_fast16_t, ::int_fast16_t>);
static_assert(std::is_same_v<std::uint_fast16_t, ::uint_fast16_t>);
static_assert(std::is_same_v<std::int32_t, ::int32_t>);
static_assert(std::is_same_v<std::uint32_t, ::uint32_t>);
static_assert(std::is_same_v<std::int_least32_t, ::int_least32_t>);
static_assert(std::is_same_v<std::uint_least32_t, ::uint_least32_t>);
static_assert(std::is_same_v<std::int_fast32_t, ::int_fast32_t>);
static_assert(std::is_same_v<std::uint_fast32_t, ::uint_fast32_t>);
static_assert(std::is_same_v<std::int64_t, ::int64_t>);
static_assert(std::is_same_v<std::uint64_t, ::uint64_t>);
static_assert(std::is_same_v<std::int_least64_t, ::int_least64_t>);
static_assert(std::is_same_v<std::uint_least64_t, ::uint_least64_t>);
static_assert(std::is_same_v<std::int_fast64_t, ::int_fast64_t>);
static_assert(std::is_same_v<std::uint_fast64_t, ::uint_fast64_t>);
static_assert(std::is_same_v<std::intmax_t, ::intmax_t> && std::is_same_v<std::uintmax_t, ::uintmax_t>);
static_assert(std::is_same_v<std::intptr_t, ::intptr_t> && std::is_same_v<std::uintptr_t, ::uintptr_t>);
