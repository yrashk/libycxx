// -*- C++ -*-  libycxx: <stdckdint.h> ([stdckdint.h.syn], [numerics.c.ckdint])   [core]
//
// C23's checked integer operations (ISO/IEC 9899:2024 7.20) as function templates in the global
// namespace: the result of the mathematical operation on the values of a and b, converted to
// type1 modulo 2^N, is stored in *result; the return value is whether it differed from the
// mathematical result. The compilers' overflow builtins compute exactly that for any combination
// of integer types. The C library's <stdckdint.h> (whose macros would hide the templates) is not
// included.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/type_traits.hpp>

#define __STDC_VERSION_STDCKDINT_H__ 202311L

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class T1, class T2, class T3>
consteval bool ckd_mandates() {
  static_assert(is_signed_or_unsigned_integer<T1> && is_signed_or_unsigned_integer<T2> &&
                    is_signed_or_unsigned_integer<T3>,
                "<stdckdint.h>: Mandates: each of type1, type2 and type3 is a signed or unsigned integer type");
  return true;
}
}} // namespace ycxx::detail

template <class type1, class type2, class type3>
inline bool ckd_add(type1* result, type2 a, type3 b) noexcept {
  static_assert(::ycxx::detail::ckd_mandates<type1, type2, type3>());
  return __builtin_add_overflow(a, b, result);
}
template <class type1, class type2, class type3>
inline bool ckd_sub(type1* result, type2 a, type3 b) noexcept {
  static_assert(::ycxx::detail::ckd_mandates<type1, type2, type3>());
  return __builtin_sub_overflow(a, b, result);
}
template <class type1, class type2, class type3>
inline bool ckd_mul(type1* result, type2 a, type3 b) noexcept {
  static_assert(::ycxx::detail::ckd_mandates<type1, type2, type3>());
  return __builtin_mul_overflow(a, b, result);
}
