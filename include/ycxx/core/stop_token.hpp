// libycxx core: stop tokens ([thread.stoptoken]). Core so that the senders of <execution> can use
// them freestanding: the stop state needs only atomics and the PAL's wait, thread identity and
// yield (the freestanding runtime's defaults: one thread, identity 1, yield does nothing).
//
// One stop state implementation (ycxx::detail::stop_state) serves stop_source/stop_token, which
// share a reference-counted heap copy of it, and inplace_stop_source, which contains one. A stop
// state is a word holding the "stop requested" bit and a lock bit for its intrusive list of
// registered callbacks. request_stop runs the callbacks one at a time with the lock released;
// a callback deregistered while it runs on another thread is waited for (an atomic wait on the
// callback's own completion flag), one deregistered from inside its own invocation is not.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/pal.h>

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// A registered callback (the base of stop_callback and inplace_stop_callback). invoke runs it.
struct stop_callback_node {
  stop_callback_node* next = nullptr;
  stop_callback_node** prev = nullptr; // the pointer to this node; null when not in a list
  void (*invoke)(stop_callback_node*) noexcept = nullptr;
  bool* removed = nullptr;   // set by request_stop while the callback runs
  unsigned char done = 0;    // the callback has finished running (atomic, waited on)
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
using adl_free::stop_callback_node;

class stop_state {
  static constexpr ycxx_pal_u32 requested_bit = 1, locked_bit = 2;
  ycxx_pal_u32 bits_ = 0;
  stop_callback_node* head_ = nullptr;
  ycxx_pal_handle requester_ = 0; // the thread running request_stop's callbacks

