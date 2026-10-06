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
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
class task_scheduler;
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
struct __ts_access;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// What task_scheduler needs of its backend beyond parallel_scheduler_backend.
struct __exec_ts_backend_base : std::execution::parallel_scheduler_replacement::parallel_scheduler_backend {
  virtual const void* __ycxx_type() const noexcept = 0;
  virtual const void* __ycxx_sched() const noexcept = 0;
  virtual std::execution::forward_progress_guarantee __ycxx_fpg() const noexcept = 0;
  virtual bool __ycxx_equal_to(const std::execution::task_scheduler& __lhs) const noexcept = 0;
};

// just-sndr-like ([exec.task.scheduler]/9)
template <class _Sch>
struct __exec_just_sndr_like {
  using sender_concept = std::execution::sender_tag;
  _Sch __sched;
  template <class _Self, class... _Env>
  using __ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t()>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return std::execution::completion_signatures<std::execution::set_value_t()>();
  }
  struct __attrs {
    _Sch __sched;
    template <class... _Envs>
      requires requires(const _Sch& s, const _Envs&... e) { std::execution::get_completion_scheduler<std::execution::set_value_t>(s, e...); }
    auto query(std::execution::get_completion_scheduler_t<std::execution::set_value_t>, const _Envs&... __envs) const noexcept {
      return std::execution::get_completion_scheduler<std::execution::set_value_t>(__sched, __envs...);
    }
    template <class... _Envs>
      requires requires(const _Sch& s, const _Envs&... e) { std::execution::get_completion_domain<std::execution::set_value_t>(s, e...); }
    auto query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const _Envs&... __envs) const noexcept {
      return std::execution::get_completion_domain<std::execution::set_value_t>(__sched, __envs...);
    }
  };
  __attrs get_env() const noexcept { return {__sched}; }
  template <class _Rcvr>
  __exec_inline_state<std::remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) const noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<_Rcvr>, _Rcvr>) {
    return {static_cast<_Rcvr&&>(__rcvr)};
  }
};

// WRAP-RCVR(r) ([exec.task.scheduler]/8) for an operation the backend placed in the proxy's
// storage (or on the heap): it destroys that operation before it completes r.
struct __exec_wrap_env {
  std::execution::parallel_scheduler_replacement::receiver_proxy* r;
  std::inplace_stop_token query(std::get_stop_token_t) const noexcept {
    return r->try_query<std::inplace_stop_token>(std::get_stop_token).value_or(std::inplace_stop_token());
  }
};
struct __exec_wrap_rcvr {
  using receiver_concept = std::execution::receiver_tag;
  std::execution::parallel_scheduler_replacement::receiver_proxy* r;
  void* op;
  void (*destroy)(void*) noexcept;

  void set_value() && noexcept {
    auto* __rr = r;
    destroy(op);
    __rr->set_value();
  }
  template <class _Ep>
  void set_error(_Ep&& e) && noexcept {
    auto* __rr = r;
    std::exception_ptr __ep = ::__ycxx::__detail::__exec::__as_except_ptr(static_cast<_Ep&&>(e));
    destroy(op);
    __rr->set_error(static_cast<std::exception_ptr&&>(__ep));
  }
  void set_stopped() && noexcept {
    auto* __rr = r;
    destroy(op);
    __rr->set_stopped();
  }
  __exec_wrap_env get_env() const noexcept { return {r}; }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

class task_scheduler {
  class __ts_domain;
  template <scheduler _Sch>
  class __backend_for;
  friend struct __ycxx::__detail::__exec::__ts_access;

  shared_ptr<__ycxx::__adl_free::__exec_ts_backend_base> __sch_;

public:
  using scheduler_concept = scheduler_tag;

  template <class _Sch, class _Allocator = allocator<void>>
    requires(!same_as<task_scheduler, remove_cvref_t<_Sch>>) && scheduler<_Sch>
  explicit task_scheduler(_Sch&& __sch, _Allocator __alloc = {});
  task_scheduler(const task_scheduler&) = default;
  task_scheduler& operator=(const task_scheduler&) = default;

  auto schedule() const noexcept;

