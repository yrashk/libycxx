// libycxx hosted: futures ([futures]): future_errc and future_category, future_error, promise,
// future, shared_future, packaged_task and async.
//
// The shared state (ycxx::detail::future_state<R>) holds a mutex, a condition variable, the
// result (a value in a union, a reference as a pointer, or nothing) or an exception_ptr, and a
// reference count of the providers and return objects. Kinds of state add behaviour through
// virtual functions: allocator-aware states free themselves through their allocator; a deferred
// state (async with launch::deferred) runs its function in the first non-timed wait; an async
// state owns the thread that computes the result, and joins it when a waiting function sees the
// result ready or when the last reference goes. "At thread exit" results are stored at once and
// made ready by a thread-end action (ycxx::detail::at_thread_exit), which holds a reference.
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

namespace std {

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
  error_code ec_;

public:
  explicit future_error(future_errc e) : logic_error(std::make_error_code(e).message()), ec_(std::make_error_code(e)) {}
  const error_code& code() const noexcept { return ec_; }
  const char* what() const noexcept override { return logic_error::what(); }
};

template <class R>
class future;
template <class R>
class shared_future;
template <class R>
class promise;
template <class>
class packaged_task;

} // namespace std

namespace ycxx::detail {

[[noreturn]] [[gnu::cold]] inline void throw_future_error(std::future_errc e) {
  ::ycxx::detail::raise_with(ycxx_error_future_error, "std::future_error", [e] { return std::future_error(e); });
}

// ---- shared states ------------------------------------------------------------------------------
class future_state_base {
protected:
  // storing: a value is being constructed (without m_ held, so that R's constructor may use
  // the state's future); stored: the result waits for its thread's exit.
  enum class status : unsigned char { empty, storing, stored, ready };

  std::mutex m_;
  std::condition_variable cv_;
  ycxx_pal_u32 refs_ = 1;
  status status_ = status::empty;
  bool retrieved_ = false;
  std::exception_ptr exc_;

  // m_ is held through lk: the result is stored; make it ready now or at thread exit.
  void publish(std::unique_lock<std::mutex>& lk, bool at_thread_exit) {
    if (at_thread_exit) {
      status_ = status::stored;
      retain();
      lk.unlock();
      ::ycxx::detail::at_thread_exit(
          [](void* p) {
            future_state_base* s = static_cast<future_state_base*>(p);
            {
              std::lock_guard<std::mutex> g(s->m_);
              s->status_ = status::ready;
              s->cv_.notify_all();
            }
            s->release();
          },
          this);
      return;
    }
    status_ = status::ready;
    cv_.notify_all();
  }

  // Called (m_ not held) when a waiting function has seen the state ready.
  virtual void on_ready_seen() noexcept {}
  // Called once when the last reference goes, before destroy().
  virtual void on_last_release() noexcept {}

public:
  future_state_base() = default;
  future_state_base(const future_state_base&) = delete;
  future_state_base& operator=(const future_state_base&) = delete;
  virtual ~future_state_base() = default;

  // Frees the state (through its allocator, for allocator-aware states).
  virtual void destroy() noexcept { delete this; }
  // A deferred function not yet started: run it (and make the state ready).
  virtual bool run_deferred() { return false; }
  virtual bool is_deferred() const noexcept { return false; }

  void retain() noexcept { __atomic_fetch_add(&refs_, 1, __ATOMIC_RELAXED); }
  void release() noexcept {
    if (__atomic_fetch_sub(&refs_, 1, __ATOMIC_ACQ_REL) == 1) {
      on_last_release();
      destroy();
    }
  }

  void mark_retrieved() {
    std::lock_guard<std::mutex> g(m_);
    if (retrieved_)
      ::ycxx::detail::throw_future_error(std::future_errc::future_already_retrieved);
    retrieved_ = true;
  }

