// Shared cancellation state for libycxx and the portable in-place stop tokens.
// The lock and the request bit share a word. The requesting thread takes callbacks
// off the list before invoking them. It releases the lock while the callback runs
// so the callback may deregister itself, remove another callback, or destroy the state.
// Include inside the provider's private namespace after its node and platform declarations.
#pragma once

class __stop_state {
  using __word = __stop_platform::__word;
  static constexpr __word __requested_bit = 1, __locked_bit = 2;
  __word __bits_ = 0;
  __stop_callback_node* __head_ = nullptr;
  __stop_platform::__thread_id __requester_{}; // the thread running request_stop's callbacks
  bool* __destroyed_ = nullptr;      // set by request_stop while a callback runs; the destructor marks it

  // Takes the list lock; returns the bits seen (with the lock bit clear).
  __word lock() noexcept {
    __word cur = __atomic_load_n(&__bits_, __ATOMIC_RELAXED);
    for (int __spins = 0;;) {
      if (!(cur & __locked_bit)) {
        if (__atomic_compare_exchange_n(&__bits_, &cur, cur | __locked_bit, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
          return cur;
        continue;
      }
      if (__spins < 64)
        ++__spins;
      else
        __stop_platform::__yield();
      cur = __atomic_load_n(&__bits_, __ATOMIC_RELAXED);
    }
  }
  void unlock(__word bits) noexcept { __atomic_store_n(&__bits_, bits & ~__locked_bit, __ATOMIC_RELEASE); }

public:
  constexpr __stop_state() noexcept = default;
  constexpr ~__stop_state() {
    if (__destroyed_)
      *__destroyed_ = true; // destroyed from inside a callback: request_stop is running
  }
  __stop_state(const __stop_state&) = delete;
  __stop_state& operator=(const __stop_state&) = delete;

  bool stop_requested() const noexcept { return (__atomic_load_n(&__bits_, __ATOMIC_ACQUIRE) & __requested_bit) != 0; }

  bool request_stop() noexcept {
    __word b = lock();
    if (b & __requested_bit) {
      unlock(b);
      return false;
    }
    b |= __requested_bit;
    __requester_ = __stop_platform::__self();
    __atomic_store_n(&__bits_, b | __locked_bit, __ATOMIC_RELEASE); // publish the request, keep the lock
    while (__stop_callback_node* n = __head_) {
      __head_ = n->next;
      if (__head_)
        __head_->prev = &__head_;
      n->prev = nullptr;
      bool __removed = false, __destroyed = false;
      n->__removed = &__removed;
      __destroyed_ = &__destroyed;
      unlock(b);
      n->invoke(n);
      if (!__removed) {
        n->__removed = nullptr;
        __stop_platform::__finish(n->done);
      }
      // The callback destroyed this state (with every callback still registered, which it had to
      // deregister first): nothing of *this may be touched any more.
      if (__destroyed)
        return true;
      b = lock();
    }
    __destroyed_ = nullptr;
    unlock(b);
    return true;
  }

  // Registers n; false (n not registered) if a stop was already requested.
  bool add(__stop_callback_node* n) noexcept {
    __word b = lock();
    if (b & __requested_bit) {
      unlock(b);
      return false;
    }
    n->next = __head_;
    if (__head_)
      __head_->prev = &n->next;
    n->prev = &__head_;
    __head_ = n;
    unlock(b);
    return true;
  }

  // Deregisters n, waiting for its invocation on another thread to finish.
  void remove(__stop_callback_node* n) noexcept {
    __word b = lock();
    if (n->prev) {
      *n->prev = n->next;
      if (n->next)
        n->next->prev = n->prev;
      unlock(b);
      return;
    }
    const auto __requester = __requester_;
    unlock(b);
    // Taken off the list by request_stop: it has run, or runs now.
    if (__requester == __stop_platform::__self()) {
      // On this thread: it finished already, or this is its own invocation destroying it.
      if (!__stop_platform::__finished(n->done) && n->__removed)
        *n->__removed = true;
      return;
    }
    __stop_platform::__wait(n->done);
  }
};
