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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// A small spin lock (uncontended in practice: every critical section is a few stores).
struct __exec_spin_lock {
  unsigned __word = 0;
  void lock() noexcept {
    for (int __spins = 0; __atomic_exchange_n(&__word, 1u, __ATOMIC_ACQUIRE) != 0;)
      while (__atomic_load_n(&__word, __ATOMIC_RELAXED) != 0)
        if (++__spins > 64)
          ::__ycxx_pal_thread_yield();
  }
  void unlock() noexcept { __atomic_store_n(&__word, 0u, __ATOMIC_RELEASE); }
};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.scope.concepts]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// test-sender, test-env ([exec.scope.concepts]/4)
struct __exec_test_sender {
  using sender_concept = std::execution::sender_tag;
  template <class _Self, class... _Env>
  using __ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t()>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return std::execution::completion_signatures<std::execution::set_value_t()>();
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
template <class _Assoc>
concept scope_association = movable<_Assoc> && is_nothrow_move_constructible_v<_Assoc> && is_nothrow_move_assignable_v<_Assoc> &&
                            default_initializable<_Assoc> && requires(const _Assoc __assoc) {
                              { static_cast<bool>(__assoc) } noexcept;
                              { __assoc.try_associate() } -> same_as<_Assoc>;
                            };
template <class _Token>
concept scope_token = copyable<_Token> && requires(const _Token token) {
  { token.try_associate() } -> scope_association;
  { token.wrap(declval<__ycxx::__adl_free::__exec_test_sender>()) } -> sender_in<env<>>;
};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.counting.scopes]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
struct __scope_join_t {};
struct __scope_access;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// association-t<Scope> ([exec.counting.scopes.general]/5)
template <class _Scope>
class __exec_scope_association {
  _Scope* __scope_ = nullptr;
  friend _Scope;
  friend struct ::__ycxx::__detail::__exec::__scope_access;
  constexpr explicit __exec_scope_association(_Scope* s) noexcept : __scope_(s) {}

public:
  constexpr __exec_scope_association() noexcept = default;
  __exec_scope_association(__exec_scope_association&& __o) noexcept : __scope_(__o.__scope_) { __o.__scope_ = nullptr; }
  __exec_scope_association& operator=(__exec_scope_association&& __o) noexcept {
    if (this != __builtin_addressof(__o)) {
      if (__scope_)
        __scope_->__ycxx_disassociate();
      __scope_ = __o.__scope_;
      __o.__scope_ = nullptr;
    }
    return *this;
  }
  ~__exec_scope_association() {
    if (__scope_)
      __scope_->__ycxx_disassociate();
  }
  explicit operator bool() const noexcept { return __scope_ != nullptr; }
  __exec_scope_association try_associate() const noexcept { return __scope_ ? __scope_->__ycxx_try_associate() : __exec_scope_association(); }
};

// A join operation registered with a scope until its count reaches zero.
struct __exec_join_node {
  void (*complete)(__exec_join_node*) noexcept;
  __exec_join_node* next = nullptr;
};

// The state machine of [exec.counting.scopes.general]/1 shared by both scopes.
class __exec_counting_scope_core {
protected:
  enum : unsigned char { __y_unused, open, __closed, __open_and_joining, __closed_and_joining, __unused_and_closed, __joined };
  ::__ycxx::__detail::__exec::__exec_spin_lock __lock_;
  unsigned char __state_ = __y_unused;
  std::size_t __count_ = 0;
  __exec_join_node* __joiners_ = nullptr;

