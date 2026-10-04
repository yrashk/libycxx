// Test-harness shim: the configuration macros libstdc++'s testsuite_hooks.h consults.
// This is NOT part of libycxx; it is only on the include path when running the libstdc++ testsuite.
#pragma once
#define _GLIBCXX_HAVE_SYS_STAT_H 1
#define _GLIBCXX_HAVE_UNISTD_H 1
#define _GLIBCXX_HOSTED 1
