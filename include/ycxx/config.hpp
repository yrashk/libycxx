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
// ---------------------------------------------------------------------------------------------
#if defined(__cpp_exceptions) && __cpp_exceptions
#  define YCXX_HAS_EXCEPTIONS 1
#else
#  define YCXX_HAS_EXCEPTIONS 0
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
#if defined(__cpp_rtti) || defined(__GXX_RTTI)
inline constexpr bool rtti = true;
#else
inline constexpr bool rtti = false;
#endif
inline constexpr bool hosted = __STDC_HOSTED__;
inline constexpr bool hardened = YCXX_HARDENED;
#if defined(__SIZEOF_INT128__)
inline constexpr bool has_int128 = true;
#else
inline constexpr bool has_int128 = false;
#endif
inline constexpr unsigned pointer_bits = __SIZEOF_POINTER__ * __CHAR_BIT__;
inline constexpr unsigned long biggest_alignment = __BIGGEST_ALIGNMENT__;
inline constexpr unsigned long default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
} // namespace cfg

// ---- types whose availability or spelling depends on the compiler ---------------------------
#if defined(__SIZEOF_INT128__)
using int128 = __int128;
using uint128 = unsigned __int128;
#else
struct int128_unavailable; // never a complete type; keeps generic code well-formed
using int128 = int128_unavailable;
using uint128 = int128_unavailable;
#endif

// Extended floating-point types ([basic.extended.fp]). Unavailable ones alias a distinct
// incomplete type, so generic code (type lists, overload sets) stays well-formed.
template <int>
struct fp_unavailable;
#if defined(__STDCPP_FLOAT16_T__)
using float16 = _Float16;
#else
using float16 = fp_unavailable<16>;
#endif
#if defined(__STDCPP_FLOAT32_T__)
using float32 = _Float32;
#else
using float32 = fp_unavailable<32>;
#endif
#if defined(__STDCPP_FLOAT64_T__)
using float64 = _Float64;
#else
using float64 = fp_unavailable<64>;
#endif
#if defined(__STDCPP_FLOAT128_T__)
using float128 = _Float128;
#else
using float128 = fp_unavailable<128>;
#endif
#if defined(__STDCPP_BFLOAT16_T__)
using bfloat16 = decltype(0.0bf16);
#else
using bfloat16 = fp_unavailable<-16>;
#endif

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

