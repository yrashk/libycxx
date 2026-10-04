// libycxx -- internal configuration. Included (directly or indirectly) by every header.
//
// MACRO POLICY (see DECISIONS.md, "Macros"):
//   * This file is the ONLY place that inspects compiler, target, or language-mode macros
//     (__clang__, __GNUC__, __cpp_exceptions, __SIZEOF_INT128__, __has_builtin, ...).
//   * Each answer is converted ONCE into a `constexpr` constant in ycxx::detail::cfg, or into
//     a type alias / alias template in ycxx::detail. Library code consumes those through
//     `if constexpr`, `requires`, and ordinary templates.
//   * Library code defines no function-like macros and no attribute macros. The only macros
//     libycxx defines are include guards, macros the standard mandates (NULL, offsetof,
//     INT_MAX, __cpp_lib_*, assert, ...), and the YCXX_* switches in this file.
//   * `#if` outside this file is allowed only where code cannot be *parsed or declared* in
//     some configuration (a `throw` expression under -fno-exceptions, a declaration that needs
//     a builtin only one compiler has, a standard macro that must work inside `#if`). Such an
//     `#if` tests a YCXX_HAS_* / YCXX_* switch from this file -- never a compiler name.
#pragma once

#if !defined(__cplusplus) || __cplusplus <= 202302L
#  error "libycxx requires C++26 (-std=c++26 or -std=c++2c)"
#endif
#if !defined(__clang__) && !defined(__GNUC__)
#  error "libycxx supports only GCC and Clang"
#endif

// ---------------------------------------------------------------------------------------------
// User-settable switches (define on the command line).
// ---------------------------------------------------------------------------------------------
#ifndef YCXX_HARDENED // 1: check library preconditions at run time
#  define YCXX_HARDENED 0
#endif

// ---------------------------------------------------------------------------------------------
// Parse-level switches. Use with #if only where the code cannot be written otherwise.
//
// Function-style builtins are NOT detected here: a concept over a dependent call
// (`requires(T* p) { __builtin_foo(p); }`) is false when the builtin does not exist, so they are
// probed in-language (see ycxx::detail::builtin below). The YCXX_HAS_* switches for them exist
// only because the standard's __cpp_lib_* feature-test macros must be preprocessor-visible.
// Type-taking builtins (__is_integral(T), __builtin_type_order(T, U)) cannot be probed that way:
// an unknown one is a hard parse error.
// ---------------------------------------------------------------------------------------------
#if defined(__cpp_exceptions) && __cpp_exceptions
#  define YCXX_HAS_EXCEPTIONS 1
#else
#  define YCXX_HAS_EXCEPTIONS 0
#endif
#if __STDC_HOSTED__
#  define YCXX_HOSTED 1
#else
#  define YCXX_HOSTED 0
#endif
// FLT_ROUNDS (<cfloat>): the current rounding mode where the compiler can report it, otherwise
// 1 (to nearest), which is also what GCC's own <float.h> defines.
#if __has_builtin(__builtin_flt_rounds)
#  define YCXX_FLT_ROUNDS (__builtin_flt_rounds())
#else
#  define YCXX_FLT_ROUNDS 1
#endif
// Width of long long, for the <climits> macros (usable in #if): the compilers spell the
// predefined macro differently.
#if defined(__LLONG_WIDTH__)
#  define YCXX_LLONG_WIDTH __LLONG_WIDTH__
#else
#  define YCXX_LLONG_WIDTH __LONG_LONG_WIDTH__
#endif
// RTTI selects how the exception classes that the ABI runtime also defines are declared
// (exception_base.hpp): a non-template class cannot constrain its destructor.
#if defined(__cpp_rtti) || defined(__GXX_RTTI)
#  define YCXX_HAS_RTTI 1
#else
#  define YCXX_HAS_RTTI 0
#endif
// The exception classes the ABI runtime throws itself get out-of-line destructors (their key
// function) only in hosted builds without RTTI: that is where a vtable with no type_info could
// otherwise win the link against the runtime's (exception_base.hpp). The runtime's own
// src/abi/exception_classes.cpp defines those destructors, built with RTTI and with
// YCXX_EXCEPTION_KEY_FUNCTIONS. Freestanding builds have no ABI runtime, so they keep the inline
// constexpr destructors, which need none.
#if (!YCXX_HAS_RTTI && YCXX_HOSTED) || defined(YCXX_EXCEPTION_KEY_FUNCTIONS)
#  define YCXX_EXCEPTION_DTOR_OUT_OF_LINE 1
#else
#  define YCXX_EXCEPTION_DTOR_OUT_OF_LINE 0
#endif
#if __has_builtin(__builtin_is_within_lifetime)
#  define YCXX_HAS_IS_WITHIN_LIFETIME 1
#else
#  define YCXX_HAS_IS_WITHIN_LIFETIME 0
#endif
#if __has_builtin(__builtin_is_corresponding_member) && \
    __has_builtin(__builtin_is_pointer_interconvertible_with_class)