  bool __try_associate_core(std::size_t max) noexcept {
    __lock_.lock();
    bool ok = false;
    if (__count_ != max) {
      if (__state_ == __y_unused) {
        ++__count_;
        __state_ = open;
        ok = true;
      } else if (__state_ == open || __state_ == __open_and_joining) {
        ++__count_;
        ok = true;
      }
    }
    __lock_.unlock();
    return ok;
  }
  void __disassociate_core() noexcept {
    __lock_.lock();
    __exec_join_node* done = nullptr;
    if (--__count_ == 0 && (__state_ == __open_and_joining || __state_ == __closed_and_joining)) {
      __state_ = __joined;
      done = __joiners_;
      __joiners_ = nullptr;
    }
    __lock_.unlock();
    // complete() may destroy the scope: nothing of *this is touched from here on.
    while (done) {
      __exec_join_node* next = done->next;
      done->complete(done);
      done = next;
    }
  }
  void __close_core() noexcept {
    __lock_.lock();
    if (__state_ == __y_unused)
      __state_ = __unused_and_closed;
    else if (__state_ == open)
      __state_ = __closed;
    else if (__state_ == __open_and_joining)
      __state_ = __closed_and_joining;
    __lock_.unlock();
  }
  // start-join-sender: true if the count is already zero (the join completes inline).
  bool __start_join_core(__exec_join_node* n) noexcept {
    __lock_.lock();
    if (__count_ == 0) {
      __state_ = __joined;
      __lock_.unlock();
      return true;
    }
    if (__state_ == open || __state_ == __open_and_joining || __state_ == __y_unused)
      __state_ = __open_and_joining;
    else
      __state_ = __closed_and_joining;
    n->next = __joiners_;
    __joiners_ = n;
    __lock_.unlock();
    return false;
  }
  void __check_destroy() noexcept {
    if (__state_ != __joined && __state_ != __y_unused && __state_ != __unused_and_closed)
      std::terminate();
  }

public:
  __exec_counting_scope_core() noexcept = default;
  __exec_counting_scope_core(__exec_counting_scope_core&&) = delete;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
struct __scope_access {
  template <class _Scope>
  static bool __start_join(_Scope* s, ::__ycxx::__adl_free::__exec_join_node* n) noexcept {
    return s->__start_join_core(n);
  }
};

template <class _Scope, class _Rcvr>
struct __scope_join_state : ::__ycxx::__adl_free::__exec_join_node {
  struct __rcvr_t {
    using receiver_concept = std::execution::receiver_tag;
    _Rcvr& __rcvr;
    void set_value() && noexcept { std::execution::set_value(static_cast<_Rcvr&&>(__rcvr)); }
    template <class _Ep>
    void set_error(_Ep&& e) && noexcept {
      std::execution::set_error(static_cast<_Rcvr&&>(__rcvr), static_cast<_Ep&&>(e));
    }
    void set_stopped() && noexcept { std::execution::set_stopped(static_cast<_Rcvr&&>(__rcvr)); }
    decltype(auto) get_env() const noexcept { return std::execution::get_env(__rcvr); }
  };
  using __sched_sender = decltype(std::execution::schedule(std::execution::get_start_scheduler(std::execution::get_env(std::declval<_Rcvr&>()))));
  using __op_t = std::execution::connect_result_t<__sched_sender, __rcvr_t>;

  _Scope* scope;
  _Rcvr& receiver;
  __op_t op;

  __scope_join_state(_Scope* s, _Rcvr& r) noexcept(__nothrow_callable<std::execution::connect_t, __sched_sender, __rcvr_t>)
      : ::__ycxx::__adl_free::__exec_join_node{&__run_complete}, scope(s), receiver(r),
        op(std::execution::connect(std::execution::schedule(std::execution::get_start_scheduler(std::execution::get_env(r))), __rcvr_t{r})) {}
  __scope_join_state(__scope_join_state&&) = delete;

  static void __run_complete(::__ycxx::__adl_free::__exec_join_node* n) noexcept { std::execution::start(static_cast<__scope_join_state*>(n)->op); }
  void __complete_inline() noexcept { std::execution::set_value(static_cast<_Rcvr&&>(receiver)); }
};

template <class _Env>
struct __scope_join_sigs {
  static auto __pick() {
    if constexpr (requires(const _Env& e) { std::execution::schedule(std::execution::get_start_scheduler(e)); })
      return std::type_identity<__sigs_concat_t<std::execution::completion_signatures<set_value_t()>,
                                              __csigs_of_t<decltype(std::execution::schedule(std::execution::get_start_scheduler(std::declval<const _Env&>()))), _Env>>>{};
    else
      return std::type_identity<__invalid_sigs<__environment_has_no_start_scheduler, _Env>>{};
  }
  using type = typename decltype(__pick())::type;
};

template <>
struct __impls_for<__scope_join_t> : __default_impls {
  template <class _Data>
  static constexpr auto __get_attrs(const _Data&) noexcept {
    return std::execution::env<>();
  }
  template <class _Sndr, class _Rcvr>
  static auto __get_state(_Sndr&& sender, _Rcvr& receiver) noexcept(
      std::is_nothrow_constructible_v<__scope_join_state<std::remove_pointer_t<std::decay_t<__data_type<_Sndr>>>, _Rcvr>,
                                      std::decay_t<__data_type<_Sndr>>, _Rcvr&>) {
    auto __self = sender.template get<1>();
    return __scope_join_state<std::remove_pointer_t<decltype(__self)>, _Rcvr>(__self, receiver);
  }
  template <class _State, class _Rcvr>
  static void start(_State& s, _Rcvr&) noexcept {
    if (__scope_access::__start_join(s.scope, &s))
      s.__complete_inline();
  }
  template <class _Sndr, class... _Env>
  struct __sigs {
    using type = __dependent_sigs;
  };
  template <class _Sndr, class _Env>
  struct __sigs<_Sndr, _Env> {
    using type = typename __scope_join_sigs<_Env>::type;
  };
  template <class _Sndr, class... _Env>
  using __csigs = typename __sigs<_Sndr, _Env...>::type;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

class simple_counting_scope : __ycxx::__adl_free::__exec_counting_scope_core {
  friend struct __ycxx::__detail::__exec::__scope_access;
  friend class __ycxx::__adl_free::__exec_scope_association<simple_counting_scope>;
  using __assoc_t = __ycxx::__adl_free::__exec_scope_association<simple_counting_scope>;