  void set_exception(std::exception_ptr p, bool at_thread_exit) {
    std::unique_lock<std::mutex> lk(m_);
    if (status_ != status::empty)
      ::ycxx::detail::throw_future_error(std::future_errc::promise_already_satisfied);
    exc_ = static_cast<std::exception_ptr&&>(p);
    publish(lk, at_thread_exit);
  }
  // [futures.state]/7: a provider gives up its state.
  void abandon() noexcept {
    {
      std::unique_lock<std::mutex> lk(m_);
      if (status_ == status::empty) {
        exc_ = std::make_exception_ptr(std::future_error(std::future_errc::broken_promise));
        publish(lk, false);
      }
    }
    release();
  }

  void wait() {
    if (run_deferred())
      return;
    {
      std::unique_lock<std::mutex> lk(m_);
      while (status_ != status::ready)
        cv_.wait(lk);
    }
    on_ready_seen();
  }
  template <class Clock, class Duration>
  std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs) {
    if (is_deferred())
      return std::future_status::deferred;
    {
      std::unique_lock<std::mutex> lk(m_);
      while (status_ != status::ready)
        if (cv_.wait_until(lk, abs) == std::cv_status::timeout && status_ != status::ready)
          return std::future_status::timeout;
    }
    on_ready_seen();
    return std::future_status::ready;
  }

  // After wait(): rethrows a stored exception.
  void rethrow_if_exception() const {
    if (exc_)
      std::rethrow_exception(exc_);
  }
};

template <class R>
class future_state : public future_state_base {
  union {
    R value_;
  };
  bool has_value_ = false;

public:
  future_state() noexcept {}
  ~future_state() override {
    if (has_value_)
      value_.~R();
  }

  // The value is constructed with m_ released (its constructor may wait on this state, for a
  // timeout); the state is claimed first, so a concurrent set_* still fails.
  template <class... A>
  void set_value(bool at_thread_exit, A&&... a) {
    std::unique_lock<std::mutex> lk(m_);
    if (status_ != status::empty)
      ::ycxx::detail::throw_future_error(std::future_errc::promise_already_satisfied);
    status_ = status::storing;
    lk.unlock();
    struct unclaim {
      future_state* s;
      ~unclaim() {
        if (s) {
          std::lock_guard<std::mutex> g(s->m_);
          s->status_ = status::empty;
        }
      }
    } u{this};
    ::new (static_cast<void*>(__builtin_addressof(value_))) R(static_cast<A&&>(a)...);
    u.s = nullptr;
    lk.lock();
    has_value_ = true;
    publish(lk, at_thread_exit);
  }
  R& value() noexcept { return value_; }
};

template <class R>
class future_state<R&> : public future_state_base {
  R* value_ = nullptr;

public:
  void set_value(bool at_thread_exit, R& r) {
    std::unique_lock<std::mutex> lk(m_);
    if (status_ != status::empty)
      ::ycxx::detail::throw_future_error(std::future_errc::promise_already_satisfied);
    value_ = __builtin_addressof(r);
    publish(lk, at_thread_exit);
  }
  R& value() noexcept { return *value_; }
};

template <>
class future_state<void> : public future_state_base {
public:
  void set_value(bool at_thread_exit) {
    std::unique_lock<std::mutex> lk(m_);
    if (status_ != status::empty)
      ::ycxx::detail::throw_future_error(std::future_errc::promise_already_satisfied);
    publish(lk, at_thread_exit);
  }
  void value() noexcept {}
};

// Stores the result of f() (a value or the exception it throws) in s.
template <class R, class F>
void future_set_from(future_state<R>& s, bool at_thread_exit, F&& f) {
  auto store = [&] {
    if constexpr (std::is_void_v<R>) {
      static_cast<F&&>(f)();
      s.set_value(at_thread_exit);
    } else if constexpr (std::is_reference_v<R>) {
      s.set_value(at_thread_exit, static_cast<F&&>(f)());
    } else {
      s.set_value(at_thread_exit, static_cast<F&&>(f)());
    }
  };
  if constexpr (cfg::exceptions) {
    try {
      store();
    } catch (...) {
      if (::ycxx::detail::handling_foreign_exception()) {
        // A forced unwind (pthread_exit, cancellation) ends the thread: the result will never
        // come ([futures.state]/7), and the unwind goes on.
        s.set_exception(std::make_exception_ptr(std::future_error(std::future_errc::broken_promise)), at_thread_exit);
        throw;
      }
      s.set_exception(std::current_exception(), at_thread_exit);
    }
  } else {
    store();
  }
}

