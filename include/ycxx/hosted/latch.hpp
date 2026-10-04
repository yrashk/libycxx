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

namespace std {

// [thread.latch.class]
class latch {
  ptrdiff_t counter_;

public:
  [[nodiscard]] static constexpr ptrdiff_t max() noexcept { return __PTRDIFF_MAX__; }

  constexpr explicit latch(ptrdiff_t expected) : counter_(expected) {
    ycxx::detail::precondition(expected >= 0, "latch: the expected count must not be negative");
  }
  ~latch() = default;
  latch(const latch&) = delete;
  latch& operator=(const latch&) = delete;

  void count_down(ptrdiff_t update = 1) {
    ycxx::detail::precondition(update >= 0, "latch::count_down: update must not be negative");
    const ptrdiff_t old = __atomic_fetch_sub(&counter_, update, __ATOMIC_RELEASE);
    ycxx::detail::precondition(update <= old, "latch::count_down: update exceeds the counter");
    if (old == update)
      ycxx::detail::atomic_notify(&counter_);
  }
  [[nodiscard]] bool try_wait() const noexcept { return __atomic_load_n(&counter_, __ATOMIC_ACQUIRE) == 0; }
  void wait() const {
    ycxx::detail::atomic_wait_until_done(&counter_, [this] { return try_wait(); });
  }
  void arrive_and_wait(ptrdiff_t update = 1) {
    count_down(update);
    wait();
  }
};

} // namespace std

namespace ycxx::detail {
// The default CompletionFunction of barrier: does nothing.
struct barrier_no_completion {
  void operator()() noexcept {}
};
} // namespace ycxx::detail

namespace std {

// [thread.barrier.class]
template <class CompletionFunction = ycxx::detail::barrier_no_completion>
class barrier {
  static_assert(is_nothrow_invocable_v<CompletionFunction&>,
                "barrier: CompletionFunction must be nothrow invocable as an lvalue");

  mutable ycxx::detail::futex_mutex m_;
  ptrdiff_t expected_;  // the expected count of each phase, less the drops
  ptrdiff_t remaining_; // still to arrive in the current phase
  unsigned phase_ = 0;  // read atomically by waiters
  [[no_unique_address]] CompletionFunction completion_;

  // m_ is held: the phase is complete; run the completion step and start the next phase.
  void complete_phase() noexcept {
    completion_();
    remaining_ = expected_;
    __atomic_store_n(&phase_, phase_ + 1, __ATOMIC_RELEASE);
    ycxx::detail::atomic_notify(&phase_);
  }

public:
  class arrival_token {
    unsigned phase_;
    explicit arrival_token(unsigned p) noexcept : phase_(p) {}
    friend class barrier;

  public:
    arrival_token(arrival_token&&) noexcept = default;
    arrival_token& operator=(arrival_token&&) noexcept = default;
  };

  [[nodiscard]] static constexpr ptrdiff_t max() noexcept { return __PTRDIFF_MAX__; }

  constexpr explicit barrier(ptrdiff_t expected, CompletionFunction f = CompletionFunction())
      : expected_(expected), remaining_(expected), completion_(static_cast<CompletionFunction&&>(f)) {
    ycxx::detail::precondition(expected >= 0, "barrier: the expected count must not be negative");
  }
  ~barrier() = default;
  barrier(const barrier&) = delete;
  barrier& operator=(const barrier&) = delete;

  [[nodiscard]] arrival_token arrive(ptrdiff_t update = 1) {
    m_.lock();
    ycxx::detail::precondition(update > 0 && update <= remaining_,
                               "barrier::arrive: update must be positive and at most the expected count");
    const unsigned phase = phase_;
    remaining_ -= update;
    if (remaining_ == 0)
      complete_phase();
    m_.unlock();
    return arrival_token(phase);
  }
  void wait(arrival_token&& arrival) const {
    const unsigned phase = arrival.phase_;
    ycxx::detail::atomic_wait_until_done(&phase_,
                                         [this, phase] { return __atomic_load_n(&phase_, __ATOMIC_ACQUIRE) != phase; });
  }
  void arrive_and_wait() { wait(arrive()); }
  void arrive_and_drop() {
    m_.lock();
    ycxx::detail::precondition(remaining_ > 0, "barrier::arrive_and_drop: the expected count is zero");
    --expected_;
    if (--remaining_ == 0)
      complete_phase();
    m_.unlock();
  }
};

} // namespace std
