// libycxx, YCXX_PAL=none without the 'threads' hosted layer (DECISIONS §18): there is one thread
// of execution, which ends only when the program does, so its thread_local destructors
// (__cxa_thread_atexit, src/abi/support.cpp) run with the static destructors: they are
// registered with the environment's __cxa_atexit, which runs them in reverse order of
// registration at exit, before the static objects constructed earlier. Built only when the layer
// is absent; with it the provider defines ycxx_pal_thread_atexit.
#include <ycxx/pal.h>

// The environment's (the C library's, or a bare-metal program's own): the Itanium C++ ABI's
// registration of a destructor to run at exit (3.3.5.3).
extern "C" int __cxa_atexit(void (*__f)(void*), void* __obj, void* __dso) noexcept;

extern "C" int ycxx_pal_thread_atexit(void (*__f)(void*), void* __obj, void* __dso) noexcept {
  return __cxa_atexit(__f, __obj, __dso);
}
