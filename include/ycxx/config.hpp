// libycxx -- internal configuration. Included (directly or indirectly) by every header.
//
// MACRO POLICY (see DECISIONS.md, "Macros"):
//   * This file is the ONLY place that inspects compiler, target, or language-mode macros
//     (__clang__, __GNUC__, __cpp_exceptions, __SIZEOF_INT128__, __has_builtin, ...).
//   * Each answer is converted ONCE into a `constexpr` constant in __ycxx::__detail::__cfg, or into
//     a type alias / alias template in __ycxx::__detail. Library code consumes those through
//     `if constexpr`, `requires`, and ordinary templates.
//   * Library code defines no function-like macros and no attribute macros. The only macros
//     libycxx defines are include guards, macros the standard mandates (NULL, offsetof,
//     INT_MAX, __cpp_lib_*, assert, ...), and the YCXX_* switches in this file.
//   * `#if` outside this file is allowed only where code cannot be *parsed or declared* in
//     some configuration (a `throw` expression under -fno-exceptions, a declaration that needs
//     a builtin only one compiler has, a standard macro that must work inside `#if`). Such an
//     `#if` tests a _YCXX_HAS_* / YCXX_* switch from this file -- never a compiler name.
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
// YCXX_NO_TRANSITIVE_INCLUDES (defined, with any value or none): each public header includes only
// what the draft and the implementation need, without the transitive includes programs commonly
// rely on (the block at the end of each header, DECISIONS §19). Tested only here, with defined().
#ifdef YCXX_NO_TRANSITIVE_INCLUDES
#  define _YCXX_TRANSITIVE_INCLUDES 0
#else
#  define _YCXX_TRANSITIVE_INCLUDES 1
#endif
// YCXX_SHARED (1: shared mode, 0: static mode; DECISIONS §20): whether this translation unit is
// built for libycxx's shared library or its static archives. The default is the installation's
// (<ycxx/generated/linkage.hpp>, written by the build when it has a shared library only), else
// static. Every image is built in one mode; the images of a process may differ.
#ifndef YCXX_SHARED
#  if __has_include(<ycxx/generated/linkage.hpp>)
#    include <ycxx/generated/linkage.hpp>
#  endif
#  ifdef _YCXX_DEFAULT_SHARED
#    define YCXX_SHARED _YCXX_DEFAULT_SHARED
#  else
#    define YCXX_SHARED 0
#  endif
#endif
// The visibility of every block of the library's namespaces (`namespace
// [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {`, DECISIONS §2,
// §20.3): hidden in static mode, exported from the shared library in shared mode. A macro because
// the attribute takes only a string literal (§1 rule 3's one exception; tools/check_preprocessor.py
// allows it only as that attribute's argument).
#if YCXX_SHARED
#  define _YCXX_VISIBILITY "default"
#else
#  define _YCXX_VISIBILITY "hidden"
#endif