  __assoc_t __ycxx_try_associate() noexcept { return __try_associate_core(max_associations) ? __assoc_t(this) : __assoc_t(); }
  void __ycxx_disassociate() noexcept { __disassociate_core(); }

public:
  struct token {
    template <sender _Sender>
    _Sender&& wrap(_Sender&& __snd) const noexcept {
      return static_cast<_Sender&&>(__snd);
    }
    __assoc_t try_associate() const noexcept { return scope->__ycxx_try_associate(); }

  private:
    friend class simple_counting_scope;
    explicit token(simple_counting_scope* s) noexcept : scope(s) {}
    simple_counting_scope* scope;
  };

  static constexpr size_t max_associations = static_cast<size_t>(-1) >> 1;

  simple_counting_scope() noexcept = default;
  simple_counting_scope(simple_counting_scope&&) = delete;
  ~simple_counting_scope() { __check_destroy(); }

  token get_token() noexcept { return token(this); }
  void close() noexcept { __close_core(); }
  sender auto join() noexcept;
};

class counting_scope : __ycxx::__adl_free::__exec_counting_scope_core {
  friend struct __ycxx::__detail::__exec::__scope_access;
  friend class __ycxx::__adl_free::__exec_scope_association<counting_scope>;
  using __assoc_t = __ycxx::__adl_free::__exec_scope_association<counting_scope>;

  inplace_stop_source __s_source;
  __assoc_t __ycxx_try_associate() noexcept { return __try_associate_core(max_associations) ? __assoc_t(this) : __assoc_t(); }
  void __ycxx_disassociate() noexcept { __disassociate_core(); }

public:
  struct token {
    template <sender _Sender>
    sender auto wrap(_Sender&& __snd) const noexcept(is_nothrow_constructible_v<remove_cvref_t<_Sender>, _Sender>) {
      return __ycxx::__detail::__exec::__stop_when(static_cast<_Sender&&>(__snd), scope->__s_source.get_token());
    }
    __assoc_t try_associate() const noexcept { return scope->__ycxx_try_associate(); }

  private:
    friend class counting_scope;
    explicit token(counting_scope* s) noexcept : scope(s) {}
    counting_scope* scope;
  };

  static constexpr size_t max_associations = static_cast<size_t>(-1) >> 1;

  counting_scope() noexcept = default;
  counting_scope(counting_scope&&) = delete;
  ~counting_scope() { __check_destroy(); }

  token get_token() noexcept { return token(this); }
  void close() noexcept { __close_core(); }
  sender auto join() noexcept;
  void request_stop() noexcept { __s_source.request_stop(); }
};

// Defined after the classes: the join sender's impls-for needs them complete.
inline sender auto simple_counting_scope::join() noexcept { return __ycxx::__detail::__exec::__make_sender(__ycxx::__detail::__exec::__scope_join_t(), this); }
inline sender auto counting_scope::join() noexcept { return __ycxx::__detail::__exec::__make_sender(__ycxx::__detail::__exec::__scope_join_t(), this); }

}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.associate]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Token, class _Sender>
struct __exec_associate_data {
  using __wrap_sender = std::remove_cvref_t<decltype(std::declval<_Token&>().wrap(std::declval<_Sender>()))>;
  using __assoc_t = decltype(std::declval<_Token&>().try_associate());
  // sender-ref: owns the wrapped sender (destroy_at on destruction).
  struct __sender_ref {
    __wrap_sender* p = nullptr;
    __sender_ref() = default;
    explicit __sender_ref(__wrap_sender* __q) noexcept : p(__q) {}
    __sender_ref(__sender_ref&& __o) noexcept : p(__o.p) { __o.p = nullptr; }
    ~__sender_ref() {
      if (p)
        std::destroy_at(p);
    }
    __wrap_sender* release() noexcept {
      __wrap_sender* __q = p;
      p = nullptr;
      return __q;
    }
    __wrap_sender& operator*() const noexcept { return *p; }
  };

  explicit __exec_associate_data(_Token t, _Sender&& s)
      : __sndr(t.wrap(static_cast<_Sender&&>(s))), __assoc([&] {
          __sender_ref __guard{__builtin_addressof(__sndr)};
          auto a = t.try_associate();
          if (a)
            __guard.release();
          return a;
        }()) {}
  __exec_associate_data(const __exec_associate_data& other) noexcept(std::is_nothrow_copy_constructible_v<__wrap_sender> &&
                                                                 noexcept(other.__assoc.try_associate()))
    requires std::copy_constructible<__wrap_sender>
      : __assoc(other.__assoc.try_associate()) {
    if (__assoc)
      std::construct_at(__builtin_addressof(__sndr), other.__sndr);
  }
  __exec_associate_data(__exec_associate_data&& other) noexcept(std::is_nothrow_move_constructible_v<__wrap_sender>)
      : __exec_associate_data(static_cast<__exec_associate_data&&>(other).release()) {}
  ~__exec_associate_data() {
    if (__assoc)
      __sndr.~__wrap_sender();
  }
  std::pair<__assoc_t, __sender_ref> release() && noexcept {
    __sender_ref __u(__assoc ? __builtin_addressof(__sndr) : nullptr);
    return std::pair<__assoc_t, __sender_ref>(static_cast<__assoc_t&&>(__assoc), static_cast<__sender_ref&&>(__u));
  }

private:
  explicit __exec_associate_data(std::pair<__assoc_t, __sender_ref> __parts) : __assoc(static_cast<__assoc_t&&>(__parts.first)) {
    if (__assoc)
      std::construct_at(__builtin_addressof(__sndr), static_cast<__wrap_sender&&>(*__parts.second));
  }
  union {
    __wrap_sender __sndr;
  };
  __assoc_t __assoc;
};

