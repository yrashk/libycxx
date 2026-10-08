// libycxx core: the byte-string builtins (__builtin_memchr, __builtin_memcmp) as the run-time
// branches of constexpr functions call them.
//
// GCC 16.2's constant evaluator folds these builtins wrongly when the pointer points into a string
// literal at an offset: the offset is counted twice (__builtin_memchr("abcabcab" + 3, 'c', 5)
// gives the literal + 8, not + 5; __builtin_memcmp reads past the offset). GCC also evaluates the
// `if !consteval` branch of a constexpr function when it folds a call that is not manifestly
// constant-evaluated (an initializer, a call with constant arguments, even at -O0), so
// string_view("abcabcab").find('c', 3) was 8 at run time. Called through these functions, which
// are not constexpr, the builtins are never constant-evaluated; the optimizer still folds and
// inlines them, correctly. (Clang folds them correctly; the wrappers cost it nothing.)
#pragma once

#include <ycxx/core/cstddef.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

[[__gnu__::__always_inline__]] inline const void* __rt_memchr(const void* __s, int __c,
                                                               std::size_t __n) noexcept {
  return __builtin_memchr(__s, __c, __n);
}
[[__gnu__::__always_inline__]] inline int __rt_memcmp(const void* __s1, const void* __s2,
                                                       std::size_t __n) noexcept {
  return __builtin_memcmp(__s1, __s2, __n);
}

}} // namespace __ycxx::__detail