// ---------------------------------------------------------------------------------------------
// Parse-level switches. Use with #if only where the code cannot be written otherwise.
//
// Function-style builtins are NOT detected here: a concept over a dependent call
// (`requires(_Tp* p) { __builtin_foo(p); }`) is false when the builtin does not exist, so they are
// probed in-language (see __ycxx::__detail::__y_builtin below). The _YCXX_HAS_* switches for them exist
// only because the standard's __cpp_lib_* feature-test macros must be preprocessor-visible.
// Type-taking builtins (__is_integral(T), __builtin_type_order(T, U)) cannot be probed that way:
// an unknown one is a hard parse error.
// ---------------------------------------------------------------------------------------------
#if defined(__cpp_exceptions) && __cpp_exceptions
#  define _YCXX_HAS_EXCEPTIONS 1
#else
#  define _YCXX_HAS_EXCEPTIONS 0
#endif
#if __STDC_HOSTED__
#  define _YCXX_HOSTED 1
#else
#  define _YCXX_HOSTED 0
#endif
// va_start with one argument (<cstdarg>): GCC has the C23 builtin in C++ as well, Clang 23 only
// in C.
#if __has_builtin(__builtin_c23_va_start)
#  define _YCXX_HAS_C23_VA_START 1
#else
#  define _YCXX_HAS_C23_VA_START 0
#endif
// FLT_ROUNDS (<cfloat>): the current rounding mode where the compiler can report it, otherwise
// 1 (to nearest), which is also what GCC's own <float.h> defines.
#if __has_builtin(__builtin_flt_rounds)
#  define _YCXX_FLT_ROUNDS (__builtin_flt_rounds())
#else
#  define _YCXX_FLT_ROUNDS 1
#endif
// Width of long long, for the <climits> macros (usable in #if): the compilers spell the
// predefined macro differently.
#if defined(__LLONG_WIDTH__)
#  define _YCXX_LLONG_WIDTH __LLONG_WIDTH__
#else
#  define _YCXX_LLONG_WIDTH __LONG_LONG_WIDTH__
#endif
// RTTI selects how the exception classes that the ABI runtime also defines are declared
// (exception_base.hpp): a non-template class cannot constrain its destructor.
#if defined(__cpp_rtti) || defined(__GXX_RTTI)
#  define _YCXX_HAS_RTTI 1
#else
#  define _YCXX_HAS_RTTI 0
#endif
// The exception classes the ABI runtime throws itself get out-of-line destructors (their key
// function) only in hosted builds without RTTI: that is where a vtable with no type_info could
// otherwise win the link against the runtime's (exception_base.hpp). The runtime's own
// src/abi/exception_classes.cpp defines those destructors, built with RTTI and with
// _YCXX_EXCEPTION_KEY_FUNCTIONS. Freestanding builds have no ABI runtime, so they keep the inline
// constexpr destructors, which need none.
#if (!_YCXX_HAS_RTTI && _YCXX_HOSTED) || defined(_YCXX_EXCEPTION_KEY_FUNCTIONS)
#  define _YCXX_EXCEPTION_DTOR_OUT_OF_LINE 1
#else
#  define _YCXX_EXCEPTION_DTOR_OUT_OF_LINE 0
#endif
#if __has_builtin(__builtin_is_within_lifetime)
#  define _YCXX_HAS_IS_WITHIN_LIFETIME 1
#else
#  define _YCXX_HAS_IS_WITHIN_LIFETIME 0
#endif
#if __has_builtin(__builtin_is_corresponding_member) && \
    __has_builtin(__builtin_is_pointer_interconvertible_with_class)