  friend bool operator==(const task_scheduler& __lhs, const task_scheduler& __rhs) noexcept { return __rhs.__sch_->__ycxx_equal_to(__lhs); }
  // TS deduced (only task_scheduler is accepted) so that the operator, found by argument-dependent
  // lookup for any type with task_scheduler among its template arguments, checks scheduler<Sch>
  // only when the left operand is a task_scheduler.
  template <class _TS, class _Sch>
    requires same_as<_TS, task_scheduler> && (!same_as<task_scheduler, _Sch>) && scheduler<_Sch>
  friend bool operator==(const _TS& __lhs, const _Sch& __rhs) noexcept {
    if (__lhs.__sch_->__ycxx_type() != &__ycxx::__detail::__exec::__type_key<_Sch>)
      return false;
    return *static_cast<const _Sch*>(__lhs.__sch_->__ycxx_sched()) == __rhs;
  }

  forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept { return __sch_->__ycxx_fpg(); }
  template <class... _Envs>
  __ts_domain query(get_completion_domain_t<set_value_t>, const _Envs&...) const noexcept;
};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
struct __ts_access {
  static const auto& __backend(const std::execution::task_scheduler& s) noexcept { return s.__sch_; }
};

// An operation of type Op placed in a proxy's backend storage when it fits, else on the heap.
template <class _Op_>
struct __placed_op {
  static void __destroy_in_place(void* p) noexcept { static_cast<_Op_*>(p)->~_Op_(); }
  static void __destroy_heap(void* p) noexcept {
    static_cast<_Op_*>(p)->~_Op_();
    ::operator delete(p, std::align_val_t(alignof(_Op_)));
  }
  // Constructs make(wrap_rcvr) in s or on the heap and starts it; failures complete r.
  template <class _Make>
  static void run(std::execution::parallel_scheduler_replacement::receiver_proxy& r, std::span<std::byte> s, _Make __make) noexcept {
    void* __mem = s.data();
    std::size_t space = s.size();
    bool __heap = false;
    if (!__mem || !std::align(alignof(_Op_), sizeof(_Op_), __mem, space)) {
      __mem = ::operator new(sizeof(_Op_), std::align_val_t(alignof(_Op_)), std::nothrow);
      if (!__mem) {
        r.set_error(std::make_exception_ptr(std::bad_alloc()));
        return;
      }
      __heap = true;
    }
    ::__ycxx::__adl_free::__exec_wrap_rcvr __w{&r, __mem, __heap ? &__destroy_heap : &__destroy_in_place};
    auto construct = [&] { return ::new (__mem) _Op_(__make(__w)); };
    _Op_* op;
    if constexpr (noexcept(__make(__w)) || !__cfg::exceptions) {
      op = construct();
    } else {
      try {
        op = construct();
      } catch (...) {
        if (__heap)
          ::operator delete(__mem, std::align_val_t(alignof(_Op_)));
        r.set_error(std::current_exception());
        return;
      }
    }
    std::execution::start(*op);
  }
};

// fn of schedule_bulk_chunked / schedule_bulk_unchunked ([exec.task.scheduler]/11-12)
struct __ts_bulk_fn {
  std::execution::parallel_scheduler_replacement::bulk_item_receiver_proxy* r;
  std::size_t chunk;
  std::size_t __shape;
  void operator()(std::size_t i) const noexcept {
    const std::size_t b = i * chunk;
    r->execute(b, b + chunk < __shape ? b + chunk : __shape);
  }
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <scheduler _Sch>
class task_scheduler::__backend_for : public __ycxx::__adl_free::__exec_ts_backend_base {
  using __wrap_rcvr = __ycxx::__adl_free::__exec_wrap_rcvr;
  using __bulk_sndr = decltype(bulk(declval<__ycxx::__adl_free::__exec_just_sndr_like<_Sch>>(), par, size_t(), __ycxx::__detail::__exec::__ts_bulk_fn()));

