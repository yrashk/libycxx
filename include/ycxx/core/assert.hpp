// libycxx core: the failure path of the assert macro.
#pragma once

#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Appends `s` to buf[pos..cap), returns the new position.
constexpr unsigned __append_text(char* __buf, unsigned __pos, unsigned __cap, const char* s) noexcept {
  while (*s && __pos + 1 < __cap)
    __buf[__pos++] = *s++;
  return __pos;
}
constexpr unsigned __append_uint(char* __buf, unsigned __pos, unsigned __cap, unsigned __v) noexcept {
  char digits[10];
  unsigned n = 0;
  do
    digits[n++] = static_cast<char>('0' + __v % 10);
  while (__v /= 10);
  while (n && __pos + 1 < __cap)
    __buf[__pos++] = digits[--n];
  return __pos;
}

// Not constexpr: reaching it during constant evaluation is the compile-time diagnostic.
[[noreturn, __gnu__::__cold__, __gnu__::__noinline__]] inline void __assert_failed(const char* __expr, const char* __file = __builtin_FILE(),
                                                                 unsigned line = __builtin_LINE(),
                                                                 const char* __func = __builtin_FUNCTION()) noexcept {
  char __msg[512];
  unsigned p = 0;
  p = __append_text(__msg, p, sizeof __msg, __file);
  p = __append_text(__msg, p, sizeof __msg, ":");
  p = __append_uint(__msg, p, sizeof __msg, line);
  p = __append_text(__msg, p, sizeof __msg, ": ");
  p = __append_text(__msg, p, sizeof __msg, __func);
  p = __append_text(__msg, p, sizeof __msg, ": Assertion `");
  p = __append_text(__msg, p, sizeof __msg, __expr);
  p = __append_text(__msg, p, sizeof __msg, "' failed.");
  __msg[p] = '\0';
  ::ycxx_error_handler(ycxx_error_assertion, __msg);
}

}} // namespace __ycxx::__detail
