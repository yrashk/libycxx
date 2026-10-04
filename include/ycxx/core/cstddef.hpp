// libycxx core: fundamental types from <cstddef>, defined without any C header.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/prim_traits.hpp>

// [headers]/7 leaves it unspecified whether these names are also declared at global scope.
// Every C library's <stddef.h> declares ::size_t and ::ptrdiff_t, and much existing code relies
// on that, so they are declared there too. A typedef of the same type may be redeclared, so a
// later <stddef.h> stays valid. (max_align_t is a class, so only std::max_align_t is ours.)
typedef decltype(sizeof(0)) size_t;
typedef decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr)) ptrdiff_t;

namespace std {
using ::size_t;
using ::ptrdiff_t;
using nullptr_t = decltype(nullptr);

// Same definition as the compilers' own <stddef.h> uses, so std::max_align_t has the
// alignment the compiler expects (alignof == __BIGGEST_ALIGNMENT__ on x86_64).
struct alignas(__BIGGEST_ALIGNMENT__ > alignof(long double) ? __BIGGEST_ALIGNMENT__ : alignof(long double))
    max_align_t {
  long long ll;
  long double ld;
};

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

