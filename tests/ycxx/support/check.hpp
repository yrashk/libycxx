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
