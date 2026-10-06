// libycxx freestanding runtime: the default PAL thread identity (one thread of execution). A
// freestanding program with threads supplies its own (the stop tokens of <stop_token> use it).
#include <ycxx/pal.h>

extern "C" ycxx_pal_handle ycxx_pal_thread_self(void) noexcept { return 1; }
