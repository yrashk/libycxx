// libycxx core: whether the process is known to have a single thread (the PAL's
// ycxx_pal_single_threaded flag). Reference counts and uncontended locks of process-private
// objects then use plain arithmetic instead of atomic read-modify-write instructions, which cost
// tens of cycles each even without contention. The flag is cleared before a second thread
// starts, and thread creation synchronizes with the new thread, so every plain update made while
// it was set happens before the other threads' accesses (DECISIONS §3).
#pragma once

#include <ycxx/pal.h>

namespace ycxx::detail {

[[gnu::always_inline]] inline bool single_threaded() noexcept { return *::ycxx_pal_single_threaded != 0; }

} // namespace ycxx::detail
