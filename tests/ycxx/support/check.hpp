// Minimal test support for libycxx's own suite. Deliberately independent of every other
// test suite. CHECK works at run time; use static_assert for compile-time checks.
#pragma once

extern "C" int printf(const char*, ...);
extern "C" [[noreturn]] void abort();

#define CHECK(...)                                                                                   \
  do {                                                                                               \
    if (!(__VA_ARGS__)) {                                                                            \
      printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                         \
      abort();                                                                                       \
    }                                                                                                \
  } while (0)
