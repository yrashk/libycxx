// libycxx freestanding runtime: the default PAL wake (nothing blocks in the default wait).
#include <ycxx/pal.h>

extern "C" void ycxx_pal_wake_all(const ycxx_pal_u32*) noexcept {}