// A state whose storage came from an allocator (promise and packaged_task with allocator_arg).
template <class Base, class Alloc>
class future_state_alloc final : public Base {
  using self_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<future_state_alloc>;
  using traits = std::allocator_traits<self_alloc>;
  [[no_unique_address]] self_alloc alloc_;

public:
  template <class... A>
  explicit future_state_alloc(const self_alloc& a, A&&... args) : Base(static_cast<A&&>(args)...), alloc_(a) {}

  static future_state_alloc* create(const Alloc& a) {
    self_alloc sa(a);
    future_state_alloc* p = traits::allocate(sa, 1);
    struct guard {
      self_alloc& a;
      future_state_alloc* p;
      ~guard() {
        if (p)
          traits::deallocate(a, p, 1);
      }
    } g{sa, p};
    ::new (static_cast<void*>(p)) future_state_alloc(sa);
    g.p = nullptr;
    return p;
  }
  void destroy() noexcept override {
    self_alloc a(static_cast<self_alloc&&>(alloc_));
    this->~future_state_alloc();
    traits::deallocate(a, this, 1);
  }
};

// ---- packaged_task states -----------------------------------------------------------------------
template <class R, class... ArgTypes>
class task_state : public future_state<R> {
  bool invoked_ = false; // guarded by m_

protected:
  // [futures.task.members]/23: a second invocation throws before the task runs again.
  void begin() {
    std::lock_guard<std::mutex> g(this->m_);
    if (invoked_)
      ::ycxx::detail::throw_future_error(std::future_errc::promise_already_satisfied);
    invoked_ = true;
  }

public:
  virtual void run(bool at_thread_exit, ArgTypes&&... args) = 0;
  // A new state (from the same allocator) holding this state's task, moved.
  virtual task_state* reset_clone() = 0;
};

template <class F, class Alloc, class R, class... ArgTypes>
class task_state_impl final : public task_state<R, ArgTypes...> {
  using self_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<task_state_impl>;
  using traits = std::allocator_traits<self_alloc>;
  F f_;
  [[no_unique_address]] self_alloc alloc_;

public:
  template <class G>
  task_state_impl(const self_alloc& a, G&& g) : f_(static_cast<G&&>(g)), alloc_(a) {}

  template <class G>
  static task_state_impl* create(const Alloc& a, G&& g) {
    self_alloc sa(a);
    task_state_impl* p = traits::allocate(sa, 1);
    struct guard {
      self_alloc& a;
      task_state_impl* p;
      ~guard() {
        if (p)
          traits::deallocate(a, p, 1);
      }
    } gd{sa, p};
    ::new (static_cast<void*>(p)) task_state_impl(sa, static_cast<G&&>(g));
    gd.p = nullptr;
    return p;
  }
  void run(bool at_thread_exit, ArgTypes&&... args) override {
    this->begin();
    ::ycxx::detail::future_set_from(*this, at_thread_exit, [&]() -> R {
      return ::ycxx::detail::invoke_r<R>(f_, static_cast<ArgTypes&&>(args)...);
    });
  }
  task_state<R, ArgTypes...>* reset_clone() override { return create(Alloc(alloc_), static_cast<F&&>(f_)); }
  void destroy() noexcept override {
    self_alloc a(static_cast<self_alloc&&>(alloc_));
    this->~task_state_impl();
    traits::deallocate(a, this, 1);
  }
};

// ---- async states -------------------------------------------------------------------------------
template <class R, class... T>
class deferred_state final : public future_state<R> {
  std::tuple<T...> fn_;
  bool started_ = false; // guarded by m_

public:
  template <class... U>
  explicit deferred_state(U&&... u) : fn_(static_cast<U&&>(u)...) {}

