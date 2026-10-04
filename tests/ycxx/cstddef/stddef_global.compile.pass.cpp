// [support.c.headers.other]/1: <stddef.h> "behaves as if each name placed in the standard
// library namespace by the corresponding <cname> header is placed within the global namespace
// scope, except for ... the declaration of std::byte ([cstddef.syn]), and the functions and
// function templates described in [support.types.byteops]." So with both headers included,
// ::max_align_t, ::size_t, ::ptrdiff_t and ::nullptr_t are the std types.
#include <cstddef>
#include <stddef.h>
#include <type_traits>

static_assert(std::is_same_v<std::max_align_t, ::max_align_t>);
static_assert(std::is_same_v<std::size_t, ::size_t>);
static_assert(std::is_same_v<std::ptrdiff_t, ::ptrdiff_t>);
static_assert(std::is_same_v<std::nullptr_t, ::nullptr_t>);
static_assert(alignof(std::max_align_t) == alignof(::max_align_t));
