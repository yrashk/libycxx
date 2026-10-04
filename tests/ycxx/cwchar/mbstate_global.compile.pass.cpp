// [support.c.headers.other]/1: every <name.h> C header "behaves as if each name placed in the
// standard library namespace by the corresponding <cname> header is placed within the global
// namespace scope". With <wchar.h> included, ::mbstate_t, ::wint_t and ::size_t name the same
// types as std::mbstate_t, std::wint_t and std::size_t.
#include <cwchar>
#include <wchar.h>
#include <type_traits>

static_assert(std::is_same_v<std::mbstate_t, ::mbstate_t>);
static_assert(std::is_same_v<std::wint_t, ::wint_t>);
static_assert(std::is_same_v<std::size_t, ::size_t>);
