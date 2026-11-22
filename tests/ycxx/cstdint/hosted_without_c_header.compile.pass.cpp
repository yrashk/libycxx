// REQUIRES: libycxx
// FLAGS: -nostdinc
// A hosted translation unit can have libycxx headers without a C library header search path.
#if !__STDC_HOSTED__ || __has_include(<stdint.h>)
#  error "this test requires hosted mode without <stdint.h>"
#endif
#include <cstdint>

static_assert(__is_same(std::int_fast16_t, __INT_FAST16_TYPE__));
static_assert(__is_same(std::int_fast32_t, __INT_FAST32_TYPE__));
static_assert(__is_same(std::uint_fast16_t, __UINT_FAST16_TYPE__));
static_assert(__is_same(std::uint_fast32_t, __UINT_FAST32_TYPE__));
static_assert(__is_same(::int_fast16_t, std::int_fast16_t));
static_assert(__is_same(::int_fast32_t, std::int_fast32_t));
static_assert(__is_same(::uint_fast16_t, std::uint_fast16_t));
static_assert(__is_same(::uint_fast32_t, std::uint_fast32_t));

#if INT_FAST16_MIN != (-__INT_FAST16_MAX__ - 1) || INT_FAST16_MAX != __INT_FAST16_MAX__ || \
    UINT_FAST16_MAX != __UINT_FAST16_MAX__ || INT_FAST16_WIDTH != __INT_FAST16_WIDTH__ || \
    UINT_FAST16_WIDTH != __INT_FAST16_WIDTH__
#  error "incorrect limits or widths for the fallback fast 16-bit types"
#endif
#if INT_FAST32_MIN != (-__INT_FAST32_MAX__ - 1) || INT_FAST32_MAX != __INT_FAST32_MAX__ || \
    UINT_FAST32_MAX != __UINT_FAST32_MAX__ || INT_FAST32_WIDTH != __INT_FAST32_WIDTH__ || \
    UINT_FAST32_WIDTH != __INT_FAST32_WIDTH__
#  error "incorrect limits or widths for the fallback fast 32-bit types"
#endif
