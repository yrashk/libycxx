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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class T>
inline constexpr char type_key = 0;
struct proxy_query {
  const void* query;  // &type_key<Query>
  const void* result; // &type_key<P>
  void* out;          // optional<P>*
};
// Preallocated backend storage of the library's proxies ([exec.par.scheduler]/6).
inline constexpr std::size_t backend_storage_size = 128;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution { namespace parallel_scheduler_replacement {

struct receiver_proxy {
protected:
  virtual bool query_env(ycxx::detail::exec::proxy_query&) noexcept = 0; // query-env

public:
  virtual void set_value() noexcept = 0;
  virtual void set_error(exception_ptr) noexcept = 0;
  virtual void set_stopped() noexcept = 0;

  template <class P, ycxx::detail::exec::class_type Query>
  optional<P> try_query(Query) const noexcept {
    static_assert(is_object_v<P> && !is_array_v<P> && is_same_v<P, remove_cv_t<P>>,
                  "try_query: P must be a cv-unqualified non-array object type");
    optional<P> r;
    ycxx::detail::exec::proxy_query q{&ycxx::detail::exec::type_key<Query>, &ycxx::detail::exec::type_key<P>, &r};
    (void)const_cast<receiver_proxy*>(this)->query_env(q);
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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// The default backend, a thread pool (hosted runtime, src/hosted/parallel_scheduler.cpp).
std::shared_ptr<std::execution::parallel_scheduler_replacement::parallel_scheduler_backend> default_parallel_scheduler_backend();
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// The stop token a proxy reports for a receiver whose token is Token.
template <class Token>
struct exec_proxy_stop {
  std::inplace_stop_source src;
  std::optional<std::stop_callback_for_t<Token, exec_on_stop_request>> link;
  void attach(const Token& t) noexcept { link.emplace(t, exec_on_stop_request{src}); }
  void detach() noexcept { link.reset(); }
  std::inplace_stop_token token(const Token&) const noexcept { return src.get_token(); }
};
template <>
struct exec_proxy_stop<std::inplace_stop_token> {
  void attach(const std::inplace_stop_token&) noexcept {}
  void detach() noexcept {}
  std::inplace_stop_token token(const std::inplace_stop_token& t) const noexcept { return t; }
};
template <std::unstoppable_token Token>
struct exec_proxy_stop<Token> {
  void attach(const Token&) noexcept {}
  void detach() noexcept {}
  std::inplace_stop_token token(const Token&) const noexcept { return std::inplace_stop_token(); }
};

// A proxy for the receiver *rcvr with base Base ([exec.par.scheduler]/5). A completion its sender
// does not declare (Errors false: no error completion; an unstoppable token: no stopped
// completion, the backend never seeing a stop request) is not reachable and terminates.
template <class Base, class Rcvr, bool Errors = true>
struct exec_receiver_proxy_for : Base {
  using token_t = std::stop_token_of_t<std::execution::env_of_t<Rcvr>>;
  Rcvr* rcvr;
  exec_proxy_stop<token_t> stop;

  explicit exec_receiver_proxy_for(Rcvr* r) noexcept : rcvr(r) {}
  void attach() noexcept { stop.attach(std::get_stop_token(std::execution::get_env(*rcvr))); }

  void set_value() noexcept override {
    stop.detach();
    std::execution::set_value(static_cast<Rcvr&&>(*rcvr));
  }
  void set_error(std::exception_ptr e) noexcept override {
    stop.detach();
    if constexpr (Errors)
      std::execution::set_error(static_cast<Rcvr&&>(*rcvr), static_cast<std::exception_ptr&&>(e));
    else
      std::terminate();
  }
  void set_stopped() noexcept override {
    stop.detach();
    if constexpr (std::unstoppable_token<token_t>)
      std::terminate();
    else
      std::execution::set_stopped(static_cast<Rcvr&&>(*rcvr));
  }

protected:
  bool query_env(::ycxx::detail::exec::proxy_query& q) noexcept override {
    if (q.query == &::ycxx::detail::exec::type_key<std::get_stop_token_t> && q.result == &::ycxx::detail::exec::type_key<std::inplace_stop_token>) {
      static_cast<std::optional<std::inplace_stop_token>*>(q.out)->emplace(stop.token(std::get_stop_token(std::execution::get_env(*rcvr))));
      return true;
    }
    return false;
  }
};

// The operation state of the parallel scheduler's schedule sender (Errors) and of
// task_scheduler's (which has no error completion).
template <class Rcvr, bool Errors = true>
struct exec_par_sched_op {
  using operation_state_concept = std::execution::operation_state_tag;
  using backend_t = std::execution::parallel_scheduler_replacement::parallel_scheduler_backend;
  using proxy_t = exec_receiver_proxy_for<std::execution::parallel_scheduler_replacement::receiver_proxy, Rcvr, Errors>;

  Rcvr rcvr;
  std::shared_ptr<backend_t> backend;
  proxy_t proxy{__builtin_addressof(rcvr)};
  alignas(std::max_align_t) std::byte storage[::ycxx::detail::exec::backend_storage_size];

  exec_par_sched_op(Rcvr r, std::shared_ptr<backend_t> b) noexcept : rcvr(static_cast<Rcvr&&>(r)), backend(static_cast<std::shared_ptr<backend_t>&&>(b)) {}
  exec_par_sched_op(exec_par_sched_op&&) = delete;
  void start() & noexcept {
    proxy.attach();
    backend->schedule(proxy, std::span<std::byte>(storage));
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
class parallel_scheduler;
parallel_scheduler get_parallel_scheduler();
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// parallel-scheduler-domain ([exec.par.scheduler]/8)
struct exec_par_domain;
struct exec_par_sched_sender;
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {

class parallel_scheduler {
  using backend_t = parallel_scheduler_replacement::parallel_scheduler_backend;
  shared_ptr<backend_t> backend_;
  explicit parallel_scheduler(shared_ptr<backend_t> b) noexcept : backend_(static_cast<shared_ptr<backend_t>&&>(b)) {}
  friend parallel_scheduler get_parallel_scheduler();
  friend struct ycxx::adl_free::exec_par_domain;

public:
  using scheduler_concept = scheduler_tag;
  parallel_scheduler(const parallel_scheduler&) noexcept = default;
  parallel_scheduler(parallel_scheduler&&) noexcept = default;
  parallel_scheduler& operator=(const parallel_scheduler&) noexcept = default;
  parallel_scheduler& operator=(parallel_scheduler&&) noexcept = default;

  ycxx::adl_free::exec_par_sched_sender schedule() const noexcept;
  friend bool operator==(const parallel_scheduler& a, const parallel_scheduler& b) noexcept { return a.backend_.get() == b.backend_.get(); }
  forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept { return forward_progress_guarantee::parallel; }
  ycxx::adl_free::exec_par_domain query(get_domain_t) const noexcept;
  template <class... Envs>
  ycxx::adl_free::exec_par_domain query(get_completion_domain_t<set_value_t>, const Envs&...) const noexcept;
};

inline parallel_scheduler get_parallel_scheduler() {
  auto eb = parallel_scheduler_replacement::query_parallel_scheduler_backend();
  if (eb == nullptr)
    std::terminate();
  return parallel_scheduler(static_cast<shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend>&&>(eb));
}

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

struct exec_par_sched_sender {
  using sender_concept = std::execution::sender_tag;
  std::execution::parallel_scheduler sch;
  std::shared_ptr<std::execution::parallel_scheduler_replacement::parallel_scheduler_backend> backend;

  template <class Self, class... Env>
  using ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t(), std::execution::set_error_t(std::exception_ptr),
                                                           std::execution::set_stopped_t()>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ycxx_csigs<Self, Env...>();
  }
  struct attrs {
    std::execution::parallel_scheduler sch;
    template <class Tag, class... Envs>
      requires(std::is_same_v<Tag, std::execution::set_value_t> || std::is_same_v<Tag, std::execution::set_stopped_t>)
    std::execution::parallel_scheduler query(std::execution::get_completion_scheduler_t<Tag>, const Envs&...) const noexcept {
      return sch;
    }
    template <class... Envs>
    exec_par_domain query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const Envs&...) const noexcept;
  };
  attrs get_env() const noexcept { return {sch}; }
  template <class Rcvr>
  exec_par_sched_op<std::decay_t<Rcvr>> connect(Rcvr&& rcvr) const noexcept(std::is_nothrow_constructible_v<std::decay_t<Rcvr>, Rcvr>) {
    return {static_cast<Rcvr&&>(rcvr), backend};
  }
};

// The operation of a bulk_chunked/bulk_unchunked sender on the parallel scheduler
// ([exec.par.scheduler]/11-12): the child's values are kept (decay-copied) while the backend runs
// the iterations, then sent.
template <bool Chunked, class Child, class Shape, class F, class Rcvr>
struct exec_par_bulk_op {
  using operation_state_concept = std::execution::operation_state_tag;
  using backend_t = std::execution::parallel_scheduler_replacement::parallel_scheduler_backend;
  using child_sigs = std::execution::completion_signatures_of_t<Child, ::ycxx::detail::exec::fwd_env_t<std::execution::env_of_t<Rcvr>>>;
  template <class Args>
  struct values_tuple;
  template <class... Ts>
  struct values_tuple<::ycxx::detail::exec::tlist<Ts...>> {
    using type = ::ycxx::detail::exec::decayed_tuple<Ts...>;
  };
  template <class Lists>
  struct values_variant;
  template <class... Lists>
  struct values_variant<::ycxx::detail::exec::tlist<Lists...>> {
    using type = ::ycxx::detail::exec::apply_unique_t<std::variant, std::monostate, typename values_tuple<Lists>::type...>;
  };
  using args_variant = typename values_variant<::ycxx::detail::exec::sigs_args_t<std::execution::set_value_t, child_sigs>>::type;

  struct child_receiver {
    using receiver_concept = std::execution::receiver_tag;
    exec_par_bulk_op* op;
    template <class... Ts>
    void set_value(Ts&&... ts) && noexcept {
      op->run(static_cast<Ts&&>(ts)...);
    }
    template <class E>
    void set_error(E&& e) && noexcept {
      std::execution::set_error(static_cast<Rcvr&&>(op->rcvr), static_cast<E&&>(e));
    }
    void set_stopped() && noexcept { std::execution::set_stopped(static_cast<Rcvr&&>(op->rcvr)); }
    decltype(auto) get_env() const noexcept { return ::ycxx::detail::exec::fwd_env(std::execution::get_env(op->rcvr)); }
  };

  struct proxy_t : exec_receiver_proxy_for<std::execution::parallel_scheduler_replacement::bulk_item_receiver_proxy, Rcvr> {
    exec_par_bulk_op* op;
    explicit proxy_t(exec_par_bulk_op* o) noexcept
        : exec_receiver_proxy_for<std::execution::parallel_scheduler_replacement::bulk_item_receiver_proxy, Rcvr>(__builtin_addressof(o->rcvr)), op(o) {}
    void execute(std::size_t i, std::size_t j) noexcept override { op->execute(i, j); }
    void set_value() noexcept override {
      this->stop.detach();
      op->finish();
    }
  };

  Rcvr rcvr;
  bool par;
  Shape shape;
  F f;
  std::shared_ptr<backend_t> backend;
  args_variant args;
  std::exception_ptr error;
  bool failed = false;
  proxy_t proxy{this};
  alignas(std::max_align_t) std::byte storage[::ycxx::detail::exec::backend_storage_size];
  std::execution::connect_result_t<Child, child_receiver> child_op;

  template <class C>
  exec_par_bulk_op(C&& child, bool p, Shape s, F fn, std::shared_ptr<backend_t> b, Rcvr r)
      : rcvr(static_cast<Rcvr&&>(r)), par(p), shape(s), f(static_cast<F&&>(fn)), backend(static_cast<std::shared_ptr<backend_t>&&>(b)),
        child_op(std::execution::connect(static_cast<C&&>(child), child_receiver{this})) {}
  exec_par_bulk_op(exec_par_bulk_op&&) = delete;

  void start() & noexcept { std::execution::start(child_op); }

  template <class... Ts>
  void run(Ts&&... ts) noexcept {
    using tuple_t = ::ycxx::detail::exec::decayed_tuple<Ts...>;
    if constexpr (std::is_nothrow_constructible_v<tuple_t, Ts...> || !::ycxx::detail::cfg::exceptions) {
      args.template emplace<tuple_t>(static_cast<Ts&&>(ts)...);
    } else {
      try {
        args.template emplace<tuple_t>(static_cast<Ts&&>(ts)...);
      } catch (...) {
        std::execution::set_error(static_cast<Rcvr&&>(rcvr), std::current_exception());
        return;
      }
    }
    proxy.attach();
    const std::size_t n = par ? static_cast<std::size_t>(shape) : 1;
    if constexpr (Chunked)
      backend->schedule_bulk_chunked(n, proxy, std::span<std::byte>(storage));
    else
      backend->schedule_bulk_unchunked(n, proxy, std::span<std::byte>(storage));
  }
  void execute(std::size_t i, std::size_t j) noexcept {
    auto body = [&]<class Tuple>(Tuple& t) {
      if constexpr (!std::is_same_v<Tuple, std::monostate>) {
        std::apply(
            [&](auto&... vs) {
              if constexpr (Chunked) {
                if (par)
                  f(static_cast<Shape>(i), static_cast<Shape>(j), vs...);
                else
                  f(static_cast<Shape>(0), shape, vs...);
              } else {
                if (par)
                  f(static_cast<Shape>(i), vs...);
                else
                  for (Shape k = 0; k < shape; ++k)
                    f(Shape(k), vs...);
              }
            },
            t);
      }
    };
    if constexpr (::ycxx::detail::cfg::exceptions) {
      try {
        std::visit(body, args);
      } catch (...) {
        if (!__atomic_exchange_n(&failed, true, __ATOMIC_ACQ_REL))
          error = std::current_exception();
      }
    } else {
      std::visit(body, args);
    }
  }
  void finish() noexcept {
    if (__atomic_load_n(&failed, __ATOMIC_ACQUIRE)) {
      std::execution::set_error(static_cast<Rcvr&&>(rcvr), static_cast<std::exception_ptr&&>(error));
      return;
    }
    std::visit(
        [&]<class Tuple>(Tuple& t) noexcept {
          if constexpr (!std::is_same_v<Tuple, std::monostate>)
            std::apply([&](auto&... vs) noexcept { std::execution::set_value(static_cast<Rcvr&&>(rcvr), static_cast<std::remove_reference_t<decltype(vs)>&&>(vs)...); },
                       t);
        },
        args);
  }
};

template <bool Chunked, class Child, class Shape, class F>
struct exec_par_bulk_sender {
  using sender_concept = std::execution::sender_tag;
  using backend_t = std::execution::parallel_scheduler_replacement::parallel_scheduler_backend;
  Child child;
  bool par;
  Shape shape;
  F f;
  std::shared_ptr<backend_t> backend;

  template <class Self, class... Env>
  using ycxx_csigs = ::ycxx::detail::exec::sigs_concat_t<
      ::ycxx::detail::exec::sigs_map_t<::ycxx::detail::exec::csigs_of_t<::ycxx::detail::forward_like_t<Self, Child>, ::ycxx::detail::exec::fwd_env_t<Env>...>,
                                       ::ycxx::detail::exec::decayed_sig_t>,
      ::ycxx::detail::exec::eptr_sigs, std::execution::completion_signatures<std::execution::set_stopped_t()>>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ::ycxx::detail::exec::checked_sigs<ycxx_csigs<Self, Env...>>();
  }
  decltype(auto) get_env() const noexcept { return ::ycxx::detail::exec::fwd_env(std::execution::get_env(child)); }
  template <::ycxx::detail::exec::decays_to<exec_par_bulk_sender> Self, std::execution::receiver Rcvr>
  auto connect(this Self&& self, Rcvr rcvr) {
    return exec_par_bulk_op<Chunked, ::ycxx::detail::forward_like_t<Self, Child>, Shape, F, Rcvr>(
        std::forward_like<Self>(self.child), self.par, self.shape, std::forward_like<Self>(self.f), self.backend, static_cast<Rcvr&&>(rcvr));
  }
};

struct exec_par_domain {
  template <class Sndr, class Env>
    requires((std::is_same_v<std::execution::tag_of_t<Sndr>, std::execution::bulk_chunked_t> ||
              std::is_same_v<std::execution::tag_of_t<Sndr>, std::execution::bulk_unchunked_t>) &&
             requires(Sndr&& s, const Env& env) {
               { std::execution::get_completion_scheduler<std::execution::set_value_t>(std::execution::get_env(s.template get<2>()), ::ycxx::detail::exec::fwd_env(env)) }
                 -> std::same_as<std::execution::parallel_scheduler>;
             })
  static constexpr auto transform_sender(std::execution::set_value_t, Sndr&& sndr, const Env& env) {
    constexpr bool chunked = std::is_same_v<std::execution::tag_of_t<Sndr>, std::execution::bulk_chunked_t>;
    auto&& data = static_cast<Sndr&&>(sndr).template get<1>();
    auto&& child = static_cast<Sndr&&>(sndr).template get<2>();
    using Pol = std::remove_cvref_t<decltype(data.template get<0>())>;
    using Shape = std::remove_cvref_t<decltype(data.template get<1>())>;
    using F = std::remove_cvref_t<decltype(data.template get<2>())>;
    // p ([exec.par.scheduler]/10): the parallel policies; libycxx defines no other policy.
    constexpr bool p = std::is_same_v<Pol, std::execution::parallel_policy> || std::is_same_v<Pol, std::execution::parallel_unsequenced_policy>;
    auto sch = std::execution::get_completion_scheduler<std::execution::set_value_t>(std::execution::get_env(child), ::ycxx::detail::exec::fwd_env(env));
    return exec_par_bulk_sender<chunked, std::remove_cvref_t<decltype(child)>, Shape, F>{
        std::forward_like<Sndr>(child), p, Shape(data.template get<1>()), std::forward_like<Sndr>(data.template get<2>()), sch.backend_};
  }
};

template <class... Envs>
exec_par_domain exec_par_sched_sender::attrs::query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const Envs&...) const noexcept {
  return {};
}
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
inline ycxx::adl_free::exec_par_sched_sender parallel_scheduler::schedule() const noexcept { return {*this, backend_}; }
inline ycxx::adl_free::exec_par_domain parallel_scheduler::query(get_domain_t) const noexcept { return {}; }
template <class... Envs>
ycxx::adl_free::exec_par_domain parallel_scheduler::query(get_completion_domain_t<set_value_t>, const Envs&...) const noexcept {
  return {};
}
}} // namespace std::execution
