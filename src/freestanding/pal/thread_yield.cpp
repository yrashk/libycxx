// libycxx freestanding runtime: the default PAL yield (one thread of execution: nothing to yield to).
#include <ycxx/pal.h>

extern "C" void ycxx_pal_thread_yield(void) noexcept {}
