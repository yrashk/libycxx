// Minimal test support for libycxx's own suite. Deliberately independent of every other
// test suite. CHECK works at run time; use static_assert for compile-time checks.
#pragma once

extern "C" int dprintf(int, const char*, ...); // unbuffered (fd 2): abort() does not flush stdio
extern "C" void abort(); // no [[noreturn]]: clang rejects it after <stdlib.h>'s GNU-attribute declaration

#define CHECK(...)                                                                                   \
  do {                                                                                               \
    if (!(__VA_ARGS__)) {                                                                            \
      dprintf(2, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                      \
      abort();                                                                                       \
    }                                                                                                \
  } while (0)

// CHECK_SAY((cond), "printf format", args...): CHECK, and on failure also prints the message
// (the inputs a failure on another platform needs to be understood).
#define CHECK_SAY(cond, ...)                                                                         \
  do {                                                                                               \
    if (!(cond)) {                                                                                   \
      dprintf(2, "%s:%d: CHECK failed: %s\n  ", __FILE__, __LINE__, #cond);                           \
      dprintf(2, __VA_ARGS__);                                                                       \
      dprintf(2, "\n");                                                                              \
      abort();                                                                                       \
    }                                                                                                \
  } while (0)

// unelided(p): p, through an empty asm statement that the compiler must assume reads and changes
// it. The result of a new-expression passed through it is no longer known to be what the
// delete-expression receives, so the compiler cannot omit the allocation ([expr.new]/14 allows
// that, and both compilers do at -O2 when the pointer reaches only its delete-expression). For
// tests that count the calls of replaced allocation functions. (Such tests also keep the state
// the replacements share with main volatile: GCC assumes by default,
// -fassume-sane-operators-new-delete, that those functions neither read nor change it.)
template <class T>
inline T* unelided(T* p) noexcept {
  asm volatile("" : "+r"(p));
  return p;
}
