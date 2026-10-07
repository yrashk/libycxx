// at_thread_exit.pass.cpp under the simulation, on Linux, of Darwin with Clang (support/tlv_bypass.hpp):
// the program's thread_local destructors are registered with the C library directly, bypassing
// libycxx, so only the PAL's own ordering puts the thread-end actions after them (DECISIONS §3).
// FLAGS: -pthread -Wl,--wrap=__cxa_thread_atexit
// REQUIRES: linux, exceptions
#include "tlv_bypass.hpp"
#include "at_thread_exit.pass.cpp"