  // Takes the list lock; returns the bits seen (with the lock bit clear).
  ycxx_pal_u32 lock() noexcept {
    ycxx_pal_u32 cur = __atomic_load_n(&bits_, __ATOMIC_RELAXED);
    for (int spins = 0;; ++spins) {
      if (!(cur & locked_bit)) {
        if (__atomic_compare_exchange_n(&bits_, &cur, cur | locked_bit, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
          return cur;
        continue;
      }
      if (spins > 64)
        ::ycxx_pal_thread_yield();
      cur = __atomic_load_n(&bits_, __ATOMIC_RELAXED);
    }
  }
  void unlock(ycxx_pal_u32 bits) noexcept { __atomic_store_n(&bits_, bits & ~locked_bit, __ATOMIC_RELEASE); }

public:
  constexpr stop_state() noexcept = default;
  stop_state(const stop_state&) = delete;
  stop_state& operator=(const stop_state&) = delete;

  bool stop_requested() const noexcept { return (__atomic_load_n(&bits_, __ATOMIC_ACQUIRE) & requested_bit) != 0; }

  bool request_stop() noexcept {
    ycxx_pal_u32 b = lock();
    if (b & requested_bit) {
      unlock(b);
      return false;
    }
    b |= requested_bit;
    requester_ = ::ycxx_pal_thread_self();
    __atomic_store_n(&bits_, b | locked_bit, __ATOMIC_RELEASE); // publish the request, keep the lock
    while (stop_callback_node* n = head_) {
      head_ = n->next;
      if (head_)
        head_->prev = &head_;
      n->prev = nullptr;
      bool removed = false;
      n->removed = &removed;
      unlock(b);
      n->invoke(n);
      if (!removed) {
        n->removed = nullptr;
        __atomic_store_n(&n->done, 1, __ATOMIC_RELEASE);
        ::ycxx::detail::atomic_notify(&n->done);
      }
      b = lock();
    }
    unlock(b);
    return true;
  }

  // Registers n; false (n not registered) if a stop was already requested.
  bool add(stop_callback_node* n) noexcept {
    ycxx_pal_u32 b = lock();
    if (b & requested_bit) {
      unlock(b);
      return false;
    }
    n->next = head_;
    if (head_)
      head_->prev = &n->next;
    n->prev = &head_;
    head_ = n;
    unlock(b);
    return true;
  }

  // Deregisters n, waiting for its invocation on another thread to finish.
  void remove(stop_callback_node* n) noexcept {
    ycxx_pal_u32 b = lock();
    if (n->prev) {
      *n->prev = n->next;
      if (n->next)
        n->next->prev = n->prev;
      unlock(b);
      return;
    }
    const ycxx_pal_handle requester = requester_;
    unlock(b);
    // Taken off the list by request_stop: it has run, or runs now.
    if (requester == ::ycxx_pal_thread_self()) {
      // On this thread: it finished already, or this is its own invocation destroying it.
      if (__atomic_load_n(&n->done, __ATOMIC_ACQUIRE) == 0 && n->removed)
        *n->removed = true;
      return;
    }
    ::ycxx::detail::atomic_wait_until_done(&n->done, [n] { return __atomic_load_n(&n->done, __ATOMIC_ACQUIRE) != 0; });
  }
};

// The shared stop state of stop_source/stop_token/stop_callback.
struct shared_stop_state {
  stop_state state;
  ycxx_pal_u32 refs = 1;    // every owner: sources, tokens and registered callbacks
  ycxx_pal_u32 sources = 1; // the stop_source objects

  static void retain(shared_stop_state* s) noexcept {
    if (s)
      __atomic_fetch_add(&s->refs, 1, __ATOMIC_RELAXED);
  }
  static void release(shared_stop_state* s) noexcept {
    if (s && __atomic_fetch_sub(&s->refs, 1, __ATOMIC_ACQ_REL) == 1)
      delete s;
  }
};

template <template <class> class>
struct check_type_alias_exists;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

class stop_token;
class stop_source;
template <class CallbackFn>
class stop_callback;
class inplace_stop_token;
class inplace_stop_source;
template <class CallbackFn>
class inplace_stop_callback;

template <class T, class CallbackFn>
using stop_callback_for_t = typename T::template callback_type<CallbackFn>;

// [stoptoken.concepts]
template <class Token>
concept stoppable_token = requires(const Token tok) {
  typename ycxx::detail::check_type_alias_exists<Token::template callback_type>;
  { tok.stop_requested() } noexcept -> same_as<bool>;
  { tok.stop_possible() } noexcept -> same_as<bool>;
  { Token(tok) } noexcept;
} && copyable<Token> && equality_comparable<Token>;

template <class Token>
concept unstoppable_token = stoppable_token<Token> && requires(const Token tok) {
  requires bool_constant<(!tok.stop_possible())>::value;
};

struct nostopstate_t {
  explicit nostopstate_t() = default;
};
inline constexpr nostopstate_t nostopstate{};

// [stoptoken]
class stop_token {
  ycxx::detail::shared_stop_state* state_ = nullptr;

  explicit stop_token(ycxx::detail::shared_stop_state* s) noexcept : state_(s) {
    ycxx::detail::shared_stop_state::retain(s);
  }
  friend class stop_source;
  template <class>
  friend class stop_callback;

public:
  template <class CallbackFn>
  using callback_type = stop_callback<CallbackFn>;

  stop_token() noexcept = default;
  stop_token(const stop_token& t) noexcept : state_(t.state_) { ycxx::detail::shared_stop_state::retain(state_); }
  stop_token(stop_token&& t) noexcept : state_(t.state_) { t.state_ = nullptr; }
  stop_token& operator=(const stop_token& t) noexcept {
    stop_token(t).swap(*this);
    return *this;
  }
  stop_token& operator=(stop_token&& t) noexcept {
    stop_token(static_cast<stop_token&&>(t)).swap(*this);
    return *this;
  }
  ~stop_token() { ycxx::detail::shared_stop_state::release(state_); }

  void swap(stop_token& rhs) noexcept {
    auto* s = state_;
    state_ = rhs.state_;
    rhs.state_ = s;
  }
  [[nodiscard]] bool stop_requested() const noexcept { return state_ && state_->state.stop_requested(); }
  [[nodiscard]] bool stop_possible() const noexcept {
    return state_ && (state_->state.stop_requested() || __atomic_load_n(&state_->sources, __ATOMIC_ACQUIRE) != 0);
  }
  [[nodiscard]] friend bool operator==(const stop_token& a, const stop_token& b) noexcept { return a.state_ == b.state_; }
  friend void swap(stop_token& a, stop_token& b) noexcept { a.swap(b); }
};

// [stopsource]
class stop_source {
  ycxx::detail::shared_stop_state* state_;

public:
  stop_source() : state_(new ycxx::detail::shared_stop_state) {}
  explicit stop_source(nostopstate_t) noexcept : state_(nullptr) {}
  stop_source(const stop_source& s) noexcept : state_(s.state_) {
    if (state_) {
      ycxx::detail::shared_stop_state::retain(state_);
      __atomic_fetch_add(&state_->sources, 1, __ATOMIC_RELAXED);
    }
  }
  stop_source(stop_source&& s) noexcept : state_(s.state_) { s.state_ = nullptr; }
  stop_source& operator=(const stop_source& s) noexcept {
    stop_source(s).swap(*this);
    return *this;
  }
  stop_source& operator=(stop_source&& s) noexcept {
    stop_source(static_cast<stop_source&&>(s)).swap(*this);
    return *this;
  }
  ~stop_source() {
    if (state_) {
      __atomic_fetch_sub(&state_->sources, 1, __ATOMIC_RELEASE);
      ycxx::detail::shared_stop_state::release(state_);
    }
  }

  void swap(stop_source& rhs) noexcept {
    auto* s = state_;
    state_ = rhs.state_;
    rhs.state_ = s;
  }
  [[nodiscard]] stop_token get_token() const noexcept { return stop_token(state_); }
  [[nodiscard]] bool stop_possible() const noexcept { return state_ != nullptr; }
  [[nodiscard]] bool stop_requested() const noexcept { return state_ && state_->state.stop_requested(); }
  bool request_stop() noexcept { return state_ && state_->state.request_stop(); }
  [[nodiscard]] friend bool operator==(const stop_source& a, const stop_source& b) noexcept {
    return a.state_ == b.state_;
  }
  friend void swap(stop_source& a, stop_source& b) noexcept { a.swap(b); }
};

// [stopcallback]
template <class CallbackFn>
class stop_callback : ycxx::adl_free::stop_callback_node {
  static_assert(invocable<CallbackFn> && destructible<CallbackFn>,
                "stop_callback: CallbackFn must be invocable and destructible");

  CallbackFn callback_fn_;
  ycxx::detail::shared_stop_state* state_ = nullptr;

  static void run(ycxx::detail::stop_callback_node* n) noexcept {
    static_cast<CallbackFn&&>(static_cast<stop_callback*>(n)->callback_fn_)();
  }
  // Registers with st's stop state when a stop is still possible ([stoptoken.concepts]/3.2.1).
  // From an rvalue token (`from` is st), a registration takes over st's reference to the state,
  // which leaves st without one; otherwise st is unchanged.
  void attach(const stop_token& st, stop_token* from) noexcept {
    ycxx::detail::shared_stop_state* s = st.state_;
    if (!s || (!s->state.stop_requested() && __atomic_load_n(&s->sources, __ATOMIC_ACQUIRE) == 0))
      return;
    this->invoke = &run;
    if (s->state.add(this)) {
      if (from)
        from->state_ = nullptr;
      else
        ycxx::detail::shared_stop_state::retain(s);
      state_ = s;
    } else {
      static_cast<CallbackFn&&>(callback_fn_)(); // a stop was already requested
    }
  }

public:
  using callback_type = CallbackFn;

  template <class Initializer>
    requires constructible_from<CallbackFn, Initializer>
  explicit stop_callback(const stop_token& st, Initializer&& init) noexcept(is_nothrow_constructible_v<CallbackFn, Initializer>)
      : callback_fn_(static_cast<Initializer&&>(init)) {
    attach(st, nullptr);
  }
  template <class Initializer>
    requires constructible_from<CallbackFn, Initializer>
  explicit stop_callback(stop_token&& st, Initializer&& init) noexcept(is_nothrow_constructible_v<CallbackFn, Initializer>)
      : callback_fn_(static_cast<Initializer&&>(init)) {
    attach(st, __builtin_addressof(st));
  }
  ~stop_callback() {
    if (state_) {
      state_->state.remove(this);
      ycxx::detail::shared_stop_state::release(state_);
    }
  }
  stop_callback(const stop_callback&) = delete;
  stop_callback(stop_callback&&) = delete;
  stop_callback& operator=(const stop_callback&) = delete;
  stop_callback& operator=(stop_callback&&) = delete;
};
template <class CallbackFn>
stop_callback(stop_token, CallbackFn) -> stop_callback<CallbackFn>;

// [stoptoken.never]
class never_stop_token {
  struct callback_type_ {
    explicit callback_type_(never_stop_token, auto&&) noexcept {}
  };

public:
  template <class>
  using callback_type = callback_type_;
  static constexpr bool stop_requested() noexcept { return false; }
  static constexpr bool stop_possible() noexcept { return false; }
  bool operator==(const never_stop_token&) const = default;
};

// [stoptoken.inplace]
class inplace_stop_token {
  const inplace_stop_source* stop_source_ = nullptr;

  constexpr explicit inplace_stop_token(const inplace_stop_source* s) noexcept : stop_source_(s) {}
  friend class inplace_stop_source;
  template <class>
  friend class inplace_stop_callback;

public:
  template <class CallbackFn>
  using callback_type = inplace_stop_callback<CallbackFn>;

  inplace_stop_token() = default;
  bool operator==(const inplace_stop_token&) const = default;
  [[nodiscard]] bool stop_requested() const noexcept;
  [[nodiscard]] bool stop_possible() const noexcept { return stop_source_ != nullptr; }
  void swap(inplace_stop_token& rhs) noexcept {
    auto* s = stop_source_;
    stop_source_ = rhs.stop_source_;
    rhs.stop_source_ = s;
  }
  friend void swap(inplace_stop_token& a, inplace_stop_token& b) noexcept { a.swap(b); }
};

// [stopsource.inplace]
class inplace_stop_source {
  mutable ycxx::detail::stop_state state_;

  template <class>
  friend class inplace_stop_callback;

public:
  constexpr inplace_stop_source() noexcept = default;
  inplace_stop_source(inplace_stop_source&&) = delete;
  inplace_stop_source(const inplace_stop_source&) = delete;
  inplace_stop_source& operator=(inplace_stop_source&&) = delete;
  inplace_stop_source& operator=(const inplace_stop_source&) = delete;
  ~inplace_stop_source() = default;

  [[nodiscard]] constexpr inplace_stop_token get_token() const noexcept { return inplace_stop_token(this); }
  [[nodiscard]] static constexpr bool stop_possible() noexcept { return true; }
  [[nodiscard]] bool stop_requested() const noexcept { return state_.stop_requested(); }
  bool request_stop() noexcept { return state_.request_stop(); }
};

inline bool inplace_stop_token::stop_requested() const noexcept {
  return stop_source_ != nullptr && stop_source_->stop_requested();
}

// [stopcallback.inplace]
template <class CallbackFn>
class inplace_stop_callback : ycxx::adl_free::stop_callback_node {
  static_assert(invocable<CallbackFn> && destructible<CallbackFn>,
                "inplace_stop_callback: CallbackFn must be invocable and destructible");

  CallbackFn callback_fn_;
  ycxx::detail::stop_state* state_ = nullptr;

  static void run(ycxx::detail::stop_callback_node* n) noexcept {
    static_cast<CallbackFn&&>(static_cast<inplace_stop_callback*>(n)->callback_fn_)();
  }

public:
  using callback_type = CallbackFn;

  template <class Initializer>
    requires constructible_from<CallbackFn, Initializer>
  explicit inplace_stop_callback(inplace_stop_token st, Initializer&& init) noexcept(
      is_nothrow_constructible_v<CallbackFn, Initializer>)
      : callback_fn_(static_cast<Initializer&&>(init)) {
    if (!st.stop_source_)
      return;
    this->invoke = &run;
    ycxx::detail::stop_state& s = st.stop_source_->state_;
    if (s.add(this))
      state_ = &s;
    else
      static_cast<CallbackFn&&>(callback_fn_)();
  }
  ~inplace_stop_callback() {
    if (state_)
      state_->remove(this);
  }
  inplace_stop_callback(inplace_stop_callback&&) = delete;
  inplace_stop_callback(const inplace_stop_callback&) = delete;
  inplace_stop_callback& operator=(inplace_stop_callback&&) = delete;
  inplace_stop_callback& operator=(const inplace_stop_callback&) = delete;
};
template <class CallbackFn>
inplace_stop_callback(inplace_stop_token, CallbackFn) -> inplace_stop_callback<CallbackFn>;

} // namespace std
