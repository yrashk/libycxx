// libycxx core: parallel_scheduler ([exec.par.scheduler]), the replacement interface of its
// backend ([exec.parschedrepl]: receiver_proxy, bulk_item_receiver_proxy,
// parallel_scheduler_backend, query_parallel_scheduler_backend) and task_scheduler
// ([exec.task.scheduler]), which type-erases a scheduler behind the same backend interface.
//
// The classes are core; query_parallel_scheduler_backend (replaceable) and the default backend,
// a pool of threads, are in the hosted runtime (src/hosted/parallel_scheduler*.cpp).
//
// A proxy answers try_query through one virtual function, query-env, taking a request that
// names the query and result types by the address of a variable template (types are compared,
// not RTTI). The supported query is get_stop_token_t with result inplace_stop_token
// ([exec.parschedrepl.recvproxy]/4): a receiver whose token is another stoppable token gets an
// inplace_stop_token of the proxy's own source, linked to the receiver's token by a stop
// callback while the operation runs (the recommended practice of /5).
#pragma once

#include <ycxx/core/exec_scope.hpp>
#include <ycxx/core/shared_ptr.hpp>
#include <ycxx/core/span.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Tp>
inline constexpr char __type_key = 0;
struct __proxy_query {
  const void* query;  // &type_key<Query>
  const void* result; // &type_key<P>
  void* out;          // optional<P>*
};
// Preallocated backend storage of the library's proxies ([exec.par.scheduler]/6).
inline constexpr std::size_t __backend_storage_size = 128;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution { namespace parallel_scheduler_replacement {

struct receiver_proxy {
protected:
  virtual bool __query_env(__ycxx::__detail::__exec::__proxy_query&) noexcept = 0; // query-env

public:
  virtual void set_value() noexcept = 0;
  virtual void set_error(exception_ptr) noexcept = 0;
  virtual void set_stopped() noexcept = 0;

  template <class _Pp, __ycxx::__detail::__exec::__class_type _Query>
  optional<_Pp> try_query(_Query) const noexcept {
    static_assert(is_object_v<_Pp> && !is_array_v<_Pp> && is_same_v<_Pp, remove_cv_t<_Pp>>,
                  "try_query: P must be a cv-unqualified non-array object type");
    optional<_Pp> r;
    __ycxx::__detail::__exec::__proxy_query __q{&__ycxx::__detail::__exec::__type_key<_Query>, &__ycxx::__detail::__exec::__type_key<_Pp>, &r};
    (void)const_cast<receiver_proxy*>(this)->__query_env(__q);
    return r;
  }
};

struct bulk_item_receiver_proxy : receiver_proxy {
  virtual void execute(size_t, size_t) noexcept = 0;
};

struct parallel_scheduler_backend {
  virtual ~parallel_scheduler_backend() = default;
  virtual void schedule(receiver_proxy&, span<byte>) noexcept = 0;
  virtual void schedule_bulk_chunked(size_t, bulk_item_receiver_proxy&, span<byte>) noexcept = 0;
  virtual void schedule_bulk_unchunked(size_t, bulk_item_receiver_proxy&, span<byte>) noexcept = 0;
};

// Replaceable ([exec.parschedrepl.query]); the default is in the hosted runtime.
shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend();

}}} // namespace std::execution::parallel_scheduler_replacement

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// The default backend, a thread pool (hosted runtime, src/hosted/parallel_scheduler.cpp).
std::shared_ptr<std::execution::parallel_scheduler_replacement::parallel_scheduler_backend> __default_parallel_scheduler_backend();
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// The stop token a proxy reports for a receiver whose token is Token.
template <class _Token>
struct __exec_proxy_stop {
  std::inplace_stop_source __src;
  std::optional<std::stop_callback_for_t<_Token, __exec_on_stop_request>> __link;
  void __attach(const _Token& t) noexcept { __link.emplace(t, __exec_on_stop_request{__src}); }
  void detach() noexcept { __link.reset(); }
  std::inplace_stop_token token(const _Token&) const noexcept { return __src.get_token(); }
};
template <>
struct __exec_proxy_stop<std::inplace_stop_token> {
  void __attach(const std::inplace_stop_token&) noexcept {}
  void detach() noexcept {}
  std::inplace_stop_token token(const std::inplace_stop_token& t) const noexcept { return t; }
};
template <std::unstoppable_token _Token>
struct __exec_proxy_stop<_Token> {
  void __attach(const _Token&) noexcept {}
  void detach() noexcept {}
  std::inplace_stop_token token(const _Token&) const noexcept { return std::inplace_stop_token(); }
};

