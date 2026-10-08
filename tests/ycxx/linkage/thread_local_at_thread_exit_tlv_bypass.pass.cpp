// thread_local_at_thread_exit.pass.cpp, simulating on Linux how Clang on Darwin
// registers thread_local destructors (support/tlv_bypass.hpp): with the C library directly,
// bypassing libycxx, so that only the PAL's own ordering puts the thread-end actions after
// them (DECISIONS §3).
// FLAGS: -pthread -Wl,--wrap=__cxa_thread_atexit
// REQUIRES: linux, exceptions
#include "tlv_bypass.hpp"
#include "thread_local_at_thread_exit.pass.cpp"
