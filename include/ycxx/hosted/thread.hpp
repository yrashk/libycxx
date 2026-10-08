// libycxx hosted: thread, jthread and this_thread ([thread.threads]).
//
// A thread's identity (thread::id) and its native handle are the PAL's thread handle (pthread_t
// on POSIX). The constructor decay-copies the callable and its arguments into a heap state in the
// constructing thread and hands that to the runtime (__ycxx::__detail::__thread_start), which starts
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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Whether the program has the 'threads' hosted layer (DECISIONS §18); dependent, so that only
// constructing a thread fails without it.
template <class...>
inline constexpr bool __has_threads = __cfg::__layer::__threads;

// The hosted runtime (src/hosted/thread.cpp): starts a thread running run(arg) after naming it
// (name, not necessarily null-terminated, is copied first; null for none). Throws system_error
// if no thread can be started.
ycxx_pal_handle __thread_start(void (*run)(void*), void* arg, std::size_t __stack_size, const char* name,
                             std::size_t __name_size);

struct __thread_access;

// The state a new thread runs: the decayed callable and arguments, invoked as rvalues.
template <class... _Tp>
struct __thread_state {
  std::tuple<_Tp...> values;

  template <class... _Up>
  explicit __thread_state(_Up&&... __u) : values(static_cast<_Up&&>(__u)...) {}

  // [thread.thread.constr]/6: an exception from the invocation calls terminate. A foreign
  // exception (the forced unwind of pthread_exit or thread cancellation) passes through, so
  // that the thread ends as the platform intends; the state is destroyed either way.
  static void run(void* p) {
    struct __owner {
      __thread_state* s;
      ~__owner() { delete s; }
    } __o{static_cast<__thread_state*>(p)};
    auto __body = [s = __o.s]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
      ::__ycxx::__detail::invoke(static_cast<_Tp&&>(std::get<_Ip>(s->values))...);
    };
    if constexpr (__cfg::exceptions) {
      try {
        __body(std::index_sequence_for<_Tp...>{});
      } catch (...) {
        if (::__ycxx::__detail::__handling_foreign_exception())
          throw;
        std::terminate();
      }
    } else {
      __body(std::index_sequence_for<_Tp...>{});
    }
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class jthread;

// [thread.thread.class]
class thread {
public:
  class id;
  template <same_as<char> _Tp>
  class name_hint;
  template <class _Tp>
  name_hint(const _Tp*) -> name_hint<_Tp>;
  template <class _Tp>
  name_hint(basic_string<_Tp>) -> name_hint<_Tp>;
  class stack_size_hint;
  using native_handle_type = ycxx_pal_handle;

  thread() noexcept = default;
  template <class... _Args>
    requires(sizeof...(_Args) > 0) && (!is_same_v<remove_cvref_t<tuple_element_t<0, tuple<_Args...>>>, thread>)
  explicit thread(_Args&&... __args);
  ~thread() {
    if (joinable())
      std::terminate();
  }
  thread(const thread&) = delete;
  thread(thread&& __x) noexcept : __handle_(__x.__handle_) { __x.__handle_ = 0; }
  thread& operator=(const thread&) = delete;
  thread& operator=(thread&& __x) noexcept {
    if (joinable())
      std::terminate();
    __handle_ = __x.__handle_;
    __x.__handle_ = 0;
    return *this;
  }

  void swap(thread& __x) noexcept {
    native_handle_type h = __handle_;
    __handle_ = __x.__handle_;
    __x.__handle_ = h;
  }
  [[nodiscard]] bool joinable() const noexcept { return __handle_ != 0; }
  void join();
  void detach();
  [[nodiscard]] id get_id() const noexcept;
  [[nodiscard]] native_handle_type native_handle() { return __handle_; }
  [[nodiscard]] static unsigned int hardware_concurrency() noexcept { return ::ycxx_pal_hardware_concurrency(); }

private:
  native_handle_type __handle_ = 0;
  friend class jthread;
};

// [thread.thread.id]
class thread::id {
  ycxx_pal_handle __handle_ = 0;

