// libycxx hosted: latch ([thread.latch]) and barrier ([thread.barrier]).
//
// latch is an atomic counter waited on through the atomic wait slots. barrier keeps its phase
// under a futex mutex: the last arrival of a phase runs the completion function (holding the
// mutex, so arrivals for the next phase wait until the phase has completed), resets the expected
// count and advances the phase number, on which waiters block.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/hosted/thread_support.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [thread.latch.class]
class latch {
  ptrdiff_t __counter_;

public:
  [[nodiscard]] static constexpr ptrdiff_t max() noexcept { return __PTRDIFF_MAX__; }

  constexpr explicit latch(ptrdiff_t expected) : __counter_(expected) {
    __ycxx::__detail::__precondition(expected >= 0, "latch: the expected count must not be negative");
  }
  ~latch() = default;
  latch(const latch&) = delete;
  latch& operator=(const latch&) = delete;

  void count_down(ptrdiff_t __update = 1) {
    __ycxx::__detail::__precondition(__update >= 0, "latch::count_down: update must not be negative");
    const ptrdiff_t __old = __atomic_fetch_sub(&__counter_, __update, __ATOMIC_RELEASE);
    __ycxx::__detail::__precondition(__update <= __old, "latch::count_down: update exceeds the counter");
    if (__old == __update)
      __ycxx::__detail::__atomic_notify(&__counter_);
  }
  [[nodiscard]] bool try_wait() const noexcept { return __atomic_load_n(&__counter_, __ATOMIC_ACQUIRE) == 0; }
  void wait() const {
    __ycxx::__detail::__atomic_wait_until_done(&__counter_, [this] { return try_wait(); });
  }
  void arrive_and_wait(ptrdiff_t __update = 1) {
    count_down(__update);
    wait();
  }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// The default CompletionFunction of barrier: does nothing.
struct __barrier_no_completion {
  void operator()() noexcept {}
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [thread.barrier.class]
template <class _CompletionFunction = __ycxx::__detail::__barrier_no_completion>
class barrier {
  // [thread.barrier.class]/5 requires is_nothrow_invocable_v<CompletionFunction&> but does not
  // mandate it; a completion function that is not noexcept is accepted, and one that throws
  // ends the program (the completion step runs in a noexcept function; LWG 3898).
  static_assert(is_invocable_v<_CompletionFunction&>, "barrier: CompletionFunction must be invocable as an lvalue");

  mutable __ycxx::__detail::__futex_mutex __m_;
  ptrdiff_t __expected_;  // the expected count of each phase, less the drops
  ptrdiff_t __remaining_; // still to arrive in the current phase
  unsigned __phase_ = 0;  // read atomically by waiters
  [[no_unique_address]] _CompletionFunction __completion_;

  // m_ is held: the phase is complete; run the completion step and start the next phase.
  void __complete_phase() noexcept {
    __completion_();
    __remaining_ = __expected_;
    __atomic_store_n(&__phase_, __phase_ + 1, __ATOMIC_RELEASE);
    __ycxx::__detail::__atomic_notify(&__phase_);
  }

public:
  class arrival_token {
    unsigned __phase_;
    explicit arrival_token(unsigned p) noexcept : __phase_(p) {}
    friend class barrier;

  public:
    arrival_token(arrival_token&&) noexcept = default;
    arrival_token& operator=(arrival_token&&) noexcept = default;
  };

  [[nodiscard]] static constexpr ptrdiff_t max() noexcept { return __PTRDIFF_MAX__; }

  constexpr explicit barrier(ptrdiff_t expected, _CompletionFunction __f = _CompletionFunction())
      : __expected_(expected), __remaining_(expected), __completion_(static_cast<_CompletionFunction&&>(__f)) {
    __ycxx::__detail::__precondition(expected >= 0, "barrier: the expected count must not be negative");
  }
  ~barrier() = default;
  barrier(const barrier&) = delete;
  barrier& operator=(const barrier&) = delete;

  [[nodiscard]] arrival_token arrive(ptrdiff_t __update = 1) {
    __m_.lock();
    __ycxx::__detail::__precondition(__update > 0 && __update <= __remaining_,
                               "barrier::arrive: update must be positive and at most the expected count");
    const unsigned __phase = __phase_;
    __remaining_ -= __update;
    if (__remaining_ == 0)
      __complete_phase();
    __m_.unlock();
    return arrival_token(__phase);
  }
  void wait(arrival_token&& __arrival) const {
    const unsigned __phase = __arrival.__phase_;
    __ycxx::__detail::__atomic_wait_until_done(&__phase_,
                                         [this, __phase] { return __atomic_load_n(&__phase_, __ATOMIC_ACQUIRE) != __phase; });
  }
  void arrive_and_wait() { wait(arrive()); }
  void arrive_and_drop() {
    __m_.lock();
    __ycxx::__detail::__precondition(__remaining_ > 0, "barrier::arrive_and_drop: the expected count is zero");
    --__expected_;
    if (--__remaining_ == 0)
      __complete_phase();
    __m_.unlock();
  }
};

} // namespace std
