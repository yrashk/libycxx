// libycxx core: execution scopes ([exec.scope]): scope_association, scope_token,
// simple_counting_scope and counting_scope, and the algorithms that use scope tokens: associate
// ([exec.associate]), spawn ([exec.spawn]) and spawn_future ([exec.spawn.future]).
//
// A counting scope keeps its state, association count and the list of join operations waiting
// for the count to reach zero behind a small lock (the draft's operations "behave as atomic
// operations ... in a single total order"). spawn_future's shared state decides, under a lock
// of its own, which of complete, consume, try-set-stopped and abandon delivers the result and
// which destroys the state; the completions themselves run outside the lock.
#pragma once

#include <ycxx/core/exec_adapt_sched.hpp>
#include <ycxx/core/exec_run_loop.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/pair.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// A small spin lock (uncontended in practice: every critical section is a few stores).
struct exec_spin_lock {
  unsigned word = 0;
  void lock() noexcept {
    for (int spins = 0; __atomic_exchange_n(&word, 1u, __ATOMIC_ACQUIRE) != 0;)
      while (__atomic_load_n(&word, __ATOMIC_RELAXED) != 0)
        if (++spins > 64)
          ::ycxx_pal_thread_yield();
  }
  void unlock() noexcept { __atomic_store_n(&word, 0u, __ATOMIC_RELEASE); }
};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.scope.concepts]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// test-sender, test-env ([exec.scope.concepts]/4)
struct exec_test_sender {
  using sender_concept = std::execution::sender_tag;
  template <class Self, class... Env>
  using ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t()>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return std::execution::completion_signatures<std::execution::set_value_t()>();
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
template <class Assoc>
concept scope_association = movable<Assoc> && is_nothrow_move_constructible_v<Assoc> && is_nothrow_move_assignable_v<Assoc> &&
                            default_initializable<Assoc> && requires(const Assoc assoc) {
                              { static_cast<bool>(assoc) } noexcept;
                              { assoc.try_associate() } -> same_as<Assoc>;
                            };
template <class Token>
concept scope_token = copyable<Token> && requires(const Token token) {
  { token.try_associate() } -> scope_association;
  { token.wrap(declval<ycxx::adl_free::exec_test_sender>()) } -> sender_in<env<>>;
};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.counting.scopes]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
struct scope_join_t {};
struct scope_access;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// association-t<Scope> ([exec.counting.scopes.general]/5)
template <class Scope>
class exec_scope_association {
  Scope* scope_ = nullptr;
  friend Scope;
  friend struct ::ycxx::detail::exec::scope_access;
  constexpr explicit exec_scope_association(Scope* s) noexcept : scope_(s) {}

public:
  constexpr exec_scope_association() noexcept = default;
  exec_scope_association(exec_scope_association&& o) noexcept : scope_(o.scope_) { o.scope_ = nullptr; }
  exec_scope_association& operator=(exec_scope_association&& o) noexcept {
    if (this != __builtin_addressof(o)) {
      if (scope_)
        scope_->ycxx_disassociate();
      scope_ = o.scope_;
      o.scope_ = nullptr;
    }
    return *this;
  }
  ~exec_scope_association() {
    if (scope_)
      scope_->ycxx_disassociate();
  }
  explicit operator bool() const noexcept { return scope_ != nullptr; }
  exec_scope_association try_associate() const noexcept { return scope_ ? scope_->ycxx_try_associate() : exec_scope_association(); }
};

// A join operation registered with a scope until its count reaches zero.
struct exec_join_node {
  void (*complete)(exec_join_node*) noexcept;
  exec_join_node* next = nullptr;
};

// The state machine of [exec.counting.scopes.general]/1 shared by both scopes.
class exec_counting_scope_core {
protected:
  enum : unsigned char { unused, open, closed, open_and_joining, closed_and_joining, unused_and_closed, joined };
  ::ycxx::detail::exec::exec_spin_lock lock_;
  unsigned char state_ = unused;
  std::size_t count_ = 0;
  exec_join_node* joiners_ = nullptr;

