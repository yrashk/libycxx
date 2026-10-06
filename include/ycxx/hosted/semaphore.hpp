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

namespace [[__gnu__::__visibility__("hidden")]] std {

template <ptrdiff_t _LeastMaxValue = __PTRDIFF_MAX__>
class counting_semaphore {
  static_assert(_LeastMaxValue >= 0, "counting_semaphore: LeastMaxValue must not be negative");

  ptrdiff_t __counter_;

  bool __try_take() noexcept {
    ptrdiff_t c = __atomic_load_n(&__counter_, __ATOMIC_RELAXED);
    while (c > 0)
      if (__atomic_compare_exchange_n(&__counter_, &c, c - 1, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return true;
    return false;
  }
  bool __positive() const noexcept { return __atomic_load_n(&__counter_, __ATOMIC_RELAXED) > 0; }

public:
  [[nodiscard]] static constexpr ptrdiff_t max() noexcept { return _LeastMaxValue; }

  constexpr explicit counting_semaphore(ptrdiff_t __desired) : __counter_(__desired) {
    __ycxx::__detail::__precondition(__desired >= 0 && __desired <= max(),
                               "counting_semaphore: the initial count must be in [0, max()]");
  }
  ~counting_semaphore() = default;
  counting_semaphore(const counting_semaphore&) = delete;
  counting_semaphore& operator=(const counting_semaphore&) = delete;

  void release(ptrdiff_t __update = 1) {
    __ycxx::__detail::__precondition(__update >= 0, "counting_semaphore::release: update must not be negative");
    const ptrdiff_t __old = __atomic_fetch_add(&__counter_, __update, __ATOMIC_RELEASE);
    __ycxx::__detail::__precondition(__update <= max() - __old, "counting_semaphore::release: the count would exceed max()");
    if (__update > 0)
      __ycxx::__detail::__atomic_notify(&__counter_);
  }
  void acquire() {
    while (!__try_take())
      __ycxx::__detail::__atomic_wait_until_done(&__counter_, [this] { return __positive(); });
  }
  [[nodiscard]] bool try_acquire() noexcept { return __try_take(); }
  template <class _Rep, class _Period>
  [[nodiscard]] bool try_acquire_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    return try_acquire_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  [[nodiscard]] bool try_acquire_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    static_assert(chrono::is_clock_v<_Clock>, "try_acquire_until: Clock must meet the Cpp17Clock requirements");
    for (;;) {
      if (__try_take())
        return true;
      if (!__ycxx::__detail::__atomic_wait_until_done_by(&__counter_, [this] { return __positive(); }, __abs_time))
        return __try_take();
    }
  }
};

using binary_semaphore = counting_semaphore<1>;

} // namespace std
