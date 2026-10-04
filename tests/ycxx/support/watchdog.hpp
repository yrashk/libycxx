// A deadlock guard for concurrency tests: watchdog(s) arms SIGALRM (POSIX alarm()), whose
// default action terminates the process, so a test that would hang (e.g. a deadlock the draft
// forbids) fails quickly instead of running into the lit timeout. Uses only the C library.
#pragma once

#include <unistd.h>

inline void watchdog(unsigned seconds = 10) { alarm(seconds); }
