// libycxx hosted: futures ([futures]): future_errc and future_category, future_error, promise,
// future, shared_future, packaged_task and async.
//
// The shared state (__ycxx::__detail::__future_state<R>) holds a mutex, a condition variable, the
// result (a value in a union, a reference as a pointer, or nothing) or an exception_ptr, and a
// reference count of the providers and return objects. Kinds of state add behaviour through
// virtual functions: allocator-aware states free themselves through their allocator; a deferred
// state (async with launch::deferred) runs its function in the first non-timed wait; an async
// state owns the thread that computes the result, and joins it when a waiting function sees the
// result ready or when the last reference goes. "At thread exit" results are stored at once and
// made ready by a thread-end action (__ycxx::__detail::__at_thread_exit), which holds a reference.
//
// async(launch::async | launch::deferred, ...) starts a thread, and falls back to deferred
// evaluation when no thread can be started.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/exception_ptr.hpp>
#include <ycxx/core/function_wrappers.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/stdexcept.hpp>
#include <ycxx/core/system_error.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/hosted/condition_variable.hpp>
#include <ycxx/hosted/mutex.hpp>
#include <ycxx/hosted/thread.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

enum class future_errc { future_already_retrieved = 1, promise_already_satisfied = 2, no_state = 3, broken_promise = 4 };