  void __run_bulk(size_t __num_chunks, size_t chunk, size_t __shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& r,
                span<byte> s) noexcept {
    using __op_t = connect_result_t<__bulk_sndr, __wrap_rcvr>;
    __ycxx::__detail::__exec::__placed_op<__op_t>::run(r, s, [&](__wrap_rcvr __w) {
      return connect(bulk(__ycxx::__adl_free::__exec_just_sndr_like<_Sch>{__sched_}, par, __num_chunks, __ycxx::__detail::__exec::__ts_bulk_fn{&r, chunk, __shape}),
                     static_cast<__wrap_rcvr&&>(__w));
    });
  }

public:
  explicit __backend_for(_Sch __sch) : __sched_(static_cast<_Sch&&>(__sch)) {}

  void schedule(parallel_scheduler_replacement::receiver_proxy& r, span<byte> s) noexcept override {
    using __op_t = connect_result_t<schedule_result_t<_Sch&>, __wrap_rcvr>;
    __ycxx::__detail::__exec::__placed_op<__op_t>::run(r, s, [&](__wrap_rcvr __w) { return connect(execution::schedule(__sched_), static_cast<__wrap_rcvr&&>(__w)); });
  }
  void schedule_bulk_chunked(size_t __shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& r, span<byte> s) noexcept override {
    // chunk_size: at most 64 chunks (the scheduler behind is unknown).
    const size_t chunk = __shape == 0 ? 1 : (__shape + 63) / 64;
    __run_bulk((__shape + chunk - 1) / chunk, chunk, __shape, r, s);
  }
  void schedule_bulk_unchunked(size_t __shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& r, span<byte> s) noexcept override {
    __run_bulk(__shape, 1, __shape, r, s);
  }