// A proxy for the receiver *rcvr with base Base ([exec.par.scheduler]/5). Fallible: the proxy of
// a parallel_scheduler operation, whose senders declare set_error(exception_ptr) and
// set_stopped() whatever the environment: a backend may cancel work it was never asked to stop
// ([exec.parschedrepl.psb]/2.1.3), and the proxy's set_stopped is set_stopped(rcvr) (/5.3).
// Otherwise, the proxy of task_scheduler's schedule sender, whose completions are set_value()
// alone with an unstoppable token ([exec.task.scheduler]/13.4): its backend (an infallible
// scheduler's, /2) completes with no error and is never stopped then, and the unreachable
// completions terminate.
template <class _Base, class _Rcvr, bool _Fallible = true>
struct __exec_receiver_proxy_for : _Base {
  using __token_t = std::stop_token_of_t<std::execution::env_of_t<_Rcvr>>;
  _Rcvr* __rcvr;
  __exec_proxy_stop<__token_t> __stop;

  explicit __exec_receiver_proxy_for(_Rcvr* r) noexcept : __rcvr(r) {}
  void __attach() noexcept { __stop.__attach(std::get_stop_token(std::execution::get_env(*__rcvr))); }

  void set_value() noexcept override {
    __stop.detach();
    std::execution::set_value(static_cast<_Rcvr&&>(*__rcvr));
  }
  void set_error(std::exception_ptr e) noexcept override {
    __stop.detach();
    if constexpr (_Fallible)
      std::execution::set_error(static_cast<_Rcvr&&>(*__rcvr), static_cast<std::exception_ptr&&>(e));
    else
      std::terminate();
  }
  void set_stopped() noexcept override {
    __stop.detach();
    if constexpr (!_Fallible && std::unstoppable_token<__token_t>)
      std::terminate();
    else
      std::execution::set_stopped(static_cast<_Rcvr&&>(*__rcvr));
  }

protected:
  bool __query_env(::__ycxx::__detail::__exec::__proxy_query& __q) noexcept override {
    if (__q.query == &::__ycxx::__detail::__exec::__type_key<std::get_stop_token_t> && __q.result == &::__ycxx::__detail::__exec::__type_key<std::inplace_stop_token>) {
      static_cast<std::optional<std::inplace_stop_token>*>(__q.out)->emplace(__stop.token(std::get_stop_token(std::execution::get_env(*__rcvr))));
      return true;
    }
    return false;
  }
};

// The operation state of the parallel scheduler's schedule sender (Fallible) and of
// task_scheduler's.
template <class _Rcvr, bool _Fallible = true>
struct __exec_par_sched_op {
  using operation_state_concept = std::execution::operation_state_tag;
  using __backend_t = std::execution::parallel_scheduler_replacement::parallel_scheduler_backend;
  using __proxy_t = __exec_receiver_proxy_for<std::execution::parallel_scheduler_replacement::receiver_proxy, _Rcvr, _Fallible>;

  _Rcvr __rcvr;
  std::shared_ptr<__backend_t> __backend;
  __proxy_t proxy{__builtin_addressof(__rcvr)};
  alignas(std::max_align_t) std::byte __storage[::__ycxx::__detail::__exec::__backend_storage_size];