#  define _YCXX_HAS_MEMBER_INTERCONVERTIBILITY 1
#else
#  define _YCXX_HAS_MEMBER_INTERCONVERTIBILITY 0
#endif
// Non-null exception_ptrs in constant evaluation (P3068), GCC 16: __builtin_current_exception()
// makes a std::exception_ptr (whose one data member is the object pointer) for the exception being
// handled, __builtin_eh_ptr_adjust_ref(p, n) adds n to such an object's reference count; both
// work during constant evaluation only. A no-argument builtin cannot be probed in-language
// (§1 rule 4), so exception_ptr.hpp spells it under this switch.
#if __has_builtin(__builtin_current_exception) && __has_builtin(__builtin_eh_ptr_adjust_ref)
#  define _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR 1
#else
#  define _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR 0
#endif
// Throwing and catching during constant evaluation (P3068's core-language part,
// __cpp_constexpr_exceptions): GCC 16 can, Clang 23 cannot (a throw there ends the constant
// evaluation). Together with _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR it decides
// __cpp_lib_constexpr_exceptions (version.hpp), which must be usable in #if.
#if defined(__cpp_constexpr_exceptions) && defined(__cpp_exceptions)
#  define _YCXX_HAS_CONSTEXPR_EXCEPTIONS 1
#else
#  define _YCXX_HAS_CONSTEXPR_EXCEPTIONS 0
#endif
// Contract assertions (P2900): __cpp_lib_contracts is defined only where the compiler has them.
#if defined(__cpp_contracts)
#  define _YCXX_HAS_CONTRACTS 1
#else
#  define _YCXX_HAS_CONTRACTS 0
#endif
// Reflection (P2996, GCC 16 with -freflection): <meta> cannot even be parsed without it
// (`^^`), so its declarations are under this switch, as is __cpp_lib_reflection.
#if defined(__cpp_impl_reflection)
#  define _YCXX_HAS_REFLECTION 1
#else
#  define _YCXX_HAS_REFLECTION 0
#endif
// std::is_structural: a type-taking builtin (GCC 16; not Clang 23).
#if __has_builtin(__builtin_is_structural)
#  define _YCXX_HAS_IS_STRUCTURAL 1
#else
#  define _YCXX_HAS_IS_STRUCTURAL 0
#endif
#if __has_builtin(__builtin_type_order)
#  define _YCXX_HAS_BUILTIN_TYPE_ORDER 1
#else
#  define _YCXX_HAS_BUILTIN_TYPE_ORDER 0
#endif
// <stdfloat> declares each std::floatN_t alias only where the type exists ([stdfloat.syn]): an
// alias cannot be declared conditionally without the preprocessor.
#if defined(__STDCPP_FLOAT16_T__)
#  define _YCXX_HAS_FLOAT16_T 1
#else
#  define _YCXX_HAS_FLOAT16_T 0
#endif
#if defined(__STDCPP_FLOAT32_T__)
#  define _YCXX_HAS_FLOAT32_T 1
#else
#  define _YCXX_HAS_FLOAT32_T 0
#endif
#if defined(__STDCPP_FLOAT64_T__)
#  define _YCXX_HAS_FLOAT64_T 1
#else
#  define _YCXX_HAS_FLOAT64_T 0
#endif
#if defined(__STDCPP_FLOAT128_T__)
#  define _YCXX_HAS_FLOAT128_T 1
#else
#  define _YCXX_HAS_FLOAT128_T 0
#endif
#if defined(__STDCPP_BFLOAT16_T__)
#  define _YCXX_HAS_BFLOAT16_T 1
#else
#  define _YCXX_HAS_BFLOAT16_T 0
#endif
// <atomic>'s ATOMIC_*_LOCK_FREE macros must be usable in #if: the compiler's answers, which match
// atomic<T>::is_always_lock_free (ycxx/core/atomic_base.hpp asks __atomic_always_lock_free).
#define _YCXX_ATOMIC_BOOL_LOCK_FREE __GCC_ATOMIC_BOOL_LOCK_FREE
#define _YCXX_ATOMIC_CHAR_LOCK_FREE __GCC_ATOMIC_CHAR_LOCK_FREE
#define _YCXX_ATOMIC_CHAR8_T_LOCK_FREE __GCC_ATOMIC_CHAR8_T_LOCK_FREE
#define _YCXX_ATOMIC_CHAR16_T_LOCK_FREE __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define _YCXX_ATOMIC_CHAR32_T_LOCK_FREE __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define _YCXX_ATOMIC_WCHAR_T_LOCK_FREE __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define _YCXX_ATOMIC_SHORT_LOCK_FREE __GCC_ATOMIC_SHORT_LOCK_FREE
#define _YCXX_ATOMIC_INT_LOCK_FREE __GCC_ATOMIC_INT_LOCK_FREE
#define _YCXX_ATOMIC_LONG_LOCK_FREE __GCC_ATOMIC_LONG_LOCK_FREE
#define _YCXX_ATOMIC_LLONG_LOCK_FREE __GCC_ATOMIC_LLONG_LOCK_FREE
#define _YCXX_ATOMIC_POINTER_LOCK_FREE __GCC_ATOMIC_POINTER_LOCK_FREE
// The target's C library family: Darwin (Apple's libSystem, BSD heritage) or the Linux C
// libraries (glibc, musl), which are also the default for bare-metal targets. Core spells out
// some of the C library's values without its headers: the errno numbers (std::errc and the
// freestanding <cerrno>), the FP_* classification macros, mbstate_t's layout. Those macros must
// be usable in #if, so the freestanding headers that define them test this switch; everything
// else uses cfg::darwin. Each value is checked against the C library's headers when libycxx is
// built or a hosted header is used (<system_error>, <cwchar>, src/hosted/cmath_check.cpp).
#if defined(__APPLE__)
#  define _YCXX_TARGET_DARWIN 1
#else
#  define _YCXX_TARGET_DARWIN 0
#endif
// What the C library provides, where the preprocessor cannot see it: <ycxx/generated/c_library.hpp>,
// generated in the build tree by cmake/ycxx-c-library.cmake from probes compiled against the C
// library. _YCXX_C_HAS_STRFROM: <stdlib.h> has C23's strfromd/f/l; _YCXX_C_HAS_MBRTOC8: <uchar.h>
// has mbrtoc8/c8rtomb; _YCXX_C_HAS_TIMESPEC_GETRES: <time.h> has timespec_getres. Where one is 0
// the wrapper declares libycxx's own (src/hosted/strfrom.cpp, uchar.cpp, ctime.cpp). Without the
// file (the headers used without a configured build) a C23 C library is assumed.
#if __has_include(<ycxx/generated/c_library.hpp>)
#  include <ycxx/generated/c_library.hpp>
#endif
#ifndef _YCXX_C_HAS_STRFROM
#  define _YCXX_C_HAS_STRFROM 1
#endif
#ifndef _YCXX_C_HAS_MBRTOC8
#  define _YCXX_C_HAS_MBRTOC8 1
#endif
#ifndef _YCXX_C_HAS_TIMESPEC_GETRES
#  define _YCXX_C_HAS_TIMESPEC_GETRES 1
#endif
// _YCXX_C_HAS_ERA_NUM_ENTRIES (used by src/hosted/locale_named.cpp only): <langinfo.h> has glibc's
// _NL_TIME_ERA_NUM_ENTRIES; without the file, POSIX's ERA string is assumed.
#ifndef _YCXX_C_HAS_ERA_NUM_ENTRIES
#  define _YCXX_C_HAS_ERA_NUM_ENTRIES 0
#endif
// The hosted layers (DECISIONS §18): <ycxx/generated/hosted_layers.hpp>, generated by CMake in
// YCXX_PAL=none builds, says which layers the integrator's providers supply (_YCXX_LAYER_<name>
// 1 or 0). Without it (YCXX_PAL=posix, or the headers used without a configured build) a hosted
// build has every layer and a freestanding one none.
#if __has_include(<ycxx/generated/hosted_layers.hpp>)
#  include <ycxx/generated/hosted_layers.hpp>
// The C library layer decides how libycxx was compiled: hosted with it, freestanding without
// (__ycxx::__headers adds -ffreestanding then). A program must be compiled the same way.
#  if _YCXX_LAYER_CLIB && !_YCXX_HOSTED
#    error "libycxx was configured with the 'clib' hosted layer (the C library), but this translation unit is compiled freestanding"
#  elif !_YCXX_LAYER_CLIB && _YCXX_HOSTED
#    error "libycxx was configured without the 'clib' hosted layer: compile freestanding (-ffreestanding -nostdinc, as ycxx::headers does)"
#  endif
#endif
#ifndef _YCXX_LAYER_ABORT
#  define _YCXX_LAYER_ABORT _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_MEMORY
#  define _YCXX_LAYER_MEMORY _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_CONSOLE
#  define _YCXX_LAYER_CONSOLE _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_CLOCK
#  define _YCXX_LAYER_CLOCK _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_THREADS
#  define _YCXX_LAYER_THREADS _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_RANDOM
#  define _YCXX_LAYER_RANDOM _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_FILES
#  define _YCXX_LAYER_FILES _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_ENVIRONMENT
#  define _YCXX_LAYER_ENVIRONMENT _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_DEBUG
#  define _YCXX_LAYER_DEBUG _YCXX_HOSTED
#endif
#ifndef _YCXX_LAYER_FILESYSTEM
#  define _YCXX_LAYER_FILESYSTEM _YCXX_HOSTED
#endif
// <cuchar>: whether the C library has <uchar.h> (libycxx's own <uchar.h> is skipped: the search
// continues after libycxx's include directory, which holds this file's directory). Where it is
// missing, <cuchar> declares libycxx's own functions (src/hosted/uchar.cpp).
#if __has_include_next(<uchar.h>)
#  define _YCXX_C_HAS_UCHAR_H 1
#else
#  define _YCXX_C_HAS_UCHAR_H 0
#endif
// Initialization priorities (init_priority) order static initializers across object files only
// in ELF (.init_array.NNNNN sections, sorted by the linker). Mach-O has one __mod_init_func list
// in link order (Clang orders priorities within one object file; GCC rejects the attribute), so
// there the runtime cannot run before the program's objects, and <iostream> defines an
// ios_base::Init object in each translation unit instead (DECISIONS §7).
#if defined(__ELF__)
#  define _YCXX_HAS_INIT_PRIORITY 1
#else
#  define _YCXX_HAS_INIT_PRIORITY 0
#endif
// <cmath> macros that depend on the target and the options; they must be usable in #if.
// FP_FAST_FMA* are defined where fma is as fast as a multiply and an add.
#if defined(__FP_FAST_FMA)
#  define _YCXX_FP_FAST_FMA 1
#else
#  define _YCXX_FP_FAST_FMA 0
#endif
#if defined(__FP_FAST_FMAF)
#  define _YCXX_FP_FAST_FMAF 1
#else
#  define _YCXX_FP_FAST_FMAF 0
#endif
#if defined(__FP_FAST_FMAL)
#  define _YCXX_FP_FAST_FMAL 1
#else
#  define _YCXX_FP_FAST_FMAL 0
#endif
// math_errhandling: glibc and musl set errno unless the program is built with -fno-math-errno.
// Darwin's libm never sets errno: it reports errors through the floating-point exception flags
// only (its <math.h> defines math_errhandling as a run-time call, __math_errhandling(), which is
// not a constant expression; Clang defaults to -fno-math-errno there). So on Darwin libycxx's
// value is MATH_ERREXCEPT whatever the options: -fmath-errno cannot make that libm set errno.
#if defined(__NO_MATH_ERRNO__) || defined(__APPLE__)
#  define _YCXX_MATH_ERRNO 0
#else
#  define _YCXX_MATH_ERRNO 1
#endif
// FP_ILOGB0: glibc on AArch64 returns -INT_MAX for zero; the other supported targets use
// INT_MIN. Both values are permitted by C ([library.c]); cmath_check.cpp checks the C header.
#if defined(__aarch64__) && defined(__gnu_linux__)
#  define _YCXX_FP_ILOGB0 (-2147483647)
#else
#  define _YCXX_FP_ILOGB0 (-2147483647 - 1)
#endif
// FP_ILOGBNAN: what the C library's ilogb returns for a NaN (glibc: INT_MIN on x86, INT_MAX
// elsewhere; Darwin: INT_MIN on every architecture). src/hosted/cmath_check.cpp verifies it
// against <math.h>.
#if defined(__x86_64__) || defined(__i386__) || defined(__APPLE__)
#  define _YCXX_FP_ILOGBNAN (-2147483647 - 1)
#else
#  define _YCXX_FP_ILOGBNAN 2147483647
#endif
// Clang's predefined int_fast16/32 types disagree with glibc on 64-bit Linux (glibc: long).
// The <cstdint> limit macros must be usable in #if, so this is a preprocessor switch. Freestanding
// (no C library; DECISIONS §18) the compiler's own <stdint.h> is the C library's header, and it
// agrees with the predefined types (Clang's <unwind.h> includes it).
#if defined(__clang__) && defined(__gnu_linux__) && __SIZEOF_POINTER__ == 8 && __STDC_HOSTED__
#  define _YCXX_FAST16_IS_LONG 1
#else
#  define _YCXX_FAST16_IS_LONG 0
#endif

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