  const void* __ycxx_type() const noexcept override { return &__ycxx::__detail::__exec::__type_key<_Sch>; }
  const void* __ycxx_sched() const noexcept override { return __builtin_addressof(__sched_); }
  forward_progress_guarantee __ycxx_fpg() const noexcept override { return get_forward_progress_guarantee(__sched_); }
  bool __ycxx_equal_to(const task_scheduler& __lhs) const noexcept override { return __lhs == __sched_; }

private:
  _Sch __sched_;
};

template <class _Sch, class _Allocator>
  requires(!same_as<task_scheduler, remove_cvref_t<_Sch>>) && scheduler<_Sch>
task_scheduler::task_scheduler(_Sch&& __sch, _Allocator __alloc)
    : __sch_(allocate_shared<__backend_for<remove_cvref_t<_Sch>>>(__alloc, static_cast<_Sch&&>(__sch))) {
  static_assert(__ycxx::__detail::__exec::__infallible_scheduler<_Sch, env<>>, "task_scheduler: the scheduler must be infallible");
}

class task_scheduler::__ts_domain : public default_domain {
public:
  template <class _BulkSndr, class _Env>
    requires(is_same_v<tag_of_t<_BulkSndr>, bulk_chunked_t> || is_same_v<tag_of_t<_BulkSndr>, bulk_unchunked_t>) &&
            requires(_BulkSndr&& b) { auto(static_cast<_BulkSndr&&>(b)); }
  static constexpr auto transform_sender(set_value_t, _BulkSndr&& __bulk_sndr, const _Env& env) noexcept(is_nothrow_constructible_v<decay_t<_BulkSndr>, _BulkSndr>) {
    auto&& data = static_cast<_BulkSndr&&>(__bulk_sndr).template get<1>();
    auto&& __child = static_cast<_BulkSndr&&>(__bulk_sndr).template get<2>();
    auto __sch = __ycxx::__detail::__exec::__call_with_default(get_completion_scheduler<set_value_t>, __ycxx::__adl_free::__exec_not_a_scheduler(),
                                                     get_env(__child), __ycxx::__detail::__exec::__fwd_env(env));
    if constexpr (!is_same_v<decltype(__sch), task_scheduler>) {
      return __ycxx::__adl_free::__exec_not_a_sender();
    } else {
      constexpr bool __chunked = is_same_v<tag_of_t<_BulkSndr>, bulk_chunked_t>;
      using _Shape = remove_cvref_t<decltype(data.template get<1>())>;
      using _Fp = remove_cvref_t<decltype(data.template get<2>())>;
      return __ycxx::__adl_free::__exec_par_bulk_sender<__chunked, remove_cvref_t<decltype(__child)>, _Shape, _Fp>{
          std::forward_like<_BulkSndr>(__child), true, _Shape(data.template get<1>()), std::forward_like<_BulkSndr>(data.template get<2>()), __sch.__sch_};
    }
  }
};

template <class... _Envs>
task_scheduler::__ts_domain task_scheduler::query(get_completion_domain_t<set_value_t>, const _Envs&...) const noexcept {
  return {};
}

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// ts-sndr ([exec.task.scheduler]/13)
struct __exec_ts_sender {
  using sender_concept = std::execution::sender_tag;
  std::execution::task_scheduler __sch;
  template <class _Self, class... _Env>
  using __ycxx_csigs = std::conditional_t<(std::unstoppable_token<std::stop_token_of_t<_Env>> && ...) && sizeof...(_Env) != 0,
                                        std::execution::completion_signatures<std::execution::set_value_t()>,
                                        std::execution::completion_signatures<std::execution::set_value_t(), std::execution::set_stopped_t()>>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return __ycxx_csigs<_Self, _Env...>();
  }
  struct __attrs {
    std::execution::task_scheduler __sch;
    template <class... _Envs>
    std::execution::task_scheduler query(std::execution::get_completion_scheduler_t<std::execution::set_value_t>, const _Envs&...) const noexcept {
      return __sch;
    }
    template <class... _Envs>
    auto query(std::execution::get_completion_domain_t<std::execution::set_value_t>, const _Envs&... __envs) const noexcept {
      return __sch.query(std::execution::get_completion_domain<std::execution::set_value_t>, __envs...);
    }
  };
  __attrs get_env() const noexcept { return {__sch}; }
  template <class _Rcvr>
  __exec_par_sched_op<std::decay_t<_Rcvr>, false> connect(_Rcvr&& __rcvr) const noexcept(std::is_nothrow_constructible_v<std::decay_t<_Rcvr>, _Rcvr>) {
    return {static_cast<_Rcvr&&>(__rcvr), ::__ycxx::__detail::__exec::__ts_access::__backend(__sch)};
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
inline auto task_scheduler::schedule() const noexcept { return __ycxx::__adl_free::__exec_ts_sender{*this}; }

template <class _Ep>
struct with_error {
  using type = remove_cvref_t<_Ep>;
  type error;
};
template <class _Ep>
with_error(_Ep) -> with_error<_Ep>;
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.task]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Env>
struct __task_types {
  static auto __alloc() {
    if constexpr (requires { typename _Env::allocator_type; })
      return std::type_identity<typename _Env::allocator_type>{};
    else
      return std::type_identity<std::allocator<std::byte>>{};
  }
  static auto __sched() {
    if constexpr (requires { typename _Env::start_scheduler_type; })
      return std::type_identity<typename _Env::start_scheduler_type>{};
    else
      return std::type_identity<std::execution::task_scheduler>{};
  }
  static auto __source() {
    if constexpr (requires { typename _Env::stop_source_type; })
      return std::type_identity<typename _Env::stop_source_type>{};
    else
      return std::type_identity<std::inplace_stop_source>{};
  }
  static auto __errors() {
    if constexpr (requires { typename _Env::error_types; })
      return std::type_identity<typename _Env::error_types>{};
    else
      return std::type_identity<std::execution::completion_signatures<std::execution::set_error_t(std::exception_ptr)>>{};
  }
  using allocator_type = typename decltype(__alloc())::type;
  using start_scheduler_type = typename decltype(__sched())::type;
  using stop_source_type = typename decltype(__source())::type;
  using error_types = typename decltype(__errors())::type;
};

template <class _CS>
struct __task_error_args {
  static constexpr bool valid = false;
};
template <class... _Es>
struct __task_error_args<std::execution::completion_signatures<set_error_t(_Es)...>> {
  static constexpr bool valid = true;
  using variant = std::variant<std::monostate, _Es...>;
  template <class _Ep>
  static constexpr std::size_t __convertible_count = (std::size_t{0} + ... + std::size_t{std::is_convertible_v<_Ep, _Es>});
  template <class _Ep>
  using target = _Es...[__first_true<std::is_convertible_v<_Ep, _Es>...>];
};

// A unit of the frame allocation ([task.promise]/14): __STDCPP_DEFAULT_NEW_ALIGNMENT__ in size and
// alignment.
struct alignas(__cfg::__default_new_alignment) __task_frame_unit {
  std::byte b[__cfg::__default_new_alignment];
};
inline constexpr std::size_t __task_units(std::size_t bytes) noexcept { return (bytes + sizeof(__task_frame_unit) - 1) / sizeof(__task_frame_unit); }

template <class _Alloc>
void* __task_frame_allocate(std::size_t size, const _Alloc& __alloc) {
  using _PAlloc = typename std::allocator_traits<_Alloc>::template rebind_alloc<__task_frame_unit>;
  static_assert(std::is_pointer_v<typename std::allocator_traits<_PAlloc>::pointer>, "task: the allocator's pointer must be a pointer type");
  using __dealloc_fn = void (*)(void*, std::size_t) noexcept;
  _PAlloc __palloc(__alloc);
  const std::size_t n = __task_units(size) + __task_units(sizeof(__dealloc_fn)) + __task_units(sizeof(_PAlloc));
  __task_frame_unit* p = std::allocator_traits<_PAlloc>::allocate(__palloc, n);
  std::byte* __tail = reinterpret_cast<std::byte*>(p + __task_units(size));
  __dealloc_fn __fn = [](void* __frame, std::size_t __sz) noexcept {
    std::byte* t = static_cast<std::byte*>(__frame) + __task_units(__sz) * sizeof(__task_frame_unit);
    _PAlloc* __pa = std::launder(reinterpret_cast<_PAlloc*>(t + __task_units(sizeof(__dealloc_fn)) * sizeof(__task_frame_unit)));
    _PAlloc a(static_cast<_PAlloc&&>(*__pa));
    __pa->~_PAlloc();
    const std::size_t m = __task_units(__sz) + __task_units(sizeof(__dealloc_fn)) + __task_units(sizeof(_PAlloc));
    std::allocator_traits<_PAlloc>::deallocate(a, static_cast<__task_frame_unit*>(__frame), m);
  };
  ::new (static_cast<void*>(__tail)) __dealloc_fn(__fn);
  ::new (static_cast<void*>(__tail + __task_units(sizeof(__dealloc_fn)) * sizeof(__task_frame_unit))) _PAlloc(static_cast<_PAlloc&&>(__palloc));
  return p;
}
inline void __task_frame_deallocate(void* p, std::size_t size) noexcept {
  using __dealloc_fn = void (*)(void*, std::size_t) noexcept;
  __dealloc_fn __fn = *std::launder(reinterpret_cast<__dealloc_fn*>(static_cast<std::byte*>(p) + __task_units(size) * sizeof(__task_frame_unit)));
  __fn(p, size);
}

template <class _Sig, class _CS>
inline constexpr bool __csigs_contain = false;
template <class _Sig, class... _Sigs>
inline constexpr bool __csigs_contain<_Sig, std::execution::completion_signatures<_Sigs...>> = (std::is_same_v<_Sig, _Sigs> || ...);

// Requests a stop of the task's own stop source when the receiver's token is stopped.
template <class _Source>
struct __task_stop_forward {
  _Source* __src;
  void operator()() noexcept { __src->request_stop(); }
};

// The part of task<T, Environment>::state<Rcvr> the promise sees.
template <class _Tp, class _Environment>
struct __task_state_base {
  using __types = __task_types<_Environment>;
  using __errors = __task_error_args<typename __types::error_types>;
  using __result_t = std::conditional_t<std::is_void_v<_Tp>, std::monostate, std::conditional_t<std::is_reference_v<_Tp>, std::reference_wrapper<std::remove_reference_t<_Tp>>, _Tp>>;

