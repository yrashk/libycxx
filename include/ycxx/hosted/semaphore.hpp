// libycxx hosted: counting_semaphore and binary_semaphore ([thread.sema]).
//
// The counter is an atomic ptrdiff_t; acquire waits on it through the atomic wait slots while it
// is zero, and the timed forms use the runtime's timed slot wait.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>
#include <ycxx/hosted/thread_support.hpp>

namespace std {

template <ptrdiff_t LeastMaxValue = __PTRDIFF_MAX__>
class counting_semaphore {
  static_assert(LeastMaxValue >= 0, "counting_semaphore: LeastMaxValue must not be negative");

  ptrdiff_t counter_;

  bool try_take() noexcept {
    ptrdiff_t c = __atomic_load_n(&counter_, __ATOMIC_RELAXED);
    while (c > 0)
      if (__atomic_compare_exchange_n(&counter_, &c, c - 1, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return true;
    return false;
  }
  bool positive() const noexcept { return __atomic_load_n(&counter_, __ATOMIC_RELAXED) > 0; }

public:
  [[nodiscard]] static constexpr ptrdiff_t max() noexcept { return LeastMaxValue; }

  constexpr explicit counting_semaphore(ptrdiff_t desired) : counter_(desired) {
    ycxx::detail::precondition(desired >= 0 && desired <= max(),
                               "counting_semaphore: the initial count must be in [0, max()]");
  }
  ~counting_semaphore() = default;
  counting_semaphore(const counting_semaphore&) = delete;
  counting_semaphore& operator=(const counting_semaphore&) = delete;

  void release(ptrdiff_t update = 1) {
    ycxx::detail::precondition(update >= 0, "counting_semaphore::release: update must not be negative");
    const ptrdiff_t old = __atomic_fetch_add(&counter_, update, __ATOMIC_RELEASE);
    ycxx::detail::precondition(update <= max() - old, "counting_semaphore::release: the count would exceed max()");
    if (update > 0)
      ycxx::detail::atomic_notify(&counter_);
  }
  void acquire() {
    while (!try_take())
      ycxx::detail::atomic_wait_until_done(&counter_, [this] { return positive(); });
  }
  [[nodiscard]] bool try_acquire() noexcept { return try_take(); }
  template <class Rep, class Period>
  [[nodiscard]] bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time) {
    return try_acquire_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  [[nodiscard]] bool try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time) {
    static_assert(chrono::is_clock_v<Clock>, "try_acquire_until: Clock must meet the Cpp17Clock requirements");
    for (;;) {
      if (try_take())
        return true;
      if (!ycxx::detail::atomic_wait_until_done_by(&counter_, [this] { return positive(); }, abs_time))
        return try_take();
    }
  }
};

using binary_semaphore = counting_semaphore<1>;

} // namespace std
