// Simulates on Linux how Clang on Darwin registers thread_local destructors: it calls the C
// library's _tlv_atexit directly (Darwin's TLV ABI), never libycxx's __cxa_thread_atexit, so
// libycxx does not see a program's thread_local objects being constructed (DECISIONS §3, the
// thread-end actions). Here every reference to __cxa_thread_atexit in the program, the test's
// objects and libycxx's archives alike, is bound to the wrapper below by the linker, which hands
// the destructor straight to glibc's __cxa_thread_atexit_impl, the function the POSIX PAL itself
// registers its thread-end sentinel with (as it does with _tlv_atexit on Darwin). Only the PAL's
// own registrations then reach the PAL, as on Darwin with Clang.
//
// A test using it is linked with  -Wl,--wrap=__cxa_thread_atexit  (GNU ld, lld; FLAGS) and
// REQUIRES linux. Before main, a check that the simulation is in effect: a thread_local's
// registration must arrive here, else the test would pass without simulating anything.
#pragma once
#include <cstdio>
#include <cstdlib>

extern "C" int __cxa_thread_atexit_impl(void (*)(void*), void*, void*);

namespace tlv_bypass {
inline int registrations = 0; // thread_local destructors registered bypassing libycxx (any thread)
}

extern "C" int __wrap___cxa_thread_atexit(void (*f)(void*), void* obj, void* dso) {
  __atomic_fetch_add(&tlv_bypass::registrations, 1, __ATOMIC_RELAXED);
  return __cxa_thread_atexit_impl(f, obj, dso);
}

namespace tlv_bypass {
struct witness {
  ~witness() {}
};
inline thread_local witness w;
struct self_check {
  self_check() {
    static_cast<void>(&w);
    if (__atomic_load_n(&registrations, __ATOMIC_RELAXED) == 0) {
      std::fputs("FAIL: tlv_bypass.hpp: thread_local destructors still reach __cxa_thread_atexit "
                 "(link with -Wl,--wrap=__cxa_thread_atexit)\n",
                 stderr);
      std::_Exit(3);
    }
  }
};
inline self_check check_at_startup;
} // namespace tlv_bypass
