// libycxx core: the failure path of the assert macro.
#pragma once

#include <ycxx/core/error.hpp>

namespace ycxx::detail {

// Appends `s` to buf[pos..cap), returns the new position.
constexpr unsigned append_text(char* buf, unsigned pos, unsigned cap, const char* s) noexcept {
  while (*s && pos + 1 < cap)
    buf[pos++] = *s++;
  return pos;
}
constexpr unsigned append_uint(char* buf, unsigned pos, unsigned cap, unsigned v) noexcept {
  char digits[10];
  unsigned n = 0;
  do
    digits[n++] = static_cast<char>('0' + v % 10);
  while (v /= 10);
  while (n && pos + 1 < cap)
    buf[pos++] = digits[--n];
  return pos;
}

// Not constexpr: reaching it during constant evaluation is the compile-time diagnostic.
[[noreturn, gnu::cold, gnu::noinline]] inline void assert_failed(const char* expr, const char* file = __builtin_FILE(),
                                                                 unsigned line = __builtin_LINE(),
                                                                 const char* func = __builtin_FUNCTION()) noexcept {
  char msg[512];
  unsigned p = 0;
  p = append_text(msg, p, sizeof msg, file);
  p = append_text(msg, p, sizeof msg, ":");
  p = append_uint(msg, p, sizeof msg, line);
  p = append_text(msg, p, sizeof msg, ": ");
  p = append_text(msg, p, sizeof msg, func);
  p = append_text(msg, p, sizeof msg, ": Assertion `");
  p = append_text(msg, p, sizeof msg, expr);
  p = append_text(msg, p, sizeof msg, "' failed.");
  msg[p] = '\0';
  ::ycxx_error_handler(ycxx_error_assertion, msg);
}

} // namespace ycxx::detail