  std::coroutine_handle<> __handle_;
  std::optional<__result_t> result;
  std::exception_ptr error;
  typename __errors::variant __yielded_error;
  std::optional<typename __types::start_scheduler_type> __sched;
  std::optional<typename __types::allocator_type> __alloc;
  _Environment* __env_ptr = nullptr;

  virtual decltype(std::declval<typename __types::stop_source_type&>().get_token()) __ycxx_stop_token() noexcept = 0;
  virtual void __ycxx_complete() noexcept = 0;
  virtual void __ycxx_complete_stopped() noexcept = 0;
  virtual void __ycxx_complete_yielded_error() noexcept = 0;

protected:
  ~__task_state_base() = default;
};
// own-env-t ([task.state]/1)
template <class _Environment, class _RcvrEnv>
struct __task_own_env {
  using type = std::execution::env<>;
};
template <class _Environment, class _RcvrEnv>
  requires requires { typename _Environment::template env_type<_RcvrEnv>; }
struct __task_own_env<_Environment, _RcvrEnv> {
  using type = typename _Environment::template env_type<_RcvrEnv>;
};

// return_value or return_void ([task.promise]/10-11): a promise type may not declare both.
template <class _Tp, class _StateBase>
struct __task_promise_return {
  _StateBase* __st_ = nullptr;
  template <class _Vp = _Tp>
    requires std::is_convertible_v<_Vp, _Tp>
  void return_value(_Vp&& value) {
    if constexpr (std::is_reference_v<_Tp>)
      __st_->result.emplace(static_cast<_Tp>(static_cast<_Vp&&>(value)));
    else
      __st_->result.emplace(static_cast<_Vp&&>(value));
  }
};
template <class _StateBase>
struct __task_promise_return<void, _StateBase> {
  _StateBase* __st_ = nullptr;
  void return_void() noexcept {}
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

template <class _Tp = void, class _Environment = env<>>
class task {
  static_assert(is_void_v<_Tp> || is_reference_v<_Tp> || (is_object_v<_Tp> && !is_array_v<_Tp> && is_same_v<_Tp, remove_cv_t<_Tp>>),
                "task<T, E>: T must be void, a reference type or a cv-unqualified non-array object type");
  static_assert(is_class_v<_Environment>, "task<T, E>: E must be a class type");
  using __types = __ycxx::__detail::__exec::__task_types<_Environment>;
  using __state_base = __ycxx::__detail::__exec::__task_state_base<_Tp, _Environment>;

public:
  using sender_concept = sender_tag;
  using allocator_type = typename __types::allocator_type;
  using start_scheduler_type = typename __types::start_scheduler_type;
  using stop_source_type = typename __types::stop_source_type;
  using stop_token_type = decltype(declval<stop_source_type>().get_token());
  using error_types = typename __types::error_types;
  static_assert(__ycxx::__detail::__exec::__task_error_args<error_types>::valid,
                "task: error_types must be a completion_signatures of set_error_t(E) signatures");