template <class _AD, class _Rcvr>
struct __exec_associate_op_state {
  using __assoc_t = typename _AD::__assoc_t;
  using __sender_ref_t = typename _AD::__sender_ref;
  using __op_t = std::execution::connect_result_t<typename _AD::__wrap_sender, _Rcvr>;
  __assoc_t __assoc;
  union {
    _Rcvr* __rcvr;
    __op_t op;
  };
  explicit __exec_associate_op_state(std::pair<__assoc_t, __sender_ref_t> __parts, _Rcvr& r) : __assoc(static_cast<__assoc_t&&>(__parts.first)) {
    if (__assoc)
      ::new (static_cast<void*>(__builtin_addressof(op))) __op_t(std::execution::connect(static_cast<typename _AD::__wrap_sender&&>(*__parts.second), static_cast<_Rcvr&&>(r)));
    else
      __rcvr = __builtin_addressof(r);
  }
  explicit __exec_associate_op_state(_AD&& __ad, _Rcvr& r) : __exec_associate_op_state(static_cast<_AD&&>(__ad).release(), r) {}
  explicit __exec_associate_op_state(const _AD& __ad, _Rcvr& r)
    requires std::copy_constructible<_AD>
      : __exec_associate_op_state(_AD(__ad).release(), r) {}
  __exec_associate_op_state(__exec_associate_op_state&&) = delete;
  ~__exec_associate_op_state() {
    if (__assoc)
      op.~__op_t();
  }
  void run() noexcept {
    if (__assoc)
      std::execution::start(op);
    else
      std::execution::set_stopped(static_cast<_Rcvr&&>(*__rcvr));
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Token>
struct __bind_scope_token : std::bool_constant<std::execution::scope_token<std::remove_cvref_t<_Token>>> {};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct associate_t : __ycxx::__detail::__exec::__pipeable_adaptor<associate_t, 1, __ycxx::__detail::__exec::__bind_scope_token> {
  using __ycxx::__detail::__exec::__pipeable_adaptor<associate_t, 1, __ycxx::__detail::__exec::__bind_scope_token>::operator();
  template <sender _Sndr, class _Token>
    requires scope_token<remove_cvref_t<_Token>>
  auto operator()(_Sndr&& __sndr, _Token&& token) const {
    return __ycxx::__detail::__exec::__make_sender(
        *this, __ycxx::__adl_free::__exec_associate_data<remove_cvref_t<_Token>, _Sndr>(static_cast<_Token&&>(token), static_cast<_Sndr&&>(__sndr)));
  }
};
inline constexpr associate_t associate{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::associate_t> : __default_impls {
  template <class _Data>
  static constexpr auto __get_attrs(const _Data&) noexcept {
    return std::execution::env<>();
  }
  template <class _Sndr, class _Rcvr>
  static auto __get_state(_Sndr&& __sndr, _Rcvr& __rcvr) noexcept(
      (std::is_same_v<_Sndr, std::remove_cvref_t<_Sndr>> || std::is_nothrow_constructible_v<std::remove_cvref_t<_Sndr>, _Sndr>) &&
      __nothrow_callable<std::execution::connect_t, typename std::remove_cvref_t<__data_type<_Sndr>>::__wrap_sender, _Rcvr>) {
    using _AD = std::remove_cvref_t<__data_type<_Sndr>>;
    return ::__ycxx::__adl_free::__exec_associate_op_state<_AD, _Rcvr>(static_cast<_Sndr&&>(__sndr).template get<1>(), __rcvr);
  }
  template <class _State, class _Rcvr>
  static void start(_State& state, _Rcvr&) noexcept {
    state.run();
  }
  template <class _Sndr, class... _Env>
  using __csigs = __sigs_concat_t<__csigs_of_t<typename std::remove_cvref_t<__data_type<_Sndr>>::__wrap_sender, __fwd_env_t<_Env>...>,
                              std::execution::completion_signatures<set_stopped_t()>>;
};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.spawn]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
struct __exec_spawn_state_base {
  virtual void complete() noexcept = 0;

protected:
  ~__exec_spawn_state_base() = default;
};
struct __exec_spawn_receiver {
  using receiver_concept = std::execution::receiver_tag;
  __exec_spawn_state_base* state;
  void set_value() && noexcept { state->complete(); }
  void set_stopped() && noexcept { state->complete(); }
};

template <class _Alloc, class _Token, class _Sender>
struct __exec_spawn_state final : __exec_spawn_state_base {
  using __op_t = std::execution::connect_result_t<_Sender, __exec_spawn_receiver>;
  using __assoc_t = std::remove_cvref_t<decltype(std::declval<_Token&>().try_associate())>;

  __exec_spawn_state(_Alloc a, _Sender&& __sndr, _Token token)
      : __alloc(static_cast<_Alloc&&>(a)), op(std::execution::connect(static_cast<_Sender&&>(__sndr), __exec_spawn_receiver{this})),
        __assoc(token.try_associate()) {}
  void run() noexcept {
    if (__assoc)
      std::execution::start(op);
    else
      complete();
  }
  void complete() noexcept override {
    auto a = static_cast<__assoc_t&&>(__assoc);
    using __traits = typename std::allocator_traits<_Alloc>::template rebind_traits<__exec_spawn_state>;
    typename __traits::allocator_type __al(__alloc);
    __traits::destroy(__al, this);
    __traits::deallocate(__al, this, 1);
  }

private:
  _Alloc __alloc;
  __op_t op;
  __assoc_t __assoc;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// Allocates and constructs a T with an allocator (rebound), destroying and deallocating if the
// construction throws.
template <class _Tp, class _Alloc, class... _Args>
_Tp* __new_with_allocator(const _Alloc& __alloc, _Args&&... __args) {
  using __traits = typename std::allocator_traits<_Alloc>::template rebind_traits<_Tp>;
  typename __traits::allocator_type __al(__alloc);
  _Tp* p = __traits::allocate(__al, 1);
  if constexpr (std::is_nothrow_constructible_v<_Tp, _Args...> || !__cfg::exceptions) {
    __traits::construct(__al, p, static_cast<_Args&&>(__args)...);
  } else {
    try {
      __traits::construct(__al, p, static_cast<_Args&&>(__args)...);
    } catch (...) {
      __traits::deallocate(__al, p, 1);
      throw;
    }
  }
  return p;
}

// The allocator and environment of spawn and spawn_future ([exec.spawn]/9, [exec.spawn.future]/19),
// passed to f(alloc, senv).
template <class _NewSender, class _Env, class _Fp>
decltype(auto) __with_spawn_allocator(const _NewSender& __new_sender, _Env&& env, _Fp&& __f) {
  if constexpr (requires { std::get_allocator(env); })
    return static_cast<_Fp&&>(__f)(std::get_allocator(env), static_cast<_Env&&>(env));
  else if constexpr (requires { std::get_allocator(std::execution::get_env(__new_sender)); }) {
    auto __alloc = std::get_allocator(std::execution::get_env(__new_sender));
    return static_cast<_Fp&&>(__f)(__alloc, ::__ycxx::__detail::__exec::__join_env(std::execution::prop(std::get_allocator, __alloc), static_cast<_Env&&>(env)));
  } else
    return static_cast<_Fp&&>(__f)(std::allocator<void>(), static_cast<_Env&&>(env));
}
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct spawn_t {
  template <sender _Sndr, class _Token, class _Env = env<>>
    requires scope_token<remove_cvref_t<_Token>> && __ycxx::__detail::__exec::__queryable<remove_cvref_t<_Env>>
  void operator()(_Sndr&& __sndr, _Token&& token, _Env&& env = {}) const {
    using _Tp = remove_cvref_t<_Token>;
    _Tp __tok(static_cast<_Token&&>(token));
    auto&& __new_sender = __tok.wrap(static_cast<_Sndr&&>(__sndr));
    __ycxx::__detail::__exec::__with_spawn_allocator(__new_sender, static_cast<_Env&&>(env), [&](auto __alloc, auto&& __senv) {
      using _Sp = decltype(write_env(static_cast<decltype(__new_sender)&&>(__new_sender), static_cast<decltype(__senv)&&>(__senv)));
      using _State = __ycxx::__adl_free::__exec_spawn_state<decltype(__alloc), _Tp, _Sp>;
      auto* __o = __ycxx::__detail::__exec::__new_with_allocator<_State>(
          __alloc, __alloc, write_env(static_cast<decltype(__new_sender)&&>(__new_sender), static_cast<decltype(__senv)&&>(__senv)), __tok);
      __o->run();
    });
  }
};
inline constexpr spawn_t spawn{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.spawn.future]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
struct __exec_try_cancelable {
  virtual void __try_cancel() noexcept = 0;

protected:
  ~__exec_try_cancelable() = default;
};

template <class _Completions>
struct __exec_spawn_future_state_base;
template <class... _Sigs>
struct __exec_spawn_future_state_base<std::execution::completion_signatures<_Sigs...>> : __exec_try_cancelable {
  using __variant_t = std::conditional_t<
      (::__ycxx::__detail::__exec::__nothrow_decay_copy_sig<_Sigs> && ...),
      ::__ycxx::__detail::__exec::__apply_unique_t<std::variant, std::monostate, std::tuple<std::execution::set_stopped_t>,
                                           typename ::__ycxx::__detail::__exec::__as_tuple_of_sig<_Sigs>::type...>,
      ::__ycxx::__detail::__exec::__apply_unique_t<std::variant, std::monostate, std::tuple<std::execution::set_stopped_t>,
                                           std::tuple<std::execution::set_error_t, std::exception_ptr>,
                                           typename ::__ycxx::__detail::__exec::__as_tuple_of_sig<_Sigs>::type...>>;
  __variant_t result;
  virtual void complete() noexcept = 0;

protected:
  ~__exec_spawn_future_state_base() = default;
};

template <class _Completions>
struct __exec_spawn_future_receiver {
  using receiver_concept = std::execution::receiver_tag;
  __exec_spawn_future_state_base<_Completions>* state;
  template <class... _Tp>
  void set_value(_Tp&&... t) && noexcept {
    __set_complete<std::execution::set_value_t>(static_cast<_Tp&&>(t)...);
  }
  template <class _Ep>
  void set_error(_Ep&& e) && noexcept {
    __set_complete<std::execution::set_error_t>(static_cast<_Ep&&>(e));
  }
  void set_stopped() && noexcept { __set_complete<std::execution::set_stopped_t>(); }

private:
  template <class _CPO, class... _Tp>
  void __set_complete(_Tp&&... t) noexcept {
    constexpr bool nothrow = (std::is_nothrow_constructible_v<std::decay_t<_Tp>, _Tp> && ...);
    using __tuple_t = ::__ycxx::__detail::__exec::__decayed_tuple<_CPO, _Tp...>;
    if constexpr (nothrow || !::__ycxx::__detail::__cfg::exceptions) {
      state->result.template emplace<__tuple_t>(_CPO{}, static_cast<_Tp&&>(t)...);
    } else {
      try {
        state->result.template emplace<__tuple_t>(_CPO{}, static_cast<_Tp&&>(t)...);
      } catch (...) {
        state->result.template emplace<std::tuple<std::execution::set_error_t, std::exception_ptr>>(std::execution::set_error_t{},
                                                                                                  std::current_exception());
      }
    }
    state->complete();
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Sender, class _Env>
using __future_spawned_sender =
    decltype(std::execution::write_env(::__ycxx::__detail::__exec::__stop_when(std::declval<_Sender>(), std::declval<std::inplace_stop_token>()),
                                       std::declval<_Env>()));
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Alloc, class _Token, class _Sender, class _Env>
struct __exec_spawn_future_state final
    : __exec_spawn_future_state_base<std::execution::completion_signatures_of_t<::__ycxx::__detail::__exec::__future_spawned_sender<_Sender, _Env>,
                                                                              std::execution::env<>>> {
  using __sigs_t = std::execution::completion_signatures_of_t<::__ycxx::__detail::__exec::__future_spawned_sender<_Sender, _Env>, std::execution::env<>>;
  using __base_t = __exec_spawn_future_state_base<__sigs_t>;
  using __receiver_t = __exec_spawn_future_receiver<__sigs_t>;
  using __op_t = std::execution::connect_result_t<::__ycxx::__detail::__exec::__future_spawned_sender<_Sender, _Env>, __receiver_t>;

  __exec_spawn_future_state(_Alloc a, _Sender&& __sndr, _Token token, _Env env)
      : __alloc(static_cast<_Alloc&&>(a)),
        op(std::execution::connect(
            std::execution::write_env(::__ycxx::__detail::__exec::__stop_when(static_cast<_Sender&&>(__sndr), __ssource.get_token()), static_cast<_Env&&>(env)),
            __receiver_t{this})),
        __assoc(token.try_associate()) {
    if (__assoc)
      std::execution::start(op);
    else
      std::execution::set_stopped(__receiver_t{this});
  }

  // A registered receiver (the future operation's), type-erased.
  struct __registration {
    void* __rcvr = nullptr;
    void (*__deliver)(void*, typename __base_t::__variant_t&) noexcept = nullptr;
    void (*__stopped)(void*) noexcept = nullptr;
  };

  // complete ([exec.spawn.future]/10)
  void complete() noexcept override {
    __lock_.lock();
    __flags_ |= __completed;
    const unsigned __f = __flags_;
    __lock_.unlock();
    if (__f & __abandoned)
      __doom();
    else if ((__f & __consumed) && !(__f & __cancelled)) {
      __reg_.__deliver(__reg_.__rcvr, this->result);
      __doom();
    } else if (__f & __consumed)
      __doom(); // the receiver was completed with set_stopped by try-set-stopped
  }
  // consume ([exec.spawn.future]/11)
  template <class _Rcvr>
  void consume(_Rcvr& __rcvr) noexcept {
    __lock_.lock();
    __flags_ |= __consumed;
    const unsigned __f = __flags_;
    if (!(__f & (__completed | __cancelled)))
      __reg_ = {__builtin_addressof(__rcvr), &__deliver_to<_Rcvr>, &__stopped_to<_Rcvr>};
    __lock_.unlock();
    if (__f & __completed) {
      __deliver_to<_Rcvr>(__builtin_addressof(__rcvr), this->result);
      __doom();
    } else if (__f & __cancelled) {
      std::execution::set_stopped(static_cast<_Rcvr&&>(__rcvr)); // complete() destroys the state later
    }
  }
  // try-cancel and try-set-stopped ([exec.spawn.future]/8, /12). The state is pinned meanwhile:
  // the request can complete the spawned operation, whose complete() (or a racing consume) would
  // otherwise destroy the state under this call.
  void __try_cancel() noexcept override {
    __lock_.lock();
    ++__pins_;
    __lock_.unlock();
    __ssource.request_stop();
    __try_set_stopped();
    __lock_.lock();
    const bool last = --__pins_ == 0 && __doomed_;
    __lock_.unlock();
    if (last)
      destroy();
  }
  void __try_set_stopped() noexcept {
    __lock_.lock();
    const unsigned before = __flags_;
    __flags_ |= __cancelled;
    __registration r = __reg_;
    __lock_.unlock();
    if ((before & __consumed) && !(before & __completed))
      r.__stopped(r.__rcvr);
  }
  // abandon ([exec.spawn.future]/13)
  void abandon() noexcept {
    __lock_.lock();
    const bool done = (__flags_ & __completed) != 0;
    __lock_.unlock();
    if (done) {
      __doom();
      return;
    }
    __ssource.request_stop();
    __lock_.lock();
    __flags_ |= __abandoned;
    const bool __done2 = (__flags_ & __completed) != 0;
    __lock_.unlock();
    if (__done2)
      __doom();
  }

private:
  using __assoc_t = std::remove_cvref_t<decltype(std::declval<_Token&>().try_associate())>;
  enum : unsigned { __completed = 1, __consumed = 2, __cancelled = 4, __abandoned = 8 };

  template <class _Rcvr>
  static void __deliver_to(void* p, typename __base_t::__variant_t& result) noexcept {
    _Rcvr& __rcvr = *static_cast<_Rcvr*>(p);
    std::visit(
        [&__rcvr](auto& tuple) noexcept {
          if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(tuple)>, std::monostate>) {
            std::apply([&__rcvr](auto __cpo, auto&... __vals) noexcept { __cpo(static_cast<_Rcvr&&>(__rcvr), static_cast<std::remove_reference_t<decltype(__vals)>&&>(__vals)...); },
                       tuple);
          }
        },
        result);
  }
  template <class _Rcvr>
  static void __stopped_to(void* p) noexcept {
    std::execution::set_stopped(static_cast<_Rcvr&&>(*static_cast<_Rcvr*>(p)));
  }
  // Destroys the state, or leaves that to the try_cancel that pins it.
  void __doom() noexcept {
    __lock_.lock();
    __doomed_ = true;
    const bool now = __pins_ == 0;
    __lock_.unlock();
    if (now)
      destroy();
  }
  void destroy() noexcept {
    auto __associated = static_cast<__assoc_t&&>(__assoc);
    using __traits = typename std::allocator_traits<_Alloc>::template rebind_traits<__exec_spawn_future_state>;
    typename __traits::allocator_type __al(static_cast<_Alloc&&>(__alloc));
    __traits::destroy(__al, this);
    __traits::deallocate(__al, this, 1);
  }

  _Alloc __alloc;
  std::inplace_stop_source __ssource;
  __op_t op;
  __assoc_t __assoc;
  ::__ycxx::__detail::__exec::__exec_spin_lock __lock_;
  unsigned __flags_ = 0;
  unsigned __pins_ = 0;
  bool __doomed_ = false;
  __registration __reg_;
};

// The owner of a spawn-future-state: destroying it abandons the state ([exec.spawn.future]/20.2).
template <class _State>
struct __exec_future_state_ptr {
  using __ycxx_sigs = typename _State::__sigs_t;
  _State* p = nullptr;
  __exec_future_state_ptr() = default;
  explicit __exec_future_state_ptr(_State* __q) noexcept : p(__q) {}
  __exec_future_state_ptr(__exec_future_state_ptr&& __o) noexcept : p(__o.p) { __o.p = nullptr; }
  __exec_future_state_ptr& operator=(__exec_future_state_ptr&& __o) noexcept {
    if (this != __builtin_addressof(__o)) {
      reset();
      p = __o.p;
      __o.p = nullptr;
    }
    return *this;
  }
  ~__exec_future_state_ptr() { reset(); }
  void reset() noexcept {
    if (_State* __q = p) {
      p = nullptr;
      __q->abandon();
    }
  }
  _State* get() const noexcept { return p; }
  _State* release() noexcept {
    _State* __q = p;
    p = nullptr;
    return __q;
  }
};

// future-operation ([exec.spawn.future]/15)
template <class _StatePtr, class _Rcvr>
struct __exec_future_operation {
  struct __y_callback {
    __exec_try_cancelable* state;
    void operator()() noexcept { state->__try_cancel(); }
  };
  using __stop_token_t = std::stop_token_of_t<std::execution::env_of_t<_Rcvr>>;
  using __stop_callback_t = std::stop_callback_for_t<__stop_token_t, __y_callback>;
  struct __rcvr_t {
    using receiver_concept = std::execution::receiver_tag;
    __exec_future_operation* op;
    template <class... _Tp>
    void set_value(_Tp&&... __ts) && noexcept {
      op->template __set_complete<std::execution::set_value_t>(static_cast<_Tp&&>(__ts)...);
    }
    template <class _Ep>
    void set_error(_Ep&& e) && noexcept {
      op->template __set_complete<std::execution::set_error_t>(static_cast<_Ep&&>(e));
    }
    void set_stopped() && noexcept { op->template __set_complete<std::execution::set_stopped_t>(); }
    std::execution::env_of_t<_Rcvr> get_env() const noexcept { return std::execution::get_env(op->__rcvr); }
  };

  _Rcvr __rcvr;
  _StatePtr state;
  __rcvr_t __inner;
  std::optional<__stop_callback_t> __stopCallback;

  __exec_future_operation(_StatePtr s, _Rcvr r) noexcept
      : __rcvr(static_cast<_Rcvr&&>(r)), state(static_cast<_StatePtr&&>(s)), __inner{this} {}
  __exec_future_operation(__exec_future_operation&&) = delete;

  void run() & noexcept {
    __stopCallback.emplace(std::get_stop_token(std::execution::get_env(__rcvr)), __y_callback{state.get()});
    state.release()->consume(__inner);
  }
  template <class _CPO, class... _Tp>
  void __set_complete(_Tp&&... __ts) noexcept {
    __stopCallback.reset();
    _CPO{}(static_cast<_Rcvr&&>(__rcvr), static_cast<_Tp&&>(__ts)...);
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct spawn_future_t {
  template <sender _Sndr, class _Token, class _Env = env<>>
    requires scope_token<remove_cvref_t<_Token>> && __ycxx::__detail::__exec::__queryable<remove_cvref_t<_Env>>
  auto operator()(_Sndr&& __sndr, _Token&& token, _Env&& env = {}) const {
    using _Tp = remove_cvref_t<_Token>;
    _Tp __tok(static_cast<_Token&&>(token));
    auto&& __new_sender = __tok.wrap(static_cast<_Sndr&&>(__sndr));
    return __ycxx::__detail::__exec::__with_spawn_allocator(__new_sender, static_cast<_Env&&>(env), [&](auto __alloc, auto&& __senv) {
      using _NS = decltype(__new_sender);
      using _SE = remove_cvref_t<decltype(__senv)>;
      using _State = __ycxx::__adl_free::__exec_spawn_future_state<decltype(__alloc), _Tp, _NS, _SE>;
      auto* s = __ycxx::__detail::__exec::__new_with_allocator<_State>(__alloc, __alloc, static_cast<_NS&&>(__new_sender), __tok,
                                                              _SE(static_cast<decltype(__senv)&&>(__senv)));
      return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__adl_free::__exec_future_state_ptr<_State>(s));
    });
  }
};
inline constexpr spawn_future_t spawn_future{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::spawn_future_t> : __default_impls {
  template <class _Data>
  static constexpr auto __get_attrs(const _Data&) noexcept {
    return std::execution::env<>();
  }
  template <class _Sndr, class _Rcvr>
  static auto __get_state(_Sndr&& __sndr, _Rcvr& __rcvr) noexcept {
    using __state_ptr = std::remove_cvref_t<__data_type<_Sndr>>;
    return ::__ycxx::__adl_free::__exec_future_operation<__state_ptr, _Rcvr>(static_cast<_Sndr&&>(__sndr).template get<1>(), static_cast<_Rcvr&&>(__rcvr));
  }
  template <class _State, class _Rcvr>
  static void start(_State& state, _Rcvr&) noexcept {
    state.run();
  }
  template <class _Sndr, class... _Env>
  using __csigs = __sigs_concat_t<__sigs_map_t<typename std::remove_cvref_t<__data_type<_Sndr>>::__ycxx_sigs, __decayed_sig_t>,
                              std::execution::completion_signatures<set_stopped_t()>,
                              std::conditional_t<__nothrow_decay_copy_sigs<typename std::remove_cvref_t<__data_type<_Sndr>>::__ycxx_sigs>, __no_sigs, __eptr_sigs>>;
};
}}} // namespace __ycxx::__detail::__exec