#  define YCXX_HAS_MEMBER_INTERCONVERTIBILITY 1
#else
#  define YCXX_HAS_MEMBER_INTERCONVERTIBILITY 0
#endif
#if __has_builtin(__builtin_type_order)
#  define YCXX_HAS_BUILTIN_TYPE_ORDER 1
#else
#  define YCXX_HAS_BUILTIN_TYPE_ORDER 0
#endif
// Clang's predefined int_fast16/32 types disagree with glibc on 64-bit Linux (glibc: long).
// The <cstdint> limit macros must be usable in #if, so this is a preprocessor switch.
#if defined(__clang__) && defined(__gnu_linux__) && __SIZEOF_POINTER__ == 8
#  define YCXX_FAST16_IS_LONG 1
#else
#  define YCXX_FAST16_IS_LONG 0
#endif

namespace ycxx::detail {

namespace cfg {
#if defined(__clang__)
inline constexpr bool clang = true;
#else
inline constexpr bool clang = false;
#endif
inline constexpr bool gcc = !clang;

inline constexpr bool exceptions = YCXX_HAS_EXCEPTIONS;
inline constexpr bool rtti = YCXX_HAS_RTTI;
inline constexpr bool hosted = YCXX_HOSTED;
inline constexpr bool hardened = YCXX_HARDENED;
#if defined(__SIZEOF_INT128__)
inline constexpr bool has_int128 = true;
#else
inline constexpr bool has_int128 = false;
#endif
// Integer division by zero raises a hardware trap (numeric_limits<int>::traps).
#if defined(__x86_64__) || defined(__i386__)
inline constexpr bool integer_division_traps = true;
#else
inline constexpr bool integer_division_traps = false;
#endif
inline constexpr unsigned pointer_bits = __SIZEOF_POINTER__ * __CHAR_BIT__;
inline constexpr unsigned long biggest_alignment = __BIGGEST_ALIGNMENT__;
inline constexpr unsigned long default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
} // namespace cfg

// In-language probes for function-style builtins (no preprocessor needed).
namespace builtin {
template <class T>
concept has_is_within_lifetime = requires(const T* p) { __builtin_is_within_lifetime(p); };
template <class S1, class S2, class M1, class M2>
concept has_is_corresponding_member =
    requires(M1 S1::* a, M2 S2::* b) { __builtin_is_corresponding_member(a, b); };
template <class S, class M>
concept has_is_pointer_interconvertible_with_class =
    requires(M S::* m) { __builtin_is_pointer_interconvertible_with_class(m); };
} // namespace builtin

// ---- types whose availability or spelling depends on the compiler ---------------------------
#if defined(__SIZEOF_INT128__)
using int128 = __int128;
using uint128 = unsigned __int128;
#else
struct int128_unavailable; // never a complete type; keeps generic code well-formed
using int128 = int128_unavailable;
using uint128 = int128_unavailable;
#endif

// Floating-point formats: the only per-type facts read from the compiler. Everything else in
// numeric_limits is derived from these three numbers in constexpr code (see <limits>).
struct fp_format_info {
  int digits;  // mantissa digits including the implicit bit (radix 2)
  int min_exp; // 1 + exponent of the smallest normal number
  int max_exp; // 1 + exponent of the largest finite number
};
template <class T>
inline constexpr fp_format_info fp_format{0, 0, 0};
template <>
inline constexpr fp_format_info fp_format<float>{__FLT_MANT_DIG__, __FLT_MIN_EXP__, __FLT_MAX_EXP__};
template <>
inline constexpr fp_format_info fp_format<double>{__DBL_MANT_DIG__, __DBL_MIN_EXP__, __DBL_MAX_EXP__};
template <>
inline constexpr fp_format_info fp_format<long double>{__LDBL_MANT_DIG__, __LDBL_MIN_EXP__, __LDBL_MAX_EXP__};

// Extended floating-point types ([basic.extended.fp]). Unavailable ones alias a distinct
// incomplete type, so generic code (type lists, overload sets) stays well-formed.
template <int>
struct fp_unavailable;
#if defined(__STDCPP_FLOAT16_T__)
using float16 = _Float16;
template <>
inline constexpr fp_format_info fp_format<float16>{__FLT16_MANT_DIG__, __FLT16_MIN_EXP__, __FLT16_MAX_EXP__};
#else
using float16 = fp_unavailable<16>;
#endif
#if defined(__STDCPP_FLOAT32_T__)
using float32 = _Float32;
template <>
inline constexpr fp_format_info fp_format<float32>{__FLT32_MANT_DIG__, __FLT32_MIN_EXP__, __FLT32_MAX_EXP__};
#else
using float32 = fp_unavailable<32>;
#endif
#if defined(__STDCPP_FLOAT64_T__)
using float64 = _Float64;
template <>
inline constexpr fp_format_info fp_format<float64>{__FLT64_MANT_DIG__, __FLT64_MIN_EXP__, __FLT64_MAX_EXP__};
#else
using float64 = fp_unavailable<64>;
#endif
#if defined(__STDCPP_FLOAT128_T__)
using float128 = _Float128;
template <>
inline constexpr fp_format_info fp_format<float128>{__FLT128_MANT_DIG__, __FLT128_MIN_EXP__, __FLT128_MAX_EXP__};
#else
using float128 = fp_unavailable<128>;
#endif
#if defined(__STDCPP_BFLOAT16_T__)
using bfloat16 = decltype(0.0bf16);
template <>
inline constexpr fp_format_info fp_format<bfloat16>{__BFLT16_MANT_DIG__, __BFLT16_MIN_EXP__, __BFLT16_MAX_EXP__};
#else
using bfloat16 = fp_unavailable<-16>;
#endif

// GNU __float128 (distinct from _Float128 in C++): numeric_limits supports it as an extension.
#if defined(__SIZEOF_FLOAT128__)
using gnu_float128 = __float128;
template <>
inline constexpr fp_format_info fp_format<gnu_float128>{113, -16381, 16384};
#else
using gnu_float128 = fp_unavailable<-128>;
#endif

// Bit-precise integers (_BitInt(N); a Clang extension in C++). bitint_info<T>::width is 0 for
// every other type, so library code tests `bitint_info<T>::width != 0` with no #if.
template <class T>
struct bitint_info {
  static constexpr int width = 0;
  static constexpr bool is_signed = false;
};
#if defined(__BITINT_MAXWIDTH__) && defined(__clang__)
template <unsigned N>
struct bitint_info<unsigned _BitInt(N)> {
  static constexpr int width = N;
  static constexpr bool is_signed = false;
};
template <unsigned N>
struct bitint_info<signed _BitInt(N)> {
  static constexpr int width = N;
  static constexpr bool is_signed = true;
};
#endif
template <class T>
inline constexpr int bitint_width = bitint_info<__remove_cv(T)>::width;

// remove_reference: the builtin is spelled differently.
#if defined(__clang__)
template <class T>
using remove_ref_t = __remove_reference_t(T);
#else
template <class T>
using remove_ref_t = __remove_reference(T);
#endif

// make_integer_seq<Seq, T, N> = Seq<T, 0, ..., N-1>: different builtins.
#if defined(__clang__)
template <template <class U, U...> class Seq, class T, T N>
using make_integer_seq = __make_integer_seq<Seq, T, N>;
#else
template <template <class U, U...> class Seq, class T, T N>
using make_integer_seq = Seq<T, __integer_pack(N)...>;
#endif

} // namespace ycxx::detail

