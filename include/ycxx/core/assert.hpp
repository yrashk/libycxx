// libycxx core: the failure path of the assert macro.
#pragma once

#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

constexpr unsigned __text_length(const char* s) noexcept {
  unsigned n = 0;
  while (s[n])
    ++n;
  return n;
}
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
  // Sized to fit ([assertions.assert]/2.3: the diagnostic contains #__VA_ARGS__ and the names of
  // the file and the function): a fixed buffer would cut the expression after a long function
  // name, and the name of a function template's specialization can run to kilobytes. 48 bytes
  // hold the punctuation, the line number and the terminator.
  const unsigned __cap = __text_length(__file) + __text_length(__func) + __text_length(__expr) + 48;
  char* const __msg = static_cast<char*>(__builtin_alloca(__cap));
  unsigned p = 0;
  p = __append_text(__msg, p, __cap, __file);
  p = __append_text(__msg, p, __cap, ":");
  p = __append_uint(__msg, p, __cap, line);
  p = __append_text(__msg, p, __cap, ": ");
  p = __append_text(__msg, p, __cap, __func);
  p = __append_text(__msg, p, __cap, ": Assertion `");
  p = __append_text(__msg, p, __cap, __expr);
  p = __append_text(__msg, p, __cap, "' failed.");
  __msg[p] = '\0';
  ::ycxx_error_handler(ycxx_error_assertion, __msg);
}

}} // namespace __ycxx::__detail
