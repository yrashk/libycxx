// libycxx core: whether the process is known to have a single thread (the PAL's
// ycxx_pal_single_threaded flag). Reference counts and uncontended locks of process-private
// objects then use plain arithmetic instead of atomic read-modify-write instructions, which cost
// tens of cycles each even without contention. The flag is cleared before a second thread
// starts, and thread creation synchronizes with the new thread, so every plain update made while
// it was set happens before the other threads' accesses (DECISIONS §3).
#pragma once

#include <ycxx/pal.h>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

[[gnu::always_inline]] inline bool single_threaded() noexcept { return *::ycxx_pal_single_threaded != 0; }

// A reference count: a new reference is made from an existing one (relaxed increment); the
// decrement releases, and the one that reaches zero acquires. ref_release returns whether the
// count reached zero.
template <class T>
[[gnu::always_inline]] inline void ref_add(T& count) noexcept {
  if (::ycxx::detail::single_threaded())
    ++count;
  else
    __atomic_fetch_add(&count, 1, __ATOMIC_RELAXED);
}
template <class T>
[[gnu::always_inline]] inline bool ref_release(T& count) noexcept {
  if (::ycxx::detail::single_threaded())
    return --count == 0;
  if (__atomic_sub_fetch(&count, 1, __ATOMIC_RELEASE) != 0)
    return false;
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
  return true;
}

}} // namespace ycxx::detail