  class promise_type;

  template <receiver _Rcvr>
  class state : __state_base {
    friend class task;
    using __own_env_t = typename __ycxx::__detail::__exec::__task_own_env<_Environment, decltype(get_env(declval<_Rcvr>()))>::type;
    using __rcvr_token_t = stop_token_of_t<env_of_t<_Rcvr>>;

    coroutine_handle<promise_type> handle;
    remove_cvref_t<_Rcvr> __rcvr;
    optional<stop_source_type> __source;
    optional<stop_callback_for_t<__rcvr_token_t, __ycxx::__detail::__exec::__task_stop_forward<stop_source_type>>> __source_link;
    __own_env_t __own_env;
    _Environment environment;

    static __own_env_t __make_own_env(const remove_cvref_t<_Rcvr>& r) {
      if constexpr (requires { __own_env_t(get_env(r)); })
        return __own_env_t(get_env(r));
      else
        return __own_env_t();
    }
    static _Environment __make_environment(const __own_env_t& __o, const remove_cvref_t<_Rcvr>& r) {
      if constexpr (requires { _Environment(__o); })
        return _Environment(__o);
      else if constexpr (requires { _Environment(get_env(r)); })
        return _Environment(get_env(r));
      else
        return _Environment();
    }

    void __ycxx_destroy_frame() noexcept {
      if (handle) {
        auto h = handle;
        handle = {};
        this->__handle_ = {};
        __source_link.reset();
        h.destroy();
      }
    }

  public:
    using operation_state_concept = operation_state_tag;

    template <class _Rp>
    state(coroutine_handle<promise_type> h, _Rp&& __rr)
        : handle(static_cast<coroutine_handle<promise_type>&&>(h)), __rcvr(static_cast<_Rp&&>(__rr)), __own_env(__make_own_env(__rcvr)),
          environment(__make_environment(__own_env, __rcvr)) {}
    state(state&&) = delete;
    ~state() {
      __source_link.reset();
      if (handle)
        handle.destroy();
    }