enum class launch : unsigned { async = 1, deferred = 2 };
constexpr launch operator&(launch a, launch b) noexcept {
  return static_cast<launch>(static_cast<unsigned>(a) & static_cast<unsigned>(b));
}
constexpr launch operator|(launch a, launch b) noexcept {
  return static_cast<launch>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
constexpr launch operator^(launch a, launch b) noexcept {
  return static_cast<launch>(static_cast<unsigned>(a) ^ static_cast<unsigned>(b));
}
constexpr launch operator~(launch a) noexcept { return static_cast<launch>(~static_cast<unsigned>(a) & 3u); }
constexpr launch& operator&=(launch& a, launch b) noexcept { return a = a & b; }
constexpr launch& operator|=(launch& a, launch b) noexcept { return a = a | b; }
constexpr launch& operator^=(launch& a, launch b) noexcept { return a = a ^ b; }

enum class future_status { ready, timeout, deferred };

// [futures.errors]
template <>
struct is_error_code_enum<future_errc> : public true_type {};
const error_category& future_category() noexcept; // the hosted runtime (src/hosted/future.cpp)
inline error_code make_error_code(future_errc e) noexcept { return error_code(static_cast<int>(e), future_category()); }
inline error_condition make_error_condition(future_errc e) noexcept {
  return error_condition(static_cast<int>(e), future_category());
}

// [futures.future.error]
class future_error : public logic_error {
  error_code __ec_;

public:
  explicit future_error(future_errc e) : logic_error(std::make_error_code(e).message()), __ec_(std::make_error_code(e)) {}
  const error_code& code() const noexcept { return __ec_; }
  const char* what() const noexcept override { return logic_error::what(); }
};

template <class _Rp>
class future;
template <class _Rp>
class shared_future;
template <class _Rp>
class promise;
template <class>
class packaged_task;

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

[[noreturn]] [[__gnu__::__cold__]] inline void __throw_future_error(std::future_errc e) {
  ::__ycxx::__detail::__raise_with(ycxx_error_future_error, "std::future_error", [e] { return std::future_error(e); });
}

// ---- shared states ------------------------------------------------------------------------------
class __future_state_base {
protected:
  // storing: a value is being constructed (without m_ held, so that R's constructor may use
  // the state's future); stored: the result waits for its thread's exit.
  enum class status : unsigned char { empty, __storing, __stored, ready };

  std::mutex __m_;
  std::condition_variable __cv_;
  ycxx_pal_u32 __refs_ = 1;
  status __status_ = status::empty;
  bool __retrieved_ = false;
  std::exception_ptr __exc_;

  // m_ is held: registers the thread-exit action of an "at thread exit" result before the
  // result is stored, so that a failure to register changes nothing. The action holds a
  // reference and makes a stored result ready; it does nothing if no result was stored after
  // all (constructing the value threw).
  void __schedule_ready_at_thread_exit() {
    __retain();
    struct __undo {
      __future_state_base* s;
      ~__undo() {
        if (s) // never the last reference: the caller holds one
          __atomic_fetch_sub(&s->__refs_, 1, __ATOMIC_RELAXED);
      }
    } __u{this};
    ::__ycxx::__detail::__at_thread_exit(
        [](void* p) {
          __future_state_base* s = static_cast<__future_state_base*>(p);
          {
            std::lock_guard<std::mutex> __g(s->__m_);
            if (s->__status_ == status::__stored) {
              s->__status_ = status::ready;
              s->__cv_.notify_all();
            }
          }
          s->release();
        },
        this);
    __u.s = nullptr;
  }

  // m_ is held: the result is stored; make it ready now, or leave it to the action that
  // schedule_ready_at_thread_exit registered.
  void __publish(bool __at_thread_exit) noexcept {
    if (__at_thread_exit) {
      __status_ = status::__stored;
      return;
    }
    __status_ = status::ready;
    __cv_.notify_all();
  }

  // Called (m_ not held) when a waiting function has seen the state ready.
  virtual void __on_ready_seen() noexcept {}
  // Called once when the last reference goes, before destroy().
  virtual void __on_last_release() noexcept {}

public:
  __future_state_base() = default;
  __future_state_base(const __future_state_base&) = delete;
  __future_state_base& operator=(const __future_state_base&) = delete;
  virtual ~__future_state_base() = default;

  // Frees the state (through its allocator, for allocator-aware states).
  virtual void destroy() noexcept { delete this; }
  // A deferred function not yet started: run it (and make the state ready).
  virtual bool __run_deferred() { return false; }
  virtual bool __is_deferred() const noexcept { return false; }

  void __retain() noexcept { __atomic_fetch_add(&__refs_, 1, __ATOMIC_RELAXED); }
  void release() noexcept {
    if (__atomic_fetch_sub(&__refs_, 1, __ATOMIC_ACQ_REL) == 1) {
      __on_last_release();
      destroy();
    }
  }

  void __mark_retrieved() {
    std::lock_guard<std::mutex> __g(__m_);
    if (__retrieved_)
      ::__ycxx::__detail::__throw_future_error(std::future_errc::future_already_retrieved);
    __retrieved_ = true;
  }

  void set_exception(std::exception_ptr p, bool __at_thread_exit) {
    std::unique_lock<std::mutex> __lk(__m_);
    if (__status_ != status::empty)
      ::__ycxx::__detail::__throw_future_error(std::future_errc::promise_already_satisfied);
    if (__at_thread_exit)
      __schedule_ready_at_thread_exit();
    __exc_ = static_cast<std::exception_ptr&&>(p);
    __publish(__at_thread_exit);
  }
  // [futures.state]/7: a provider gives up its state.
  void abandon() noexcept {
    {
      std::unique_lock<std::mutex> __lk(__m_);
      if (__status_ == status::empty) {
        __exc_ = std::make_exception_ptr(std::future_error(std::future_errc::broken_promise));
        __publish(false);
      }
    }
    release();
  }

  void wait() {
    if (__run_deferred())
      return;
    {
      std::unique_lock<std::mutex> __lk(__m_);
      while (__status_ != status::ready)
        __cv_.wait(__lk);
    }
    __on_ready_seen();
  }
  template <class _Clock, class _Duration>
  std::future_status wait_until(const std::chrono::time_point<_Clock, _Duration>& abs) {
    if (__is_deferred())
      return std::future_status::deferred;
    {
      std::unique_lock<std::mutex> __lk(__m_);
      while (__status_ != status::ready)
        if (__cv_.wait_until(__lk, abs) == std::cv_status::timeout && __status_ != status::ready)
          return std::future_status::timeout;
    }
    __on_ready_seen();
    return std::future_status::ready;
  }

  // After wait(): rethrows a stored exception.
  void __rethrow_if_exception() const {
    if (__exc_)
      std::rethrow_exception(__exc_);
  }
};

template <class _Rp>
class __future_state : public __future_state_base {
  union {
    _Rp __value_;
  };
  bool __has_value_ = false;

public:
  __future_state() noexcept {}
  ~__future_state() override {
    if (__has_value_)
      __value_.~_Rp();
  }

  // The value is constructed with m_ released (its constructor may wait on this state, for a
  // timeout); the state is claimed first, so a concurrent set_* still fails.
  template <class... _Ap>
  void set_value(bool __at_thread_exit, _Ap&&... a) {
    std::unique_lock<std::mutex> __lk(__m_);
    if (__status_ != status::empty)
      ::__ycxx::__detail::__throw_future_error(std::future_errc::promise_already_satisfied);
    if (__at_thread_exit)
      __schedule_ready_at_thread_exit();
    __status_ = status::__storing;
    __lk.unlock();
    struct __unclaim {
      __future_state* s;
      ~__unclaim() {
        if (s) {
          std::lock_guard<std::mutex> __g(s->__m_);
          s->__status_ = status::empty;
        }
      }
    } __u{this};
    ::new (static_cast<void*>(__builtin_addressof(__value_))) _Rp(static_cast<_Ap&&>(a)...);
    __u.s = nullptr;
    __lk.lock();
    __has_value_ = true;
    __publish(__at_thread_exit);
  }
  _Rp& value() noexcept { return __value_; }
};

template <class _Rp>
class __future_state<_Rp&> : public __future_state_base {
  _Rp* __value_ = nullptr;

public:
  void set_value(bool __at_thread_exit, _Rp& r) {
    std::unique_lock<std::mutex> __lk(__m_);
    if (__status_ != status::empty)
      ::__ycxx::__detail::__throw_future_error(std::future_errc::promise_already_satisfied);
    if (__at_thread_exit)
      __schedule_ready_at_thread_exit();
    __value_ = __builtin_addressof(r);
    __publish(__at_thread_exit);
  }
  _Rp& value() noexcept { return *__value_; }
};

template <>
class __future_state<void> : public __future_state_base {
public:
  void set_value(bool __at_thread_exit) {
    std::unique_lock<std::mutex> __lk(__m_);
    if (__status_ != status::empty)
      ::__ycxx::__detail::__throw_future_error(std::future_errc::promise_already_satisfied);
    if (__at_thread_exit)
      __schedule_ready_at_thread_exit();
    __publish(__at_thread_exit);
  }
  void value() noexcept {}
};

// Stores the result of f() (a value or the exception it throws) in s.
template <class _Rp, class _Fp>
void __future_set_from(__future_state<_Rp>& s, bool __at_thread_exit, _Fp&& __f) {
  auto store = [&] {
    if constexpr (std::is_void_v<_Rp>) {
      static_cast<_Fp&&>(__f)();
      s.set_value(__at_thread_exit);
    } else if constexpr (std::is_reference_v<_Rp>) {
      s.set_value(__at_thread_exit, static_cast<_Fp&&>(__f)());
    } else {
      s.set_value(__at_thread_exit, static_cast<_Fp&&>(__f)());
    }
  };
  if constexpr (__cfg::exceptions) {
    try {
      store();
    } catch (...) {
      if (::__ycxx::__detail::__handling_foreign_exception()) {
        // A forced unwind (pthread_exit, cancellation) ends the thread: the result will never
        // come ([futures.state]/7), and the unwind goes on.
        s.set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)), __at_thread_exit);
        throw;
      }
      s.set_exception(std::current_exception(), __at_thread_exit);
    }
  } else {
    store();
  }
}

