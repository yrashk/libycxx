// libycxx core: fundamental types from <cstddef>, defined without the C library's headers.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/prim_traits.hpp>

// The compiler's own <stddef.h> (not the C library's: GCC and Clang provide it, for freestanding
// environments too) defines ::max_align_t, and [support.c.headers.other]/1 makes it the same type
// as std::max_align_t, so core reads it, past libycxx's own <stddef.h> (which includes this
// header). Its other names (::size_t, ::ptrdiff_t, NULL, offsetof) are what core declares below
// anyway: [headers]/5 leaves it unspecified whether these names are also declared at global
// scope, code relies on that, and <stddef.h> needs them there. A typedef of the same type may be
// redeclared, so the order in which the headers are read does not matter.
#include_next <stddef.h>

typedef decltype(sizeof(0)) size_t;
typedef decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr)) ptrdiff_t;
typedef decltype(nullptr) nullptr_t;

// In plain std (DECISIONS §20.5): GCC lets std::byte alias any object ([basic.lval]/11.3) only
// under that name; in an inline namespace it would compile and then miscompile.
namespace [[__gnu__::__visibility__("hidden")]] std { // plain std (DECISIONS §20.5)
using ::size_t;
using ::ptrdiff_t;
using ::nullptr_t;
using ::max_align_t;

enum class byte : unsigned char {};

template <class _IntType>
  requires ::__ycxx::__detail::is_integral_v<_IntType>
constexpr byte& operator<<=(byte& b, _IntType shift) noexcept {
  return b = byte(static_cast<unsigned int>(b) << shift);
}
template <class _IntType>
  requires ::__ycxx::__detail::is_integral_v<_IntType>
constexpr byte operator<<(byte b, _IntType shift) noexcept {
  return byte(static_cast<unsigned int>(b) << shift);
}
template <class _IntType>
  requires ::__ycxx::__detail::is_integral_v<_IntType>
constexpr byte& operator>>=(byte& b, _IntType shift) noexcept {
  return b = byte(static_cast<unsigned int>(b) >> shift);
}
template <class _IntType>
  requires ::__ycxx::__detail::is_integral_v<_IntType>
constexpr byte operator>>(byte b, _IntType shift) noexcept {
  return byte(static_cast<unsigned int>(b) >> shift);
}
constexpr byte& operator|=(byte& __l, byte r) noexcept {
  return __l = byte(static_cast<unsigned int>(__l) | static_cast<unsigned int>(r));
}
constexpr byte& operator&=(byte& __l, byte r) noexcept {
  return __l = byte(static_cast<unsigned int>(__l) & static_cast<unsigned int>(r));
}
constexpr byte& operator^=(byte& __l, byte r) noexcept {
  return __l = byte(static_cast<unsigned int>(__l) ^ static_cast<unsigned int>(r));
}
constexpr byte operator|(byte __l, byte r) noexcept {
  return byte(static_cast<unsigned int>(__l) | static_cast<unsigned int>(r));
}
constexpr byte operator&(byte __l, byte r) noexcept {
  return byte(static_cast<unsigned int>(__l) & static_cast<unsigned int>(r));
}
constexpr byte operator^(byte __l, byte r) noexcept {
  return byte(static_cast<unsigned int>(__l) ^ static_cast<unsigned int>(r));
}
constexpr byte operator~(byte b) noexcept { return byte(~static_cast<unsigned int>(b)); }
template <class _IntType>
  requires ::__ycxx::__detail::is_integral_v<_IntType>
constexpr _IntType to_integer(byte b) noexcept {
  return static_cast<_IntType>(b);
}
} // namespace std

#ifndef NULL
#  define NULL __null
#endif
#ifndef offsetof
#  define offsetof(type, __member) __builtin_offsetof(type, __member)
#endif

