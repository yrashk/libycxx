// libycxx core: fundamental types from <cstddef>, defined without any C header.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/prim_traits.hpp>

namespace std {
using size_t = decltype(sizeof(0));
using ptrdiff_t = decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr));
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

