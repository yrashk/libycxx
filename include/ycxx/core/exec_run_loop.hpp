// libycxx core: execution::run_loop ([exec.run.loop]) and the consumers this_thread::sync_wait
// and this_thread::sync_wait_with_variant ([exec.sync.wait], [exec.sync.wait.var]).
//
// run_loop is an intrusive FIFO of operation states (no allocation) behind a small lock; run()
// blocks on an address wait (the PAL's, through <atomic>'s wait tables) for a counter that every
// push and finish advances.
#pragma once

#include <ycxx/core/atomic_base.hpp>
#include <ycxx/core/exec_adapt.hpp>
#include <ycxx/core/exception.hpp>
#include <ycxx/core/optional.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
class run_loop;
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
struct __exec_run_loop_opstate_base {
  void (*execute)(__exec_run_loop_opstate_base*) noexcept;
  std::execution::run_loop* __loop;
  __exec_run_loop_opstate_base* next = nullptr;
};
template <class _Rcvr>
struct __exec_run_loop_opstate;
class __exec_run_loop_scheduler;
class __exec_run_loop_sender;
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

class run_loop {
  template <class>
  friend struct __ycxx::__adl_free::__exec_run_loop_opstate;
  using base = __ycxx::__adl_free::__exec_run_loop_opstate_base;
  enum : unsigned char { __starting, __running, __finishing, __finished };

  unsigned __lock_ = 0;
  unsigned __signal_ = 0; // advanced by every push-back and by finish; run() waits on it
  unsigned char __state_ = __starting;
  size_t __count_ = 0;
  base* __head_ = nullptr;
  base* __tail_ = nullptr;

  void lock() noexcept {
    for (int __spins = 0; __atomic_exchange_n(&__lock_, 1u, __ATOMIC_ACQUIRE) != 0;) {
      while (__atomic_load_n(&__lock_, __ATOMIC_RELAXED) != 0)
        if (++__spins > 64)
          ::ycxx_pal_thread_yield();
    }
  }
  void unlock() noexcept { __atomic_store_n(&__lock_, 0u, __ATOMIC_RELEASE); }

  // pop-front ([exec.run.loop.members]/1)
  base* pop_front() noexcept {
    for (;;) {
      lock();
      if (base* __item = __head_) {
        __head_ = __item->next;
        if (!__head_)
          __tail_ = nullptr;
        --__count_;
        unlock();
        return __item;
      }
      if (__state_ == __finishing) {
        __state_ = __finished;
        unlock();
        return nullptr;
      }
      const unsigned __seen = __atomic_load_n(&__signal_, __ATOMIC_RELAXED);
      unlock();
      __ycxx::__detail::__atomic_wait_until_done(&__signal_, [&] { return __atomic_load_n(&__signal_, __ATOMIC_ACQUIRE) != __seen; });
    }
  }
  // push-back ([exec.run.loop.members]/2)
  void push_back(base* __item) noexcept {
    __item->next = nullptr;
    lock();
    if (__tail_)
      __tail_->next = __item;
    else
      __head_ = __item;
    __tail_ = __item;
    ++__count_;
    __atomic_fetch_add(&__signal_, 1u, __ATOMIC_RELEASE);
    unlock();
    __ycxx::__detail::__atomic_notify(&__signal_);
  }

public:
  run_loop() noexcept = default;
  run_loop(run_loop&&) = delete;
  ~run_loop() {
    if (__count_ != 0 || __state_ == __running)
      std::terminate();
  }

  __ycxx::__adl_free::__exec_run_loop_scheduler get_scheduler() noexcept;

  void run() noexcept {
    lock();
    if (__state_ == __starting)
      __state_ = __running;
    unlock();
    while (base* op = pop_front())
      op->execute(op);
  }
  void finish() noexcept {
    lock();
    __state_ = __finishing;
    __atomic_fetch_add(&__signal_, 1u, __ATOMIC_RELEASE);
    unlock();
    __ycxx::__detail::__atomic_notify(&__signal_);
  }
};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