  bool is_deferred() const noexcept override {
    std::lock_guard<std::mutex> g(const_cast<std::mutex&>(this->m_));
    return !started_;
  }
  bool run_deferred() override {
    {
      std::lock_guard<std::mutex> g(this->m_);
      if (started_)
        return false;
      started_ = true;
    }
    ::ycxx::detail::future_set_from(*this, false, [this]() -> R {
      return [this]<std::size_t... I>(std::index_sequence<I...>) -> R {
        return ::ycxx::detail::invoke(static_cast<T&&>(std::get<I>(fn_))...);
      }(std::index_sequence_for<T...>{});
    });
    return true;
  }
};

template <class R, class... T>
class async_state final : public future_state<R> {
  std::tuple<T...> fn_;
  std::thread thread_;
  std::mutex join_m_;

  void join() noexcept {
    std::lock_guard<std::mutex> g(join_m_);
    if (thread_.joinable())
      thread_.join();
  }
  void on_ready_seen() noexcept override { join(); }
  void on_last_release() noexcept override { join(); }

public:
  template <class... U>
  explicit async_state(U&&... u) : fn_(static_cast<U&&>(u)...) {}
  std::tuple<T...>& fn_ref() noexcept { return fn_; }

  void start() {
    thread_ = std::thread([this] {
      ::ycxx::detail::future_set_from(*this, false, [this]() -> R {
        return [this]<std::size_t... I>(std::index_sequence<I...>) -> R {
          return ::ycxx::detail::invoke(static_cast<T&&>(std::get<I>(fn_))...);
        }(std::index_sequence_for<T...>{});
      });
    });
  }
};

// The return type of shared_future<R>::get ([futures.shared.future]/18).
template <class R>
struct shared_get {
  using type = const R&;
};
template <class R>
struct shared_get<R&> {
  using type = R&;
};
template <>
struct shared_get<void> {
  using type = void;
};

// Grants the return objects access to each other's state.
struct future_access {
  template <class R>
  static future_state<R>* take(std::future<R>& f) noexcept {
    future_state<R>* s = f.state_;
    f.state_ = nullptr;
    return s;
  }
  template <class R>
  static std::future<R> make(future_state<R>* s) noexcept {
    return std::future<R>(s);
  }
};

} // namespace ycxx::detail

namespace std {

// [futures.promise]
template <class R>
class promise {
  ycxx::detail::future_state<R>* state_;