  bool try_associate_core(std::size_t max) noexcept {
    lock_.lock();
    bool ok = false;
    if (count_ != max) {
      if (state_ == unused) {
        ++count_;
        state_ = open;
        ok = true;
      } else if (state_ == open || state_ == open_and_joining) {
        ++count_;
        ok = true;
      }
    }
    lock_.unlock();
    return ok;
  }
  void disassociate_core() noexcept {
    lock_.lock();
    exec_join_node* done = nullptr;
    if (--count_ == 0 && (state_ == open_and_joining || state_ == closed_and_joining)) {
      state_ = joined;
      done = joiners_;
      joiners_ = nullptr;
    }
    lock_.unlock();
    // complete() may destroy the scope: nothing of *this is touched from here on.
    while (done) {
      exec_join_node* next = done->next;
      done->complete(done);
      done = next;
    }
  }
  void close_core() noexcept {
    lock_.lock();
    if (state_ == unused)
      state_ = unused_and_closed;
    else if (state_ == open)
      state_ = closed;
    else if (state_ == open_and_joining)
      state_ = closed_and_joining;
    lock_.unlock();
  }
  // start-join-sender: true if the count is already zero (the join completes inline).
  bool start_join_core(exec_join_node* n) noexcept {
    lock_.lock();
    if (count_ == 0) {
      state_ = joined;
      lock_.unlock();
      return true;
    }
    if (state_ == open || state_ == open_and_joining || state_ == unused)
      state_ = open_and_joining;
    else
      state_ = closed_and_joining;
    n->next = joiners_;
    joiners_ = n;
    lock_.unlock();
    return false;
  }
  void check_destroy() noexcept {
    if (state_ != joined && state_ != unused && state_ != unused_and_closed)
      std::terminate();
  }

public:
  exec_counting_scope_core() noexcept = default;
  exec_counting_scope_core(exec_counting_scope_core&&) = delete;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
struct scope_access {
  template <class Scope>
  static bool start_join(Scope* s, ::ycxx::adl_free::exec_join_node* n) noexcept {
    return s->start_join_core(n);
  }
};

template <class Scope, class Rcvr>
struct scope_join_state : ::ycxx::adl_free::exec_join_node {
  struct rcvr_t {
    using receiver_concept = std::execution::receiver_tag;
    Rcvr& rcvr;
    void set_value() && noexcept { std::execution::set_value(static_cast<Rcvr&&>(rcvr)); }
    template <class E>
    void set_error(E&& e) && noexcept {
      std::execution::set_error(static_cast<Rcvr&&>(rcvr), static_cast<E&&>(e));
    }
    void set_stopped() && noexcept { std::execution::set_stopped(static_cast<Rcvr&&>(rcvr)); }
    decltype(auto) get_env() const noexcept { return std::execution::get_env(rcvr); }
  };
  using sched_sender = decltype(std::execution::schedule(std::execution::get_start_scheduler(std::execution::get_env(std::declval<Rcvr&>()))));
  using op_t = std::execution::connect_result_t<sched_sender, rcvr_t>;

  Scope* scope;
  Rcvr& receiver;
  op_t op;

  scope_join_state(Scope* s, Rcvr& r) noexcept(nothrow_callable<std::execution::connect_t, sched_sender, rcvr_t>)
      : ::ycxx::adl_free::exec_join_node{&run_complete}, scope(s), receiver(r),
        op(std::execution::connect(std::execution::schedule(std::execution::get_start_scheduler(std::execution::get_env(r))), rcvr_t{r})) {}
  scope_join_state(scope_join_state&&) = delete;

  static void run_complete(::ycxx::adl_free::exec_join_node* n) noexcept { std::execution::start(static_cast<scope_join_state*>(n)->op); }
  void complete_inline() noexcept { std::execution::set_value(static_cast<Rcvr&&>(receiver)); }
};

template <class Env>
struct scope_join_sigs {
  static auto pick() {
    if constexpr (requires(const Env& e) { std::execution::schedule(std::execution::get_start_scheduler(e)); })
      return std::type_identity<sigs_concat_t<std::execution::completion_signatures<set_value_t()>,
                                              csigs_of_t<decltype(std::execution::schedule(std::execution::get_start_scheduler(std::declval<const Env&>()))), Env>>>{};
    else
      return std::type_identity<invalid_sigs<environment_has_no_start_scheduler, Env>>{};
  }
  using type = typename decltype(pick())::type;
};

template <>
struct impls_for<scope_join_t> : default_impls {
  template <class Data>
  static constexpr auto get_attrs(const Data&) noexcept {
    return std::execution::env<>();
  }
  template <class Sndr, class Rcvr>
  static auto get_state(Sndr&& sender, Rcvr& receiver) noexcept(
      std::is_nothrow_constructible_v<scope_join_state<std::remove_pointer_t<std::decay_t<data_type<Sndr>>>, Rcvr>,
                                      std::decay_t<data_type<Sndr>>, Rcvr&>) {
    auto self = sender.template get<1>();
    return scope_join_state<std::remove_pointer_t<decltype(self)>, Rcvr>(self, receiver);
  }
  template <class State, class Rcvr>
  static void start(State& s, Rcvr&) noexcept {
    if (scope_access::start_join(s.scope, &s))
      s.complete_inline();
  }
  template <class Sndr, class... Env>
  struct sigs {
    using type = dependent_sigs;
  };
  template <class Sndr, class Env>
  struct sigs<Sndr, Env> {
    using type = typename scope_join_sigs<Env>::type;
  };
  template <class Sndr, class... Env>
  using csigs = typename sigs<Sndr, Env...>::type;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

class simple_counting_scope : ycxx::adl_free::exec_counting_scope_core {
  friend struct ycxx::detail::exec::scope_access;
  friend class ycxx::adl_free::exec_scope_association<simple_counting_scope>;
  using assoc_t = ycxx::adl_free::exec_scope_association<simple_counting_scope>;