// A state whose storage came from an allocator (promise with allocator_arg).
template <class _Base, class _Alloc>
class __future_state_alloc final : public _Base {
  using __self_alloc = typename std::allocator_traits<_Alloc>::template rebind_alloc<__future_state_alloc>;
  using __traits = std::allocator_traits<__self_alloc>;
  [[no_unique_address]] __self_alloc __alloc_;

public:
  template <class... _Ap>
  explicit __future_state_alloc(const __self_alloc& a, _Ap&&... __args) : _Base(static_cast<_Ap&&>(__args)...), __alloc_(a) {}

  // The allocator's pointer may be a fancy pointer: the state is built at its address.
  static __future_state_alloc* __create(const _Alloc& a) {
    __self_alloc __sa(a);
    typename __traits::pointer __fp = __traits::allocate(__sa, 1);
    struct __guard {
      __self_alloc& a;
      typename __traits::pointer& p;
      bool __armed = true;
      ~__guard() {
        if (__armed)
          __traits::deallocate(a, p, 1);
      }
    } __g{__sa, __fp};
    __future_state_alloc* p = std::to_address(__fp);
    ::new (static_cast<void*>(p)) __future_state_alloc(__sa);
    __g.__armed = false;
    return p;
  }
  void destroy() noexcept override {
    __self_alloc a(static_cast<__self_alloc&&>(__alloc_));
    typename __traits::pointer __fp = std::pointer_traits<typename __traits::pointer>::pointer_to(*this);
    this->~__future_state_alloc();
    __traits::deallocate(a, __fp, 1);
  }
};

