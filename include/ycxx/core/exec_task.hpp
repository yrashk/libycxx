// libycxx core: execution::task_scheduler ([exec.task.scheduler]) and the coroutine type
// execution::task ([exec.task]), with with_error.
//
// task<T, Environment>::state<Rcvr> derives from a base that does not depend on the receiver
// (task_state_base), which the promise points to: STATE(prom), the stop token, the start
// scheduler and the allocator are reached through it, the completions through three virtual
// functions that destroy the frame first ([task.promise]/3, /5, /8).
//
// The coroutine frame is allocated with the allocator of [task.promise]/14: the frame is
// followed by a deallocation function and the rebound allocator, so the non-template operator
// delete can free it.
#pragma once

#include <ycxx/core/exec_parallel.hpp>

// ---------------------------------------------------------------------------------------------
// [exec.task.scheduler]
namespace [[gnu::visibility("hidden")]] std { namespace execution {
class task_scheduler;
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
struct ts_access;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// What task_scheduler needs of its backend beyond parallel_scheduler_backend.
struct exec_ts_backend_base : std::execution::parallel_scheduler_replacement::parallel_scheduler_backend {
  virtual const void* ycxx_type() const noexcept = 0;
  virtual const void* ycxx_sched() const noexcept = 0;
  virtual std::execution::forward_progress_guarantee ycxx_fpg() const noexcept = 0;
  virtual bool ycxx_equal_to(const std::execution::task_scheduler& lhs) const noexcept = 0;
};

// just-sndr-like ([exec.task.scheduler]/9)
template <class Sch>
struct exec_just_sndr_like {
  using sender_concept = std::execution::sender_tag;
  Sch sched;
  template <class Self, class... Env>
  using ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t()>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return std::execution::completion_signatures<std::execution::set_value_t()>();
  }
  struct attrs {
    Sch sched;
    template <class... Envs>
      requires requires(const Sch& s, const Envs&... e) { std::execution::get_completion_scheduler<std::execution::set_value_t>(s, e...); }
    auto query(std::execution::get_completion_scheduler_t<std::execution::set_value_t>, const Envs&... envs) const noexcept {
      return std::execution::get_completion_scheduler<std::execution::set_value_t>(sched, envs...);
    }
    template <class... Envs>
      requires requires(const Sch& s, const Envs&... e) { std::execution::get_completion_domain<std::execution::set_value_t>(s, e...); }
    auto query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const Envs&... envs) const noexcept {
      return std::execution::get_completion_domain<std::execution::set_value_t>(sched, envs...);
    }
  };
  attrs get_env() const noexcept { return {sched}; }
  template <class Rcvr>
  exec_inline_state<std::remove_cvref_t<Rcvr>> connect(Rcvr&& rcvr) const noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<Rcvr>, Rcvr>) {
    return {static_cast<Rcvr&&>(rcvr)};
  }
};

// WRAP-RCVR(r) ([exec.task.scheduler]/8) for an operation the backend placed in the proxy's
// storage (or on the heap): it destroys that operation before it completes r.
struct exec_wrap_env {
  std::execution::parallel_scheduler_replacement::receiver_proxy* r;
  std::inplace_stop_token query(std::get_stop_token_t) const noexcept {
    return r->try_query<std::inplace_stop_token>(std::get_stop_token).value_or(std::inplace_stop_token());
  }
};
struct exec_wrap_rcvr {
  using receiver_concept = std::execution::receiver_tag;
  std::execution::parallel_scheduler_replacement::receiver_proxy* r;
  void* op;
  void (*destroy)(void*) noexcept;