template <class _Rcvr>
struct __exec_run_loop_opstate : private __exec_run_loop_opstate_base {
  using operation_state_concept = std::execution::operation_state_tag;
  _Rcvr __rcvr;

  __exec_run_loop_opstate(std::execution::run_loop* __l, _Rcvr r) noexcept(std::is_nothrow_move_constructible_v<_Rcvr>)
      : __exec_run_loop_opstate_base{&run, __l}, __rcvr(static_cast<_Rcvr&&>(r)) {}
  __exec_run_loop_opstate(__exec_run_loop_opstate&&) = delete;

  void start() & noexcept { this->__loop->push_back(this); }

private:
  static void run(__exec_run_loop_opstate_base* b) noexcept {
    auto& __o = *static_cast<__exec_run_loop_opstate*>(b);
    // With an unstoppable token the only completion signature is set_value_t()
    // ([exec.run.loop.types]/6): set_stopped is then not even potentially evaluated.
    if constexpr (std::unstoppable_token<std::stop_token_of_t<std::execution::env_of_t<_Rcvr>>>) {
      std::execution::set_value(static_cast<_Rcvr&&>(__o.__rcvr));
    } else {
      if (std::get_stop_token(std::execution::get_env(__o.__rcvr)).stop_requested())
        std::execution::set_stopped(static_cast<_Rcvr&&>(__o.__rcvr));
      else
        std::execution::set_value(static_cast<_Rcvr&&>(__o.__rcvr));
    }
  }
};

class __exec_run_loop_scheduler {
  friend class std::execution::run_loop;
  std::execution::run_loop* __loop_;
  constexpr explicit __exec_run_loop_scheduler(std::execution::run_loop* __l) noexcept : __loop_(__l) {}
  friend class __exec_run_loop_sender;

public:
  using scheduler_concept = std::execution::scheduler_tag;
  inline __exec_run_loop_sender schedule() const noexcept;
  constexpr bool operator==(const __exec_run_loop_scheduler&) const noexcept = default;
  constexpr std::execution::forward_progress_guarantee query(std::execution::get_forward_progress_guarantee_t) const noexcept {
    return std::execution::forward_progress_guarantee::parallel;
  }
};

class __exec_run_loop_sender {
  friend class __exec_run_loop_scheduler;
  std::execution::run_loop* __loop_;
  constexpr explicit __exec_run_loop_sender(std::execution::run_loop* __l) noexcept : __loop_(__l) {}

  struct __attrs {
    std::execution::run_loop* __loop;
    template <class _Tag>
      requires(std::is_same_v<_Tag, std::execution::set_value_t> || std::is_same_v<_Tag, std::execution::set_stopped_t>)
    constexpr __exec_run_loop_scheduler query(std::execution::get_completion_scheduler_t<_Tag>) const noexcept {
      return __exec_run_loop_scheduler(__loop);
    }
    // [exec.sched]/6: as the scheduler's, which has no domain of its own: default_domain when
    // given an environment ([exec.get.compl.domain]/2.4), else none.
    template <class _Tag, class _Env>
      requires(std::is_same_v<_Tag, std::execution::set_value_t> || std::is_same_v<_Tag, std::execution::set_stopped_t>)
    constexpr std::execution::default_domain query(std::execution::get_completion_domain_t<_Tag>, const _Env&) const noexcept {
      return {};
    }
  };

public:
  using sender_concept = std::execution::sender_tag;
  template <class _Self, class... _Env>
  using __ycxx_csigs = std::conditional_t<(std::unstoppable_token<std::stop_token_of_t<_Env>> && ...) && sizeof...(_Env) != 0,
                                        std::execution::completion_signatures<std::execution::set_value_t()>,
                                        std::execution::completion_signatures<std::execution::set_value_t(), std::execution::set_stopped_t()>>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return __ycxx_csigs<_Self, _Env...>();
  }
  constexpr __attrs get_env() const noexcept { return {__loop_}; }
  template <class _Rcvr>
  __exec_run_loop_opstate<std::decay_t<_Rcvr>> connect(_Rcvr&& __rcvr) const noexcept(std::is_nothrow_constructible_v<std::decay_t<_Rcvr>, _Rcvr>) {
    return {__loop_, static_cast<_Rcvr&&>(__rcvr)};
  }
};