  ycxx::detail::future_state<R>& state() const {
    if (!state_)
      ycxx::detail::throw_future_error(future_errc::no_state);
    return *state_;
  }

public:
  promise() : state_(new ycxx::detail::future_state<R>) {}
  template <class Allocator>
  promise(allocator_arg_t, const Allocator& a)
      : state_(ycxx::detail::future_state_alloc<ycxx::detail::future_state<R>, Allocator>::create(a)) {}
  promise(promise&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  promise(const promise&) = delete;
  ~promise() {
    if (state_)
      state_->abandon();
  }
  promise& operator=(promise&& rhs) noexcept {
    promise(static_cast<promise&&>(rhs)).swap(*this);
    return *this;
  }
  promise& operator=(const promise&) = delete;
  void swap(promise& other) noexcept {
    auto* s = state_;
    state_ = other.state_;
    other.state_ = s;
  }

  future<R> get_future() {
    state().mark_retrieved();
    state_->retain();
    return ycxx::detail::future_access::make(state_);
  }

  void set_value(const R& r)
    requires(!is_reference_v<R> && !is_void_v<R>)
  {
    state().set_value(false, r);
  }
  void set_value(R&& r)
    requires(!is_reference_v<R> && !is_void_v<R>)
  {
    state().set_value(false, static_cast<R&&>(r));
  }
  void set_exception(exception_ptr p) {
    ycxx::detail::precondition(p != nullptr, "promise::set_exception: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), false);
  }
  void set_value_at_thread_exit(const R& r)
    requires(!is_reference_v<R> && !is_void_v<R>)
  {
    state().set_value(true, r);
  }
  void set_value_at_thread_exit(R&& r)
    requires(!is_reference_v<R> && !is_void_v<R>)
  {
    state().set_value(true, static_cast<R&&>(r));
  }
  void set_exception_at_thread_exit(exception_ptr p) {
    ycxx::detail::precondition(p != nullptr, "promise::set_exception_at_thread_exit: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), true);
  }
};

template <class R>
class promise<R&> {
  ycxx::detail::future_state<R&>* state_;

  ycxx::detail::future_state<R&>& state() const {
    if (!state_)
      ycxx::detail::throw_future_error(future_errc::no_state);
    return *state_;
  }

public:
  promise() : state_(new ycxx::detail::future_state<R&>) {}
  template <class Allocator>
  promise(allocator_arg_t, const Allocator& a)
      : state_(ycxx::detail::future_state_alloc<ycxx::detail::future_state<R&>, Allocator>::create(a)) {}
  promise(promise&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  promise(const promise&) = delete;
  ~promise() {
    if (state_)
      state_->abandon();
  }
  promise& operator=(promise&& rhs) noexcept {
    promise(static_cast<promise&&>(rhs)).swap(*this);
    return *this;
  }
  promise& operator=(const promise&) = delete;
  void swap(promise& other) noexcept {
    auto* s = state_;
    state_ = other.state_;
    other.state_ = s;
  }

  future<R&> get_future() {
    state().mark_retrieved();
    state_->retain();
    return ycxx::detail::future_access::make(state_);
  }
  void set_value(R& r) { state().set_value(false, r); }
  void set_exception(exception_ptr p) {
    ycxx::detail::precondition(p != nullptr, "promise::set_exception: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), false);
  }
  void set_value_at_thread_exit(R& r) { state().set_value(true, r); }
  void set_exception_at_thread_exit(exception_ptr p) {
    ycxx::detail::precondition(p != nullptr, "promise::set_exception_at_thread_exit: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), true);
  }
};

template <>
class promise<void> {
  ycxx::detail::future_state<void>* state_;

  ycxx::detail::future_state<void>& state() const {
    if (!state_)
      ycxx::detail::throw_future_error(future_errc::no_state);
    return *state_;
  }

public:
  promise() : state_(new ycxx::detail::future_state<void>) {}
  template <class Allocator>
  promise(allocator_arg_t, const Allocator& a)
      : state_(ycxx::detail::future_state_alloc<ycxx::detail::future_state<void>, Allocator>::create(a)) {}
  promise(promise&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  promise(const promise&) = delete;
  ~promise() {
    if (state_)
      state_->abandon();
  }
  promise& operator=(promise&& rhs) noexcept {
    promise(static_cast<promise&&>(rhs)).swap(*this);
    return *this;
  }
  promise& operator=(const promise&) = delete;
  void swap(promise& other) noexcept {
    auto* s = state_;
    state_ = other.state_;
    other.state_ = s;
  }

  inline future<void> get_future();
  void set_value() { state().set_value(false); }
  void set_exception(exception_ptr p) {
    ycxx::detail::precondition(p != nullptr, "promise::set_exception: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), false);
  }
  void set_value_at_thread_exit() { state().set_value(true); }
  void set_exception_at_thread_exit(exception_ptr p) {
    ycxx::detail::precondition(p != nullptr, "promise::set_exception_at_thread_exit: the exception_ptr is null");
    state().set_exception(static_cast<exception_ptr&&>(p), true);
  }
};

template <class R>
void swap(promise<R>& x, promise<R>& y) noexcept {
  x.swap(y);
}

// [futures.unique.future]
template <class R>
class future {
  ycxx::detail::future_state<R>* state_ = nullptr;

  explicit future(ycxx::detail::future_state<R>* s) noexcept : state_(s) {}
  friend struct ycxx::detail::future_access;

  ycxx::detail::future_state<R>& state() const {
    if (!state_)
      ycxx::detail::throw_future_error(future_errc::no_state);
    return *state_;
  }

public:
  future() noexcept = default;
  future(future&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  future(const future&) = delete;
  ~future() {
    if (state_)
      state_->release();
  }
  future& operator=(const future&) = delete;
  future& operator=(future&& rhs) noexcept {
    if (__builtin_addressof(rhs) != this) {
      if (state_)
        state_->release();
      state_ = rhs.state_;
      rhs.state_ = nullptr;
    }
    return *this;
  }
  shared_future<R> share() noexcept { return shared_future<R>(static_cast<future&&>(*this)); }

  // [futures.unique.future]/16-19: the state is released also when get() throws.
  R get() {
    state().wait();
    struct release_on_exit {
      ycxx::detail::future_state<R>* s;
      ~release_on_exit() { s->release(); }
    } r{state_};
    state_ = nullptr;
    r.s->rethrow_if_exception();
    if constexpr (is_void_v<R>)
      return;
    else if constexpr (is_reference_v<R>)
      return r.s->value();
    else
      return static_cast<R&&>(r.s->value());
  }

  [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }
  void wait() const { state().wait(); }
  template <class Rep, class Period>
  future_status wait_for(const chrono::duration<Rep, Period>& rel_time) const {
    return wait_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  future_status wait_until(const chrono::time_point<Clock, Duration>& abs_time) const {
    return state().wait_until(abs_time);
  }
};

// [futures.shared.future]
template <class R>
class shared_future {
  ycxx::detail::future_state<R>* state_ = nullptr;

  ycxx::detail::future_state<R>& state() const {
    if (!state_)
      ycxx::detail::throw_future_error(future_errc::no_state);
    return *state_;
  }

public:
  shared_future() noexcept = default;
  shared_future(const shared_future& rhs) noexcept : state_(rhs.state_) {
    if (state_)
      state_->retain();
  }
  shared_future(future<R>&& rhs) noexcept : state_(ycxx::detail::future_access::take(rhs)) {}
  shared_future(shared_future&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  ~shared_future() {
    if (state_)
      state_->release();
  }
  shared_future& operator=(const shared_future& rhs) noexcept {
    if (__builtin_addressof(rhs) != this) {
      if (rhs.state_)
        rhs.state_->retain();
      if (state_)
        state_->release();
      state_ = rhs.state_;
    }
    return *this;
  }
  shared_future& operator=(shared_future&& rhs) noexcept {
    if (__builtin_addressof(rhs) != this) {
      if (state_)
        state_->release();
      state_ = rhs.state_;
      rhs.state_ = nullptr;
    }
    return *this;
  }

  typename ycxx::detail::shared_get<R>::type get() const {
    state().wait();
    state_->rethrow_if_exception();
    if constexpr (!is_void_v<R>)
      return state_->value();
  }

  [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }
  void wait() const { state().wait(); }
  template <class Rep, class Period>
  future_status wait_for(const chrono::duration<Rep, Period>& rel_time) const {
    return wait_until(ycxx::detail::steady_deadline(rel_time));
  }
  template <class Clock, class Duration>
  future_status wait_until(const chrono::time_point<Clock, Duration>& abs_time) const {
    return state().wait_until(abs_time);
  }
};

inline future<void> promise<void>::get_future() {
  state().mark_retrieved();
  state_->retain();
  return ycxx::detail::future_access::make(state_);
}

// [futures.task]
template <class R, class... ArgTypes>
class packaged_task<R(ArgTypes...)> {
  ycxx::detail::task_state<R, ArgTypes...>* state_ = nullptr;

  ycxx::detail::task_state<R, ArgTypes...>& state() const {
    if (!state_)
      ycxx::detail::throw_future_error(future_errc::no_state);
    return *state_;
  }

public:
  packaged_task() noexcept = default;
  template <class F>
    requires(!is_same_v<remove_cvref_t<F>, packaged_task>)
  explicit packaged_task(F&& f) : packaged_task(allocator_arg, allocator<int>(), static_cast<F&&>(f)) {}
  template <class F, class Allocator>
    requires(!is_same_v<remove_cvref_t<F>, packaged_task>)
  explicit packaged_task(allocator_arg_t, const Allocator& a, F&& f)
      : state_(ycxx::detail::task_state_impl<decay_t<F>, Allocator, R, ArgTypes...>::create(a, static_cast<F&&>(f))) {
    static_assert(is_invocable_r_v<R, decay_t<F>&, ArgTypes...>,
                  "packaged_task: the task is not invocable with ArgTypes returning R");
  }
  ~packaged_task() {
    if (state_)
      state_->abandon();
  }
  packaged_task(const packaged_task&) = delete;
  packaged_task& operator=(const packaged_task&) = delete;
  packaged_task(packaged_task&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  packaged_task& operator=(packaged_task&& rhs) noexcept {
    packaged_task(static_cast<packaged_task&&>(rhs)).swap(*this);
    return *this;
  }
  void swap(packaged_task& other) noexcept {
    auto* s = state_;
    state_ = other.state_;
    other.state_ = s;
  }
  [[nodiscard]] bool valid() const noexcept { return state_ != nullptr; }

  future<R> get_future() {
    state().mark_retrieved();
    state_->retain();
    return ycxx::detail::future_access::make(static_cast<ycxx::detail::future_state<R>*>(state_));
  }

  void operator()(ArgTypes... args) { state().run(false, static_cast<ArgTypes&&>(args)...); }
  void make_ready_at_thread_exit(ArgTypes... args) { state().run(true, static_cast<ArgTypes&&>(args)...); }
  void reset() {
    ycxx::detail::task_state<R, ArgTypes...>* fresh = state().reset_clone();
    ycxx::detail::task_state<R, ArgTypes...>* old = state_;
    state_ = fresh;
    old->abandon();
  }
};

template <class R, class... ArgTypes>
packaged_task(R (*)(ArgTypes...)) -> packaged_task<R(ArgTypes...)>;
template <class F>
  requires requires { typename ycxx::detail::fw::function_guide<F>::type; }
packaged_task(F) -> packaged_task<typename ycxx::detail::fw::function_guide<F>::type>;

template <class R, class... ArgTypes>
void swap(packaged_task<R(ArgTypes...)>& x, packaged_task<R(ArgTypes...)>& y) noexcept {
  x.swap(y);
}

// [futures.async]
template <class F, class... Args>
[[nodiscard]] future<invoke_result_t<decay_t<F>, decay_t<Args>...>> async(launch policy, F&& f, Args&&... args) {
  static_assert(is_constructible_v<decay_t<F>, F> && (is_constructible_v<decay_t<Args>, Args> && ...),
                "async: the callable and the arguments must be decay-copyable");
  static_assert(is_invocable_v<decay_t<F>, decay_t<Args>...>, "async: the callable is not invocable with the arguments");
  using R = invoke_result_t<decay_t<F>, decay_t<Args>...>;
  if ((policy & launch::async) == launch::async) {
    using S = ycxx::detail::async_state<R, decay_t<F>, decay_t<Args>...>;
    S* s = new S(static_cast<F&&>(f), static_cast<Args&&>(args)...);
    struct guard {
      S* s;
      ~guard() {
        if (s)
          s->destroy();
      }
    } g{s};
    if constexpr (ycxx::detail::cfg::exceptions) {
      if ((policy & launch::deferred) == launch::deferred) {
        try {
          s->start();
        } catch (const system_error&) {
          // No thread could be started: evaluate deferred instead (the copies are in s).
          using D = ycxx::detail::deferred_state<R, decay_t<F>, decay_t<Args>...>;
          D* d = [&]<size_t... I>(index_sequence<I...>) {
            return new D(static_cast<decay_t<F>&&>(get<0>(s->fn_ref())), static_cast<decay_t<Args>&&>(get<I + 1>(s->fn_ref()))...);
          }(index_sequence_for<Args...>{});
          return ycxx::detail::future_access::make(static_cast<ycxx::detail::future_state<R>*>(d));
        }
      } else {
        s->start();
      }
    } else {
      s->start();
    }
    g.s = nullptr;
    return ycxx::detail::future_access::make(static_cast<ycxx::detail::future_state<R>*>(s));
  }
  using D = ycxx::detail::deferred_state<R, decay_t<F>, decay_t<Args>...>;
  return ycxx::detail::future_access::make(
      static_cast<ycxx::detail::future_state<R>*>(new D(static_cast<F&&>(f), static_cast<Args&&>(args)...)));
}
template <class F, class... Args>
[[nodiscard]] future<invoke_result_t<decay_t<F>, decay_t<Args>...>> async(F&& f, Args&&... args) {
  return std::async(launch::async | launch::deferred, static_cast<F&&>(f), static_cast<Args&&>(args)...);
}

} // namespace std
