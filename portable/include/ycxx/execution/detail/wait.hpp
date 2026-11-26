// The hosted implementation of the execution run loop's signal operations.
#pragma once

#include <atomic>

namespace __ycxx_portable_exec::__detail::__exec {

static_assert(std::atomic_ref<unsigned>::required_alignment <= alignof(unsigned));

inline unsigned __load_signal(unsigned& __signal) noexcept {
  return std::atomic_ref(__signal).load(std::memory_order_relaxed);
}

inline void __wait_for_signal(unsigned& __signal, unsigned __seen) noexcept {
  std::atomic_ref(__signal).wait(__seen, std::memory_order_acquire);
}

inline void __advance_signal(unsigned& __signal) noexcept {
  std::atomic_ref __ref(__signal);
  __ref.fetch_add(1u, std::memory_order_release);
  __ref.notify_all();
}

} // namespace __ycxx_portable_exec::__detail::__exec
