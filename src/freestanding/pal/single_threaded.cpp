// libycxx freestanding runtime: the default single-threaded flag of the PAL, constant zero (the
// library always uses atomic instructions). A freestanding program that knows when it has one
// thread can define its own ycxx_pal_single_threaded.
#include <ycxx/pal.h>

namespace {
constinit const char never_single_threaded = 0;
}

extern "C" constinit const char* const ycxx_pal_single_threaded = &never_single_threaded;
