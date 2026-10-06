// libycxx core: stop tokens ([thread.stoptoken]). Core so that the senders of <execution> can use
// them freestanding: the stop state needs only atomics and the PAL's wait, thread identity and
// yield (the freestanding runtime's defaults: one thread, identity 1, yield does nothing).
//
// One stop state implementation (__ycxx::__detail::__stop_state) serves stop_source/stop_token, which
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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// A registered callback (the base of stop_callback and inplace_stop_callback). invoke runs it.
struct __stop_callback_node {
  __stop_callback_node* next = nullptr;
  __stop_callback_node** prev = nullptr; // the pointer to this node; null when not in a list
  void (*invoke)(__stop_callback_node*) noexcept = nullptr;
  bool* __removed = nullptr;   // set by request_stop while the callback runs
  unsigned char done = 0;    // the callback has finished running (atomic, waited on)
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
using __adl_free::__stop_callback_node;

class __stop_state {
  static constexpr __ycxx_pal_u32 __requested_bit = 1, __locked_bit = 2;
  __ycxx_pal_u32 __bits_ = 0;
  __stop_callback_node* __head_ = nullptr;
  __ycxx_pal_handle __requester_ = 0; // the thread running request_stop's callbacks

  // Takes the list lock; returns the bits seen (with the lock bit clear).
  __ycxx_pal_u32 lock() noexcept {
    __ycxx_pal_u32 cur = __atomic_load_n(&__bits_, __ATOMIC_RELAXED);
    for (int __spins = 0;; ++__spins) {
      if (!(cur & __locked_bit)) {
        if (__atomic_compare_exchange_n(&__bits_, &cur, cur | __locked_bit, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
          return cur;
        continue;
      }
      if (__spins > 64)
        ::__ycxx_pal_thread_yield();
      cur = __atomic_load_n(&__bits_, __ATOMIC_RELAXED);
    }
  }
  void unlock(__ycxx_pal_u32 bits) noexcept { __atomic_store_n(&__bits_, bits & ~__locked_bit, __ATOMIC_RELEASE); }

public:
  constexpr __stop_state() noexcept = default;
  __stop_state(const __stop_state&) = delete;
  __stop_state& operator=(const __stop_state&) = delete;

  bool stop_requested() const noexcept { return (__atomic_load_n(&__bits_, __ATOMIC_ACQUIRE) & __requested_bit) != 0; }

  bool request_stop() noexcept {
    __ycxx_pal_u32 b = lock();
    if (b & __requested_bit) {
      unlock(b);
      return false;
    }
    b |= __requested_bit;
    __requester_ = ::__ycxx_pal_thread_self();
    __atomic_store_n(&__bits_, b | __locked_bit, __ATOMIC_RELEASE); // publish the request, keep the lock
    while (__stop_callback_node* n = __head_) {
      __head_ = n->next;
      if (__head_)
        __head_->prev = &__head_;
      n->prev = nullptr;
      bool __removed = false;
      n->__removed = &__removed;
      unlock(b);
      n->invoke(n);
      if (!__removed) {
        n->__removed = nullptr;
        __atomic_store_n(&n->done, 1, __ATOMIC_RELEASE);
        ::__ycxx::__detail::__atomic_notify(&n->done);
      }
      b = lock();
    }
    unlock(b);
    return true;
  }

  // Registers n; false (n not registered) if a stop was already requested.
  bool add(__stop_callback_node* n) noexcept {
    __ycxx_pal_u32 b = lock();
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
    __ycxx_pal_u32 b = lock();
    if (n->prev) {
      *n->prev = n->next;
      if (n->next)
        n->next->prev = n->prev;
      unlock(b);
      return;
    }
    const __ycxx_pal_handle __requester = __requester_;
    unlock(b);
    // Taken off the list by request_stop: it has run, or runs now.
    if (__requester == ::__ycxx_pal_thread_self()) {
      // On this thread: it finished already, or this is its own invocation destroying it.
      if (__atomic_load_n(&n->done, __ATOMIC_ACQUIRE) == 0 && n->__removed)
        *n->__removed = true;
      return;
    }
    ::__ycxx::__detail::__atomic_wait_until_done(&n->done, [n] { return __atomic_load_n(&n->done, __ATOMIC_ACQUIRE) != 0; });
  }
};

// The shared stop state of stop_source/stop_token/stop_callback.
struct __shared_stop_state {
  __stop_state state;
  __ycxx_pal_u32 __refs = 1;    // every owner: sources, tokens and registered callbacks
  __ycxx_pal_u32 __sources = 1; // the stop_source objects

  static void __retain(__shared_stop_state* s) noexcept {
    if (s)
      __atomic_fetch_add(&s->__refs, 1, __ATOMIC_RELAXED);
  }
  static void release(__shared_stop_state* s) noexcept {
    if (s && __atomic_fetch_sub(&s->__refs, 1, __ATOMIC_ACQ_REL) == 1)
      delete s;
  }
};

template <template <class> class>
struct __check_type_alias_exists;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

class stop_token;
class stop_source;
template <class _CallbackFn>
class stop_callback;
class inplace_stop_token;
class inplace_stop_source;
template <class _CallbackFn>
class inplace_stop_callback;

template <class _Tp, class _CallbackFn>
using stop_callback_for_t = typename _Tp::template callback_type<_CallbackFn>;

// [stoptoken.concepts]
template <class _Token>
concept stoppable_token = requires(const _Token __tok) {
  typename __ycxx::__detail::__check_type_alias_exists<_Token::template callback_type>;
  { __tok.stop_requested() } noexcept -> same_as<bool>;
  { __tok.stop_possible() } noexcept -> same_as<bool>;
  { _Token(__tok) } noexcept;
} && copyable<_Token> && equality_comparable<_Token>;

template <class _Token>
concept unstoppable_token = stoppable_token<_Token> && requires(const _Token __tok) {
  requires bool_constant<(!__tok.stop_possible())>::value;
};

struct nostopstate_t {
  explicit nostopstate_t() = default;
};
inline constexpr nostopstate_t nostopstate{};

// [stoptoken]
class stop_token {
  __ycxx::__detail::__shared_stop_state* __state_ = nullptr;

  explicit stop_token(__ycxx::__detail::__shared_stop_state* s) noexcept : __state_(s) {
    __ycxx::__detail::__shared_stop_state::__retain(s);
  }
  friend class stop_source;
  template <class>
  friend class stop_callback;

public:
  template <class _CallbackFn>
  using callback_type = stop_callback<_CallbackFn>;

  stop_token() noexcept = default;
  stop_token(const stop_token& t) noexcept : __state_(t.__state_) { __ycxx::__detail::__shared_stop_state::__retain(__state_); }
  stop_token(stop_token&& t) noexcept : __state_(t.__state_) { t.__state_ = nullptr; }
  stop_token& operator=(const stop_token& t) noexcept {
    stop_token(t).swap(*this);
    return *this;
  }
  stop_token& operator=(stop_token&& t) noexcept {
    stop_token(static_cast<stop_token&&>(t)).swap(*this);
    return *this;
  }
  ~stop_token() { __ycxx::__detail::__shared_stop_state::release(__state_); }

  void swap(stop_token& __rhs) noexcept {
    auto* s = __state_;
    __state_ = __rhs.__state_;
    __rhs.__state_ = s;
  }
  [[nodiscard]] bool stop_requested() const noexcept { return __state_ && __state_->state.stop_requested(); }
  [[nodiscard]] bool stop_possible() const noexcept {
    return __state_ && (__state_->state.stop_requested() || __atomic_load_n(&__state_->__sources, __ATOMIC_ACQUIRE) != 0);
  }
  [[nodiscard]] friend bool operator==(const stop_token& a, const stop_token& b) noexcept { return a.__state_ == b.__state_; }
  friend void swap(stop_token& a, stop_token& b) noexcept { a.swap(b); }
};

// [stopsource]
class stop_source {
  __ycxx::__detail::__shared_stop_state* __state_;

public:
  stop_source() : __state_(new __ycxx::__detail::__shared_stop_state) {}
  explicit stop_source(nostopstate_t) noexcept : __state_(nullptr) {}
  stop_source(const stop_source& s) noexcept : __state_(s.__state_) {
    if (__state_) {
      __ycxx::__detail::__shared_stop_state::__retain(__state_);
      __atomic_fetch_add(&__state_->__sources, 1, __ATOMIC_RELAXED);
    }
  }
  stop_source(stop_source&& s) noexcept : __state_(s.__state_) { s.__state_ = nullptr; }
  stop_source& operator=(const stop_source& s) noexcept {
    stop_source(s).swap(*this);
    return *this;
  }
  stop_source& operator=(stop_source&& s) noexcept {
    stop_source(static_cast<stop_source&&>(s)).swap(*this);
    return *this;
  }
  ~stop_source() {
    if (__state_) {
      __atomic_fetch_sub(&__state_->__sources, 1, __ATOMIC_RELEASE);
      __ycxx::__detail::__shared_stop_state::release(__state_);
    }
  }

  void swap(stop_source& __rhs) noexcept {
    auto* s = __state_;
    __state_ = __rhs.__state_;
    __rhs.__state_ = s;
  }
  [[nodiscard]] stop_token get_token() const noexcept { return stop_token(__state_); }
  [[nodiscard]] bool stop_possible() const noexcept { return __state_ != nullptr; }
  [[nodiscard]] bool stop_requested() const noexcept { return __state_ && __state_->state.stop_requested(); }
  bool request_stop() noexcept { return __state_ && __state_->state.request_stop(); }
  [[nodiscard]] friend bool operator==(const stop_source& a, const stop_source& b) noexcept {
    return a.__state_ == b.__state_;
  }
  friend void swap(stop_source& a, stop_source& b) noexcept { a.swap(b); }
};

// [stopcallback]
template <class _CallbackFn>
class stop_callback : __ycxx::__adl_free::__stop_callback_node {
  static_assert(invocable<_CallbackFn> && destructible<_CallbackFn>,
                "stop_callback: CallbackFn must be invocable and destructible");

  _CallbackFn __callback_fn_;
  __ycxx::__detail::__shared_stop_state* __state_ = nullptr;

  static void run(__ycxx::__detail::__stop_callback_node* n) noexcept {
    static_cast<_CallbackFn&&>(static_cast<stop_callback*>(n)->__callback_fn_)();
  }
  // Registers with st's stop state when a stop is still possible ([stoptoken.concepts]/3.2.1).
  // From an rvalue token (`from` is st), a registration takes over st's reference to the state,
  // which leaves st without one; otherwise st is unchanged.
  void __attach(const stop_token& __st, stop_token* from) noexcept {
    __ycxx::__detail::__shared_stop_state* s = __st.__state_;
    if (!s || (!s->state.stop_requested() && __atomic_load_n(&s->__sources, __ATOMIC_ACQUIRE) == 0))
      return;
    this->invoke = &run;
    if (s->state.add(this)) {
      if (from)
        from->__state_ = nullptr;
      else
        __ycxx::__detail::__shared_stop_state::__retain(s);
      __state_ = s;
    } else {
      static_cast<_CallbackFn&&>(__callback_fn_)(); // a stop was already requested
    }
  }

public:
  using callback_type = _CallbackFn;

  template <class _Initializer>
    requires constructible_from<_CallbackFn, _Initializer>
  explicit stop_callback(const stop_token& __st, _Initializer&& init) noexcept(is_nothrow_constructible_v<_CallbackFn, _Initializer>)
      : __callback_fn_(static_cast<_Initializer&&>(init)) {
    __attach(__st, nullptr);
  }
  template <class _Initializer>
    requires constructible_from<_CallbackFn, _Initializer>
  explicit stop_callback(stop_token&& __st, _Initializer&& init) noexcept(is_nothrow_constructible_v<_CallbackFn, _Initializer>)
      : __callback_fn_(static_cast<_Initializer&&>(init)) {
    __attach(__st, __builtin_addressof(__st));
  }
  ~stop_callback() {
    if (__state_) {
      __state_->state.remove(this);
      __ycxx::__detail::__shared_stop_state::release(__state_);
    }
  }
  stop_callback(const stop_callback&) = delete;
  stop_callback(stop_callback&&) = delete;
  stop_callback& operator=(const stop_callback&) = delete;
  stop_callback& operator=(stop_callback&&) = delete;
};
template <class _CallbackFn>
stop_callback(stop_token, _CallbackFn) -> stop_callback<_CallbackFn>;

// [stoptoken.never]
class never_stop_token {
  struct __callback_type_ {
    explicit __callback_type_(never_stop_token, auto&&) noexcept {}
  };

public:
  template <class>
  using callback_type = __callback_type_;
  static constexpr bool stop_requested() noexcept { return false; }
  static constexpr bool stop_possible() noexcept { return false; }
  bool operator==(const never_stop_token&) const = default;
};

// [stoptoken.inplace]
class inplace_stop_token {
  const inplace_stop_source* __stop_source_ = nullptr;

  constexpr explicit inplace_stop_token(const inplace_stop_source* s) noexcept : __stop_source_(s) {}
  friend class inplace_stop_source;
  template <class>
  friend class inplace_stop_callback;

public:
  template <class _CallbackFn>
  using callback_type = inplace_stop_callback<_CallbackFn>;

  inplace_stop_token() = default;
  bool operator==(const inplace_stop_token&) const = default;
  [[nodiscard]] bool stop_requested() const noexcept;
  [[nodiscard]] bool stop_possible() const noexcept { return __stop_source_ != nullptr; }
  void swap(inplace_stop_token& __rhs) noexcept {
    auto* s = __stop_source_;
    __stop_source_ = __rhs.__stop_source_;
    __rhs.__stop_source_ = s;
  }
  friend void swap(inplace_stop_token& a, inplace_stop_token& b) noexcept { a.swap(b); }
};

// [stopsource.inplace]
class inplace_stop_source {
  mutable __ycxx::__detail::__stop_state __state_;

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
  [[nodiscard]] bool stop_requested() const noexcept { return __state_.stop_requested(); }
  bool request_stop() noexcept { return __state_.request_stop(); }
};

inline bool inplace_stop_token::stop_requested() const noexcept {
  return __stop_source_ != nullptr && __stop_source_->stop_requested();
}

// [stopcallback.inplace]
template <class _CallbackFn>
class inplace_stop_callback : __ycxx::__adl_free::__stop_callback_node {
  static_assert(invocable<_CallbackFn> && destructible<_CallbackFn>,
                "inplace_stop_callback: CallbackFn must be invocable and destructible");

  _CallbackFn __callback_fn_;
  __ycxx::__detail::__stop_state* __state_ = nullptr;

  static void run(__ycxx::__detail::__stop_callback_node* n) noexcept {
    static_cast<_CallbackFn&&>(static_cast<inplace_stop_callback*>(n)->__callback_fn_)();
  }

public:
  using callback_type = _CallbackFn;

  template <class _Initializer>
    requires constructible_from<_CallbackFn, _Initializer>
  explicit inplace_stop_callback(inplace_stop_token __st, _Initializer&& init) noexcept(
      is_nothrow_constructible_v<_CallbackFn, _Initializer>)
      : __callback_fn_(static_cast<_Initializer&&>(init)) {
    if (!__st.__stop_source_)
      return;
    this->invoke = &run;
    __ycxx::__detail::__stop_state& s = __st.__stop_source_->__state_;
    if (s.add(this))
      __state_ = &s;
    else
      static_cast<_CallbackFn&&>(__callback_fn_)();
  }
  ~inplace_stop_callback() {
    if (__state_)
      __state_->remove(this);
  }
  inplace_stop_callback(inplace_stop_callback&&) = delete;
  inplace_stop_callback(const inplace_stop_callback&) = delete;
  inplace_stop_callback& operator=(inplace_stop_callback&&) = delete;
  inplace_stop_callback& operator=(const inplace_stop_callback&) = delete;
};
template <class _CallbackFn>
inplace_stop_callback(inplace_stop_token, _CallbackFn) -> inplace_stop_callback<_CallbackFn>;

} // namespace std