  constexpr explicit id(ycxx_pal_handle h) noexcept : __handle_(h) {}
  friend class thread;
  friend struct __ycxx::__detail::__thread_access;
  friend bool operator==(thread::id __x, thread::id y) noexcept;
  friend strong_ordering operator<=>(thread::id __x, thread::id y) noexcept;
  friend struct hash<thread::id>;

public:
  id() noexcept = default;
};

inline bool operator==(thread::id __x, thread::id y) noexcept { return __x.__handle_ == y.__handle_; }
inline strong_ordering operator<=>(thread::id __x, thread::id y) noexcept { return __x.__handle_ <=> y.__handle_; }

template <>
struct hash<thread::id> {
  size_t operator()(thread::id __x) const noexcept { return static_cast<size_t>(__x.__handle_); }
};

// [thread.attributes]
template <same_as<char> _Tp>
class thread::name_hint {
  basic_string_view<_Tp> __name_;
  friend struct __ycxx::__detail::__thread_access;

public:
  constexpr explicit name_hint(basic_string_view<_Tp> n) noexcept : __name_(n) {}
  name_hint(name_hint&&) = delete;
  name_hint(const name_hint&) = delete;
};

class thread::stack_size_hint {
  size_t __size_;
  friend struct __ycxx::__detail::__thread_access;

public:
  constexpr explicit stack_size_hint(size_t s) noexcept : __size_(s) {}
};

inline void swap(thread& __x, thread& y) noexcept { __x.swap(y); }

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_thread_attribute = false;
template <>
inline constexpr bool __is_thread_attribute<std::thread::stack_size_hint> = true;
template <class _Tp>
inline constexpr bool __is_thread_attribute<std::thread::name_hint<_Tp>> = true;

// The index of the first argument whose decayed type is not a thread attribute.
template <class... _Args>
consteval std::size_t __thread_callable_index() {
  constexpr bool __attr[] = {__is_thread_attribute<std::decay_t<_Args>>..., false};
  std::size_t i = 0;
  while (i < sizeof...(_Args) && __attr[i])
    ++i;
  return i;
}

template <class... _Attrs>
consteval bool __thread_attributes_distinct() {
  using _Lp = std::tuple<std::remove_cvref_t<_Attrs>...>;
  bool ok = true;
  [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
    (
        [&] {
          constexpr std::size_t i = _Ip;
          [&]<std::size_t... _Jp>(std::index_sequence<_Jp...>) {
            ((ok = ok && (_Jp == i || !std::is_same_v<std::tuple_element_t<i, _Lp>, std::tuple_element_t<_Jp, _Lp>>)), ...);
          }(std::index_sequence_for<_Attrs...>{});
        }(),
        ...);
  }(std::index_sequence_for<_Attrs...>{});
  return ok;
}

struct __thread_access {
  static constexpr std::thread::id __make_id(ycxx_pal_handle h) noexcept { return std::thread::id(h); }
  static constexpr ycxx_pal_handle __handle_of(std::thread::id i) noexcept { return i.__handle_; }

  struct __attributes {
    std::size_t stack = 0;
    const char* name = nullptr;
    std::size_t __name_size = 0;
  };
  static void apply(__attributes& a, const std::thread::stack_size_hint& h) noexcept { a.stack = h.__size_; }
  static void apply(__attributes& a, const std::thread::name_hint<char>& h) noexcept {
    a.name = h.__name_.data();
    a.__name_size = h.__name_.size();
  }

  // Deletes the state unless the thread took it.
  template <class _Sp>
  struct __state_guard {
    _Sp* s;
    ~__state_guard() { delete s; }
  };