  assoc_t ycxx_try_associate() noexcept { return try_associate_core(max_associations) ? assoc_t(this) : assoc_t(); }
  void ycxx_disassociate() noexcept { disassociate_core(); }

public:
  struct token {
    template <sender Sender>
    Sender&& wrap(Sender&& snd) const noexcept {
      return static_cast<Sender&&>(snd);
    }
    assoc_t try_associate() const noexcept { return scope->ycxx_try_associate(); }

  private:
    friend class simple_counting_scope;
    explicit token(simple_counting_scope* s) noexcept : scope(s) {}
    simple_counting_scope* scope;
  };

  static constexpr size_t max_associations = static_cast<size_t>(-1) >> 1;

  simple_counting_scope() noexcept = default;
  simple_counting_scope(simple_counting_scope&&) = delete;
  ~simple_counting_scope() { check_destroy(); }

  token get_token() noexcept { return token(this); }
  void close() noexcept { close_core(); }
  sender auto join() noexcept;
};

class counting_scope : ycxx::adl_free::exec_counting_scope_core {
  friend struct ycxx::detail::exec::scope_access;
  friend class ycxx::adl_free::exec_scope_association<counting_scope>;
  using assoc_t = ycxx::adl_free::exec_scope_association<counting_scope>;

  inplace_stop_source s_source;
  assoc_t ycxx_try_associate() noexcept { return try_associate_core(max_associations) ? assoc_t(this) : assoc_t(); }
  void ycxx_disassociate() noexcept { disassociate_core(); }

public:
  struct token {
    template <sender Sender>
    sender auto wrap(Sender&& snd) const noexcept(is_nothrow_constructible_v<remove_cvref_t<Sender>, Sender>) {
      return ycxx::detail::exec::stop_when(static_cast<Sender&&>(snd), scope->s_source.get_token());
    }
    assoc_t try_associate() const noexcept { return scope->ycxx_try_associate(); }

  private:
    friend class counting_scope;
    explicit token(counting_scope* s) noexcept : scope(s) {}
    counting_scope* scope;
  };

  static constexpr size_t max_associations = static_cast<size_t>(-1) >> 1;

  counting_scope() noexcept = default;
  counting_scope(counting_scope&&) = delete;
  ~counting_scope() { check_destroy(); }

  token get_token() noexcept { return token(this); }
  void close() noexcept { close_core(); }
  sender auto join() noexcept;
  void request_stop() noexcept { s_source.request_stop(); }
};

// Defined after the classes: the join sender's impls-for needs them complete.
inline sender auto simple_counting_scope::join() noexcept { return ycxx::detail::exec::make_sender(ycxx::detail::exec::scope_join_t(), this); }
inline sender auto counting_scope::join() noexcept { return ycxx::detail::exec::make_sender(ycxx::detail::exec::scope_join_t(), this); }

}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.associate]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Token, class Sender>
struct exec_associate_data {
  using wrap_sender = std::remove_cvref_t<decltype(std::declval<Token&>().wrap(std::declval<Sender>()))>;
  using assoc_t = decltype(std::declval<Token&>().try_associate());
  // sender-ref: owns the wrapped sender (destroy_at on destruction).
  struct sender_ref {
    wrap_sender* p = nullptr;
    sender_ref() = default;
    explicit sender_ref(wrap_sender* q) noexcept : p(q) {}
    sender_ref(sender_ref&& o) noexcept : p(o.p) { o.p = nullptr; }
    ~sender_ref() {
      if (p)
        std::destroy_at(p);
    }
    wrap_sender* release() noexcept {
      wrap_sender* q = p;
      p = nullptr;
      return q;
    }
    wrap_sender& operator*() const noexcept { return *p; }
  };

