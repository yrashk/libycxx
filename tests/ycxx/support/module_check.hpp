// CHECK for tests that `import std;` (tests/ycxx/modules): include after the import. check.hpp
// declares the C library's dprintf and abort itself, which GCC rejects once the C library's own
// declarations are reachable through the module ("conflicting 'noexcept' specifier for imported
// declaration"); this one uses only names the module exports.
#pragma once

#define CHECK(...)                                                                                   \
  do {                                                                                               \
    if (!(__VA_ARGS__)) {                                                                            \
      std::println(std::cerr, "{}:{}: CHECK failed: {}", __FILE__, __LINE__, #__VA_ARGS__);          \
      std::abort();                                                                                  \
    }                                                                                                \
  } while (0)

// unelided(p): as check.hpp's (the result of a new-expression whose allocation must not be
// omitted).
template <class T>
inline T* unelided(T* p) noexcept {
  asm volatile("" : "+r"(p));
  return p;
}