    void start() & noexcept {
      promise_type& __prom = handle.promise();
      __prom.__st_ = this;
      this->__handle_ = handle;
      this->__env_ptr = __builtin_addressof(environment);
      if constexpr (requires { start_scheduler_type(get_start_scheduler(get_env(__rcvr))); })
        this->__sched.emplace(get_start_scheduler(get_env(__rcvr)));
      else
        this->__sched.emplace();
      if constexpr (requires { allocator_type(get_allocator(get_env(__rcvr))); })
        this->__alloc.emplace(get_allocator(get_env(__rcvr)));
      else
        this->__alloc.emplace();
      handle.resume();
    }

    // get-stop-token ([task.state]/5)
    stop_token_type __ycxx_stop_token() noexcept override {
      if constexpr (same_as<stop_token_type, __rcvr_token_t>) {
        return get_stop_token(get_env(__rcvr));
      } else {
        if (!__source.has_value()) {
          auto __tok = get_stop_token(get_env(__rcvr));
          // /5.2: the source's stop_possible() is the receiver token's. A source type with a
          // no-state constructor (stop_source) can have it false; inplace_stop_source cannot.
          if constexpr (is_constructible_v<stop_source_type, nostopstate_t>) {
            if (!__tok.stop_possible()) {
              __source.emplace(nostopstate);
              return __source->get_token();
            }
          }
          __source.emplace();
          if constexpr (!unstoppable_token<__rcvr_token_t>)
            __source_link.emplace(__tok, __ycxx::__detail::__exec::__task_stop_forward<stop_source_type>{&*__source});
        }
        return __source->get_token();
      }
    }
    // [task.promise]/3
    void __ycxx_complete() noexcept override {
      __ycxx_destroy_frame();
      if (this->error) {
        if constexpr (requires { set_error(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr), static_cast<exception_ptr&&>(this->error)); })
          set_error(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr), static_cast<exception_ptr&&>(this->error));
      } else if constexpr (is_void_v<_Tp>) {
        set_value(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr));
      } else if constexpr (is_reference_v<_Tp>) {
        set_value(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr), static_cast<_Tp>(this->result->get()));
      } else {
        set_value(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr), static_cast<_Tp&&>(*this->result));
      }
    }
    // [task.promise]/8
    void __ycxx_complete_stopped() noexcept override {
      __ycxx_destroy_frame();
      set_stopped(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr));
    }
    // [task.promise]/5
    void __ycxx_complete_yielded_error() noexcept override {
      __ycxx_destroy_frame();
      visit(
          [this]<class _Ep>(_Ep& e) noexcept {
            if constexpr (!is_same_v<_Ep, monostate>)
              set_error(static_cast<remove_cvref_t<_Rcvr>&&>(__rcvr), static_cast<_Ep&&>(e));
          },
          this->__yielded_error);
    }
  };

  task(task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
  ~task() {
    if (handle)
      handle.destroy();
  }

  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return __ycxx_csigs<_Self, _Env...>();
  }
  template <class _Self, class... _Env>
  using __ycxx_csigs = __ycxx::__detail::__exec::__sigs_concat_t<completion_signatures<__ycxx::__detail::__exec::__set_value_sig_t<_Tp>>, error_types,
                                                       completion_signatures<set_stopped_t()>>;

  template <receiver _Rcvr>
  state<_Rcvr> connect(_Rcvr&& __recv) && {
    static_assert(requires { allocator_type(get_allocator(get_env(__recv))); } || requires { allocator_type(); },
                  "task: the receiver's allocator cannot be converted to the task's allocator_type");
    __ycxx::__detail::__precondition(static_cast<bool>(handle), "task::connect: the task has no coroutine (moved from or connected)");
    return state<_Rcvr>(std::exchange(handle, {}), static_cast<_Rcvr&&>(__recv));
  }

private:
  explicit task(coroutine_handle<promise_type> h) noexcept : handle(h) {}
  coroutine_handle<promise_type> handle;
};

