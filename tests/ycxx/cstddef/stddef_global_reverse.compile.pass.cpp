// [support.c.headers.other]/1, as in stddef_global, with <stddef.h> included before
// <cstddef>: the order of inclusion does not matter ([using.headers]/2: "A translation unit
// may include library headers in any order").
#include <stddef.h>
#include <cstddef>
#include <type_traits>

static_assert(std::is_same_v<std::max_align_t, ::max_align_t>);
static_assert(std::is_same_v<std::size_t, ::size_t>);
static_assert(std::is_same_v<std::ptrdiff_t, ::ptrdiff_t>);
static_assert(std::is_same_v<std::nullptr_t, ::nullptr_t>);
