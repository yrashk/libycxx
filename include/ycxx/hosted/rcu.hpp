// libycxx hosted: read-copy update ([saferecl.rcu]).
//
// The one rcu_domain (rcu_default_domain()) lives in the hosted runtime
// (src/hosted/rcu.cpp, which also gives the memory-ordering argument). A global epoch counter
// advances with every scheduled evaluation and every rcu_synchronize. Each thread that enters a
// region owns a reader record (released when the thread ends); its outermost lock stores the
// current epoch there, followed by a fence, and its outermost unlock clears it; nested locks
// only count the depth.
//
// Scheduled evaluations (retire, rcu_retire) are queued in the domain without blocking, each
// with the epoch it advanced. An evaluation of epoch e may run once every record is clear or
// holds an epoch above e, i.e. every region that began before it was scheduled has ended.
// rcu_synchronize advances the epoch and waits likewise. Evaluations run by rcu_barrier, and by
// the outermost unlock or a retire outside any region once the queue has grown past a bound,
// one batch at a time under the domain's evaluation lock. rcu_barrier inside a region evaluates
// what was scheduled before the region began, which the region itself does not hold back, and
// blocks for ever if the calling thread retired something since; inside a scheduled evaluation
// it evaluates everything scheduled before it but the evaluations in progress on its thread.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/unique_ptr.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// A scheduled evaluation (the base of every rcu_obj_base, and of rcu_retire's records).
struct __rcu_node {
  __rcu_node* __rcu_next_;
  void (*__rcu_run_)(__rcu_node*) noexcept; // evaluates it
  unsigned long long __rcu_epoch_;         // the domain's epoch when it was scheduled
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
class rcu_domain;
rcu_domain& rcu_default_domain() noexcept;
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
using __adl_free::__rcu_node;

// ---- the hosted runtime (src/hosted/rcu.cpp) ---------------------------------------------------
void __rcu_lock() noexcept;
void __rcu_unlock() noexcept;
void rcu_synchronize() noexcept;
void rcu_barrier() noexcept;
// True when rcu_barrier() would never return: the calling thread is inside a region and retired
// something since it began ([saferecl.rcu.domain.func]/4, [saferecl.rcu.general]/5).
bool __rcu_barrier_would_block() noexcept;
// True inside a region of RCU protection (where rcu_synchronize would never return).
bool __rcu_inside_region() noexcept;
// Queues n (its rcu_run_ is set); may evaluate queued evaluations.
void __rcu_schedule(__rcu_node* n) noexcept;

// [saferecl.rcu.general]/2: exactly one public, non-virtual base rcu_obj_base<T, D>.
template <class _Tp>
concept __rcu_protectable = requires { typename _Tp::__ycxx_rcu_base; } &&
                          std::is_same_v<typename _Tp::__ycxx_rcu_base::__ycxx_rcu_object, _Tp> &&
                          requires(_Tp* p, typename _Tp::__ycxx_rcu_base* b) {
                            static_cast<typename _Tp::__ycxx_rcu_base*>(p);
                            static_cast<_Tp*>(b);
                          };

// rcu_retire's record: the pointer and the deleter.
template <class _Tp, class _Dp>
struct __rcu_retired final : __rcu_node {
  _Tp* p;
  [[no_unique_address]] _Dp d;

  __rcu_retired(_Tp* __q, _Dp&& e) : __rcu_node{nullptr, &run, 0}, p(__q), d(static_cast<_Dp&&>(e)) {}
  static void run(__rcu_node* n) noexcept {
    __rcu_retired* __self = static_cast<__rcu_retired*>(n);
    __self->d(__self->p);
    delete __self;
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [saferecl.rcu.domain]
class rcu_domain {
  constexpr rcu_domain() noexcept = default;
  friend rcu_domain& rcu_default_domain() noexcept;

public:
  rcu_domain(const rcu_domain&) = delete;
  rcu_domain& operator=(const rcu_domain&) = delete;

  void lock() noexcept { __ycxx::__detail::__rcu_lock(); }
  bool try_lock() noexcept {
    __ycxx::__detail::__rcu_lock();
    return true;
  }
  void unlock() noexcept { __ycxx::__detail::__rcu_unlock(); }
};

// The one rcu_domain object; it has no state of its own (the domain's state is the runtime's).
inline rcu_domain& rcu_default_domain() noexcept {
  static constinit rcu_domain __domain;
  return __domain;
}

// rcu_synchronize inside a region waits for that region's end (/2): for ever. With YCXX_HARDENED
// that is reported instead.
inline void rcu_synchronize(rcu_domain& = rcu_default_domain()) noexcept {
  if constexpr (__ycxx::__detail::__cfg::__hardened)
    __ycxx::__detail::__precondition(!__ycxx::__detail::__rcu_inside_region(),
                                     "rcu_synchronize: called inside a region of RCU protection");
  __ycxx::__detail::rcu_synchronize();
}
// rcu_barrier inside a region after a retire of the same thread in it blocks for ever (/4 with
// [saferecl.rcu.general]/5); with YCXX_HARDENED that is reported instead (DECISIONS §3).
inline void rcu_barrier(rcu_domain& = rcu_default_domain()) noexcept {
  if constexpr (__ycxx::__detail::__cfg::__hardened)
    __ycxx::__detail::__precondition(!__ycxx::__detail::__rcu_barrier_would_block(),
                                     "rcu_barrier: called inside a region of RCU protection after retiring an "
                                     "object in it (it would wait for the region's end for ever)");
  __ycxx::__detail::rcu_barrier();
}

// [saferecl.rcu.base]
template <class _Tp, class _Dp = default_delete<_Tp>>
class rcu_obj_base : __ycxx::__adl_free::__rcu_node {
  [[no_unique_address]] _Dp __deleter_;

  static void run(__ycxx::__adl_free::__rcu_node* n) noexcept {
    rcu_obj_base* __self = static_cast<rcu_obj_base*>(n);
    __self->__deleter_(static_cast<_Tp*>(__self));
  }

public:
  // For the rcu-protectable check (ambiguous, so absent, when T has several rcu_obj_base bases).
  using __ycxx_rcu_base = rcu_obj_base;
  using __ycxx_rcu_object = _Tp;

  void retire(_Dp d = _Dp(), rcu_domain& = rcu_default_domain()) noexcept {
    static_assert(__ycxx::__detail::__rcu_protectable<_Tp>, "rcu_obj_base::retire: T is not rcu-protectable");
    __deleter_ = static_cast<_Dp&&>(d);
    this->__rcu_run_ = &run;
    __ycxx::__detail::__rcu_schedule(this);
  }

protected:
  rcu_obj_base() = default;
  rcu_obj_base(const rcu_obj_base&) = default;
  rcu_obj_base(rcu_obj_base&&) = default;
  rcu_obj_base& operator=(const rcu_obj_base&) = default;
  rcu_obj_base& operator=(rcu_obj_base&&) = default;
  ~rcu_obj_base() = default;
};

// [saferecl.rcu.domain.func]
template <class _Tp, class _Dp = default_delete<_Tp>>
void rcu_retire(_Tp* p, _Dp d = _Dp(), rcu_domain& = rcu_default_domain()) {
  static_assert(is_move_constructible_v<_Dp>, "rcu_retire: D must be move constructible");
  static_assert(is_invocable_v<_Dp&, _Tp*&>, "rcu_retire: d(p) must be well-formed");
  __ycxx::__detail::__rcu_schedule(new __ycxx::__detail::__rcu_retired<_Tp, _Dp>(p, static_cast<_Dp&&>(d)));
}

}} // namespace std