namespace __cfg {
#if defined(__clang__)
inline constexpr bool __y_clang = true;
#else
inline constexpr bool __y_clang = false;
#endif
inline constexpr bool __gcc = !__y_clang;

// The target's C library family (see _YCXX_TARGET_DARWIN): Darwin's libSystem, or else the Linux
// C libraries' conventions.
inline constexpr bool __darwin = _YCXX_TARGET_DARWIN;
// mbstate_t's layout in the C library ([cwchar.syn]; core defines std::mbstate_t without it):
// glibc and musl 8 bytes aligned to 4; Darwin a union of char[128] and long long.
inline constexpr unsigned long __mbstate_size = __darwin ? 128 : 8;
inline constexpr unsigned long __mbstate_align = __darwin ? 8 : 4;
// Whether the C library's libm has the binary128 functions of ISO/IEC 9899:2024 Annex H
// (acosf128, ..., which <cmath> calls through __builtin_*f128 for std::float128_t when no
// standard type has its format): glibc does (2.26 and later); Darwin's libm does not, so there
// libycxx's own implementations compute float128_t at run time too (ycxx/core/cmath_impl.hpp).
inline constexpr bool __c_math_float128 = !__darwin;
// Whether init_priority orders static initialization across object files (_YCXX_HAS_INIT_PRIORITY).
inline constexpr bool __init_priority = _YCXX_HAS_INIT_PRIORITY;
// Clang's Apple arm64 C++ ABI marks a type_info whose object may be duplicated across linked
// images (vague linkage, default visibility) by setting bit 63 of its name pointer; such type_infos
// compare by name. The bit is cleared before the name is read (std::type_info, the ABI runtime).
// Clearing it is harmless where it is never set (GCC), so this holds for every Apple arm64 target.
#if defined(__APPLE__) && defined(__aarch64__)
inline constexpr bool __rtti_non_unique_bit = true;
#else
inline constexpr bool __rtti_non_unique_bit = false;
#endif

inline constexpr bool exceptions = _YCXX_HAS_EXCEPTIONS;
inline constexpr bool __rtti = _YCXX_HAS_RTTI;
// Whether exception_ptr can hold an exception during constant evaluation (_YCXX_HAS_CONSTEXPR_EXCEPTION_PTR).
inline constexpr bool __constexpr_exception_ptr = _YCXX_HAS_CONSTEXPR_EXCEPTION_PTR && _YCXX_HAS_EXCEPTIONS;
// Whether a constant evaluation can throw and catch exceptions (_YCXX_HAS_CONSTEXPR_EXCEPTIONS).
inline constexpr bool __constexpr_exceptions = _YCXX_HAS_CONSTEXPR_EXCEPTIONS;
inline constexpr bool __hosted = _YCXX_HOSTED;
// The hosted layers whose primitives the program has (DECISIONS §18). The C library ('clib') is
// `__hosted`.
namespace __layer {
inline constexpr bool abort = _YCXX_LAYER_ABORT;
inline constexpr bool __memory = _YCXX_LAYER_MEMORY;
inline constexpr bool __console = _YCXX_LAYER_CONSOLE;
inline constexpr bool clock = _YCXX_LAYER_CLOCK;
inline constexpr bool __threads = _YCXX_LAYER_THREADS;
inline constexpr bool __random = _YCXX_LAYER_RANDOM;
inline constexpr bool __files = _YCXX_LAYER_FILES;
inline constexpr bool environment = _YCXX_LAYER_ENVIRONMENT;
inline constexpr bool __debug = _YCXX_LAYER_DEBUG;
inline constexpr bool filesystem = _YCXX_LAYER_FILESYSTEM;
} // namespace layer
inline constexpr bool __hardened = YCXX_HARDENED;
// Shared mode (YCXX_SHARED, DECISIONS §20).
inline constexpr bool __shared = YCXX_SHARED;
inline constexpr bool __reflection = _YCXX_HAS_REFLECTION;
#if defined(__SIZEOF_INT128__)
inline constexpr bool __has_int128 = true;
#else
inline constexpr bool __has_int128 = false;
#endif
// Integer division by zero raises a hardware trap (numeric_limits<int>::traps).
#if defined(__x86_64__) || defined(__i386__)
inline constexpr bool __integer_division_traps = true;
#else
inline constexpr bool __integer_division_traps = false;
#endif
// Floating-point tininess is detected before rounding (numeric_limits<T>::tinyness_before): Arm's
// floating point (AArch64, AArch32, and their software formats) does; x86 and the others detect it
// after rounding.
#if defined(__aarch64__) || defined(__arm__)
inline constexpr bool __tinyness_before_rounding = true;
#else
inline constexpr bool __tinyness_before_rounding = false;
#endif
// The processor family, for the few run-time functions that need an instruction of their own
// (std::breakpoint).
enum class __cpu_family { __x86, __aarch64, __y_arm, __y_riscv, other };
#if defined(__x86_64__) || defined(__i386__)
inline constexpr __cpu_family __cpu = __cpu_family::__x86;
#elif defined(__aarch64__)
inline constexpr __cpu_family __cpu = __cpu_family::__aarch64;
#elif defined(__arm__)
inline constexpr __cpu_family __cpu = __cpu_family::__y_arm;
#elif defined(__riscv)
inline constexpr __cpu_family __cpu = __cpu_family::__y_riscv;
#else
inline constexpr __cpu_family __cpu = __cpu_family::other;
#endif
// The name of the ordinary literal encoding (std::text_encoding::literal()); empty if the
// compiler does not say.
#if defined(__clang_literal_encoding__)
inline constexpr char __literal_encoding[] = __clang_literal_encoding__;
#elif defined(__GNUC_EXECUTION_CHARSET_NAME)
inline constexpr char __literal_encoding[] = __GNUC_EXECUTION_CHARSET_NAME;
#else
inline constexpr char __literal_encoding[] = "";
#endif
// The widest vector register <simd>'s native ABI uses, in bytes: 64 with AVX-512, 32 with AVX,
// else 16 (SSE2, NEON, and the generic lowering of other targets).
#if defined(__AVX512F__)
inline constexpr int __simd_register_bytes = 64;
#elif defined(__AVX__)
inline constexpr int __simd_register_bytes = 32;
#else
inline constexpr int __simd_register_bytes = 16;
#endif
// FLT_EVAL_METHOD (<cfloat>), which selects float_t and double_t (<cmath>).
inline constexpr int __flt_eval_method = __FLT_EVAL_METHOD__;
inline constexpr unsigned __pointer_bits = __SIZEOF_POINTER__ * __CHAR_BIT__;
inline constexpr unsigned long __biggest_alignment = __BIGGEST_ALIGNMENT__;
inline constexpr unsigned long __default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
} // namespace cfg

