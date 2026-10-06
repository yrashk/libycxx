// libycxx freestanding runtime: the default PAL wake (nothing blocks in the default wait).
#include <ycxx/pal.h>

extern "C" void ycxx_pal_wake_one(const ycxx_pal_u32*) noexcept {}