// ---- packaged_task states -----------------------------------------------------------------------
template <class _Rp, class... _ArgTypes>
class __task_state : public __future_state<_Rp> {
  bool __invoked_ = false; // guarded by m_

protected:
  // [futures.task.members]/23: a second invocation throws before the task runs again.
  void begin() {
    std::lock_guard<std::mutex> __g(this->__m_);
    if (__invoked_)
      ::__ycxx::__detail::__throw_future_error(std::future_errc::promise_already_satisfied);
    __invoked_ = true;
  }

public:
  virtual void run(bool __at_thread_exit, _ArgTypes&&... __args) = 0;
  // A new state (from the same allocator) holding this state's task, moved.
  virtual __task_state* __reset_clone() = 0;
};

// The task's state, in storage from the packaged_task's allocator (rebound; its pointer may be a
// fancy pointer), which reset() reuses ([futures.task.members]/28).
template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
class __task_state_impl final : public __task_state<_Rp, _ArgTypes...> {
  using __self_alloc = typename std::allocator_traits<_Alloc>::template rebind_alloc<__task_state_impl>;
  using __traits = std::allocator_traits<__self_alloc>;
  _Fp __f_;
  [[no_unique_address]] __self_alloc __alloc_;

public:
  template <class _Gp>
  __task_state_impl(const __self_alloc& a, _Gp&& __g) : __f_(static_cast<_Gp&&>(__g)), __alloc_(a) {}

  template <class _Gp>
  static __task_state_impl* __create(const __self_alloc& a, _Gp&& __g) {
    __self_alloc __sa(a);
    typename __traits::pointer __fp = __traits::allocate(__sa, 1);
    struct __guard {
      __self_alloc& a;
      typename __traits::pointer& p;
      bool __armed = true;
      ~__guard() {
        if (__armed)
          __traits::deallocate(a, p, 1);
      }
    } __gd{__sa, __fp};
    __task_state_impl* p = std::to_address(__fp);
    ::new (static_cast<void*>(p)) __task_state_impl(__sa, static_cast<_Gp&&>(__g));
    __gd.__armed = false;
    return p;
  }
  void run(bool __at_thread_exit, _ArgTypes&&... __args) override {
    this->begin();
    ::__ycxx::__detail::__future_set_from(*this, __at_thread_exit, [&]() -> _Rp {
      return ::__ycxx::__detail::invoke_r<_Rp>(__f_, static_cast<_ArgTypes&&>(__args)...);
    });
  }
  __task_state<_Rp, _ArgTypes...>* __reset_clone() override { return __create(__alloc_, static_cast<_Fp&&>(__f_)); }
  void destroy() noexcept override {
    __self_alloc a(static_cast<__self_alloc&&>(__alloc_));
    typename __traits::pointer __fp = std::pointer_traits<typename __traits::pointer>::pointer_to(*this);
    this->~__task_state_impl();
    __traits::deallocate(a, __fp, 1);
  }
};

// ---- async states -------------------------------------------------------------------------------
template <class _Rp, class... _Tp>
class __deferred_state final : public __future_state<_Rp> {
  std::tuple<_Tp...> __fn_;
  bool __started_ = false; // guarded by m_

public:
  template <class... _Up>
  explicit __deferred_state(_Up&&... __u) : __fn_(static_cast<_Up&&>(__u)...) {}

  bool __is_deferred() const noexcept override {
    std::lock_guard<std::mutex> __g(const_cast<std::mutex&>(this->__m_));
    return !__started_;
  }
  bool __run_deferred() override {
    {
      std::lock_guard<std::mutex> __g(this->__m_);
      if (__started_)
        return false;
      __started_ = true;
    }
    ::__ycxx::__detail::__future_set_from(*this, false, [this]() -> _Rp {
      return [this]<std::size_t... _Ip>(std::index_sequence<_Ip...>) -> _Rp {
        return ::__ycxx::__detail::invoke(static_cast<_Tp&&>(std::get<_Ip>(__fn_))...);
      }(std::index_sequence_for<_Tp...>{});
    });
    return true;
  }
};

template <class _Rp, class... _Tp>
class __async_state final : public __future_state<_Rp> {
  std::tuple<_Tp...> __fn_;
  std::thread __thread_;
  std::mutex __join_m_;

  void join() noexcept {
    std::lock_guard<std::mutex> __g(__join_m_);
    if (__thread_.joinable())
      __thread_.join();
  }
  void __on_ready_seen() noexcept override { join(); }
  void __on_last_release() noexcept override { join(); }

public:
  template <class... _Up>
  explicit __async_state(_Up&&... __u) : __fn_(static_cast<_Up&&>(__u)...) {}
  std::tuple<_Tp...>& __fn_ref() noexcept { return __fn_; }

