// libycxx hosted: thread, jthread and this_thread ([thread.threads]).
//
// A thread's identity (thread::id) and its native handle are the PAL's thread handle (pthread_t
// on POSIX). The constructor decay-copies the callable and its arguments into a heap state in the
// constructing thread and hands that to the runtime (ycxx::detail::thread_start), which starts
// the PAL thread through a trampoline that applies the name hint and then runs the state.
//
// operator<< and formatter<thread::id> are in ycxx/hosted/thread_format.hpp.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/chrono_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/exception.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/hosted/chrono_clocks.hpp>
#include <ycxx/core/stop_token.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/pal.h>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// The hosted runtime (src/hosted/thread.cpp): starts a thread running run(arg) after naming it
// (name, not necessarily null-terminated, is copied first; null for none). Throws system_error
// if no thread can be started.
ycxx_pal_handle thread_start(void (*run)(void*), void* arg, std::size_t stack_size, const char* name,
                             std::size_t name_size);

struct thread_access;

// The state a new thread runs: the decayed callable and arguments, invoked as rvalues.
template <class... T>
struct thread_state {
  std::tuple<T...> values;

  template <class... U>
  explicit thread_state(U&&... u) : values(static_cast<U&&>(u)...) {}

  // [thread.thread.constr]/6: an exception from the invocation calls terminate. A foreign
  // exception (the forced unwind of pthread_exit or thread cancellation) passes through, so
  // that the thread ends as the platform intends; the state is destroyed either way.
  static void run(void* p) {
    struct owner {
      thread_state* s;
      ~owner() { delete s; }
    } o{static_cast<thread_state*>(p)};
    auto body = [s = o.s]<std::size_t... I>(std::index_sequence<I...>) {
      ::ycxx::detail::invoke(static_cast<T&&>(std::get<I>(s->values))...);
    };
    if constexpr (cfg::exceptions) {
      try {
        body(std::index_sequence_for<T...>{});
      } catch (...) {
        if (::ycxx::detail::handling_foreign_exception())
          throw;
        std::terminate();
      }
    } else {
      body(std::index_sequence_for<T...>{});
    }
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

class jthread;

// [thread.thread.class]
class thread {
public:
  class id;
  template <same_as<char> T>
  class name_hint;
  template <class T>
  name_hint(const T*) -> name_hint<T>;
  template <class T>
  name_hint(basic_string<T>) -> name_hint<T>;
  class stack_size_hint;
  using native_handle_type = ycxx_pal_handle;

  thread() noexcept = default;
  template <class... Args>
    requires(sizeof...(Args) > 0) && (!is_same_v<remove_cvref_t<tuple_element_t<0, tuple<Args...>>>, thread>)
  explicit thread(Args&&... args);
  ~thread() {
    if (joinable())
      std::terminate();
  }
  thread(const thread&) = delete;
  thread(thread&& x) noexcept : handle_(x.handle_) { x.handle_ = 0; }
  thread& operator=(const thread&) = delete;
  thread& operator=(thread&& x) noexcept {
    if (joinable())
      std::terminate();
    handle_ = x.handle_;
    x.handle_ = 0;
    return *this;
  }

  void swap(thread& x) noexcept {
    native_handle_type h = handle_;
    handle_ = x.handle_;
    x.handle_ = h;
  }
  [[nodiscard]] bool joinable() const noexcept { return handle_ != 0; }
  void join();
  void detach();
  [[nodiscard]] id get_id() const noexcept;
  [[nodiscard]] native_handle_type native_handle() { return handle_; }
  [[nodiscard]] static unsigned int hardware_concurrency() noexcept { return ::ycxx_pal_hardware_concurrency(); }

private:
  native_handle_type handle_ = 0;
  friend class jthread;
};

// [thread.thread.id]
class thread::id {
  ycxx_pal_handle handle_ = 0;

  constexpr explicit id(ycxx_pal_handle h) noexcept : handle_(h) {}
  friend class thread;
  friend struct ycxx::detail::thread_access;
  friend bool operator==(thread::id x, thread::id y) noexcept;
  friend strong_ordering operator<=>(thread::id x, thread::id y) noexcept;
  friend struct hash<thread::id>;

public:
  id() noexcept = default;
};

inline bool operator==(thread::id x, thread::id y) noexcept { return x.handle_ == y.handle_; }
inline strong_ordering operator<=>(thread::id x, thread::id y) noexcept { return x.handle_ <=> y.handle_; }

template <>
struct hash<thread::id> {
  size_t operator()(thread::id x) const noexcept { return static_cast<size_t>(x.handle_); }
};

// [thread.attributes]
template <same_as<char> T>
class thread::name_hint {
  basic_string_view<T> name_;
  friend struct ycxx::detail::thread_access;

public:
  constexpr explicit name_hint(basic_string_view<T> n) noexcept : name_(n) {}
  name_hint(name_hint&&) = delete;
  name_hint(const name_hint&) = delete;
};

class thread::stack_size_hint {
  size_t size_;
  friend struct ycxx::detail::thread_access;

public:
  constexpr explicit stack_size_hint(size_t s) noexcept : size_(s) {}
};

inline void swap(thread& x, thread& y) noexcept { x.swap(y); }

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
inline constexpr bool is_thread_attribute = false;
template <>
inline constexpr bool is_thread_attribute<std::thread::stack_size_hint> = true;
template <class T>
inline constexpr bool is_thread_attribute<std::thread::name_hint<T>> = true;

// The index of the first argument whose decayed type is not a thread attribute.
template <class... Args>
consteval std::size_t thread_callable_index() {
  constexpr bool attr[] = {is_thread_attribute<std::decay_t<Args>>..., false};
  std::size_t i = 0;
  while (i < sizeof...(Args) && attr[i])
    ++i;
  return i;
}

template <class... Attrs>
consteval bool thread_attributes_distinct() {
  using L = std::tuple<std::remove_cvref_t<Attrs>...>;
  bool ok = true;
  [&]<std::size_t... I>(std::index_sequence<I...>) {
    (
        [&] {
          constexpr std::size_t i = I;
          [&]<std::size_t... J>(std::index_sequence<J...>) {
            ((ok = ok && (J == i || !std::is_same_v<std::tuple_element_t<i, L>, std::tuple_element_t<J, L>>)), ...);
          }(std::index_sequence_for<Attrs...>{});
        }(),
        ...);
  }(std::index_sequence_for<Attrs...>{});
  return ok;
}

struct thread_access {
  static constexpr std::thread::id make_id(ycxx_pal_handle h) noexcept { return std::thread::id(h); }
  static constexpr ycxx_pal_handle handle_of(std::thread::id i) noexcept { return i.handle_; }

  struct attributes {
    std::size_t stack = 0;
    const char* name = nullptr;
    std::size_t name_size = 0;
  };
  static void apply(attributes& a, const std::thread::stack_size_hint& h) noexcept { a.stack = h.size_; }
  static void apply(attributes& a, const std::thread::name_hint<char>& h) noexcept {
    a.name = h.name_.data();
    a.name_size = h.name_.size();
  }

  // Deletes the state unless the thread took it.
  template <class S>
  struct state_guard {
    S* s;
    ~state_guard() { delete s; }
  };

  // [thread.thread.constr]/4-6, [thread.jthread.cons]/4-6: starts the thread for the arguments
  // of a thread or jthread constructor; ss is the jthread's stop source (null for thread).
  template <bool Jthread, class... Args>
  static ycxx_pal_handle start(const std::stop_source* ss, Args&&... args) {
    constexpr std::size_t n = sizeof...(Args);
    constexpr std::size_t i = ::ycxx::detail::thread_callable_index<Args...>();
    static_assert(i < n, "thread: the arguments are all thread attributes; the callable is missing");
    using all = std::tuple<Args&&...>;
    all refs(static_cast<Args&&>(args)...);
    attributes attrs;
    [&]<std::size_t... J>(std::index_sequence<J...>) {
      static_assert(::ycxx::detail::thread_attributes_distinct<std::tuple_element_t<J, all>...>(),
                    "thread: a thread attribute type is given more than once");
      (thread_access::apply(attrs, std::get<J>(refs)), ...);
    }(std::make_index_sequence<i>{});
    return [&]<std::size_t... K>(std::index_sequence<K...>) {
      using F = std::tuple_element_t<i, all>;
      static_assert(std::is_constructible_v<std::decay_t<F>, F>, "thread: the callable cannot be decay-copied");
      static_assert((std::is_constructible_v<std::decay_t<std::tuple_element_t<i + 1 + K, all>>,
                                             std::tuple_element_t<i + 1 + K, all>> &&
                     ...),
                    "thread: an argument cannot be decay-copied");
      constexpr bool with_token =
          Jthread &&
          std::is_invocable_v<std::decay_t<F>, std::stop_token, std::decay_t<std::tuple_element_t<i + 1 + K, all>>...>;
      static_assert(with_token ||
                        std::is_invocable_v<std::decay_t<F>, std::decay_t<std::tuple_element_t<i + 1 + K, all>>...>,
                    "thread: the callable is not invocable with the decayed arguments");
      if constexpr (with_token) {
        using S = thread_state<std::decay_t<F>, std::stop_token, std::decay_t<std::tuple_element_t<i + 1 + K, all>>...>;
        state_guard<S> g{new S(static_cast<F&&>(std::get<i>(refs)), ss->get_token(),
                               static_cast<std::tuple_element_t<i + 1 + K, all>&&>(std::get<i + 1 + K>(refs))...)};
        ycxx_pal_handle h = ::ycxx::detail::thread_start(&S::run, g.s, attrs.stack, attrs.name, attrs.name_size);
        g.s = nullptr;
        return h;
      } else {
        using S = thread_state<std::decay_t<F>, std::decay_t<std::tuple_element_t<i + 1 + K, all>>...>;
        state_guard<S> g{new S(static_cast<F&&>(std::get<i>(refs)),
                               static_cast<std::tuple_element_t<i + 1 + K, all>&&>(std::get<i + 1 + K>(refs))...)};
        ycxx_pal_handle h = ::ycxx::detail::thread_start(&S::run, g.s, attrs.stack, attrs.name, attrs.name_size);
        g.s = nullptr;
        return h;
      }
    }(std::make_index_sequence<n - i - 1>{});
  }

  static void join(ycxx_pal_handle& h) {
    if (h == 0)
      ::ycxx::detail::raise_system_error(std::errc::invalid_argument, "thread::join: the thread is not joinable");
    if (h == ::ycxx_pal_thread_self())
      ::ycxx::detail::raise_system_error(std::errc::resource_deadlock_would_occur,
                                         "thread::join: a thread cannot join itself");
    if (int r = ::ycxx_pal_thread_join(h); r != 0)
      ::ycxx::detail::raise_system_error(static_cast<std::errc>(r), "thread::join");
    h = 0;
  }
  static void detach(ycxx_pal_handle& h) {
    if (h == 0)
      ::ycxx::detail::raise_system_error(std::errc::invalid_argument, "thread::detach: the thread is not joinable");
    if (int r = ::ycxx_pal_thread_detach(h); r != 0)
      ::ycxx::detail::raise_system_error(static_cast<std::errc>(r), "thread::detach");
    h = 0;
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class... Args>
  requires(sizeof...(Args) > 0) && (!is_same_v<remove_cvref_t<tuple_element_t<0, tuple<Args...>>>, thread>)
thread::thread(Args&&... args)
    : handle_(ycxx::detail::thread_access::start<false>(nullptr, static_cast<Args&&>(args)...)) {}

inline void thread::join() { ycxx::detail::thread_access::join(handle_); }
inline void thread::detach() { ycxx::detail::thread_access::detach(handle_); }
inline thread::id thread::get_id() const noexcept { return id(handle_); }

// [thread.jthread.class]
class jthread {
public:
  using id = thread::id;
  using native_handle_type = thread::native_handle_type;
  template <class T>
  using name_hint = thread::name_hint<T>;
  using stack_size_hint = thread::stack_size_hint;

  jthread() noexcept : ssource_(nostopstate) {}
  template <class... Args>
    requires(sizeof...(Args) > 0) && (!is_same_v<remove_cvref_t<tuple_element_t<0, tuple<Args...>>>, jthread>)
  explicit jthread(Args&&... args) : ssource_() {
    thread_.handle_ = ycxx::detail::thread_access::start<true>(&ssource_, static_cast<Args&&>(args)...);
  }
  ~jthread() {
    if (joinable()) {
      request_stop();
      join();
    }
  }
  jthread(const jthread&) = delete;
  jthread(jthread&& x) noexcept
      : ssource_(static_cast<stop_source&&>(x.ssource_)), thread_(static_cast<thread&&>(x.thread_)) {}
  jthread& operator=(const jthread&) = delete;
  jthread& operator=(jthread&& x) noexcept {
    if (__builtin_addressof(x) == this)
      return *this;
    if (joinable()) {
      request_stop();
      join();
    }
    ssource_ = static_cast<stop_source&&>(x.ssource_);
    thread_ = static_cast<thread&&>(x.thread_);
    return *this;
  }

  void swap(jthread& x) noexcept {
    ssource_.swap(x.ssource_);
    thread_.swap(x.thread_);
  }
  [[nodiscard]] bool joinable() const noexcept { return thread_.joinable(); }
  void join() { thread_.join(); }
  void detach() { thread_.detach(); }
  [[nodiscard]] id get_id() const noexcept { return thread_.get_id(); }
  [[nodiscard]] native_handle_type native_handle() { return thread_.native_handle(); }

  [[nodiscard]] stop_source get_stop_source() noexcept { return ssource_; }
  [[nodiscard]] stop_token get_stop_token() const noexcept { return ssource_.get_token(); }
  bool request_stop() noexcept { return ssource_.request_stop(); }

  friend void swap(jthread& lhs, jthread& rhs) noexcept { lhs.swap(rhs); }
  [[nodiscard]] static unsigned int hardware_concurrency() noexcept { return thread::hardware_concurrency(); }

private:
  stop_source ssource_; // initialized before the thread starts
  thread thread_;
};

// [thread.thread.this]
namespace this_thread {

[[nodiscard]] inline thread::id get_id() noexcept { return ycxx::detail::thread_access::make_id(::ycxx_pal_thread_self()); }

inline void yield() noexcept { ::ycxx_pal_thread_yield(); }

template <class Clock, class Duration>
void sleep_until(const chrono::time_point<Clock, Duration>& abs_time) {
  static_assert(chrono::is_clock_v<Clock>, "sleep_until: Clock must meet the Cpp17Clock requirements");
  for (auto now = Clock::now(); now < abs_time; now = Clock::now()) {
    const ycxx::detail::pal_deadline d = ycxx::detail::deadline_at(abs_time, now);
    ::ycxx_pal_sleep_until(d.clock, d.sec, d.nsec);
    if constexpr (is_same_v<Clock, chrono::system_clock> || is_same_v<Clock, chrono::steady_clock>)
      return;
  }
}

template <class Rep, class Period>
void sleep_for(const chrono::duration<Rep, Period>& rel_time) {
  if (!(rel_time > rel_time.zero()))
    return;
  const ycxx::detail::pal_deadline d = ycxx::detail::deadline_after(rel_time);
  ::ycxx_pal_sleep_until(d.clock, d.sec, d.nsec);
}

} // namespace this_thread

} // namespace std