// In-language probes for function-style builtins (no preprocessor needed).
namespace __y_builtin {
template <class _Tp>
concept __has_is_within_lifetime = requires(const _Tp* p) { __builtin_is_within_lifetime(p); };
// Clang's __builtin_is_aligned: also usable during constant evaluation (pointer_tag_pair's
// from_overaligned checks its promise with it there).
template <class _Tp>
concept __has_is_aligned = requires(const _Tp* p) { __builtin_is_aligned(p, 1); };
template <class _S1, class _S2, class _M1, class _M2>
concept __has_is_corresponding_member =
    requires(_M1 _S1::* a, _M2 _S2::* b) { __builtin_is_corresponding_member(a, b); };
template <class _Sp, class _Mp>
concept __has_is_pointer_interconvertible_with_class =
    requires(_Mp _Sp::* m) { __builtin_is_pointer_interconvertible_with_class(m); };
} // namespace builtin

// ---- types whose availability or spelling depends on the compiler ---------------------------
#if defined(__SIZEOF_INT128__)
using __y_int128 = __int128;
using __uint128 = unsigned __int128;
#else
struct __int128_unavailable; // never a complete type; keeps generic code well-formed
using __y_int128 = __int128_unavailable;
using __uint128 = __int128_unavailable;
#endif

