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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _T1, class _T2, class _T3>
consteval bool __ckd_mandates() {
  static_assert(__is_signed_or_unsigned_integer<_T1> && __is_signed_or_unsigned_integer<_T2> &&
                    __is_signed_or_unsigned_integer<_T3>,
                "<stdckdint.h>: Mandates: each of type1, type2 and type3 is a signed or unsigned integer type");
  return true;
}
}} // namespace __ycxx::__detail

template <class __type1, class __type2, class __type3>
inline bool ckd_add(__type1* result, __type2 a, __type3 b) noexcept {
  static_assert(::__ycxx::__detail::__ckd_mandates<__type1, __type2, __type3>());
  return __builtin_add_overflow(a, b, result);
}
template <class __type1, class __type2, class __type3>
inline bool ckd_sub(__type1* result, __type2 a, __type3 b) noexcept {
  static_assert(::__ycxx::__detail::__ckd_mandates<__type1, __type2, __type3>());
  return __builtin_sub_overflow(a, b, result);
}
template <class __type1, class __type2, class __type3>
inline bool ckd_mul(__type1* result, __type2 a, __type3 b) noexcept {
  static_assert(::__ycxx::__detail::__ckd_mandates<__type1, __type2, __type3>());
  return __builtin_mul_overflow(a, b, result);
}
