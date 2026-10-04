// libycxx freestanding runtime: the default PAL address wait, which returns at once (waits
// become spins). A freestanding program with a scheduler can define its own (with
// ycxx_pal_wake_all and ycxx_pal_wake_one) to block instead.
#include <ycxx/pal.h>

extern "C" void ycxx_pal_wait(const ycxx_pal_u32*, ycxx_pal_u32) noexcept {}
