// -*- C++ -*-  libycxx: <stdbit.h> ([stdbit.h.syn])   [core; all freestanding]
//
// The C23 bit utilities as C++ declares them: the typed functions of ISO/IEC 9899:2024 7.18 and
// one function template per type-generic function, on top of <bit>. They are inline C++
// functions (not constexpr: [constexpr.functions]); the C library's <stdbit.h> is not included,
// since its type-generic macros would hide the templates. The generic functions return unsigned
// int (/1: an unsigned type that holds every result), and bit_ceil returns 0 where the power of
// two is not representable, as C's does.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/bit.hpp>

#define __STDC_VERSION_STDBIT_H__ 202311L
#define __STDC_ENDIAN_LITTLE__ __ORDER_LITTLE_ENDIAN__
#define __STDC_ENDIAN_BIG__ __ORDER_BIG_ENDIAN__
#define __STDC_ENDIAN_NATIVE__ __BYTE_ORDER__

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::stdbit {
template <class T>
inline void mandates() noexcept {
  static_assert(::ycxx::detail::bit_unsigned<T>, "<stdbit.h>: Mandates: T is an unsigned integer type");
}
}} // namespace ycxx::detail::stdbit


template <class T>
inline unsigned int stdc_leading_zeros(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(std::countl_zero(v));
}
inline unsigned int stdc_leading_zeros_uc(unsigned char value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_us(unsigned short value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_ui(unsigned int value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_ul(unsigned long int value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_ull(unsigned long long int value) noexcept { return stdc_leading_zeros(value); }

template <class T>
inline unsigned int stdc_leading_ones(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(std::countl_one(v));
}
inline unsigned int stdc_leading_ones_uc(unsigned char value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_us(unsigned short value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_ui(unsigned int value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_ul(unsigned long int value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_ull(unsigned long long int value) noexcept { return stdc_leading_ones(value); }

template <class T>
inline unsigned int stdc_trailing_zeros(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(std::countr_zero(v));
}
inline unsigned int stdc_trailing_zeros_uc(unsigned char value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_us(unsigned short value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_ui(unsigned int value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_ul(unsigned long int value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_ull(unsigned long long int value) noexcept { return stdc_trailing_zeros(value); }

template <class T>
inline unsigned int stdc_trailing_ones(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(std::countr_one(v));
}
inline unsigned int stdc_trailing_ones_uc(unsigned char value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_us(unsigned short value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_ui(unsigned int value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_ul(unsigned long int value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_ull(unsigned long long int value) noexcept { return stdc_trailing_ones(value); }

template <class T>
inline unsigned int stdc_first_leading_zero(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return v == static_cast<T>(~T(0)) ? 0u : static_cast<unsigned>(std::countl_one(v)) + 1;
}
inline unsigned int stdc_first_leading_zero_uc(unsigned char value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_us(unsigned short value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_ui(unsigned int value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_ul(unsigned long int value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_ull(unsigned long long int value) noexcept { return stdc_first_leading_zero(value); }

template <class T>
inline unsigned int stdc_first_leading_one(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return v == 0 ? 0u : static_cast<unsigned>(std::countl_zero(v)) + 1;
}
inline unsigned int stdc_first_leading_one_uc(unsigned char value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_us(unsigned short value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_ui(unsigned int value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_ul(unsigned long int value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_ull(unsigned long long int value) noexcept { return stdc_first_leading_one(value); }

template <class T>
inline unsigned int stdc_first_trailing_zero(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return v == static_cast<T>(~T(0)) ? 0u : static_cast<unsigned>(std::countr_one(v)) + 1;
}
inline unsigned int stdc_first_trailing_zero_uc(unsigned char value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_us(unsigned short value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_ui(unsigned int value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_ul(unsigned long int value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_ull(unsigned long long int value) noexcept { return stdc_first_trailing_zero(value); }

template <class T>
inline unsigned int stdc_first_trailing_one(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return v == 0 ? 0u : static_cast<unsigned>(std::countr_zero(v)) + 1;
}
inline unsigned int stdc_first_trailing_one_uc(unsigned char value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_us(unsigned short value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_ui(unsigned int value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_ul(unsigned long int value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_ull(unsigned long long int value) noexcept { return stdc_first_trailing_one(value); }

template <class T>
inline unsigned int stdc_count_zeros(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(::ycxx::detail::bit_digits<T> - std::popcount(v));
}
inline unsigned int stdc_count_zeros_uc(unsigned char value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_us(unsigned short value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_ui(unsigned int value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_ul(unsigned long int value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_ull(unsigned long long int value) noexcept { return stdc_count_zeros(value); }

template <class T>
inline unsigned int stdc_count_ones(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(std::popcount(v));
}
inline unsigned int stdc_count_ones_uc(unsigned char value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_us(unsigned short value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_ui(unsigned int value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_ul(unsigned long int value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_ull(unsigned long long int value) noexcept { return stdc_count_ones(value); }

template <class T>
inline bool stdc_has_single_bit(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return std::has_single_bit(v);
}
inline bool stdc_has_single_bit_uc(unsigned char value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_us(unsigned short value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_ui(unsigned int value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_ul(unsigned long int value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_ull(unsigned long long int value) noexcept { return stdc_has_single_bit(value); }

template <class T>
inline unsigned int stdc_bit_width(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return static_cast<unsigned>(std::bit_width(v));
}
inline unsigned int stdc_bit_width_uc(unsigned char value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_us(unsigned short value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_ui(unsigned int value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_ul(unsigned long int value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_ull(unsigned long long int value) noexcept { return stdc_bit_width(value); }

template <class T>
inline T stdc_bit_floor(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return std::bit_floor(v);
}
inline unsigned char stdc_bit_floor_uc(unsigned char value) noexcept { return stdc_bit_floor(value); }
inline unsigned short stdc_bit_floor_us(unsigned short value) noexcept { return stdc_bit_floor(value); }
inline unsigned int stdc_bit_floor_ui(unsigned int value) noexcept { return stdc_bit_floor(value); }
inline unsigned long int stdc_bit_floor_ul(unsigned long int value) noexcept { return stdc_bit_floor(value); }
inline unsigned long long int stdc_bit_floor_ull(unsigned long long int value) noexcept { return stdc_bit_floor(value); }

template <class T>
inline T stdc_bit_ceil(T value) noexcept {
  ::ycxx::detail::stdbit::mandates<T>();
  const T v = value;
  return v <= 1 ? T(1) : (std::bit_width(static_cast<T>(v - 1)) >= ::ycxx::detail::bit_digits<T> ? T(0) : std::bit_ceil(v));
}
inline unsigned char stdc_bit_ceil_uc(unsigned char value) noexcept { return stdc_bit_ceil(value); }
inline unsigned short stdc_bit_ceil_us(unsigned short value) noexcept { return stdc_bit_ceil(value); }
inline unsigned int stdc_bit_ceil_ui(unsigned int value) noexcept { return stdc_bit_ceil(value); }
inline unsigned long int stdc_bit_ceil_ul(unsigned long int value) noexcept { return stdc_bit_ceil(value); }
inline unsigned long long int stdc_bit_ceil_ull(unsigned long long int value) noexcept { return stdc_bit_ceil(value); }
