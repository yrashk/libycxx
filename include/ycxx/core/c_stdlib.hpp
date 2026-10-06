// libycxx core: the freestanding part of <cstdlib> ([cstdlib.syn]) for a freestanding
// implementation, which has no C library to take it from.
//
// Only the freestanding <cstdlib> includes this; hosted, the types and functions are the C
// library's. The start and termination functions ([support.start.term]) call the environment's
// functions of the same name (as memcpy and friends must be provided for the compilers);
// libycxx-freestanding.a does not define them. They are reached through declarations in
// __ycxx::__detail whose assembler names are the C names, so that a C library header a freestanding
// program may still include (with its own exception specifications and C++ overloads) declares
// different entities and does not conflict. Every function here is a template with a defaulted
// parameter, like <cmath>'s, so such a C function wins ties under `using namespace std;`. qsort
// is defined here; abs, div, bsearch and memalignment are shared with the hosted header.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/c_bsearch.hpp>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// An assembler name is the object-file symbol verbatim: Mach-O prefixes C symbols with '_'.
// The functions are the environment's: default visibility, since a hidden reference could not
// bind to a shared C library (DECISIONS §2).
namespace [[__gnu__::__visibility__("default")]] __ycxx { namespace __detail::__c_rt {
#if _YCXX_TARGET_DARWIN
[[noreturn]] void abort() noexcept __asm__("_abort");
int atexit(void (*__func)()) noexcept __asm__("_atexit");
int at_quick_exit(void (*__func)()) noexcept __asm__("_at_quick_exit");
[[noreturn]] void exit(int status) __asm__("_exit");
[[noreturn]] void __exit_now(int status) noexcept __asm__("__Exit");
[[noreturn]] void quick_exit(int status) noexcept __asm__("_quick_exit");
#else
[[noreturn]] void abort() noexcept __asm__("abort");
int atexit(void (*__func)()) noexcept __asm__("atexit");
int at_quick_exit(void (*__func)()) noexcept __asm__("at_quick_exit");
[[noreturn]] void exit(int status) __asm__("exit");
[[noreturn]] void __exit_now(int status) noexcept __asm__("_Exit");
[[noreturn]] void quick_exit(int status) noexcept __asm__("quick_exit");
#endif
}} // namespace __ycxx::__detail::__c_rt

namespace [[__gnu__::__visibility__("hidden")]] std {
struct div_t {
  int quot;
  int rem;
};
struct ldiv_t {
  long quot;
  long rem;
};
struct lldiv_t {
  long long quot;
  long long rem;
};

template <class = void>
[[noreturn]] inline void abort() noexcept {
  ::__ycxx::__detail::__c_rt::abort();
}
template <class = void>
inline int atexit(void (*__func)()) noexcept {
  return ::__ycxx::__detail::__c_rt::atexit(__func);
}
template <class = void>
inline int at_quick_exit(void (*__func)()) noexcept {
  return ::__ycxx::__detail::__c_rt::at_quick_exit(__func);
}
template <class = void>
[[noreturn]] inline void exit(int status) {
  ::__ycxx::__detail::__c_rt::exit(status);
}
template <class = void>
[[noreturn]] inline void _Exit(int status) noexcept {
  ::__ycxx::__detail::__c_rt::__exit_now(status);
}
template <class = void>
[[noreturn]] inline void quick_exit(int status) noexcept {
  ::__ycxx::__detail::__c_rt::quick_exit(status);
}

// Heapsort: no recursion, no allocation, O(n log n) comparisons.
template <class = void>
void qsort(void* base, size_t __nmemb, size_t size, int (*__compar)(const void*, const void*)) {
  auto* b = static_cast<unsigned char*>(base);
  auto at = [&](size_t i) { return b + i * size; };
  auto swap = [&](size_t i, size_t __j) {
    unsigned char* __x = at(i);
    unsigned char* y = at(__j);
    for (size_t k = 0; k != size; ++k) {
      const unsigned char t = __x[k];
      __x[k] = y[k];
      y[k] = t;
    }
  };
  auto __sift_down = [&](size_t __root, size_t end) {
    for (;;) {
      size_t __child = 2 * __root + 1;
      if (__child >= end)
        return;
      if (__child + 1 < end && __compar(at(__child), at(__child + 1)) < 0)
        ++__child;
      if (__compar(at(__root), at(__child)) >= 0)
        return;
      swap(__root, __child);
      __root = __child;
    }
  };
  if (__nmemb < 2 || size == 0)
    return;
  for (size_t i = __nmemb / 2; i-- != 0;)
    __sift_down(i, __nmemb);
  for (size_t end = __nmemb - 1; end != 0; --end) {
    swap(0, end);
    __sift_down(0, end);
  }
}
} // namespace std