inline __exec_run_loop_sender __exec_run_loop_scheduler::schedule() const noexcept { return __exec_run_loop_sender(__loop_); }

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
inline __ycxx::__adl_free::__exec_run_loop_scheduler run_loop::get_scheduler() noexcept { return __ycxx::__adl_free::__exec_run_loop_scheduler(this); }
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.sync.wait], [exec.sync.wait.var]
namespace [[__gnu__::__visibility__("hidden")]] std {
class error_code;
class system_error;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// AS-EXCEPT-PTR(err) ([exec.general]/8)
template <class _Err>
std::exception_ptr __as_except_ptr(_Err&& __err) noexcept {
  if constexpr (std::is_same_v<std::decay_t<_Err>, std::exception_ptr>) {
    ::__ycxx::__detail::__precondition(static_cast<bool>(__err), "AS-EXCEPT-PTR: a null exception_ptr");
    return static_cast<_Err&&>(__err);
  } else if constexpr (std::is_same_v<std::decay_t<_Err>, __dependent_t<std::error_code, _Err>>) {
    return std::make_exception_ptr(__dependent_t<std::system_error, _Err>(__err));
  } else {
    return std::make_exception_ptr(static_cast<_Err&&>(__err));
  }
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
struct __exec_sync_wait_env {
  std::execution::run_loop* __loop;
  auto query(std::execution::get_scheduler_t) const noexcept { return __loop->get_scheduler(); }
  auto query(std::execution::get_start_scheduler_t) const noexcept { return __loop->get_scheduler(); }
  auto query(std::execution::get_delegation_scheduler_t) const noexcept { return __loop->get_scheduler(); }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
using __sync_wait_env = ::__ycxx::__adl_free::__exec_sync_wait_env;
template <std::execution::sender_in<__sync_wait_env> _Sndr>
using __sync_wait_result_type =
    std::optional<std::execution::value_types_of_t<_Sndr, __sync_wait_env, __decayed_tuple, std::type_identity_t>>;
template <std::execution::sender_in<__sync_wait_env> _Sndr>
using __sync_wait_with_variant_result_type = std::optional<std::execution::value_types_of_t<_Sndr, __sync_wait_env>>;

template <class _Sndr>
struct __sync_wait_state {
  std::execution::run_loop __loop;
  std::exception_ptr error;
  __sync_wait_result_type<_Sndr> result;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Sndr>
struct __exec_sync_wait_receiver {
  using receiver_concept = std::execution::receiver_tag;
  ::__ycxx::__detail::__exec::__sync_wait_state<_Sndr>* state;

  template <class... _Args>
  void set_value(_Args&&... __args) && noexcept {
    if constexpr (::__ycxx::__detail::__cfg::exceptions && !std::is_nothrow_constructible_v<typename decltype(state->result)::value_type, _Args...>) {
      try {
        state->result.emplace(static_cast<_Args&&>(__args)...);
      } catch (...) {
        state->error = std::current_exception();
      }
    } else {
      state->result.emplace(static_cast<_Args&&>(__args)...);
    }
    state->__loop.finish();
  }
  template <class _Error>
  void set_error(_Error&& __err) && noexcept {
    state->error = ::__ycxx::__detail::__exec::__as_except_ptr(static_cast<_Error&&>(__err));
    state->__loop.finish();
  }
  void set_stopped() && noexcept { state->__loop.finish(); }
  __exec_sync_wait_env get_env() const noexcept { return {&state->__loop}; }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace this_thread {

struct sync_wait_t {
  template <execution::sender _Sndr>
  auto operator()(_Sndr&& __sndr) const {
    using __env_t = __ycxx::__detail::__exec::__sync_wait_env;
    static_assert(execution::sender_in<_Sndr, __env_t>, "sync_wait: the sender has no completion signatures in sync_wait's environment");
    static_assert(requires { typename __ycxx::__detail::__exec::__sync_wait_result_type<_Sndr>; },
                  "sync_wait: the sender must have exactly one value completion signature");
    // COMPL-DOMAIN rather than get_completion_domain<set_value_t> (ill-formed for a sender
    // without completion-domain attributes; DECISIONS: <execution>).
    using _Domain = __ycxx::__detail::__exec::__compl_domain_of_t<execution::set_value_t, _Sndr, __env_t>;
    using _Rp = decltype(execution::apply_sender(_Domain(), *this, static_cast<_Sndr&&>(__sndr)));
    static_assert(is_same_v<_Rp, __ycxx::__detail::__exec::__sync_wait_result_type<_Sndr>>, "sync_wait: a customization returned the wrong type");
    return execution::apply_sender(_Domain(), *this, static_cast<_Sndr&&>(__sndr));
  }

  template <class _Sndr>
    requires __ycxx::__detail::__exec::__sender_to<_Sndr, __ycxx::__adl_free::__exec_sync_wait_receiver<_Sndr>>
  __ycxx::__detail::__exec::__sync_wait_result_type<_Sndr> apply_sender(_Sndr&& __sndr) const {
    __ycxx::__detail::__exec::__sync_wait_state<_Sndr> state;
    auto op = execution::connect(static_cast<_Sndr&&>(__sndr), __ycxx::__adl_free::__exec_sync_wait_receiver<_Sndr>{&state});
    execution::start(op);
    state.__loop.run();
    if (state.error)
      std::rethrow_exception(static_cast<exception_ptr&&>(state.error));
    return static_cast<__ycxx::__detail::__exec::__sync_wait_result_type<_Sndr>&&>(state.result);
  }
};

struct sync_wait_with_variant_t {
  template <execution::sender _Sndr>
  auto operator()(_Sndr&& __sndr) const {
    using __env_t = __ycxx::__detail::__exec::__sync_wait_env;
    using _Sp = decltype(execution::into_variant(static_cast<_Sndr&&>(__sndr)));
    static_assert(execution::sender_in<_Sp, __env_t>, "sync_wait_with_variant: the sender has no completion signatures in sync_wait's environment");
    using _Domain = __ycxx::__detail::__exec::__compl_domain_of_t<execution::set_value_t, _Sndr, __env_t>;
    return execution::apply_sender(_Domain(), *this, static_cast<_Sndr&&>(__sndr));
  }

  // The result is optional<value_types_of_t<Sndr, sync-wait-env>> of the given sender (the
  // draft's names the into_variant sender's, whose value is a variant of tuples of that;
  // DECISIONS: <execution>).
  template <class _Sndr>
    requires requires(_Sndr&& s) { sync_wait_t()(execution::into_variant(static_cast<_Sndr&&>(s))); }
  __ycxx::__detail::__exec::__sync_wait_with_variant_result_type<_Sndr> apply_sender(_Sndr&& __sndr) const {
    using result_type = __ycxx::__detail::__exec::__sync_wait_with_variant_result_type<_Sndr>;
    if (auto __opt_value = sync_wait_t()(execution::into_variant(static_cast<_Sndr&&>(__sndr))))
      return result_type(std::move(std::get<0>(*__opt_value)));
    return result_type(nullopt);
  }
};

inline constexpr sync_wait_t sync_wait{};
inline constexpr sync_wait_with_variant_t sync_wait_with_variant{};

}} // namespace std::this_thread