  explicit exec_associate_data(Token t, Sender&& s)
      : sndr(t.wrap(static_cast<Sender&&>(s))), assoc([&] {
          sender_ref guard{__builtin_addressof(sndr)};
          auto a = t.try_associate();
          if (a)
            guard.release();
          return a;
        }()) {}
  exec_associate_data(const exec_associate_data& other) noexcept(std::is_nothrow_copy_constructible_v<wrap_sender> &&
                                                                 noexcept(other.assoc.try_associate()))
    requires std::copy_constructible<wrap_sender>
      : assoc(other.assoc.try_associate()) {
    if (assoc)
      std::construct_at(__builtin_addressof(sndr), other.sndr);
  }
  exec_associate_data(exec_associate_data&& other) noexcept(std::is_nothrow_move_constructible_v<wrap_sender>)
      : exec_associate_data(static_cast<exec_associate_data&&>(other).release()) {}
  ~exec_associate_data() {
    if (assoc)
      sndr.~wrap_sender();
  }
  std::pair<assoc_t, sender_ref> release() && noexcept {
    sender_ref u(assoc ? __builtin_addressof(sndr) : nullptr);
    return std::pair<assoc_t, sender_ref>(static_cast<assoc_t&&>(assoc), static_cast<sender_ref&&>(u));
  }

private:
  explicit exec_associate_data(std::pair<assoc_t, sender_ref> parts) : assoc(static_cast<assoc_t&&>(parts.first)) {
    if (assoc)
      std::construct_at(__builtin_addressof(sndr), static_cast<wrap_sender&&>(*parts.second));
  }
  union {
    wrap_sender sndr;
  };
  assoc_t assoc;
};