  __exec_par_sched_op(_Rcvr r, std::shared_ptr<__backend_t> b) noexcept : __rcvr(static_cast<_Rcvr&&>(r)), __backend(static_cast<std::shared_ptr<__backend_t>&&>(b)) {}
  __exec_par_sched_op(__exec_par_sched_op&&) = delete;
  void start() & noexcept {
    proxy.__attach();
    __backend->schedule(proxy, std::span<std::byte>(__storage));
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
class parallel_scheduler;
parallel_scheduler get_parallel_scheduler();
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// parallel-scheduler-domain ([exec.par.scheduler]/8)
struct __exec_par_domain;
struct __exec_par_sched_sender;
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

class parallel_scheduler {
  using __backend_t = parallel_scheduler_replacement::parallel_scheduler_backend;
  shared_ptr<__backend_t> __backend_;
  explicit parallel_scheduler(shared_ptr<__backend_t> b) noexcept : __backend_(static_cast<shared_ptr<__backend_t>&&>(b)) {}
  friend parallel_scheduler get_parallel_scheduler();
  friend struct __ycxx::__adl_free::__exec_par_domain;

public:
  using scheduler_concept = scheduler_tag;
  parallel_scheduler(const parallel_scheduler&) noexcept = default;
  parallel_scheduler(parallel_scheduler&&) noexcept = default;
  parallel_scheduler& operator=(const parallel_scheduler&) noexcept = default;
  parallel_scheduler& operator=(parallel_scheduler&&) noexcept = default;

  __ycxx::__adl_free::__exec_par_sched_sender schedule() const noexcept;
  friend bool operator==(const parallel_scheduler& a, const parallel_scheduler& b) noexcept { return a.__backend_.get() == b.__backend_.get(); }
  forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept { return forward_progress_guarantee::parallel; }
  __ycxx::__adl_free::__exec_par_domain query(get_domain_t) const noexcept;
  template <class... _Envs>
  __ycxx::__adl_free::__exec_par_domain query(get_completion_domain_t<set_value_t>, const _Envs&...) const noexcept;
};

inline parallel_scheduler get_parallel_scheduler() {
  auto __eb = parallel_scheduler_replacement::query_parallel_scheduler_backend();
  if (__eb == nullptr)
    std::terminate();
  return parallel_scheduler(static_cast<shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend>&&>(__eb));
}

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

struct __exec_par_sched_sender {
  using sender_concept = std::execution::sender_tag;
  std::execution::parallel_scheduler __sch;
  std::shared_ptr<std::execution::parallel_scheduler_replacement::parallel_scheduler_backend> __backend;

  template <class _Self, class... _Env>
  using __ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t(), std::execution::set_error_t(std::exception_ptr),
                                                           std::execution::set_stopped_t()>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return __ycxx_csigs<_Self, _Env...>();
  }
  struct __attrs {
    std::execution::parallel_scheduler __sch;
    template <class _Tag, class... _Envs>
      requires(std::is_same_v<_Tag, std::execution::set_value_t> || std::is_same_v<_Tag, std::execution::set_stopped_t>)
    std::execution::parallel_scheduler query(std::execution::get_completion_scheduler_t<_Tag>, const _Envs&...) const noexcept {
      return __sch;
    }
    template <class... _Envs>
    __exec_par_domain query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const _Envs&...) const noexcept;
  };
  __attrs get_env() const noexcept { return {__sch}; }
  template <class _Rcvr>
  __exec_par_sched_op<std::decay_t<_Rcvr>> connect(_Rcvr&& __rcvr) const noexcept(std::is_nothrow_constructible_v<std::decay_t<_Rcvr>, _Rcvr>) {
    return {static_cast<_Rcvr&&>(__rcvr), __backend};
  }
};

// The operation of a bulk_chunked/bulk_unchunked sender on the parallel scheduler
// ([exec.par.scheduler]/11-12): the child's values are kept (decay-copied) while the backend runs
// the iterations, then sent.
template <bool _Chunked, class _Child, class _Shape, class _Fp, class _Rcvr>
struct __exec_par_bulk_op {
  using operation_state_concept = std::execution::operation_state_tag;
  using __backend_t = std::execution::parallel_scheduler_replacement::parallel_scheduler_backend;
  using __child_sigs = std::execution::completion_signatures_of_t<_Child, ::__ycxx::__detail::__exec::__fwd_env_t<std::execution::env_of_t<_Rcvr>>>;
  template <class _Args>
  struct __values_tuple;
  template <class... _Ts>
  struct __values_tuple<::__ycxx::__detail::__exec::__tlist<_Ts...>> {
    using type = ::__ycxx::__detail::__exec::__decayed_tuple<_Ts...>;
  };
  template <class _Lists>
  struct __values_variant;
  template <class... _Lists>
  struct __values_variant<::__ycxx::__detail::__exec::__tlist<_Lists...>> {
    using type = ::__ycxx::__detail::__exec::__apply_unique_t<std::variant, std::monostate, typename __values_tuple<_Lists>::type...>;
  };
  using __args_variant = typename __values_variant<::__ycxx::__detail::__exec::__sigs_args_t<std::execution::set_value_t, __child_sigs>>::type;