  // [thread.thread.constr]/4-6, [thread.jthread.cons]/4-6: starts the thread for the arguments
  // of a thread or jthread constructor; ss is the jthread's stop source (null for thread).
  template <bool _Jthread, class... _Args>
  static ycxx_pal_handle start(const std::stop_source* __ss, _Args&&... __args) {
    constexpr std::size_t n = sizeof...(_Args);
    constexpr std::size_t i = ::__ycxx::__detail::__thread_callable_index<_Args...>();
    static_assert(i < n, "thread: the arguments are all thread attributes; the callable is missing");
    using all = std::tuple<_Args&&...>;
    all __refs(static_cast<_Args&&>(__args)...);
    __attributes __attrs;
    [&]<std::size_t... _Jp>(std::index_sequence<_Jp...>) {
      static_assert(::__ycxx::__detail::__thread_attributes_distinct<std::tuple_element_t<_Jp, all>...>(),
                    "thread: a thread attribute type is given more than once");
      (__thread_access::apply(__attrs, std::get<_Jp>(__refs)), ...);
    }(std::make_index_sequence<i>{});
    return [&]<std::size_t... _Kp>(std::index_sequence<_Kp...>) {
      using _Fp = std::tuple_element_t<i, all>;
      static_assert(std::is_constructible_v<std::decay_t<_Fp>, _Fp>, "thread: the callable cannot be decay-copied");
      static_assert((std::is_constructible_v<std::decay_t<std::tuple_element_t<i + 1 + _Kp, all>>,
                                             std::tuple_element_t<i + 1 + _Kp, all>> &&
                     ...),
                    "thread: an argument cannot be decay-copied");
      constexpr bool __with_token =
          _Jthread &&
          std::is_invocable_v<std::decay_t<_Fp>, std::stop_token, std::decay_t<std::tuple_element_t<i + 1 + _Kp, all>>...>;
      static_assert(__with_token ||
                        std::is_invocable_v<std::decay_t<_Fp>, std::decay_t<std::tuple_element_t<i + 1 + _Kp, all>>...>,
                    "thread: the callable is not invocable with the decayed arguments");
      if constexpr (__with_token) {
        using _Sp = __thread_state<std::decay_t<_Fp>, std::stop_token, std::decay_t<std::tuple_element_t<i + 1 + _Kp, all>>...>;
        __state_guard<_Sp> __g{new _Sp(static_cast<_Fp&&>(std::get<i>(__refs)), __ss->get_token(),
                               static_cast<std::tuple_element_t<i + 1 + _Kp, all>&&>(std::get<i + 1 + _Kp>(__refs))...)};
        ycxx_pal_handle h = ::__ycxx::__detail::__thread_start(&_Sp::run, __g.s, __attrs.stack, __attrs.name, __attrs.__name_size);
        __g.s = nullptr;
        return h;
      } else {
        using _Sp = __thread_state<std::decay_t<_Fp>, std::decay_t<std::tuple_element_t<i + 1 + _Kp, all>>...>;
        __state_guard<_Sp> __g{new _Sp(static_cast<_Fp&&>(std::get<i>(__refs)),
                               static_cast<std::tuple_element_t<i + 1 + _Kp, all>&&>(std::get<i + 1 + _Kp>(__refs))...)};
        ycxx_pal_handle h = ::__ycxx::__detail::__thread_start(&_Sp::run, __g.s, __attrs.stack, __attrs.name, __attrs.__name_size);
        __g.s = nullptr;
        return h;
      }
    }(std::make_index_sequence<n - i - 1>{});
  }

