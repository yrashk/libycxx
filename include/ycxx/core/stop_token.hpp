// libycxx core: stop tokens ([thread.stoptoken]). Core so that the senders of <execution> can use
// them freestanding: the stop state needs only atomics and the PAL's wait, thread identity and
// yield (the freestanding runtime's defaults: one thread, identity 1, yield does nothing).
//
// One stop state implementation (__ycxx::__detail::__stop_state) serves stop_source/stop_token, which
// share a reference-counted heap copy of it, and inplace_stop_source, which contains one. A stop
// state is a word holding the "stop requested" bit and a lock bit for its intrusive list of
// registered callbacks. request_stop runs the callbacks one at a time with the lock released;
// a callback deregistered while it runs on another thread is waited for (an atomic wait on the
// callback's own completion flag), one deregistered from inside its own invocation is not. A
// callback may also destroy the stop state itself (an operation state that owns the source and
// completes from inside the callback, as when_all's or task's children can make it do): the
// destructor tells the running request_stop, which then touches nothing of the state.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// A registered callback (the base of stop_callback and inplace_stop_callback). invoke runs it.
struct __stop_callback_node {
  __stop_callback_node* next = nullptr;
  __stop_callback_node** prev = nullptr; // the pointer to this node; null when not in a list
  void (*invoke)(__stop_callback_node*) noexcept = nullptr;
  bool* __removed = nullptr;   // set by request_stop while the callback runs
  unsigned char done = 0;    // the callback has finished running (atomic, waited on)
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
using __adl_free::__stop_callback_node;

struct __stop_platform {
  using __word = ycxx_pal_u32;
  using __thread_id = ycxx_pal_handle;

  static __thread_id __self() noexcept { return ::ycxx_pal_thread_self(); }
  static void __yield() noexcept { ::ycxx_pal_thread_yield(); }

  static bool __finished(const unsigned char& __done) noexcept {
    return __atomic_load_n(&__done, __ATOMIC_ACQUIRE) != 0;
  }
  static void __finish(unsigned char& __done) noexcept {
    auto* __addr = &__done;
    __atomic_store_n(__addr, 1, __ATOMIC_RELEASE);
    // After publication the callback may be destroyed. The notification only uses its address.
    ::__ycxx::__detail::__atomic_notify(__addr);
  }
  static void __wait(unsigned char& __done) noexcept {
    ::__ycxx::__detail::__atomic_wait_until_done(&__done, [&] { return __finished(__done); });
  }
};

#include <ycxx/core/stop_state.inc>

// The shared stop state of stop_source/stop_token/stop_callback.
struct __shared_stop_state {
  __stop_state state;
  ycxx_pal_u32 __refs = 1;    // every owner: sources, tokens and registered callbacks
  ycxx_pal_u32 __sources = 1; // the stop_source objects

  static void __retain(__shared_stop_state* s) noexcept {
    if (s)
      __atomic_fetch_add(&s->__refs, 1, __ATOMIC_RELAXED);
  }
  static void release(__shared_stop_state* s) noexcept {
    if (s && __atomic_fetch_sub(&s->__refs, 1, __ATOMIC_ACQ_REL) == 1)
      delete s;
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class stop_token;
class stop_source;
template <class _CallbackFn>
class stop_callback;
class inplace_stop_token;
class inplace_stop_source;
template <class _CallbackFn>
class inplace_stop_callback;

#include <ycxx/core/nostopstate.inc>

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
  // [stoptoken.general] declares a defaulted member without const, which [class.compare.default]/1
  // does not allow (a draft defect, STATUS); the const member is what it means.
  [[nodiscard]] bool operator==(const stop_token&) const noexcept = default;
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
  // [stopsource.general]: a defaulted member, as for stop_token.
  [[nodiscard]] bool operator==(const stop_source&) const noexcept = default;
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

#include <ycxx/core/stop_token_traits.inc>

using __inplace_stop_state = ::__ycxx::__detail::__stop_state;
using __inplace_stop_callback_node = ::__ycxx::__adl_free::__stop_callback_node;

#include <ycxx/core/inplace_stop_token.inc>

}} // namespace std