  struct __child_receiver {
    using receiver_concept = std::execution::receiver_tag;
    __exec_par_bulk_op* op;
    template <class... _Ts>
    void set_value(_Ts&&... __ts) && noexcept {
      op->run(static_cast<_Ts&&>(__ts)...);
    }
    template <class _Ep>
    void set_error(_Ep&& e) && noexcept {
      std::execution::set_error(static_cast<_Rcvr&&>(op->__rcvr), static_cast<_Ep&&>(e));
    }
    void set_stopped() && noexcept { std::execution::set_stopped(static_cast<_Rcvr&&>(op->__rcvr)); }
    decltype(auto) get_env() const noexcept { return ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(op->__rcvr)); }
  };

  struct __proxy_t : __exec_receiver_proxy_for<std::execution::parallel_scheduler_replacement::bulk_item_receiver_proxy, _Rcvr> {
    __exec_par_bulk_op* op;
    explicit __proxy_t(__exec_par_bulk_op* __o) noexcept
        : __exec_receiver_proxy_for<std::execution::parallel_scheduler_replacement::bulk_item_receiver_proxy, _Rcvr>(__builtin_addressof(__o->__rcvr)), op(__o) {}
    void execute(std::size_t i, std::size_t __j) noexcept override { op->execute(i, __j); }
    void set_value() noexcept override {
      this->__stop.detach();
      op->finish();
    }
  };

  _Rcvr __rcvr;
  bool par;
  _Shape __shape;
  _Fp __f;
  std::shared_ptr<__backend_t> __backend;
  __args_variant __args;
  std::exception_ptr error;
  bool failed = false;
  __proxy_t proxy{this};
  alignas(std::max_align_t) std::byte __storage[::__ycxx::__detail::__exec::__backend_storage_size];
  std::execution::connect_result_t<_Child, __child_receiver> __child_op;

  template <class _Cp>
  __exec_par_bulk_op(_Cp&& __child, bool p, _Shape s, _Fp __fn, std::shared_ptr<__backend_t> b, _Rcvr r)
      : __rcvr(static_cast<_Rcvr&&>(r)), par(p), __shape(s), __f(static_cast<_Fp&&>(__fn)), __backend(static_cast<std::shared_ptr<__backend_t>&&>(b)),
        __child_op(std::execution::connect(static_cast<_Cp&&>(__child), __child_receiver{this})) {}
  __exec_par_bulk_op(__exec_par_bulk_op&&) = delete;

  void start() & noexcept { std::execution::start(__child_op); }

  template <class... _Ts>
  void run(_Ts&&... __ts) noexcept {
    using __tuple_t = ::__ycxx::__detail::__exec::__decayed_tuple<_Ts...>;
    if constexpr (std::is_nothrow_constructible_v<__tuple_t, _Ts...> || !::__ycxx::__detail::__cfg::exceptions) {
      __args.template emplace<__tuple_t>(static_cast<_Ts&&>(__ts)...);
    } else {
      try {
        __args.template emplace<__tuple_t>(static_cast<_Ts&&>(__ts)...);
      } catch (...) {
        std::execution::set_error(static_cast<_Rcvr&&>(__rcvr), std::current_exception());
        return;
      }
    }
    proxy.__attach();
    const std::size_t n = par ? static_cast<std::size_t>(__shape) : 1;
    if constexpr (_Chunked)
      __backend->schedule_bulk_chunked(n, proxy, std::span<std::byte>(__storage));
    else
      __backend->schedule_bulk_unchunked(n, proxy, std::span<std::byte>(__storage));
  }
  void execute(std::size_t i, std::size_t __j) noexcept {
    auto __body = [&]<class _Tuple>(_Tuple& t) {
      if constexpr (!std::is_same_v<_Tuple, std::monostate>) {
        std::apply(
            [&](auto&... __vs) {
              if constexpr (_Chunked) {
                if (par)
                  __f(static_cast<_Shape>(i), static_cast<_Shape>(__j), __vs...);
                else
                  __f(static_cast<_Shape>(0), __shape, __vs...);
              } else {
                if (par)
                  __f(static_cast<_Shape>(i), __vs...);
                else
                  for (_Shape k = 0; k < __shape; ++k)
                    __f(_Shape(k), __vs...);
              }
            },
            t);
      }
    };
    if constexpr (::__ycxx::__detail::__cfg::exceptions) {
      try {
        std::visit(__body, __args);
      } catch (...) {
        if (!__atomic_exchange_n(&failed, true, __ATOMIC_ACQ_REL))
          error = std::current_exception();
      }
    } else {
      std::visit(__body, __args);
    }
  }
  void finish() noexcept {
    if (__atomic_load_n(&failed, __ATOMIC_ACQUIRE)) {
      std::execution::set_error(static_cast<_Rcvr&&>(__rcvr), static_cast<std::exception_ptr&&>(error));
      return;
    }
    std::visit(
        [&]<class _Tuple>(_Tuple& t) noexcept {
          if constexpr (!std::is_same_v<_Tuple, std::monostate>)
            std::apply([&](auto&... __vs) noexcept { std::execution::set_value(static_cast<_Rcvr&&>(__rcvr), static_cast<std::remove_reference_t<decltype(__vs)>&&>(__vs)...); },
                       t);
        },
        __args);
  }
};