  static void join(ycxx_pal_handle& h) {
    if (h == 0)
      ::__ycxx::__detail::__raise_system_error(std::errc::invalid_argument, "thread::join: the thread is not joinable");
    if (h == ::ycxx_pal_thread_self())
      ::__ycxx::__detail::__raise_system_error(std::errc::resource_deadlock_would_occur,
                                         "thread::join: a thread cannot join itself");
    if (int r = ::ycxx_pal_thread_join(h); r != 0)
      ::__ycxx::__detail::__raise_system_error(static_cast<std::errc>(r), "thread::join");
    h = 0;
  }
  static void detach(ycxx_pal_handle& h) {
    if (h == 0)
      ::__ycxx::__detail::__raise_system_error(std::errc::invalid_argument, "thread::detach: the thread is not joinable");
    if (int r = ::ycxx_pal_thread_detach(h); r != 0)
      ::__ycxx::__detail::__raise_system_error(static_cast<std::errc>(r), "thread::detach");
    h = 0;
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class... _Args>
  requires(sizeof...(_Args) > 0) && (!is_same_v<remove_cvref_t<tuple_element_t<0, tuple<_Args...>>>, thread>)
thread::thread(_Args&&... __args)
    : __handle_(__ycxx::__detail::__thread_access::start<false>(nullptr, static_cast<_Args&&>(__args)...)) {
  static_assert(__ycxx::__detail::__has_threads<_Args...>,
                "std::thread needs the 'threads' hosted layer: libycxx was configured without it "
                "(YCXX_HOSTED_LAYERS, DECISIONS §18), so there is one thread of execution");
}

inline void thread::join() { __ycxx::__detail::__thread_access::join(__handle_); }
inline void thread::detach() { __ycxx::__detail::__thread_access::detach(__handle_); }
inline thread::id thread::get_id() const noexcept { return id(__handle_); }

// [thread.jthread.class]
class jthread {
public:
  using id = thread::id;
  using native_handle_type = thread::native_handle_type;
  template <class _Tp>
  using name_hint = thread::name_hint<_Tp>;
  using stack_size_hint = thread::stack_size_hint;

  jthread() noexcept : __ssource_(nostopstate) {}
  template <class... _Args>
    requires(sizeof...(_Args) > 0) && (!is_same_v<remove_cvref_t<tuple_element_t<0, tuple<_Args...>>>, jthread>)
  explicit jthread(_Args&&... __args) : __ssource_() {
    static_assert(__ycxx::__detail::__has_threads<_Args...>,
                  "std::jthread needs the 'threads' hosted layer: libycxx was configured without it "
                  "(YCXX_HOSTED_LAYERS, DECISIONS §18), so there is one thread of execution");
    __thread_.__handle_ = __ycxx::__detail::__thread_access::start<true>(&__ssource_, static_cast<_Args&&>(__args)...);
  }
  ~jthread() {
    if (joinable()) {
      request_stop();
      join();
    }
  }
  jthread(const jthread&) = delete;
  jthread(jthread&& __x) noexcept
      : __ssource_(static_cast<stop_source&&>(__x.__ssource_)), __thread_(static_cast<thread&&>(__x.__thread_)) {}
  jthread& operator=(const jthread&) = delete;
  jthread& operator=(jthread&& __x) noexcept {
    if (__builtin_addressof(__x) == this)
      return *this;
    if (joinable()) {
      request_stop();
      join();
    }
    __ssource_ = static_cast<stop_source&&>(__x.__ssource_);
    __thread_ = static_cast<thread&&>(__x.__thread_);
    return *this;
  }

  void swap(jthread& __x) noexcept {
    __ssource_.swap(__x.__ssource_);
    __thread_.swap(__x.__thread_);
  }
  [[nodiscard]] bool joinable() const noexcept { return __thread_.joinable(); }
  void join() { __thread_.join(); }
  void detach() { __thread_.detach(); }
  [[nodiscard]] id get_id() const noexcept { return __thread_.get_id(); }
  [[nodiscard]] native_handle_type native_handle() { return __thread_.native_handle(); }

  [[nodiscard]] stop_source get_stop_source() noexcept { return __ssource_; }
  [[nodiscard]] stop_token get_stop_token() const noexcept { return __ssource_.get_token(); }
  bool request_stop() noexcept { return __ssource_.request_stop(); }

  friend void swap(jthread& __lhs, jthread& __rhs) noexcept { __lhs.swap(__rhs); }
  [[nodiscard]] static unsigned int hardware_concurrency() noexcept { return thread::hardware_concurrency(); }

private:
  stop_source __ssource_; // initialized before the thread starts
  thread __thread_;
};

// [thread.thread.this]
namespace this_thread {

[[nodiscard]] inline thread::id get_id() noexcept { return __ycxx::__detail::__thread_access::__make_id(::ycxx_pal_thread_self()); }

inline void yield() noexcept { ::ycxx_pal_thread_yield(); }

template <class _Clock, class _Duration>
void sleep_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
  static_assert(chrono::is_clock_v<_Clock>, "sleep_until: Clock must meet the Cpp17Clock requirements");
  for (auto now = _Clock::now(); now < __abs_time; now = _Clock::now()) {
    const __ycxx::__detail::__pal_deadline d = __ycxx::__detail::__deadline_at(__abs_time, now);
    ::ycxx_pal_sleep_until(d.clock, d.__sec, d.__nsec);
    if constexpr (is_same_v<_Clock, chrono::system_clock> || is_same_v<_Clock, chrono::steady_clock>)
      return;
  }
}

template <class _Rep, class _Period>
void sleep_for(const chrono::duration<_Rep, _Period>& __rel_time) {
  if (!(__rel_time > __rel_time.zero()))
    return;
  const __ycxx::__detail::__pal_deadline d = __ycxx::__detail::__deadline_after(__rel_time);
  ::ycxx_pal_sleep_until(d.clock, d.__sec, d.__nsec);
}

} // namespace this_thread

}} // namespace std