  void start() {
    __thread_ = std::thread([this] {
      ::__ycxx::__detail::__future_set_from(*this, false, [this]() -> _Rp {
        return [this]<std::size_t... _Ip>(std::index_sequence<_Ip...>) -> _Rp {
          return ::__ycxx::__detail::invoke(static_cast<_Tp&&>(std::get<_Ip>(__fn_))...);
        }(std::index_sequence_for<_Tp...>{});
      });
    });
  }
};

// The return type of shared_future<R>::get ([futures.shared.future]/18).
template <class _Rp>
struct __shared_get {
  using type = const _Rp&;
};
template <class _Rp>
struct __shared_get<_Rp&> {
  using type = _Rp&;
};
template <>
struct __shared_get<void> {
  using type = void;
};

// Grants the return objects access to each other's state.
struct __future_access {
  template <class _Rp>
  static __future_state<_Rp>* take(std::future<_Rp>& __f) noexcept {
    __future_state<_Rp>* s = __f.__state_;
    __f.__state_ = nullptr;
    return s;
  }
  template <class _Rp>
  static std::future<_Rp> __make(__future_state<_Rp>* s) noexcept {
    return std::future<_Rp>(s);
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [futures.promise]
template <class _Rp>
class promise {
  __ycxx::__detail::__future_state<_Rp>* __state_;

  __ycxx::__detail::__future_state<_Rp>& state() const {
    if (!__state_)
      __ycxx::__detail::__throw_future_error(future_errc::no_state);
    return *__state_;
  }

public:
  promise() : __state_(new __ycxx::__detail::__future_state<_Rp>) {}
  template <class _Allocator>
  promise(allocator_arg_t, const _Allocator& a)
      : __state_(__ycxx::__detail::__future_state_alloc<__ycxx::__detail::__future_state<_Rp>, _Allocator>::__create(a)) {}
  promise(promise&& __rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  promise(const promise&) = delete;
  ~promise() {
    if (__state_)
      __state_->abandon();
  }
  promise& operator=(promise&& __rhs) noexcept {
    promise(static_cast<promise&&>(__rhs)).swap(*this);
    return *this;
  }
  promise& operator=(const promise&) = delete;
  void swap(promise& other) noexcept {
    auto* s = __state_;
    __state_ = other.__state_;
    other.__state_ = s;
  }

  future<_Rp> get_future() {
    state().__mark_retrieved();
    __state_->__retain();
    return __ycxx::__detail::__future_access::__make(__state_);
  }

  void set_value(const _Rp& r)
    requires(!is_reference_v<_Rp> && !is_void_v<_Rp>)
  {
    state().set_value(false, r);
  }
  void set_value(_Rp&& r)
    requires(!is_reference_v<_Rp> && !is_void_v<_Rp>)
  {
    state().set_value(false, static_cast<_Rp&&>(r));
  }
  void set_exception(exception_ptr p) {
    __ycxx::__detail::__precondition(p != nullptr, "promise::set_exception: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), false);
  }
  void set_value_at_thread_exit(const _Rp& r)
    requires(!is_reference_v<_Rp> && !is_void_v<_Rp>)
  {
    state().set_value(true, r);
  }
  void set_value_at_thread_exit(_Rp&& r)
    requires(!is_reference_v<_Rp> && !is_void_v<_Rp>)
  {
    state().set_value(true, static_cast<_Rp&&>(r));
  }
  void set_exception_at_thread_exit(exception_ptr p) {
    __ycxx::__detail::__precondition(p != nullptr, "promise::set_exception_at_thread_exit: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), true);
  }
};

template <class _Rp>
class promise<_Rp&> {
  __ycxx::__detail::__future_state<_Rp&>* __state_;

  __ycxx::__detail::__future_state<_Rp&>& state() const {
    if (!__state_)
      __ycxx::__detail::__throw_future_error(future_errc::no_state);
    return *__state_;
  }

public:
  promise() : __state_(new __ycxx::__detail::__future_state<_Rp&>) {}
  template <class _Allocator>
  promise(allocator_arg_t, const _Allocator& a)
      : __state_(__ycxx::__detail::__future_state_alloc<__ycxx::__detail::__future_state<_Rp&>, _Allocator>::__create(a)) {}
  promise(promise&& __rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  promise(const promise&) = delete;
  ~promise() {
    if (__state_)
      __state_->abandon();
  }
  promise& operator=(promise&& __rhs) noexcept {
    promise(static_cast<promise&&>(__rhs)).swap(*this);
    return *this;
  }
  promise& operator=(const promise&) = delete;
  void swap(promise& other) noexcept {
    auto* s = __state_;
    __state_ = other.__state_;
    other.__state_ = s;
  }

  future<_Rp&> get_future() {
    state().__mark_retrieved();
    __state_->__retain();
    return __ycxx::__detail::__future_access::__make(__state_);
  }
  void set_value(_Rp& r) { state().set_value(false, r); }
  void set_exception(exception_ptr p) {
    __ycxx::__detail::__precondition(p != nullptr, "promise::set_exception: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), false);
  }
  void set_value_at_thread_exit(_Rp& r) { state().set_value(true, r); }
  void set_exception_at_thread_exit(exception_ptr p) {
    __ycxx::__detail::__precondition(p != nullptr, "promise::set_exception_at_thread_exit: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), true);
  }
};

template <>
class promise<void> {
  __ycxx::__detail::__future_state<void>* __state_;

  __ycxx::__detail::__future_state<void>& state() const {
    if (!__state_)
      __ycxx::__detail::__throw_future_error(future_errc::no_state);
    return *__state_;
  }

public:
  promise() : __state_(new __ycxx::__detail::__future_state<void>) {}
  template <class _Allocator>
  promise(allocator_arg_t, const _Allocator& a)
      : __state_(__ycxx::__detail::__future_state_alloc<__ycxx::__detail::__future_state<void>, _Allocator>::__create(a)) {}
  promise(promise&& __rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  promise(const promise&) = delete;
  ~promise() {
    if (__state_)
      __state_->abandon();
  }
  promise& operator=(promise&& __rhs) noexcept {
    promise(static_cast<promise&&>(__rhs)).swap(*this);
    return *this;
  }
  promise& operator=(const promise&) = delete;
  void swap(promise& other) noexcept {
    auto* s = __state_;
    __state_ = other.__state_;
    other.__state_ = s;
  }

  inline future<void> get_future();
  void set_value() { state().set_value(false); }
  void set_exception(exception_ptr p) {
    __ycxx::__detail::__precondition(p != nullptr, "promise::set_exception: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), false);
  }
  void set_value_at_thread_exit() { state().set_value(true); }
  void set_exception_at_thread_exit(exception_ptr p) {
    __ycxx::__detail::__precondition(p != nullptr, "promise::set_exception_at_thread_exit: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), true);
  }
};

template <class _Rp>
void swap(promise<_Rp>& __x, promise<_Rp>& y) noexcept {
  __x.swap(y);
}

// [futures.unique.future]
template <class _Rp>
class future {
  __ycxx::__detail::__future_state<_Rp>* __state_ = nullptr;

  explicit future(__ycxx::__detail::__future_state<_Rp>* s) noexcept : __state_(s) {}
  friend struct __ycxx::__detail::__future_access;

  __ycxx::__detail::__future_state<_Rp>& state() const {
    if (!__state_)
      __ycxx::__detail::__throw_future_error(future_errc::no_state);
    return *__state_;
  }

public:
  future() noexcept = default;
  future(future&& __rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  future(const future&) = delete;
  ~future() {
    if (__state_)
      __state_->release();
  }
  future& operator=(const future&) = delete;
  future& operator=(future&& __rhs) noexcept {
    if (__builtin_addressof(__rhs) != this) {
      if (__state_)
        __state_->release();
      __state_ = __rhs.__state_;
      __rhs.__state_ = nullptr;
    }
    return *this;
  }
  shared_future<_Rp> share() noexcept { return shared_future<_Rp>(static_cast<future&&>(*this)); }

  // [futures.unique.future]/16-19: the state is released also when get() throws.
  _Rp get() {
    state().wait();
    struct __release_on_exit {
      __ycxx::__detail::__future_state<_Rp>* s;
      ~__release_on_exit() { s->release(); }
    } r{__state_};
    __state_ = nullptr;
    r.s->__rethrow_if_exception();
    if constexpr (is_void_v<_Rp>)
      return;
    else if constexpr (is_reference_v<_Rp>)
      return r.s->value();
    else
      return static_cast<_Rp&&>(r.s->value());
  }

  [[nodiscard]] bool valid() const noexcept { return __state_ != nullptr; }
  void wait() const { state().wait(); }
  template <class _Rep, class _Period>
  future_status wait_for(const chrono::duration<_Rep, _Period>& __rel_time) const {
    return wait_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  future_status wait_until(const chrono::time_point<_Clock, _Duration>& __abs_time) const {
    return state().wait_until(__abs_time);
  }
};

// [futures.shared.future]
template <class _Rp>
class shared_future {
  __ycxx::__detail::__future_state<_Rp>* __state_ = nullptr;

  __ycxx::__detail::__future_state<_Rp>& state() const {
    if (!__state_)
      __ycxx::__detail::__throw_future_error(future_errc::no_state);
    return *__state_;
  }

public:
  shared_future() noexcept = default;
  shared_future(const shared_future& __rhs) noexcept : __state_(__rhs.__state_) {
    if (__state_)
      __state_->__retain();
  }
  shared_future(future<_Rp>&& __rhs) noexcept : __state_(__ycxx::__detail::__future_access::take(__rhs)) {}
  shared_future(shared_future&& __rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  ~shared_future() {
    if (__state_)
      __state_->release();
  }
  shared_future& operator=(const shared_future& __rhs) noexcept {
    if (__builtin_addressof(__rhs) != this) {
      if (__rhs.__state_)
        __rhs.__state_->__retain();
      if (__state_)
        __state_->release();
      __state_ = __rhs.__state_;
    }
    return *this;
  }
  shared_future& operator=(shared_future&& __rhs) noexcept {
    if (__builtin_addressof(__rhs) != this) {
      if (__state_)
        __state_->release();
      __state_ = __rhs.__state_;
      __rhs.__state_ = nullptr;
    }
    return *this;
  }

  typename __ycxx::__detail::__shared_get<_Rp>::type get() const {
    state().wait();
    __state_->__rethrow_if_exception();
    if constexpr (!is_void_v<_Rp>)
      return __state_->value();
  }

  [[nodiscard]] bool valid() const noexcept { return __state_ != nullptr; }
  void wait() const { state().wait(); }
  template <class _Rep, class _Period>
  future_status wait_for(const chrono::duration<_Rep, _Period>& __rel_time) const {
    return wait_until(__ycxx::__detail::__steady_deadline(__rel_time));
  }
  template <class _Clock, class _Duration>
  future_status wait_until(const chrono::time_point<_Clock, _Duration>& __abs_time) const {
    return state().wait_until(__abs_time);
  }
};

inline future<void> promise<void>::get_future() {
  state().__mark_retrieved();
  __state_->__retain();
  return __ycxx::__detail::__future_access::__make(__state_);
}

// [futures.task]
template <class _Rp, class... _ArgTypes>
class packaged_task<_Rp(_ArgTypes...)> {
  __ycxx::__detail::__task_state<_Rp, _ArgTypes...>* __state_ = nullptr;

  __ycxx::__detail::__task_state<_Rp, _ArgTypes...>& state() const {
    if (!__state_)
      __ycxx::__detail::__throw_future_error(future_errc::no_state);
    return *__state_;
  }

public:
  packaged_task() noexcept = default;
  template <class _Fp>
    requires(!is_same_v<remove_cvref_t<_Fp>, packaged_task>)
  explicit packaged_task(_Fp&& __f) : packaged_task(allocator_arg, allocator<int>(), static_cast<_Fp&&>(__f)) {}
  template <class _Fp, class _Allocator>
    requires(!is_same_v<remove_cvref_t<_Fp>, packaged_task>)
  explicit packaged_task(allocator_arg_t, const _Allocator& a, _Fp&& __f)
      : __state_(__ycxx::__detail::__task_state_impl<decay_t<_Fp>, _Allocator, _Rp, _ArgTypes...>::__create(
            typename allocator_traits<_Allocator>::template rebind_alloc<
                __ycxx::__detail::__task_state_impl<decay_t<_Fp>, _Allocator, _Rp, _ArgTypes...>>(a),
            static_cast<_Fp&&>(__f))) {
    static_assert(is_invocable_r_v<_Rp, decay_t<_Fp>&, _ArgTypes...>,
                  "packaged_task: the task is not invocable with ArgTypes returning R");
  }
  ~packaged_task() {
    if (__state_)
      __state_->abandon();
  }
  packaged_task(const packaged_task&) = delete;
  packaged_task& operator=(const packaged_task&) = delete;
  packaged_task(packaged_task&& __rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  packaged_task& operator=(packaged_task&& __rhs) noexcept {
    packaged_task(static_cast<packaged_task&&>(__rhs)).swap(*this);
    return *this;
  }
  void swap(packaged_task& other) noexcept {
    auto* s = __state_;
    __state_ = other.__state_;
    other.__state_ = s;
  }
  [[nodiscard]] bool valid() const noexcept { return __state_ != nullptr; }

  future<_Rp> get_future() {
    state().__mark_retrieved();
    __state_->__retain();
    return __ycxx::__detail::__future_access::__make(static_cast<__ycxx::__detail::__future_state<_Rp>*>(__state_));
  }

  void operator()(_ArgTypes... __args) { state().run(false, static_cast<_ArgTypes&&>(__args)...); }
  void make_ready_at_thread_exit(_ArgTypes... __args) { state().run(true, static_cast<_ArgTypes&&>(__args)...); }
  void reset() {
    __ycxx::__detail::__task_state<_Rp, _ArgTypes...>* __fresh = state().__reset_clone();
    __ycxx::__detail::__task_state<_Rp, _ArgTypes...>* __old = __state_;
    __state_ = __fresh;
    __old->abandon();
  }
};

template <class _Rp, class... _ArgTypes>
packaged_task(_Rp (*)(_ArgTypes...)) -> packaged_task<_Rp(_ArgTypes...)>;
template <class _Fp>
  requires requires { typename __ycxx::__detail::__fw::__function_guide<_Fp>::type; }
packaged_task(_Fp) -> packaged_task<typename __ycxx::__detail::__fw::__function_guide<_Fp>::type>;

template <class _Rp, class... _ArgTypes>
void swap(packaged_task<_Rp(_ArgTypes...)>& __x, packaged_task<_Rp(_ArgTypes...)>& y) noexcept {
  __x.swap(y);
}

// [futures.async]
template <class _Fp, class... _Args>
[[nodiscard]] future<invoke_result_t<decay_t<_Fp>, decay_t<_Args>...>> async(launch __policy, _Fp&& __f, _Args&&... __args) {
  static_assert(is_constructible_v<decay_t<_Fp>, _Fp> && (is_constructible_v<decay_t<_Args>, _Args> && ...),
                "async: the callable and the arguments must be decay-copyable");
  static_assert(is_invocable_v<decay_t<_Fp>, decay_t<_Args>...>, "async: the callable is not invocable with the arguments");
  using _Rp = invoke_result_t<decay_t<_Fp>, decay_t<_Args>...>;
  if ((__policy & launch::async) == launch::async) {
    using _Sp = __ycxx::__detail::__async_state<_Rp, decay_t<_Fp>, decay_t<_Args>...>;
    _Sp* s = new _Sp(static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...);
    struct __guard {
      _Sp* s;
      ~__guard() {
        if (s)
          s->destroy();
      }
    } __g{s};
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      if ((__policy & launch::deferred) == launch::deferred) {
        try {
          s->start();
        } catch (const system_error&) {
          // No thread could be started: evaluate deferred instead (the copies are in s).
          using _Dp = __ycxx::__detail::__deferred_state<_Rp, decay_t<_Fp>, decay_t<_Args>...>;
          _Dp* d = [&]<size_t... _Ip>(index_sequence<_Ip...>) {
            return new _Dp(static_cast<decay_t<_Fp>&&>(get<0>(s->__fn_ref())), static_cast<decay_t<_Args>&&>(get<_Ip + 1>(s->__fn_ref()))...);
          }(index_sequence_for<_Args...>{});
          return __ycxx::__detail::__future_access::__make(static_cast<__ycxx::__detail::__future_state<_Rp>*>(d));
        }
      } else {
        s->start();
      }
    } else {
      s->start();
    }
    __g.s = nullptr;
    return __ycxx::__detail::__future_access::__make(static_cast<__ycxx::__detail::__future_state<_Rp>*>(s));
  }
  using _Dp = __ycxx::__detail::__deferred_state<_Rp, decay_t<_Fp>, decay_t<_Args>...>;
  return __ycxx::__detail::__future_access::__make(
      static_cast<__ycxx::__detail::__future_state<_Rp>*>(new _Dp(static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...)));
}
template <class _Fp, class... _Args>
[[nodiscard]] future<invoke_result_t<decay_t<_Fp>, decay_t<_Args>...>> async(_Fp&& __f, _Args&&... __args) {
  return std::async(launch::async | launch::deferred, static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...);
}

}} // namespace std