template <bool _Chunked, class _Child, class _Shape, class _Fp>
struct __exec_par_bulk_sender {
  using sender_concept = std::execution::sender_tag;
  using __backend_t = std::execution::parallel_scheduler_replacement::parallel_scheduler_backend;
  _Child __child;
  bool par;
  _Shape __shape;
  _Fp __f;
  std::shared_ptr<__backend_t> __backend;

  template <class _Self, class... _Env>
  using __ycxx_csigs = ::__ycxx::__detail::__exec::__sigs_concat_t<
      ::__ycxx::__detail::__exec::__sigs_map_t<::__ycxx::__detail::__exec::__csigs_of_t<::__ycxx::__detail::__forward_like_t<_Self, _Child>, ::__ycxx::__detail::__exec::__fwd_env_t<_Env>...>,
                                       ::__ycxx::__detail::__exec::__decayed_sig_t>,
      ::__ycxx::__detail::__exec::__eptr_sigs, std::execution::completion_signatures<std::execution::set_stopped_t()>>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return ::__ycxx::__detail::__exec::__checked_sigs<__ycxx_csigs<_Self, _Env...>>();
  }
  decltype(auto) get_env() const noexcept { return ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(__child)); }
  template <::__ycxx::__detail::__exec::__decays_to<__exec_par_bulk_sender> _Self, std::execution::receiver _Rcvr>
  auto connect(this _Self&& __self, _Rcvr __rcvr) {
    return __exec_par_bulk_op<_Chunked, ::__ycxx::__detail::__forward_like_t<_Self, _Child>, _Shape, _Fp, _Rcvr>(
        std::forward_like<_Self>(__self.__child), __self.par, __self.__shape, std::forward_like<_Self>(__self.__f), __self.__backend, static_cast<_Rcvr&&>(__rcvr));
  }
};

struct __exec_par_domain {
  template <class _Sndr, class _Env>
    requires((std::is_same_v<std::execution::tag_of_t<_Sndr>, std::execution::bulk_chunked_t> ||
              std::is_same_v<std::execution::tag_of_t<_Sndr>, std::execution::bulk_unchunked_t>) &&
             requires(_Sndr&& s, const _Env& env) {
               { std::execution::get_completion_scheduler<std::execution::set_value_t>(std::execution::get_env(s.template get<2>()), ::__ycxx::__detail::__exec::__fwd_env(env)) }
                 -> std::same_as<std::execution::parallel_scheduler>;
             })
  static constexpr auto transform_sender(std::execution::set_value_t, _Sndr&& __sndr, const _Env& env) {
    constexpr bool __chunked = std::is_same_v<std::execution::tag_of_t<_Sndr>, std::execution::bulk_chunked_t>;
    auto&& data = static_cast<_Sndr&&>(__sndr).template get<1>();
    auto&& __child = static_cast<_Sndr&&>(__sndr).template get<2>();
    using _Pol = std::remove_cvref_t<decltype(data.template get<0>())>;
    using _Shape = std::remove_cvref_t<decltype(data.template get<1>())>;
    using _Fp = std::remove_cvref_t<decltype(data.template get<2>())>;
    // p ([exec.par.scheduler]/10): the parallel policies; libycxx defines no other policy.
    constexpr bool p = std::is_same_v<_Pol, std::execution::parallel_policy> || std::is_same_v<_Pol, std::execution::parallel_unsequenced_policy>;
    auto __sch = std::execution::get_completion_scheduler<std::execution::set_value_t>(std::execution::get_env(__child), ::__ycxx::__detail::__exec::__fwd_env(env));
    return __exec_par_bulk_sender<__chunked, std::remove_cvref_t<decltype(__child)>, _Shape, _Fp>{
        std::forward_like<_Sndr>(__child), p, _Shape(data.template get<1>()), std::forward_like<_Sndr>(data.template get<2>()), __sch.__backend_};
  }
};

template <class... _Envs>
__exec_par_domain __exec_par_sched_sender::__attrs::query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const _Envs&...) const noexcept {
  return {};
}
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
inline __ycxx::__adl_free::__exec_par_sched_sender parallel_scheduler::schedule() const noexcept { return {*this, __backend_}; }
inline __ycxx::__adl_free::__exec_par_domain parallel_scheduler::query(get_domain_t) const noexcept { return {}; }
template <class... _Envs>
__ycxx::__adl_free::__exec_par_domain parallel_scheduler::query(get_completion_domain_t<set_value_t>, const _Envs&...) const noexcept {
  return {};
}
}} // namespace std::execution