template <class _Tp, class _Environment>
class task<_Tp, _Environment>::promise_type : public __ycxx::__detail::__exec::__task_promise_return<_Tp, __ycxx::__detail::__exec::__task_state_base<_Tp, _Environment>> {
  template <receiver>
  friend class task::state;
  using __ycxx::__detail::__exec::__task_promise_return<_Tp, __state_base>::__st_;
  using __errors = __ycxx::__detail::__exec::__task_error_args<error_types>;

  struct __final_awaiter {
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(coroutine_handle<promise_type> h) noexcept { h.promise().__st_->__ycxx_complete(); }
    void await_resume() noexcept {}
  };
  struct __yield_error_awaiter {
    __state_base* __st;
    static constexpr bool await_ready() noexcept { return false; }
    void await_suspend(coroutine_handle<>) noexcept { __st->__ycxx_complete_yielded_error(); }
    void await_resume() noexcept {}
  };
  struct __env_t {
    const promise_type* p;
    start_scheduler_type query(get_start_scheduler_t) const noexcept { return *p->__st_->__sched; }
    allocator_type query(get_allocator_t) const noexcept { return *p->__st_->__alloc; }
    stop_token_type query(get_stop_token_t) const noexcept { return p->__st_->__ycxx_stop_token(); }
    template <class _Qp, class... _As>
      requires(!same_as<_Qp, get_start_scheduler_t> && !same_as<_Qp, get_allocator_t> && !same_as<_Qp, get_stop_token_t>) &&
              __ycxx::__detail::__exec::__forwarding_query_c<_Qp> && __ycxx::__detail::__exec::__has_query<_Environment, _Qp, _As...>
    constexpr decltype(auto) query(_Qp __q, _As&&... __as) const noexcept(noexcept(declval<const _Environment&>().query(__q, static_cast<_As&&>(__as)...))) {
      return __ycxx::__detail::__exec::__as_const_ref(*p->__st_->__env_ptr).query(__q, static_cast<_As&&>(__as)...);
    }
  };

public:
  task get_return_object() noexcept { return task(coroutine_handle<promise_type>::from_promise(*this)); }
  static constexpr suspend_always initial_suspend() noexcept { return {}; }
  auto final_suspend() noexcept { return __final_awaiter{}; }
  // [task.promise]/7
  void unhandled_exception() {
    if constexpr (__ycxx::__detail::__exec::__csigs_contain<set_error_t(exception_ptr), error_types>)
      __st_->error = std::current_exception();
    else
      std::terminate();
  }
  coroutine_handle<> unhandled_stopped() noexcept {
    __st_->__ycxx_complete_stopped();
    return noop_coroutine();
  }
  template <class _Ep>
  auto yield_value(with_error<_Ep> error) {
    using _Err = typename with_error<_Ep>::type;
    static_assert(__errors::template __convertible_count<_Err> == 1,
                  "task: co_yield with_error: the error must be convertible to exactly one of error_types");
    using _Cerr = typename __errors::template target<_Err>;
    __st_->__yielded_error.template emplace<_Cerr>(static_cast<_Err&&>(error.error));
    return __yield_error_awaiter{__st_};
  }
  template <sender _Sender>
  auto await_transform(_Sender&& __sndr) {
    if constexpr (same_as<inline_scheduler, start_scheduler_type>)
      return as_awaitable(static_cast<_Sender&&>(__sndr), *this);
    else
      return as_awaitable(affine(static_cast<_Sender&&>(__sndr)), *this);
  }
  __env_t get_env() const noexcept { return {this}; }

  void* operator new(size_t size) { return operator new(size, allocator_arg, allocator_type()); }
  template <class _Alloc, class... _Args>
  void* operator new(size_t size, allocator_arg_t, _Alloc __alloc, _Args&&...) {
    return __ycxx::__detail::__exec::__task_frame_allocate(size, __alloc);
  }
  template <class _This, class _Alloc, class... _Args>
  void* operator new(size_t size, const _This&, allocator_arg_t, _Alloc __alloc, _Args&&...) {
    return __ycxx::__detail::__exec::__task_frame_allocate(size, __alloc);
  }
  void operator delete(void* pointer, size_t size) noexcept { __ycxx::__detail::__exec::__task_frame_deallocate(pointer, size); }
};

}} // namespace std::execution