// Floating-point formats: the only per-type facts read from the compiler. Everything else in
// numeric_limits is derived from these three numbers in constexpr code (see <limits>).
struct __fp_format_info {
  int digits;  // mantissa digits including the implicit bit (radix 2)
  int __min_exp; // 1 + exponent of the smallest normal number
  int __max_exp; // 1 + exponent of the largest finite number
};
template <class _Tp>
inline constexpr __fp_format_info __fp_format{0, 0, 0};
template <>
inline constexpr __fp_format_info __fp_format<float>{__FLT_MANT_DIG__, __FLT_MIN_EXP__, __FLT_MAX_EXP__};
template <>
inline constexpr __fp_format_info __fp_format<double>{__DBL_MANT_DIG__, __DBL_MIN_EXP__, __DBL_MAX_EXP__};
template <>
inline constexpr __fp_format_info __fp_format<long double>{__LDBL_MANT_DIG__, __LDBL_MIN_EXP__, __LDBL_MAX_EXP__};

// std::meta::info, the reflection type ([basic.fundamental]); without reflection support a
// distinct incomplete type, so is_reflection is false for every type.
#if defined(__cpp_impl_reflection)
using __reflection = decltype(^^::);
#else
struct __reflection_unavailable;
using __reflection = __reflection_unavailable;
#endif