template <class AD, class Rcvr>
struct exec_associate_op_state {
  using assoc_t = typename AD::assoc_t;
  using sender_ref_t = typename AD::sender_ref;
  using op_t = std::execution::connect_result_t<typename AD::wrap_sender, Rcvr>;
  assoc_t assoc;
  union {
    Rcvr* rcvr;
    op_t op;
  };
  explicit exec_associate_op_state(std::pair<assoc_t, sender_ref_t> parts, Rcvr& r) : assoc(static_cast<assoc_t&&>(parts.first)) {
    if (assoc)
      ::new (static_cast<void*>(__builtin_addressof(op))) op_t(std::execution::connect(static_cast<typename AD::wrap_sender&&>(*parts.second), static_cast<Rcvr&&>(r)));
    else
      rcvr = __builtin_addressof(r);
  }
  explicit exec_associate_op_state(AD&& ad, Rcvr& r) : exec_associate_op_state(static_cast<AD&&>(ad).release(), r) {}
  explicit exec_associate_op_state(const AD& ad, Rcvr& r)
    requires std::copy_constructible<AD>
      : exec_associate_op_state(AD(ad).release(), r) {}
  exec_associate_op_state(exec_associate_op_state&&) = delete;
  ~exec_associate_op_state() {
    if (assoc)
      op.~op_t();
  }
  void run() noexcept {
    if (assoc)
      std::execution::start(op);
    else
      std::execution::set_stopped(static_cast<Rcvr&&>(*rcvr));
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Token>
struct bind_scope_token : std::bool_constant<std::execution::scope_token<std::remove_cvref_t<Token>>> {};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct associate_t : ycxx::detail::exec::pipeable_adaptor<associate_t, 1, ycxx::detail::exec::bind_scope_token> {
  using ycxx::detail::exec::pipeable_adaptor<associate_t, 1, ycxx::detail::exec::bind_scope_token>::operator();
  template <sender Sndr, class Token>
    requires scope_token<remove_cvref_t<Token>>
  auto operator()(Sndr&& sndr, Token&& token) const {
    return ycxx::detail::exec::make_sender(
        *this, ycxx::adl_free::exec_associate_data<remove_cvref_t<Token>, Sndr>(static_cast<Token&&>(token), static_cast<Sndr&&>(sndr)));
  }
};
inline constexpr associate_t associate{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::associate_t> : default_impls {
  template <class Data>
  static constexpr auto get_attrs(const Data&) noexcept {
    return std::execution::env<>();
  }
  template <class Sndr, class Rcvr>
  static auto get_state(Sndr&& sndr, Rcvr& rcvr) noexcept(
      (std::is_same_v<Sndr, std::remove_cvref_t<Sndr>> || std::is_nothrow_constructible_v<std::remove_cvref_t<Sndr>, Sndr>) &&
      nothrow_callable<std::execution::connect_t, typename std::remove_cvref_t<data_type<Sndr>>::wrap_sender, Rcvr>) {
    using AD = std::remove_cvref_t<data_type<Sndr>>;
    return ::ycxx::adl_free::exec_associate_op_state<AD, Rcvr>(static_cast<Sndr&&>(sndr).template get<1>(), rcvr);
  }
  template <class State, class Rcvr>
  static void start(State& state, Rcvr&) noexcept {
    state.run();
  }
  template <class Sndr, class... Env>
  using csigs = sigs_concat_t<csigs_of_t<typename std::remove_cvref_t<data_type<Sndr>>::wrap_sender, fwd_env_t<Env>...>,
                              std::execution::completion_signatures<set_stopped_t()>>;
};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.spawn]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
struct exec_spawn_state_base {
  virtual void complete() noexcept = 0;

protected:
  ~exec_spawn_state_base() = default;
};
struct exec_spawn_receiver {
  using receiver_concept = std::execution::receiver_tag;
  exec_spawn_state_base* state;
  void set_value() && noexcept { state->complete(); }
  void set_stopped() && noexcept { state->complete(); }
};

template <class Alloc, class Token, class Sender>
struct exec_spawn_state final : exec_spawn_state_base {
  using op_t = std::execution::connect_result_t<Sender, exec_spawn_receiver>;
  using assoc_t = std::remove_cvref_t<decltype(std::declval<Token&>().try_associate())>;

  exec_spawn_state(Alloc a, Sender&& sndr, Token token)
      : alloc(static_cast<Alloc&&>(a)), op(std::execution::connect(static_cast<Sender&&>(sndr), exec_spawn_receiver{this})),
        assoc(token.try_associate()) {}
  void run() noexcept {
    if (assoc)
      std::execution::start(op);
    else
      complete();
  }
  void complete() noexcept override {
    auto a = static_cast<assoc_t&&>(assoc);
    using traits = typename std::allocator_traits<Alloc>::template rebind_traits<exec_spawn_state>;
    typename traits::allocator_type al(alloc);
    traits::destroy(al, this);
    traits::deallocate(al, this, 1);
  }

private:
  Alloc alloc;
  op_t op;
  assoc_t assoc;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// Allocates and constructs a T with an allocator (rebound), destroying and deallocating if the
// construction throws.
template <class T, class Alloc, class... Args>
T* new_with_allocator(const Alloc& alloc, Args&&... args) {
  using traits = typename std::allocator_traits<Alloc>::template rebind_traits<T>;
  typename traits::allocator_type al(alloc);
  T* p = traits::allocate(al, 1);
  if constexpr (std::is_nothrow_constructible_v<T, Args...> || !cfg::exceptions) {
    traits::construct(al, p, static_cast<Args&&>(args)...);
  } else {
    try {
      traits::construct(al, p, static_cast<Args&&>(args)...);
    } catch (...) {
      traits::deallocate(al, p, 1);
      throw;
    }
  }
  return p;
}

// The allocator and environment of spawn and spawn_future ([exec.spawn]/9, [exec.spawn.future]/19),
// passed to f(alloc, senv).
template <class NewSender, class Env, class F>
decltype(auto) with_spawn_allocator(const NewSender& new_sender, Env&& env, F&& f) {
  if constexpr (requires { std::get_allocator(env); })
    return static_cast<F&&>(f)(std::get_allocator(env), static_cast<Env&&>(env));
  else if constexpr (requires { std::get_allocator(std::execution::get_env(new_sender)); }) {
    auto alloc = std::get_allocator(std::execution::get_env(new_sender));
    return static_cast<F&&>(f)(alloc, ::ycxx::detail::exec::join_env(std::execution::prop(std::get_allocator, alloc), static_cast<Env&&>(env)));
  } else
    return static_cast<F&&>(f)(std::allocator<void>(), static_cast<Env&&>(env));
}
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct spawn_t {
  template <sender Sndr, class Token, class Env = env<>>
    requires scope_token<remove_cvref_t<Token>> && ycxx::detail::exec::queryable<remove_cvref_t<Env>>
  void operator()(Sndr&& sndr, Token&& token, Env&& env = {}) const {
    using T = remove_cvref_t<Token>;
    T tok(static_cast<Token&&>(token));
    auto&& new_sender = tok.wrap(static_cast<Sndr&&>(sndr));
    ycxx::detail::exec::with_spawn_allocator(new_sender, static_cast<Env&&>(env), [&](auto alloc, auto&& senv) {
      using S = decltype(write_env(static_cast<decltype(new_sender)&&>(new_sender), static_cast<decltype(senv)&&>(senv)));
      using State = ycxx::adl_free::exec_spawn_state<decltype(alloc), T, S>;
      auto* o = ycxx::detail::exec::new_with_allocator<State>(
          alloc, alloc, write_env(static_cast<decltype(new_sender)&&>(new_sender), static_cast<decltype(senv)&&>(senv)), tok);
      o->run();
    });
  }
};
inline constexpr spawn_t spawn{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.spawn.future]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
struct exec_try_cancelable {
  virtual void try_cancel() noexcept = 0;

protected:
  ~exec_try_cancelable() = default;
};

template <class Completions>
struct exec_spawn_future_state_base;
template <class... Sigs>
struct exec_spawn_future_state_base<std::execution::completion_signatures<Sigs...>> : exec_try_cancelable {
  using variant_t = std::conditional_t<
      (::ycxx::detail::exec::nothrow_decay_copy_sig<Sigs> && ...),
      ::ycxx::detail::exec::apply_unique_t<std::variant, std::monostate, std::tuple<std::execution::set_stopped_t>,
                                           typename ::ycxx::detail::exec::as_tuple_of_sig<Sigs>::type...>,
      ::ycxx::detail::exec::apply_unique_t<std::variant, std::monostate, std::tuple<std::execution::set_stopped_t>,
                                           std::tuple<std::execution::set_error_t, std::exception_ptr>,
                                           typename ::ycxx::detail::exec::as_tuple_of_sig<Sigs>::type...>>;
  variant_t result;
  virtual void complete() noexcept = 0;

protected:
  ~exec_spawn_future_state_base() = default;
};

template <class Completions>
struct exec_spawn_future_receiver {
  using receiver_concept = std::execution::receiver_tag;
  exec_spawn_future_state_base<Completions>* state;
  template <class... T>
  void set_value(T&&... t) && noexcept {
    set_complete<std::execution::set_value_t>(static_cast<T&&>(t)...);
  }
  template <class E>
  void set_error(E&& e) && noexcept {
    set_complete<std::execution::set_error_t>(static_cast<E&&>(e));
  }
  void set_stopped() && noexcept { set_complete<std::execution::set_stopped_t>(); }

private:
  template <class CPO, class... T>
  void set_complete(T&&... t) noexcept {
    constexpr bool nothrow = (std::is_nothrow_constructible_v<std::decay_t<T>, T> && ...);
    using tuple_t = ::ycxx::detail::exec::decayed_tuple<CPO, T...>;
    if constexpr (nothrow || !::ycxx::detail::cfg::exceptions) {
      state->result.template emplace<tuple_t>(CPO{}, static_cast<T&&>(t)...);
    } else {
      try {
        state->result.template emplace<tuple_t>(CPO{}, static_cast<T&&>(t)...);
      } catch (...) {
        state->result.template emplace<std::tuple<std::execution::set_error_t, std::exception_ptr>>(std::execution::set_error_t{},
                                                                                                  std::current_exception());
      }
    }
    state->complete();
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Sender, class Env>
using future_spawned_sender =
    decltype(std::execution::write_env(::ycxx::detail::exec::stop_when(std::declval<Sender>(), std::declval<std::inplace_stop_token>()),
                                       std::declval<Env>()));
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Alloc, class Token, class Sender, class Env>
struct exec_spawn_future_state final
    : exec_spawn_future_state_base<std::execution::completion_signatures_of_t<::ycxx::detail::exec::future_spawned_sender<Sender, Env>,
                                                                              std::execution::env<>>> {
  using sigs_t = std::execution::completion_signatures_of_t<::ycxx::detail::exec::future_spawned_sender<Sender, Env>, std::execution::env<>>;
  using base_t = exec_spawn_future_state_base<sigs_t>;
  using receiver_t = exec_spawn_future_receiver<sigs_t>;
  using op_t = std::execution::connect_result_t<::ycxx::detail::exec::future_spawned_sender<Sender, Env>, receiver_t>;

  exec_spawn_future_state(Alloc a, Sender&& sndr, Token token, Env env)
      : alloc(static_cast<Alloc&&>(a)),
        op(std::execution::connect(
            std::execution::write_env(::ycxx::detail::exec::stop_when(static_cast<Sender&&>(sndr), ssource.get_token()), static_cast<Env&&>(env)),
            receiver_t{this})),
        assoc(token.try_associate()) {
    if (assoc)
      std::execution::start(op);
    else
      std::execution::set_stopped(receiver_t{this});
  }

  // A registered receiver (the future operation's), type-erased.
  struct registration {
    void* rcvr = nullptr;
    void (*deliver)(void*, typename base_t::variant_t&) noexcept = nullptr;
    void (*stopped)(void*) noexcept = nullptr;
  };

  // complete ([exec.spawn.future]/10)
  void complete() noexcept override {
    lock_.lock();
    flags_ |= completed;
    const unsigned f = flags_;
    lock_.unlock();
    if (f & abandoned)
      doom();
    else if ((f & consumed) && !(f & cancelled)) {
      reg_.deliver(reg_.rcvr, this->result);
      doom();
    } else if (f & consumed)
      doom(); // the receiver was completed with set_stopped by try-set-stopped
  }
  // consume ([exec.spawn.future]/11)
  template <class Rcvr>
  void consume(Rcvr& rcvr) noexcept {
    lock_.lock();
    flags_ |= consumed;
    const unsigned f = flags_;
    if (!(f & (completed | cancelled)))
      reg_ = {__builtin_addressof(rcvr), &deliver_to<Rcvr>, &stopped_to<Rcvr>};
    lock_.unlock();
    if (f & completed) {
      deliver_to<Rcvr>(__builtin_addressof(rcvr), this->result);
      doom();
    } else if (f & cancelled) {
      std::execution::set_stopped(static_cast<Rcvr&&>(rcvr)); // complete() destroys the state later
    }
  }
  // try-cancel and try-set-stopped ([exec.spawn.future]/8, /12). The state is pinned meanwhile:
  // the request can complete the spawned operation, whose complete() (or a racing consume) would
  // otherwise destroy the state under this call.
  void try_cancel() noexcept override {
    lock_.lock();
    ++pins_;
    lock_.unlock();
    ssource.request_stop();
    try_set_stopped();
    lock_.lock();
    const bool last = --pins_ == 0 && doomed_;
    lock_.unlock();
    if (last)
      destroy();
  }
  void try_set_stopped() noexcept {
    lock_.lock();
    const unsigned before = flags_;
    flags_ |= cancelled;
    registration r = reg_;
    lock_.unlock();
    if ((before & consumed) && !(before & completed))
      r.stopped(r.rcvr);
  }
  // abandon ([exec.spawn.future]/13)
  void abandon() noexcept {
    lock_.lock();
    const bool done = (flags_ & completed) != 0;
    lock_.unlock();
    if (done) {
      doom();
      return;
    }
    ssource.request_stop();
    lock_.lock();
    flags_ |= abandoned;
    const bool done2 = (flags_ & completed) != 0;
    lock_.unlock();
    if (done2)
      doom();
  }

private:
  using assoc_t = std::remove_cvref_t<decltype(std::declval<Token&>().try_associate())>;
  enum : unsigned { completed = 1, consumed = 2, cancelled = 4, abandoned = 8 };

  template <class Rcvr>
  static void deliver_to(void* p, typename base_t::variant_t& result) noexcept {
    Rcvr& rcvr = *static_cast<Rcvr*>(p);
    std::visit(
        [&rcvr](auto& tuple) noexcept {
          if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(tuple)>, std::monostate>) {
            std::apply([&rcvr](auto cpo, auto&... vals) noexcept { cpo(static_cast<Rcvr&&>(rcvr), static_cast<std::remove_reference_t<decltype(vals)>&&>(vals)...); },
                       tuple);
          }
        },
        result);
  }
  template <class Rcvr>
  static void stopped_to(void* p) noexcept {
    std::execution::set_stopped(static_cast<Rcvr&&>(*static_cast<Rcvr*>(p)));
  }
  // Destroys the state, or leaves that to the try_cancel that pins it.
  void doom() noexcept {
    lock_.lock();
    doomed_ = true;
    const bool now = pins_ == 0;
    lock_.unlock();
    if (now)
      destroy();
  }
  void destroy() noexcept {
    auto associated = static_cast<assoc_t&&>(assoc);
    using traits = typename std::allocator_traits<Alloc>::template rebind_traits<exec_spawn_future_state>;
    typename traits::allocator_type al(static_cast<Alloc&&>(alloc));
    traits::destroy(al, this);
    traits::deallocate(al, this, 1);
  }

  Alloc alloc;
  std::inplace_stop_source ssource;
  op_t op;
  assoc_t assoc;
  ::ycxx::detail::exec::exec_spin_lock lock_;
  unsigned flags_ = 0;
  unsigned pins_ = 0;
  bool doomed_ = false;
  registration reg_;
};

// The owner of a spawn-future-state: destroying it abandons the state ([exec.spawn.future]/20.2).
template <class State>
struct exec_future_state_ptr {
  using ycxx_sigs = typename State::sigs_t;
  State* p = nullptr;
  exec_future_state_ptr() = default;
  explicit exec_future_state_ptr(State* q) noexcept : p(q) {}
  exec_future_state_ptr(exec_future_state_ptr&& o) noexcept : p(o.p) { o.p = nullptr; }
  exec_future_state_ptr& operator=(exec_future_state_ptr&& o) noexcept {
    if (this != __builtin_addressof(o)) {
      reset();
      p = o.p;
      o.p = nullptr;
    }
    return *this;
  }
  ~exec_future_state_ptr() { reset(); }
  void reset() noexcept {
    if (State* q = p) {
      p = nullptr;
      q->abandon();
    }
  }
  State* get() const noexcept { return p; }
  State* release() noexcept {
    State* q = p;
    p = nullptr;
    return q;
  }
};

// future-operation ([exec.spawn.future]/15)
template <class StatePtr, class Rcvr>
struct exec_future_operation {
  struct callback {
    exec_try_cancelable* state;
    void operator()() noexcept { state->try_cancel(); }
  };
  using stop_token_t = std::stop_token_of_t<std::execution::env_of_t<Rcvr>>;
  using stop_callback_t = std::stop_callback_for_t<stop_token_t, callback>;
  struct rcvr_t {
    using receiver_concept = std::execution::receiver_tag;
    exec_future_operation* op;
    template <class... T>
    void set_value(T&&... ts) && noexcept {
      op->template set_complete<std::execution::set_value_t>(static_cast<T&&>(ts)...);
    }
    template <class E>
    void set_error(E&& e) && noexcept {
      op->template set_complete<std::execution::set_error_t>(static_cast<E&&>(e));
    }
    void set_stopped() && noexcept { op->template set_complete<std::execution::set_stopped_t>(); }
    std::execution::env_of_t<Rcvr> get_env() const noexcept { return std::execution::get_env(op->rcvr); }
  };

  Rcvr rcvr;
  StatePtr state;
  rcvr_t inner;
  std::optional<stop_callback_t> stopCallback;

  exec_future_operation(StatePtr s, Rcvr r) noexcept
      : rcvr(static_cast<Rcvr&&>(r)), state(static_cast<StatePtr&&>(s)), inner{this} {}
  exec_future_operation(exec_future_operation&&) = delete;

  void run() & noexcept {
    stopCallback.emplace(std::get_stop_token(std::execution::get_env(rcvr)), callback{state.get()});
    state.release()->consume(inner);
  }
  template <class CPO, class... T>
  void set_complete(T&&... ts) noexcept {
    stopCallback.reset();
    CPO{}(static_cast<Rcvr&&>(rcvr), static_cast<T&&>(ts)...);
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct spawn_future_t {
  template <sender Sndr, class Token, class Env = env<>>
    requires scope_token<remove_cvref_t<Token>> && ycxx::detail::exec::queryable<remove_cvref_t<Env>>
  auto operator()(Sndr&& sndr, Token&& token, Env&& env = {}) const {
    using T = remove_cvref_t<Token>;
    T tok(static_cast<Token&&>(token));
    auto&& new_sender = tok.wrap(static_cast<Sndr&&>(sndr));
    return ycxx::detail::exec::with_spawn_allocator(new_sender, static_cast<Env&&>(env), [&](auto alloc, auto&& senv) {
      using NS = decltype(new_sender);
      using SE = remove_cvref_t<decltype(senv)>;
      using State = ycxx::adl_free::exec_spawn_future_state<decltype(alloc), T, NS, SE>;
      auto* s = ycxx::detail::exec::new_with_allocator<State>(alloc, alloc, static_cast<NS&&>(new_sender), tok,
                                                              SE(static_cast<decltype(senv)&&>(senv)));
      return ycxx::detail::exec::make_sender(*this, ycxx::adl_free::exec_future_state_ptr<State>(s));
    });
  }
};
inline constexpr spawn_future_t spawn_future{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::spawn_future_t> : default_impls {
  template <class Data>
  static constexpr auto get_attrs(const Data&) noexcept {
    return std::execution::env<>();
  }
  template <class Sndr, class Rcvr>
  static auto get_state(Sndr&& sndr, Rcvr& rcvr) noexcept {
    using state_ptr = std::remove_cvref_t<data_type<Sndr>>;
    return ::ycxx::adl_free::exec_future_operation<state_ptr, Rcvr>(static_cast<Sndr&&>(sndr).template get<1>(), static_cast<Rcvr&&>(rcvr));
  }
  template <class State, class Rcvr>
  static void start(State& state, Rcvr&) noexcept {
    state.run();
  }
  template <class Sndr, class... Env>
  using csigs = sigs_concat_t<sigs_map_t<typename std::remove_cvref_t<data_type<Sndr>>::ycxx_sigs, decayed_sig_t>,
                              std::execution::completion_signatures<set_stopped_t()>,
                              std::conditional_t<nothrow_decay_copy_sigs<typename std::remove_cvref_t<data_type<Sndr>>::ycxx_sigs>, no_sigs, eptr_sigs>>;
};
}}} // namespace ycxx::detail::exec
