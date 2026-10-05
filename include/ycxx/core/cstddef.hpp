// libycxx core: fundamental types from <cstddef>, defined without the C library's headers.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/prim_traits.hpp>

// The compiler's own <stddef.h> (not the C library's: GCC and Clang provide it, for freestanding
// environments too) defines ::max_align_t, and [support.c.headers.other]/1 makes it the same type
// as std::max_align_t, so core includes it; libycxx has no <stddef.h> of its own. Its other names
// (::size_t, ::ptrdiff_t, ::nullptr_t, NULL, offsetof) are what core declares below anyway:
// [headers]/5 leaves it unspecified whether these names are also declared at global scope, and
// code relies on that. A typedef of the same type may be redeclared, so the order in which this
// header and <stddef.h> are read does not matter.
#include <stddef.h>

typedef decltype(sizeof(0)) size_t;
typedef decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr)) ptrdiff_t;
typedef decltype(nullptr) nullptr_t;

namespace [[gnu::visibility("hidden")]] std {
using ::size_t;
using ::ptrdiff_t;
using ::nullptr_t;
using ::max_align_t;

enum class byte : unsigned char {};

template <class IntType>
  requires ::ycxx::detail::is_integral_v<IntType>
constexpr byte& operator<<=(byte& b, IntType shift) noexcept {
  return b = byte(static_cast<unsigned int>(b) << shift);
}
template <class IntType>
  requires ::ycxx::detail::is_integral_v<IntType>
constexpr byte operator<<(byte b, IntType shift) noexcept {
  return byte(static_cast<unsigned int>(b) << shift);
}
template <class IntType>
  requires ::ycxx::detail::is_integral_v<IntType>
constexpr byte& operator>>=(byte& b, IntType shift) noexcept {
  return b = byte(static_cast<unsigned int>(b) >> shift);
}
template <class IntType>
  requires ::ycxx::detail::is_integral_v<IntType>
constexpr byte operator>>(byte b, IntType shift) noexcept {
  return byte(static_cast<unsigned int>(b) >> shift);
}
constexpr byte& operator|=(byte& l, byte r) noexcept {
  return l = byte(static_cast<unsigned int>(l) | static_cast<unsigned int>(r));
}
constexpr byte& operator&=(byte& l, byte r) noexcept {
  return l = byte(static_cast<unsigned int>(l) & static_cast<unsigned int>(r));
}
constexpr byte& operator^=(byte& l, byte r) noexcept {
  return l = byte(static_cast<unsigned int>(l) ^ static_cast<unsigned int>(r));
}
constexpr byte operator|(byte l, byte r) noexcept {
  return byte(static_cast<unsigned int>(l) | static_cast<unsigned int>(r));
}
constexpr byte operator&(byte l, byte r) noexcept {
  return byte(static_cast<unsigned int>(l) & static_cast<unsigned int>(r));
}
constexpr byte operator^(byte l, byte r) noexcept {
  return byte(static_cast<unsigned int>(l) ^ static_cast<unsigned int>(r));
}
constexpr byte operator~(byte b) noexcept { return byte(~static_cast<unsigned int>(b)); }
template <class IntType>
  requires ::ycxx::detail::is_integral_v<IntType>
constexpr IntType to_integer(byte b) noexcept {
  return static_cast<IntType>(b);
}
} // namespace std

#ifndef NULL
#  define NULL __null
#endif
#ifndef offsetof
#  define offsetof(type, member) __builtin_offsetof(type, member)
#endif