// Extended floating-point types ([basic.extended.fp]). Unavailable ones alias a distinct
// incomplete type, so generic code (type lists, overload sets) stays well-formed.
template <int>
struct __fp_unavailable;
#if defined(__STDCPP_FLOAT16_T__)
using __float16 = _Float16;
template <>
inline constexpr __fp_format_info __fp_format<__float16>{__FLT16_MANT_DIG__, __FLT16_MIN_EXP__, __FLT16_MAX_EXP__};
#else
using __float16 = __fp_unavailable<16>;
#endif
#if defined(__STDCPP_FLOAT32_T__)
using __float32 = _Float32;
template <>
inline constexpr __fp_format_info __fp_format<__float32>{__FLT32_MANT_DIG__, __FLT32_MIN_EXP__, __FLT32_MAX_EXP__};
#else
using __float32 = __fp_unavailable<32>;
#endif
#if defined(__STDCPP_FLOAT64_T__)
using __float64 = _Float64;
template <>
inline constexpr __fp_format_info __fp_format<__float64>{__FLT64_MANT_DIG__, __FLT64_MIN_EXP__, __FLT64_MAX_EXP__};
#else
using __float64 = __fp_unavailable<64>;
#endif
#if defined(__STDCPP_FLOAT128_T__)
using __y_float128 = _Float128;
template <>
inline constexpr __fp_format_info __fp_format<__y_float128>{__FLT128_MANT_DIG__, __FLT128_MIN_EXP__, __FLT128_MAX_EXP__};
#else
using __y_float128 = __fp_unavailable<128>;
#endif
#if defined(__STDCPP_BFLOAT16_T__)
using __bfloat16 = decltype(0.0bf16);
template <>
inline constexpr __fp_format_info __fp_format<__bfloat16>{__BFLT16_MANT_DIG__, __BFLT16_MIN_EXP__, __BFLT16_MAX_EXP__};
#else
using __bfloat16 = __fp_unavailable<-16>;
#endif