  void set_value() && noexcept {
    auto* rr = r;
    destroy(op);
    rr->set_value();
  }
  template <class E>
  void set_error(E&& e) && noexcept {
    auto* rr = r;
    std::exception_ptr ep = ::ycxx::detail::exec::as_except_ptr(static_cast<E&&>(e));
    destroy(op);
    rr->set_error(static_cast<std::exception_ptr&&>(ep));
  }
  void set_stopped() && noexcept {
    auto* rr = r;
    destroy(op);
    rr->set_stopped();
  }
  exec_wrap_env get_env() const noexcept { return {r}; }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {

class task_scheduler {
  class ts_domain;
  template <scheduler Sch>
  class backend_for;
  friend struct ycxx::detail::exec::ts_access;

  shared_ptr<ycxx::adl_free::exec_ts_backend_base> sch_;

public:
  using scheduler_concept = scheduler_tag;

  template <class Sch, class Allocator = allocator<void>>
    requires(!same_as<task_scheduler, remove_cvref_t<Sch>>) && scheduler<Sch>
  explicit task_scheduler(Sch&& sch, Allocator alloc = {});
  task_scheduler(const task_scheduler&) = default;
  task_scheduler& operator=(const task_scheduler&) = default;

  auto schedule() const noexcept;

  friend bool operator==(const task_scheduler& lhs, const task_scheduler& rhs) noexcept { return rhs.sch_->ycxx_equal_to(lhs); }
  // TS deduced (only task_scheduler is accepted) so that the operator, found by argument-dependent
  // lookup for any type with task_scheduler among its template arguments, checks scheduler<Sch>
  // only when the left operand is a task_scheduler.
  template <class TS, class Sch>
    requires same_as<TS, task_scheduler> && (!same_as<task_scheduler, Sch>) && scheduler<Sch>
  friend bool operator==(const TS& lhs, const Sch& rhs) noexcept {
    if (lhs.sch_->ycxx_type() != &ycxx::detail::exec::type_key<Sch>)
      return false;
    return *static_cast<const Sch*>(lhs.sch_->ycxx_sched()) == rhs;
  }

  forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept { return sch_->ycxx_fpg(); }
  template <class... Envs>
  ts_domain query(get_completion_domain_t<set_value_t>, const Envs&...) const noexcept;
};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
struct ts_access {
  static const auto& backend(const std::execution::task_scheduler& s) noexcept { return s.sch_; }
};

// An operation of type Op placed in a proxy's backend storage when it fits, else on the heap.
template <class Op>
struct placed_op {
  static void destroy_in_place(void* p) noexcept { static_cast<Op*>(p)->~Op(); }
  static void destroy_heap(void* p) noexcept {
    static_cast<Op*>(p)->~Op();
    ::operator delete(p, std::align_val_t(alignof(Op)));
  }
  // Constructs make(wrap_rcvr) in s or on the heap and starts it; failures complete r.
  template <class Make>
  static void run(std::execution::parallel_scheduler_replacement::receiver_proxy& r, std::span<std::byte> s, Make make) noexcept {
    void* mem = s.data();
    std::size_t space = s.size();
    bool heap = false;
    if (!mem || !std::align(alignof(Op), sizeof(Op), mem, space)) {
      mem = ::operator new(sizeof(Op), std::align_val_t(alignof(Op)), std::nothrow);
      if (!mem) {
        r.set_error(std::make_exception_ptr(std::bad_alloc()));
        return;
      }
      heap = true;
    }
    ::ycxx::adl_free::exec_wrap_rcvr w{&r, mem, heap ? &destroy_heap : &destroy_in_place};
    auto construct = [&] { return ::new (mem) Op(make(w)); };
    Op* op;
    if constexpr (noexcept(make(w)) || !cfg::exceptions) {
      op = construct();
    } else {
      try {
        op = construct();
      } catch (...) {
        if (heap)
          ::operator delete(mem, std::align_val_t(alignof(Op)));
        r.set_error(std::current_exception());
        return;
      }
    }
    std::execution::start(*op);
  }
};

// fn of schedule_bulk_chunked / schedule_bulk_unchunked ([exec.task.scheduler]/11-12)
struct ts_bulk_fn {
  std::execution::parallel_scheduler_replacement::bulk_item_receiver_proxy* r;
  std::size_t chunk;
  std::size_t shape;
  void operator()(std::size_t i) const noexcept {
    const std::size_t b = i * chunk;
    r->execute(b, b + chunk < shape ? b + chunk : shape);
  }
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <scheduler Sch>
class task_scheduler::backend_for : public ycxx::adl_free::exec_ts_backend_base {
  using wrap_rcvr = ycxx::adl_free::exec_wrap_rcvr;
  using bulk_sndr = decltype(bulk(declval<ycxx::adl_free::exec_just_sndr_like<Sch>>(), par, size_t(), ycxx::detail::exec::ts_bulk_fn()));

  void run_bulk(size_t num_chunks, size_t chunk, size_t shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& r,
                span<byte> s) noexcept {
    using op_t = connect_result_t<bulk_sndr, wrap_rcvr>;
    ycxx::detail::exec::placed_op<op_t>::run(r, s, [&](wrap_rcvr w) {
      return connect(bulk(ycxx::adl_free::exec_just_sndr_like<Sch>{sched_}, par, num_chunks, ycxx::detail::exec::ts_bulk_fn{&r, chunk, shape}),
                     static_cast<wrap_rcvr&&>(w));
    });
  }

public:
  explicit backend_for(Sch sch) : sched_(static_cast<Sch&&>(sch)) {}

  void schedule(parallel_scheduler_replacement::receiver_proxy& r, span<byte> s) noexcept override {
    using op_t = connect_result_t<schedule_result_t<Sch&>, wrap_rcvr>;
    ycxx::detail::exec::placed_op<op_t>::run(r, s, [&](wrap_rcvr w) { return connect(execution::schedule(sched_), static_cast<wrap_rcvr&&>(w)); });
  }
  void schedule_bulk_chunked(size_t shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& r, span<byte> s) noexcept override {
    // chunk_size: at most 64 chunks (the scheduler behind is unknown).
    const size_t chunk = shape == 0 ? 1 : (shape + 63) / 64;
    run_bulk((shape + chunk - 1) / chunk, chunk, shape, r, s);
  }
  void schedule_bulk_unchunked(size_t shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& r, span<byte> s) noexcept override {
    run_bulk(shape, 1, shape, r, s);
  }

  const void* ycxx_type() const noexcept override { return &ycxx::detail::exec::type_key<Sch>; }
  const void* ycxx_sched() const noexcept override { return __builtin_addressof(sched_); }
  forward_progress_guarantee ycxx_fpg() const noexcept override { return get_forward_progress_guarantee(sched_); }
  bool ycxx_equal_to(const task_scheduler& lhs) const noexcept override { return lhs == sched_; }

private:
  Sch sched_;
};

template <class Sch, class Allocator>
  requires(!same_as<task_scheduler, remove_cvref_t<Sch>>) && scheduler<Sch>
task_scheduler::task_scheduler(Sch&& sch, Allocator alloc)
    : sch_(allocate_shared<backend_for<remove_cvref_t<Sch>>>(alloc, static_cast<Sch&&>(sch))) {
  static_assert(ycxx::detail::exec::infallible_scheduler<Sch, env<>>, "task_scheduler: the scheduler must be infallible");
}

class task_scheduler::ts_domain : public default_domain {
public:
  template <class BulkSndr, class Env>
    requires(is_same_v<tag_of_t<BulkSndr>, bulk_chunked_t> || is_same_v<tag_of_t<BulkSndr>, bulk_unchunked_t>) &&
            requires(BulkSndr&& b) { auto(static_cast<BulkSndr&&>(b)); }
  static constexpr auto transform_sender(set_value_t, BulkSndr&& bulk_sndr, const Env& env) noexcept(is_nothrow_constructible_v<decay_t<BulkSndr>, BulkSndr>) {
    auto&& data = static_cast<BulkSndr&&>(bulk_sndr).template get<1>();
    auto&& child = static_cast<BulkSndr&&>(bulk_sndr).template get<2>();
    auto sch = ycxx::detail::exec::call_with_default(get_completion_scheduler<set_value_t>, ycxx::adl_free::exec_not_a_scheduler(),
                                                     get_env(child), ycxx::detail::exec::fwd_env(env));
    if constexpr (!is_same_v<decltype(sch), task_scheduler>) {
      return ycxx::adl_free::exec_not_a_sender();
    } else {
      constexpr bool chunked = is_same_v<tag_of_t<BulkSndr>, bulk_chunked_t>;
      using Shape = remove_cvref_t<decltype(data.template get<1>())>;
      using F = remove_cvref_t<decltype(data.template get<2>())>;
      return ycxx::adl_free::exec_par_bulk_sender<chunked, remove_cvref_t<decltype(child)>, Shape, F>{
          std::forward_like<BulkSndr>(child), true, Shape(data.template get<1>()), std::forward_like<BulkSndr>(data.template get<2>()), sch.sch_};
    }
  }
};

template <class... Envs>
task_scheduler::ts_domain task_scheduler::query(get_completion_domain_t<set_value_t>, const Envs&...) const noexcept {
  return {};
}

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// ts-sndr ([exec.task.scheduler]/13)
struct exec_ts_sender {
  using sender_concept = std::execution::sender_tag;
  std::execution::task_scheduler sch;
  template <class Self, class... Env>
  using ycxx_csigs = std::conditional_t<(std::unstoppable_token<std::stop_token_of_t<Env>> && ...) && sizeof...(Env) != 0,
                                        std::execution::completion_signatures<std::execution::set_value_t()>,
                                        std::execution::completion_signatures<std::execution::set_value_t(), std::execution::set_stopped_t()>>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ycxx_csigs<Self, Env...>();
  }
  struct attrs {
    std::execution::task_scheduler sch;
    template <class... Envs>
    std::execution::task_scheduler query(std::execution::get_completion_scheduler_t<std::execution::set_value_t>, const Envs&...) const noexcept {
      return sch;
    }
    template <class... Envs>
    auto query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const Envs&... envs) const noexcept {
      return sch.query(std::execution::get_completion_domain<std::execution::set_value_t>, envs...);
    }
  };
  attrs get_env() const noexcept { return {sch}; }
  template <class Rcvr>
  exec_par_sched_op<std::decay_t<Rcvr>, false> connect(Rcvr&& rcvr) const noexcept(std::is_nothrow_constructible_v<std::decay_t<Rcvr>, Rcvr>) {
    return {static_cast<Rcvr&&>(rcvr), ::ycxx::detail::exec::ts_access::backend(sch)};
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
inline auto task_scheduler::schedule() const noexcept { return ycxx::adl_free::exec_ts_sender{*this}; }

template <class E>
struct with_error {
  using type = remove_cvref_t<E>;
  type error;
};
template <class E>
with_error(E) -> with_error<E>;
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.task]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Env>
struct task_types {
  static auto alloc() {
    if constexpr (requires { typename Env::allocator_type; })
      return std::type_identity<typename Env::allocator_type>{};
    else
      return std::type_identity<std::allocator<std::byte>>{};
  }
  static auto sched() {
    if constexpr (requires { typename Env::start_scheduler_type; })
      return std::type_identity<typename Env::start_scheduler_type>{};
    else
      return std::type_identity<std::execution::task_scheduler>{};
  }
  static auto source() {
    if constexpr (requires { typename Env::stop_source_type; })
      return std::type_identity<typename Env::stop_source_type>{};
    else
      return std::type_identity<std::inplace_stop_source>{};
  }
  static auto errors() {
    if constexpr (requires { typename Env::error_types; })
      return std::type_identity<typename Env::error_types>{};
    else
      return std::type_identity<std::execution::completion_signatures<std::execution::set_error_t(std::exception_ptr)>>{};
  }
  using allocator_type = typename decltype(alloc())::type;
  using start_scheduler_type = typename decltype(sched())::type;
  using stop_source_type = typename decltype(source())::type;
  using error_types = typename decltype(errors())::type;
};

template <class CS>
struct task_error_args {
  static constexpr bool valid = false;
};
template <class... Es>
struct task_error_args<std::execution::completion_signatures<set_error_t(Es)...>> {
  static constexpr bool valid = true;
  using variant = std::variant<std::monostate, Es...>;
  template <class E>
  static constexpr std::size_t convertible_count = (std::size_t{0} + ... + std::size_t{std::is_convertible_v<E, Es>});
  template <class E>
  using target = Es...[first_true<std::is_convertible_v<E, Es>...>];
};

// A unit of the frame allocation ([task.promise]/14): __STDCPP_DEFAULT_NEW_ALIGNMENT__ in size and
// alignment.
struct alignas(cfg::default_new_alignment) task_frame_unit {
  std::byte b[cfg::default_new_alignment];
};
inline constexpr std::size_t task_units(std::size_t bytes) noexcept { return (bytes + sizeof(task_frame_unit) - 1) / sizeof(task_frame_unit); }

template <class Alloc>
void* task_frame_allocate(std::size_t size, const Alloc& alloc) {
  using PAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<task_frame_unit>;
  static_assert(std::is_pointer_v<typename std::allocator_traits<PAlloc>::pointer>, "task: the allocator's pointer must be a pointer type");
  using dealloc_fn = void (*)(void*, std::size_t) noexcept;
  PAlloc palloc(alloc);
  const std::size_t n = task_units(size) + task_units(sizeof(dealloc_fn)) + task_units(sizeof(PAlloc));
  task_frame_unit* p = std::allocator_traits<PAlloc>::allocate(palloc, n);
  std::byte* tail = reinterpret_cast<std::byte*>(p + task_units(size));
  dealloc_fn fn = [](void* frame, std::size_t sz) noexcept {
    std::byte* t = static_cast<std::byte*>(frame) + task_units(sz) * sizeof(task_frame_unit);
    PAlloc* pa = std::launder(reinterpret_cast<PAlloc*>(t + task_units(sizeof(dealloc_fn)) * sizeof(task_frame_unit)));
    PAlloc a(static_cast<PAlloc&&>(*pa));
    pa->~PAlloc();
    const std::size_t m = task_units(sz) + task_units(sizeof(dealloc_fn)) + task_units(sizeof(PAlloc));
    std::allocator_traits<PAlloc>::deallocate(a, static_cast<task_frame_unit*>(frame), m);
  };
  ::new (static_cast<void*>(tail)) dealloc_fn(fn);
  ::new (static_cast<void*>(tail + task_units(sizeof(dealloc_fn)) * sizeof(task_frame_unit))) PAlloc(static_cast<PAlloc&&>(palloc));
  return p;
}
inline void task_frame_deallocate(void* p, std::size_t size) noexcept {
  using dealloc_fn = void (*)(void*, std::size_t) noexcept;
  dealloc_fn fn = *std::launder(reinterpret_cast<dealloc_fn*>(static_cast<std::byte*>(p) + task_units(size) * sizeof(task_frame_unit)));
  fn(p, size);
}

template <class Sig, class CS>
inline constexpr bool csigs_contain = false;
template <class Sig, class... Sigs>
inline constexpr bool csigs_contain<Sig, std::execution::completion_signatures<Sigs...>> = (std::is_same_v<Sig, Sigs> || ...);

// Requests a stop of the task's own stop source when the receiver's token is stopped.
template <class Source>
struct task_stop_forward {
  Source* src;
  void operator()() noexcept { src->request_stop(); }
};

// The part of task<T, Environment>::state<Rcvr> the promise sees.
template <class T, class Environment>
struct task_state_base {
  using types = task_types<Environment>;
  using errors = task_error_args<typename types::error_types>;
  using result_t = std::conditional_t<std::is_void_v<T>, std::monostate, std::conditional_t<std::is_reference_v<T>, std::reference_wrapper<std::remove_reference_t<T>>, T>>;

  std::coroutine_handle<> handle_;
  std::optional<result_t> result;
  std::exception_ptr error;
  typename errors::variant yielded_error;
  std::optional<typename types::start_scheduler_type> sched;
  std::optional<typename types::allocator_type> alloc;
  Environment* env_ptr = nullptr;

  virtual decltype(std::declval<typename types::stop_source_type&>().get_token()) ycxx_stop_token() noexcept = 0;
  virtual void ycxx_complete() noexcept = 0;
  virtual void ycxx_complete_stopped() noexcept = 0;
  virtual void ycxx_complete_yielded_error() noexcept = 0;

protected:
  ~task_state_base() = default;
};
// own-env-t ([task.state]/1)
template <class Environment, class RcvrEnv>
struct task_own_env {
  using type = std::execution::env<>;
};
template <class Environment, class RcvrEnv>
  requires requires { typename Environment::template env_type<RcvrEnv>; }
struct task_own_env<Environment, RcvrEnv> {
  using type = typename Environment::template env_type<RcvrEnv>;
};

// return_value or return_void ([task.promise]/10-11): a promise type may not declare both.
template <class T, class StateBase>
struct task_promise_return {
  StateBase* st_ = nullptr;
  template <class V = T>
    requires std::is_convertible_v<V, T>
  void return_value(V&& value) {
    if constexpr (std::is_reference_v<T>)
      st_->result.emplace(static_cast<T>(static_cast<V&&>(value)));
    else
      st_->result.emplace(static_cast<V&&>(value));
  }
};
template <class StateBase>
struct task_promise_return<void, StateBase> {
  StateBase* st_ = nullptr;
  void return_void() noexcept {}
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

template <class T = void, class Environment = env<>>
class task {
  static_assert(is_void_v<T> || is_reference_v<T> || (is_object_v<T> && !is_array_v<T> && is_same_v<T, remove_cv_t<T>>),
                "task<T, E>: T must be void, a reference type or a cv-unqualified non-array object type");
  static_assert(is_class_v<Environment>, "task<T, E>: E must be a class type");
  using types = ycxx::detail::exec::task_types<Environment>;
  using state_base = ycxx::detail::exec::task_state_base<T, Environment>;

public:
  using sender_concept = sender_tag;
  using allocator_type = typename types::allocator_type;
  using start_scheduler_type = typename types::start_scheduler_type;
  using stop_source_type = typename types::stop_source_type;
  using stop_token_type = decltype(declval<stop_source_type>().get_token());
  using error_types = typename types::error_types;
  static_assert(ycxx::detail::exec::task_error_args<error_types>::valid,
                "task: error_types must be a completion_signatures of set_error_t(E) signatures");

  class promise_type;

  template <receiver Rcvr>
  class state : state_base {
    friend class task;
    using own_env_t = typename ycxx::detail::exec::task_own_env<Environment, decltype(get_env(declval<Rcvr>()))>::type;
    using rcvr_token_t = stop_token_of_t<env_of_t<Rcvr>>;

    coroutine_handle<promise_type> handle;
    remove_cvref_t<Rcvr> rcvr;
    optional<stop_source_type> source;
    optional<stop_callback_for_t<rcvr_token_t, ycxx::detail::exec::task_stop_forward<stop_source_type>>> source_link;
    own_env_t own_env;
    Environment environment;

    static own_env_t make_own_env(const remove_cvref_t<Rcvr>& r) {
      if constexpr (requires { own_env_t(get_env(r)); })
        return own_env_t(get_env(r));
      else
        return own_env_t();
    }
    static Environment make_environment(const own_env_t& o, const remove_cvref_t<Rcvr>& r) {
      if constexpr (requires { Environment(o); })
        return Environment(o);
      else if constexpr (requires { Environment(get_env(r)); })
        return Environment(get_env(r));
      else
        return Environment();
    }

    void ycxx_destroy_frame() noexcept {
      if (handle) {
        auto h = handle;
        handle = {};
        this->handle_ = {};
        source_link.reset();
        h.destroy();
      }
    }

  public:
    using operation_state_concept = operation_state_tag;

    template <class R>
    state(coroutine_handle<promise_type> h, R&& rr)
        : handle(static_cast<coroutine_handle<promise_type>&&>(h)), rcvr(static_cast<R&&>(rr)), own_env(make_own_env(rcvr)),
          environment(make_environment(own_env, rcvr)) {}
    state(state&&) = delete;
    ~state() {
      source_link.reset();
      if (handle)
        handle.destroy();
    }

    void start() & noexcept {
      promise_type& prom = handle.promise();
      prom.st_ = this;
      this->handle_ = handle;
      this->env_ptr = __builtin_addressof(environment);
      if constexpr (requires { start_scheduler_type(get_start_scheduler(get_env(rcvr))); })
        this->sched.emplace(get_start_scheduler(get_env(rcvr)));
      else
        this->sched.emplace();
      if constexpr (requires { allocator_type(get_allocator(get_env(rcvr))); })
        this->alloc.emplace(get_allocator(get_env(rcvr)));
      else
        this->alloc.emplace();
      handle.resume();
    }

    // get-stop-token ([task.state]/5)
    stop_token_type ycxx_stop_token() noexcept override {
      if constexpr (same_as<stop_token_type, rcvr_token_t>) {
        return get_stop_token(get_env(rcvr));
      } else {
        if (!source.has_value()) {
          source.emplace();
          auto tok = get_stop_token(get_env(rcvr));
          if constexpr (!unstoppable_token<rcvr_token_t>)
            source_link.emplace(tok, ycxx::detail::exec::task_stop_forward<stop_source_type>{&*source});
        }
        return source->get_token();
      }
    }
    // [task.promise]/3
    void ycxx_complete() noexcept override {
      ycxx_destroy_frame();
      if (this->error) {
        if constexpr (requires { set_error(static_cast<remove_cvref_t<Rcvr>&&>(rcvr), static_cast<exception_ptr&&>(this->error)); })
          set_error(static_cast<remove_cvref_t<Rcvr>&&>(rcvr), static_cast<exception_ptr&&>(this->error));
      } else if constexpr (is_void_v<T>) {
        set_value(static_cast<remove_cvref_t<Rcvr>&&>(rcvr));
      } else if constexpr (is_reference_v<T>) {
        set_value(static_cast<remove_cvref_t<Rcvr>&&>(rcvr), static_cast<T>(this->result->get()));
      } else {
        set_value(static_cast<remove_cvref_t<Rcvr>&&>(rcvr), static_cast<T&&>(*this->result));
      }
    }
    // [task.promise]/8
    void ycxx_complete_stopped() noexcept override {
      ycxx_destroy_frame();
      set_stopped(static_cast<remove_cvref_t<Rcvr>&&>(rcvr));
    }
    // [task.promise]/5
    void ycxx_complete_yielded_error() noexcept override {
      ycxx_destroy_frame();
      visit(
          [this]<class E>(E& e) noexcept {
            if constexpr (!is_same_v<E, monostate>)
              set_error(static_cast<remove_cvref_t<Rcvr>&&>(rcvr), static_cast<E&&>(e));
          },
          this->yielded_error);
    }
  };

  task(task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
  ~task() {
    if (handle)
      handle.destroy();
  }

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ycxx_csigs<Self, Env...>();
  }
  template <class Self, class... Env>
  using ycxx_csigs = ycxx::detail::exec::sigs_concat_t<completion_signatures<ycxx::detail::exec::set_value_sig_t<T>>, error_types,
                                                       completion_signatures<set_stopped_t()>>;

  template <receiver Rcvr>
  state<Rcvr> connect(Rcvr&& recv) && {
    static_assert(requires { allocator_type(get_allocator(get_env(recv))); } || requires { allocator_type(); },
                  "task: the receiver's allocator cannot be converted to the task's allocator_type");
    ycxx::detail::precondition(static_cast<bool>(handle), "task::connect: the task has no coroutine (moved from or connected)");
    return state<Rcvr>(std::exchange(handle, {}), static_cast<Rcvr&&>(recv));
  }

private:
  explicit task(coroutine_handle<promise_type> h) noexcept : handle(h) {}
  coroutine_handle<promise_type> handle;
};

template <class T, class Environment>
class task<T, Environment>::promise_type : public ycxx::detail::exec::task_promise_return<T, ycxx::detail::exec::task_state_base<T, Environment>> {
  template <receiver>
  friend class task::state;
  using ycxx::detail::exec::task_promise_return<T, state_base>::st_;
  using errors = ycxx::detail::exec::task_error_args<error_types>;

  struct final_awaiter {
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(coroutine_handle<promise_type> h) noexcept { h.promise().st_->ycxx_complete(); }
    void await_resume() noexcept {}
  };
  struct yield_error_awaiter {
    state_base* st;
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(coroutine_handle<>) noexcept { st->ycxx_complete_yielded_error(); }
    void await_resume() noexcept {}
  };
  struct env_t {
    const promise_type* p;
    start_scheduler_type query(get_start_scheduler_t) const noexcept { return *p->st_->sched; }
    allocator_type query(get_allocator_t) const noexcept { return *p->st_->alloc; }
    stop_token_type query(get_stop_token_t) const noexcept { return p->st_->ycxx_stop_token(); }
    template <class Q, class... As>
      requires(!same_as<Q, get_start_scheduler_t> && !same_as<Q, get_allocator_t> && !same_as<Q, get_stop_token_t>) &&
              ycxx::detail::exec::forwarding_query_c<Q> && ycxx::detail::exec::has_query<Environment, Q, As...>
    constexpr decltype(auto) query(Q q, As&&... as) const noexcept(noexcept(declval<const Environment&>().query(q, static_cast<As&&>(as)...))) {
      return ycxx::detail::exec::as_const_ref(*p->st_->env_ptr).query(q, static_cast<As&&>(as)...);
    }
  };

public:
  task get_return_object() noexcept { return task(coroutine_handle<promise_type>::from_promise(*this)); }
  static constexpr suspend_always initial_suspend() noexcept { return {}; }
  auto final_suspend() noexcept { return final_awaiter{}; }
  // [task.promise]/7
  void unhandled_exception() {
    if constexpr (ycxx::detail::exec::csigs_contain<set_error_t(exception_ptr), error_types>)
      st_->error = std::current_exception();
    else
      std::terminate();
  }
  coroutine_handle<> unhandled_stopped() noexcept {
    st_->ycxx_complete_stopped();
    return noop_coroutine();
  }
  template <class E>
  auto yield_value(with_error<E> error) {
    using Err = typename with_error<E>::type;
    static_assert(errors::template convertible_count<Err> == 1,
                  "task: co_yield with_error: the error must be convertible to exactly one of error_types");
    using Cerr = typename errors::template target<Err>;
    st_->yielded_error.template emplace<Cerr>(static_cast<Err&&>(error.error));
    return yield_error_awaiter{st_};
  }
  template <sender Sender>
  auto await_transform(Sender&& sndr) {
    if constexpr (same_as<inline_scheduler, start_scheduler_type>)
      return as_awaitable(static_cast<Sender&&>(sndr), *this);
    else
      return as_awaitable(affine(static_cast<Sender&&>(sndr)), *this);
  }
  env_t get_env() const noexcept { return {this}; }

  void* operator new(size_t size) { return operator new(size, allocator_arg, allocator_type()); }
  template <class Alloc, class... Args>
  void* operator new(size_t size, allocator_arg_t, Alloc alloc, Args&&...) {
    return ycxx::detail::exec::task_frame_allocate(size, alloc);
  }
  template <class This, class Alloc, class... Args>
  void* operator new(size_t size, const This&, allocator_arg_t, Alloc alloc, Args&&...) {
    return ycxx::detail::exec::task_frame_allocate(size, alloc);
  }
  void operator delete(void* pointer, size_t size) noexcept { ycxx::detail::exec::task_frame_deallocate(pointer, size); }
};

}} // namespace std::execution
