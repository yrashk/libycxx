// libycxx core: whether the process is known to have a single thread (the PAL's
// ycxx_pal_single_threaded flag). Reference counts and uncontended locks of process-private
// objects then use plain arithmetic instead of atomic read-modify-write instructions, which cost
// tens of cycles each even without contention. The flag is cleared before a second thread
// starts, and thread creation synchronizes with the new thread, so every plain update made while
// it was set happens before the other threads' accesses (DECISIONS §3).
#pragma once

#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

[[__gnu__::__always_inline__]] inline bool __single_threaded() noexcept { return *::ycxx_pal_single_threaded != 0; }

// A reference count: a new reference is made from an existing one (relaxed increment); the
// decrement releases and acquires, so the one that reaches zero sees everything the other owners
// did. (An acq_rel decrement costs the same as a release one on x86 and little elsewhere, and
// unlike a release decrement followed by an acquire fence it is understood by ThreadSanitizer,
// which does not model fences and would report the destruction as a race.) ref_release returns
// whether the count reached zero.
template <class _Tp>
[[__gnu__::__always_inline__]] inline void __ref_add(_Tp& count) noexcept {
  if (::__ycxx::__detail::__single_threaded())
    ++count;
  else
    __atomic_fetch_add(&count, 1, __ATOMIC_RELAXED);
}
template <class _Tp>
[[__gnu__::__always_inline__]] inline bool __ref_release(_Tp& count) noexcept {
  if (::__ycxx::__detail::__single_threaded())
    return --count == 0;
  return __atomic_sub_fetch(&count, 1, __ATOMIC_ACQ_REL) == 0;
}

}} // namespace __ycxx::__detail