// GNU __float128 (distinct from _Float128 in C++): numeric_limits supports it as an extension.
#if defined(__SIZEOF_FLOAT128__)
using __gnu_float128 = __float128;
template <>
inline constexpr __fp_format_info __fp_format<__gnu_float128>{113, -16381, 16384};
#else
using __gnu_float128 = __fp_unavailable<-128>;
#endif

// Bit-precise integers (_BitInt(N); a Clang extension in C++). bitint_info<T>::width is 0 for
// every other type, so library code tests `__bitint_info<_Tp>::width != 0` with no #if.
template <class _Tp>
struct __bitint_info {
  static constexpr int width = 0;
  static constexpr bool is_signed = false;
};
#if defined(__BITINT_MAXWIDTH__) && defined(__clang__)
template <unsigned _Np>
struct __bitint_info<unsigned _BitInt(_Np)> {
  static constexpr int width = _Np;
  static constexpr bool is_signed = false;
};
template <unsigned _Np>
struct __bitint_info<signed _BitInt(_Np)> {
  static constexpr int width = _Np;
  static constexpr bool is_signed = true;
};
#endif
template <class _Tp>
inline constexpr int __bitint_width = __bitint_info<__remove_cv(_Tp)>::width;

// remove_reference: the builtin is spelled differently.
#if defined(__clang__)
template <class _Tp>
using __remove_ref_t = __remove_reference_t(_Tp);
#else
template <class _Tp>
using __remove_ref_t = __remove_reference(_Tp);
#endif

// make_integer_seq<Seq, T, N> = Seq<T, 0, ..., N-1>: different builtins.
#if defined(__clang__)
template <template <class _Up, _Up...> class _Seq, class _Tp, _Tp _Np>
using __y_make_integer_seq = __make_integer_seq<_Seq, _Tp, _Np>;
#else
template <template <class _Up, _Up...> class _Seq, class _Tp, _Tp _Np>
using __y_make_integer_seq = _Seq<_Tp, __integer_pack(_Np)...>;
#endif

}} // namespace __ycxx::__detail

// The link-time mode guard (DECISIONS §20.2): every translation unit refers to the marker of its
// mode, which only that mode's library defines (src/runtime/linkage: the static archives define
// __ycxx_linkage_static_v1, hidden; libycxx.so exports __ycxx_linkage_shared_v1). A translation
// unit compiled in one mode and linked in the other fails to link, naming the missing marker. One
// relocation per translation unit.
extern "C" {
[[__gnu__::__visibility__("hidden")]] extern const char __ycxx_linkage_static_v1;
[[__gnu__::__visibility__("default")]] extern const char __ycxx_linkage_shared_v1;
}
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail { namespace {
[[__gnu__::__used__]] constexpr const char* __linkage_marker =
    __cfg::__shared ? &__ycxx_linkage_shared_v1 : &__ycxx_linkage_static_v1;
}}} // namespace __ycxx::__detail::(anonymous)
