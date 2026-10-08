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
#include <ycxx/core/version.hpp>
#include <ycxx/core/bit.hpp>

#define __STDC_VERSION_STDBIT_H__ 202311L
#define __STDC_ENDIAN_LITTLE__ __ORDER_LITTLE_ENDIAN__
#define __STDC_ENDIAN_BIG__ __ORDER_BIG_ENDIAN__
#define __STDC_ENDIAN_NATIVE__ __BYTE_ORDER__

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__stdbit {
template <class _Tp>
inline void __mandates() noexcept {
  static_assert(::__ycxx::__detail::__bit_unsigned<_Tp>, "<stdbit.h>: Mandates: T is an unsigned integer type");
}
}} // namespace __ycxx::__detail::__stdbit


template <class _Tp>
inline unsigned int stdc_leading_zeros(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(std::countl_zero(__v));
}
inline unsigned int stdc_leading_zeros_uc(unsigned char value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_us(unsigned short value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_ui(unsigned int value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_ul(unsigned long int value) noexcept { return stdc_leading_zeros(value); }
inline unsigned int stdc_leading_zeros_ull(unsigned long long int value) noexcept { return stdc_leading_zeros(value); }

template <class _Tp>
inline unsigned int stdc_leading_ones(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(std::countl_one(__v));
}
inline unsigned int stdc_leading_ones_uc(unsigned char value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_us(unsigned short value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_ui(unsigned int value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_ul(unsigned long int value) noexcept { return stdc_leading_ones(value); }
inline unsigned int stdc_leading_ones_ull(unsigned long long int value) noexcept { return stdc_leading_ones(value); }

template <class _Tp>
inline unsigned int stdc_trailing_zeros(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(std::countr_zero(__v));
}
inline unsigned int stdc_trailing_zeros_uc(unsigned char value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_us(unsigned short value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_ui(unsigned int value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_ul(unsigned long int value) noexcept { return stdc_trailing_zeros(value); }
inline unsigned int stdc_trailing_zeros_ull(unsigned long long int value) noexcept { return stdc_trailing_zeros(value); }

template <class _Tp>
inline unsigned int stdc_trailing_ones(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(std::countr_one(__v));
}
inline unsigned int stdc_trailing_ones_uc(unsigned char value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_us(unsigned short value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_ui(unsigned int value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_ul(unsigned long int value) noexcept { return stdc_trailing_ones(value); }
inline unsigned int stdc_trailing_ones_ull(unsigned long long int value) noexcept { return stdc_trailing_ones(value); }

template <class _Tp>
inline unsigned int stdc_first_leading_zero(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return __v == static_cast<_Tp>(~_Tp(0)) ? 0u : static_cast<unsigned>(std::countl_one(__v)) + 1;
}
inline unsigned int stdc_first_leading_zero_uc(unsigned char value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_us(unsigned short value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_ui(unsigned int value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_ul(unsigned long int value) noexcept { return stdc_first_leading_zero(value); }
inline unsigned int stdc_first_leading_zero_ull(unsigned long long int value) noexcept { return stdc_first_leading_zero(value); }

template <class _Tp>
inline unsigned int stdc_first_leading_one(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return __v == 0 ? 0u : static_cast<unsigned>(std::countl_zero(__v)) + 1;
}
inline unsigned int stdc_first_leading_one_uc(unsigned char value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_us(unsigned short value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_ui(unsigned int value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_ul(unsigned long int value) noexcept { return stdc_first_leading_one(value); }
inline unsigned int stdc_first_leading_one_ull(unsigned long long int value) noexcept { return stdc_first_leading_one(value); }

template <class _Tp>
inline unsigned int stdc_first_trailing_zero(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return __v == static_cast<_Tp>(~_Tp(0)) ? 0u : static_cast<unsigned>(std::countr_one(__v)) + 1;
}
inline unsigned int stdc_first_trailing_zero_uc(unsigned char value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_us(unsigned short value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_ui(unsigned int value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_ul(unsigned long int value) noexcept { return stdc_first_trailing_zero(value); }
inline unsigned int stdc_first_trailing_zero_ull(unsigned long long int value) noexcept { return stdc_first_trailing_zero(value); }

template <class _Tp>
inline unsigned int stdc_first_trailing_one(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return __v == 0 ? 0u : static_cast<unsigned>(std::countr_zero(__v)) + 1;
}
inline unsigned int stdc_first_trailing_one_uc(unsigned char value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_us(unsigned short value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_ui(unsigned int value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_ul(unsigned long int value) noexcept { return stdc_first_trailing_one(value); }
inline unsigned int stdc_first_trailing_one_ull(unsigned long long int value) noexcept { return stdc_first_trailing_one(value); }

template <class _Tp>
inline unsigned int stdc_count_zeros(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(::__ycxx::__detail::__bit_digits<_Tp> - std::popcount(__v));
}
inline unsigned int stdc_count_zeros_uc(unsigned char value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_us(unsigned short value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_ui(unsigned int value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_ul(unsigned long int value) noexcept { return stdc_count_zeros(value); }
inline unsigned int stdc_count_zeros_ull(unsigned long long int value) noexcept { return stdc_count_zeros(value); }

template <class _Tp>
inline unsigned int stdc_count_ones(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(std::popcount(__v));
}
inline unsigned int stdc_count_ones_uc(unsigned char value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_us(unsigned short value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_ui(unsigned int value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_ul(unsigned long int value) noexcept { return stdc_count_ones(value); }
inline unsigned int stdc_count_ones_ull(unsigned long long int value) noexcept { return stdc_count_ones(value); }

template <class _Tp>
inline bool stdc_has_single_bit(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return std::has_single_bit(__v);
}
inline bool stdc_has_single_bit_uc(unsigned char value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_us(unsigned short value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_ui(unsigned int value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_ul(unsigned long int value) noexcept { return stdc_has_single_bit(value); }
inline bool stdc_has_single_bit_ull(unsigned long long int value) noexcept { return stdc_has_single_bit(value); }

template <class _Tp>
inline unsigned int stdc_bit_width(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return static_cast<unsigned>(std::bit_width(__v));
}
inline unsigned int stdc_bit_width_uc(unsigned char value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_us(unsigned short value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_ui(unsigned int value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_ul(unsigned long int value) noexcept { return stdc_bit_width(value); }
inline unsigned int stdc_bit_width_ull(unsigned long long int value) noexcept { return stdc_bit_width(value); }

template <class _Tp>
inline _Tp stdc_bit_floor(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return std::bit_floor(__v);
}
inline unsigned char stdc_bit_floor_uc(unsigned char value) noexcept { return stdc_bit_floor(value); }
inline unsigned short stdc_bit_floor_us(unsigned short value) noexcept { return stdc_bit_floor(value); }
inline unsigned int stdc_bit_floor_ui(unsigned int value) noexcept { return stdc_bit_floor(value); }
inline unsigned long int stdc_bit_floor_ul(unsigned long int value) noexcept { return stdc_bit_floor(value); }
inline unsigned long long int stdc_bit_floor_ull(unsigned long long int value) noexcept { return stdc_bit_floor(value); }

template <class _Tp>
inline _Tp stdc_bit_ceil(_Tp value) noexcept {
  ::__ycxx::__detail::__stdbit::__mandates<_Tp>();
  const _Tp __v = value;
  return __v <= 1 ? _Tp(1) : (std::bit_width(static_cast<_Tp>(__v - 1)) >= ::__ycxx::__detail::__bit_digits<_Tp> ? _Tp(0) : std::bit_ceil(__v));
}
inline unsigned char stdc_bit_ceil_uc(unsigned char value) noexcept { return stdc_bit_ceil(value); }
inline unsigned short stdc_bit_ceil_us(unsigned short value) noexcept { return stdc_bit_ceil(value); }
inline unsigned int stdc_bit_ceil_ui(unsigned int value) noexcept { return stdc_bit_ceil(value); }
inline unsigned long int stdc_bit_ceil_ul(unsigned long int value) noexcept { return stdc_bit_ceil(value); }
inline unsigned long long int stdc_bit_ceil_ull(unsigned long long int value) noexcept { return stdc_bit_ceil(value); }
