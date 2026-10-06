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

namespace [[gnu::visibility("hidden")]] std { namespace execution {
class run_loop;
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
struct exec_run_loop_opstate_base {
  void (*execute)(exec_run_loop_opstate_base*) noexcept;
  std::execution::run_loop* loop;
  exec_run_loop_opstate_base* next = nullptr;
};
template <class Rcvr>
struct exec_run_loop_opstate;
class exec_run_loop_scheduler;
class exec_run_loop_sender;
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {

class run_loop {
  template <class>
  friend struct ycxx::adl_free::exec_run_loop_opstate;
  using base = ycxx::adl_free::exec_run_loop_opstate_base;
  enum : unsigned char { starting, running, finishing, finished };

  unsigned lock_ = 0;
  unsigned signal_ = 0; // advanced by every push-back and by finish; run() waits on it
  unsigned char state_ = starting;
  size_t count_ = 0;
  base* head_ = nullptr;
  base* tail_ = nullptr;

  void lock() noexcept {
    for (int spins = 0; __atomic_exchange_n(&lock_, 1u, __ATOMIC_ACQUIRE) != 0;) {
      while (__atomic_load_n(&lock_, __ATOMIC_RELAXED) != 0)
        if (++spins > 64)
          ::ycxx_pal_thread_yield();
    }
  }
  void unlock() noexcept { __atomic_store_n(&lock_, 0u, __ATOMIC_RELEASE); }

  // pop-front ([exec.run.loop.members]/1)
  base* pop_front() noexcept {
    for (;;) {
      lock();
      if (base* item = head_) {
        head_ = item->next;
        if (!head_)
          tail_ = nullptr;
        --count_;
        unlock();
        return item;
      }
      if (state_ == finishing) {
        state_ = finished;
        unlock();
        return nullptr;
      }
      const unsigned seen = __atomic_load_n(&signal_, __ATOMIC_RELAXED);
      unlock();
      ycxx::detail::atomic_wait_until_done(&signal_, [&] { return __atomic_load_n(&signal_, __ATOMIC_ACQUIRE) != seen; });
    }
  }
  // push-back ([exec.run.loop.members]/2)
  void push_back(base* item) noexcept {
    item->next = nullptr;
    lock();
    if (tail_)
      tail_->next = item;
    else
      head_ = item;
    tail_ = item;
    ++count_;
    __atomic_fetch_add(&signal_, 1u, __ATOMIC_RELEASE);
    unlock();
    ycxx::detail::atomic_notify(&signal_);
  }

public:
  run_loop() noexcept = default;
  run_loop(run_loop&&) = delete;
  ~run_loop() {
    if (count_ != 0 || state_ == running)
      std::terminate();
  }

  ycxx::adl_free::exec_run_loop_scheduler get_scheduler() noexcept;

  void run() noexcept {
    lock();
    if (state_ == starting)
      state_ = running;
    unlock();
    while (base* op = pop_front())
      op->execute(op);
  }
  void finish() noexcept {
    lock();
    state_ = finishing;
    __atomic_fetch_add(&signal_, 1u, __ATOMIC_RELEASE);
    unlock();
    ycxx::detail::atomic_notify(&signal_);
  }
};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

template <class Rcvr>
struct exec_run_loop_opstate : private exec_run_loop_opstate_base {
  using operation_state_concept = std::execution::operation_state_tag;
  Rcvr rcvr;

  exec_run_loop_opstate(std::execution::run_loop* l, Rcvr r) noexcept(std::is_nothrow_move_constructible_v<Rcvr>)
      : exec_run_loop_opstate_base{&run, l}, rcvr(static_cast<Rcvr&&>(r)) {}
  exec_run_loop_opstate(exec_run_loop_opstate&&) = delete;

  void start() & noexcept { this->loop->push_back(this); }

private:
  static void run(exec_run_loop_opstate_base* b) noexcept {
    auto& o = *static_cast<exec_run_loop_opstate*>(b);
    // With an unstoppable token the only completion signature is set_value_t()
    // ([exec.run.loop.types]/6): set_stopped is then not even potentially evaluated.
    if constexpr (std::unstoppable_token<std::stop_token_of_t<std::execution::env_of_t<Rcvr>>>) {
      std::execution::set_value(static_cast<Rcvr&&>(o.rcvr));
    } else {
      if (std::get_stop_token(std::execution::get_env(o.rcvr)).stop_requested())
        std::execution::set_stopped(static_cast<Rcvr&&>(o.rcvr));
      else
        std::execution::set_value(static_cast<Rcvr&&>(o.rcvr));
    }
  }
};

class exec_run_loop_scheduler {
  friend class std::execution::run_loop;
  std::execution::run_loop* loop_;
  constexpr explicit exec_run_loop_scheduler(std::execution::run_loop* l) noexcept : loop_(l) {}
  friend class exec_run_loop_sender;

public:
  using scheduler_concept = std::execution::scheduler_tag;
  inline exec_run_loop_sender schedule() const noexcept;
  constexpr bool operator==(const exec_run_loop_scheduler&) const noexcept = default;
  constexpr std::execution::forward_progress_guarantee query(std::execution::get_forward_progress_guarantee_t) const noexcept {
    return std::execution::forward_progress_guarantee::parallel;
  }
};

class exec_run_loop_sender {
  friend class exec_run_loop_scheduler;
  std::execution::run_loop* loop_;
  constexpr explicit exec_run_loop_sender(std::execution::run_loop* l) noexcept : loop_(l) {}

  struct attrs {
    std::execution::run_loop* loop;
    template <class Tag>
      requires(std::is_same_v<Tag, std::execution::set_value_t> || std::is_same_v<Tag, std::execution::set_stopped_t>)
    constexpr exec_run_loop_scheduler query(std::execution::get_completion_scheduler_t<Tag>) const noexcept {
      return exec_run_loop_scheduler(loop);
    }
  };

public:
  using sender_concept = std::execution::sender_tag;
  template <class Self, class... Env>
  using ycxx_csigs = std::conditional_t<(std::unstoppable_token<std::stop_token_of_t<Env>> && ...) && sizeof...(Env) != 0,
                                        std::execution::completion_signatures<std::execution::set_value_t()>,
                                        std::execution::completion_signatures<std::execution::set_value_t(), std::execution::set_stopped_t()>>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ycxx_csigs<Self, Env...>();
  }
  constexpr attrs get_env() const noexcept { return {loop_}; }
  template <class Rcvr>
  exec_run_loop_opstate<std::decay_t<Rcvr>> connect(Rcvr&& rcvr) const noexcept(std::is_nothrow_constructible_v<std::decay_t<Rcvr>, Rcvr>) {
    return {loop_, static_cast<Rcvr&&>(rcvr)};
  }
};

inline exec_run_loop_sender exec_run_loop_scheduler::schedule() const noexcept { return exec_run_loop_sender(loop_); }

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
inline ycxx::adl_free::exec_run_loop_scheduler run_loop::get_scheduler() noexcept { return ycxx::adl_free::exec_run_loop_scheduler(this); }
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.sync.wait], [exec.sync.wait.var]
namespace [[gnu::visibility("hidden")]] std {
class error_code;
class system_error;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// AS-EXCEPT-PTR(err) ([exec.general]/8)
template <class Err>
std::exception_ptr as_except_ptr(Err&& err) noexcept {
  if constexpr (std::is_same_v<std::decay_t<Err>, std::exception_ptr>) {
    ::ycxx::detail::precondition(static_cast<bool>(err), "AS-EXCEPT-PTR: a null exception_ptr");
    return static_cast<Err&&>(err);
  } else if constexpr (std::is_same_v<std::decay_t<Err>, dependent_t<std::error_code, Err>>) {
    return std::make_exception_ptr(dependent_t<std::system_error, Err>(err));
  } else {
    return std::make_exception_ptr(static_cast<Err&&>(err));
  }
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
struct exec_sync_wait_env {
  std::execution::run_loop* loop;
  auto query(std::execution::get_scheduler_t) const noexcept { return loop->get_scheduler(); }
  auto query(std::execution::get_start_scheduler_t) const noexcept { return loop->get_scheduler(); }
  auto query(std::execution::get_delegation_scheduler_t) const noexcept { return loop->get_scheduler(); }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
using sync_wait_env = ::ycxx::adl_free::exec_sync_wait_env;
template <std::execution::sender_in<sync_wait_env> Sndr>
using sync_wait_result_type =
    std::optional<std::execution::value_types_of_t<Sndr, sync_wait_env, decayed_tuple, std::type_identity_t>>;
template <std::execution::sender_in<sync_wait_env> Sndr>
using sync_wait_with_variant_result_type = std::optional<std::execution::value_types_of_t<Sndr, sync_wait_env>>;

template <class Sndr>
struct sync_wait_state {
  std::execution::run_loop loop;
  std::exception_ptr error;
  sync_wait_result_type<Sndr> result;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Sndr>
struct exec_sync_wait_receiver {
  using receiver_concept = std::execution::receiver_tag;
  ::ycxx::detail::exec::sync_wait_state<Sndr>* state;

  template <class... Args>
  void set_value(Args&&... args) && noexcept {
    if constexpr (::ycxx::detail::cfg::exceptions && !std::is_nothrow_constructible_v<typename decltype(state->result)::value_type, Args...>) {
      try {
        state->result.emplace(static_cast<Args&&>(args)...);
      } catch (...) {
        state->error = std::current_exception();
      }
    } else {
      state->result.emplace(static_cast<Args&&>(args)...);
    }
    state->loop.finish();
  }
  template <class Error>
  void set_error(Error&& err) && noexcept {
    state->error = ::ycxx::detail::exec::as_except_ptr(static_cast<Error&&>(err));
    state->loop.finish();
  }
  void set_stopped() && noexcept { state->loop.finish(); }
  exec_sync_wait_env get_env() const noexcept { return {&state->loop}; }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace this_thread {

struct sync_wait_t {
  template <execution::sender Sndr>
  auto operator()(Sndr&& sndr) const {
    using env_t = ycxx::detail::exec::sync_wait_env;
    static_assert(execution::sender_in<Sndr, env_t>, "sync_wait: the sender has no completion signatures in sync_wait's environment");
    static_assert(requires { typename ycxx::detail::exec::sync_wait_result_type<Sndr>; },
                  "sync_wait: the sender must have exactly one value completion signature");
    // COMPL-DOMAIN rather than get_completion_domain<set_value_t> (ill-formed for a sender
    // without completion-domain attributes; DECISIONS: <execution>).
    using Domain = ycxx::detail::exec::compl_domain_of_t<execution::set_value_t, Sndr, env_t>;
    using R = decltype(execution::apply_sender(Domain(), *this, static_cast<Sndr&&>(sndr)));
    static_assert(is_same_v<R, ycxx::detail::exec::sync_wait_result_type<Sndr>>, "sync_wait: a customization returned the wrong type");
    return execution::apply_sender(Domain(), *this, static_cast<Sndr&&>(sndr));
  }

  template <class Sndr>
    requires ycxx::detail::exec::sender_to<Sndr, ycxx::adl_free::exec_sync_wait_receiver<Sndr>>
  ycxx::detail::exec::sync_wait_result_type<Sndr> apply_sender(Sndr&& sndr) const {
    ycxx::detail::exec::sync_wait_state<Sndr> state;
    auto op = execution::connect(static_cast<Sndr&&>(sndr), ycxx::adl_free::exec_sync_wait_receiver<Sndr>{&state});
    execution::start(op);
    state.loop.run();
    if (state.error)
      std::rethrow_exception(static_cast<exception_ptr&&>(state.error));
    return static_cast<ycxx::detail::exec::sync_wait_result_type<Sndr>&&>(state.result);
  }
};

struct sync_wait_with_variant_t {
  template <execution::sender Sndr>
  auto operator()(Sndr&& sndr) const {
    using env_t = ycxx::detail::exec::sync_wait_env;
    using S = decltype(execution::into_variant(static_cast<Sndr&&>(sndr)));
    static_assert(execution::sender_in<S, env_t>, "sync_wait_with_variant: the sender has no completion signatures in sync_wait's environment");
    using Domain = ycxx::detail::exec::compl_domain_of_t<execution::set_value_t, Sndr, env_t>;
    return execution::apply_sender(Domain(), *this, static_cast<Sndr&&>(sndr));
  }

  // The result is optional<value_types_of_t<Sndr, sync-wait-env>> of the given sender (the
  // draft's names the into_variant sender's, whose value is a variant of tuples of that;
  // DECISIONS: <execution>).
  template <class Sndr>
    requires requires(Sndr&& s) { sync_wait_t()(execution::into_variant(static_cast<Sndr&&>(s))); }
  ycxx::detail::exec::sync_wait_with_variant_result_type<Sndr> apply_sender(Sndr&& sndr) const {
    using result_type = ycxx::detail::exec::sync_wait_with_variant_result_type<Sndr>;
    if (auto opt_value = sync_wait_t()(execution::into_variant(static_cast<Sndr&&>(sndr))))
      return result_type(std::move(std::get<0>(*opt_value)));
    return result_type(nullopt);
  }
};

inline constexpr sync_wait_t sync_wait{};
inline constexpr sync_wait_with_variant_t sync_wait_with_variant{};

}} // namespace std::this_thread
