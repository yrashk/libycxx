// libycxx -- internal configuration. Included (directly or indirectly) by every header.
//
// This is the ONLY place where differences between GCC and Clang are allowed to show up.
// Everything else uses the YCXX_* macros defined here.
#ifndef YCXX_CONFIG_HPP
#define YCXX_CONFIG_HPP

#if !defined(__cplusplus) || __cplusplus <= 202302L
#  error "libycxx requires C++26 (-std=c++26 or -std=c++2c)"
#endif

#if defined(__clang__)
#  define YCXX_COMPILER_CLANG 1
#  define YCXX_COMPILER_GCC 0
#elif defined(__GNUC__)
#  define YCXX_COMPILER_CLANG 0
#  define YCXX_COMPILER_GCC 1
#else
#  error "libycxx supports only GCC and Clang"
#endif

#define YCXX_VERSION 1

// ---------------------------------------------------------------------------------------------
// Environment
// ---------------------------------------------------------------------------------------------
#if defined(__cpp_exceptions) && __cpp_exceptions
#  define YCXX_HAS_EXCEPTIONS 1
#else
#  define YCXX_HAS_EXCEPTIONS 0
#endif

#if defined(__cpp_rtti) || defined(__GXX_RTTI)
#  define YCXX_HAS_RTTI 1
#else
#  define YCXX_HAS_RTTI 0
#endif

#if __STDC_HOSTED__
#  define YCXX_HOSTED 1
#else
#  define YCXX_HOSTED 0
#endif

#if defined(__SIZEOF_INT128__)
#  define YCXX_HAS_INT128 1
#else
#  define YCXX_HAS_INT128 0
#endif

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8) || defined(__x86_64__) || defined(__aarch64__) || \
    (defined(__riscv) && __riscv_xlen == 64)
#  define YCXX_HAS_64BIT_ATOMICS 1
#else
#  define YCXX_HAS_64BIT_ATOMICS 0
#endif

// ---------------------------------------------------------------------------------------------
// Attributes
// ---------------------------------------------------------------------------------------------
#define YCXX_ALWAYS_INLINE [[gnu::always_inline]] inline
#define YCXX_NOINLINE [[gnu::noinline]]
#define YCXX_COLD [[gnu::cold]]
#define YCXX_WEAK [[gnu::weak]]
#define YCXX_PURE [[gnu::pure]]
#if YCXX_COMPILER_CLANG
#  define YCXX_NODEBUG [[gnu::nodebug]]
#  define YCXX_LIFETIMEBOUND [[clang::lifetimebound]]
#  define YCXX_TRIVIAL_ABI [[clang::trivial_abi]]
#  define YCXX_PREFERRED_NAME(x) [[clang::preferred_name(x)]]
#else
#  define YCXX_NODEBUG
#  define YCXX_LIFETIMEBOUND
#  define YCXX_TRIVIAL_ABI
#  define YCXX_PREFERRED_NAME(x)
#endif
#define YCXX_NO_UNIQUE_ADDRESS [[no_unique_address]]

// Marks a function as an intrinsic-like helper: no debug info stepping, always inline.
#define YCXX_INTRINSIC YCXX_NODEBUG YCXX_ALWAYS_INLINE

// ---------------------------------------------------------------------------------------------
// Builtins whose spelling differs between compilers.
// ---------------------------------------------------------------------------------------------

// YCXX_MAKE_INTEGER_SEQ(Seq, T, N): Seq<T, 0, 1, ..., N-1>
#if YCXX_COMPILER_CLANG
#  define YCXX_MAKE_INTEGER_SEQ(Seq, T, N) __make_integer_seq<Seq, T, N>
#else
#  define YCXX_MAKE_INTEGER_SEQ(Seq, T, N) Seq<T, __integer_pack(N)...>
#endif

// Membership traits for which only one compiler has a builtin.
#if __has_builtin(__builtin_is_corresponding_member)
#  define YCXX_HAS_IS_CORRESPONDING_MEMBER 1
#else
#  define YCXX_HAS_IS_CORRESPONDING_MEMBER 0
#endif
#if __has_builtin(__builtin_is_pointer_interconvertible_with_class)
#  define YCXX_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS 1
#else
#  define YCXX_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS 0
#endif

#define YCXX_ASSUME(...) __builtin_assume(__VA_ARGS__)
#if YCXX_COMPILER_GCC
#  undef YCXX_ASSUME
#  define YCXX_ASSUME(...) [[assume(__VA_ARGS__)]]
#endif

#define YCXX_LIKELY(...) __builtin_expect(static_cast<bool>(__VA_ARGS__), 1)
#define YCXX_UNLIKELY(...) __builtin_expect(static_cast<bool>(__VA_ARGS__), 0)

// ---------------------------------------------------------------------------------------------
// Hardening / internal assertions.
//
// YCXX_ASSERT(cond, msg) checks a library precondition. Disabled unless YCXX_HARDENED is
// defined to a non-zero value. On failure calls the verbose abort hook.
// ---------------------------------------------------------------------------------------------
#ifndef YCXX_HARDENED
#  define YCXX_HARDENED 0
#endif

namespace ycxx::detail {
[[noreturn]] void assertion_failed(const char* msg) noexcept;
} // namespace ycxx::detail

#if YCXX_HARDENED
#  define YCXX_ASSERT(cond, msg)                                                               \
    (YCXX_LIKELY(cond) ? (void)0 : ::ycxx::detail::assertion_failed(__FILE__ ":" YCXX_STR(__LINE__) ": " msg))
#else
#  define YCXX_ASSERT(cond, msg) ((void)0)
#endif

#define YCXX_STR2(x) #x
#define YCXX_STR(x) YCXX_STR2(x)

// Diagnostics helpers
#define YCXX_DIAG_PUSH _Pragma("GCC diagnostic push")
#define YCXX_DIAG_POP _Pragma("GCC diagnostic pop")

#endif // YCXX_CONFIG_HPP
