// libycxx hosted: read-copy update ([saferecl.rcu]).
//
// The one rcu_domain (rcu_default_domain()) lives in the hosted runtime
// (src/hosted/rcu.cpp). Readers count themselves in one of two counters, selected by the
// domain's phase: the outermost lock of a thread increments the counter of the current phase
// and re-reads the phase (retrying if it changed meanwhile); nested locks only count the depth.
// rcu_synchronize flips the phase and waits (through the PAL) for the old phase's counter to
// drain, so every region that began before it has ended when it returns.
//
// Scheduled evaluations (retire, rcu_retire) are queued in the domain without blocking. They
// are evaluated, after an rcu_synchronize, by rcu_barrier, and by the outermost unlock or a
// retire outside any region once the queue has grown past a bound; evaluations run one batch at
// a time, under the domain's evaluation lock, never inside a region of the evaluating thread.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/unique_ptr.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// A scheduled evaluation (the base of every rcu_obj_base, and of rcu_retire's records).
struct rcu_node {
  rcu_node* rcu_next_;
  void (*rcu_run_)(rcu_node*) noexcept; // evaluates it
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {
class rcu_domain;
rcu_domain& rcu_default_domain() noexcept;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
using adl_free::rcu_node;

// ---- the hosted runtime (src/hosted/rcu.cpp) ---------------------------------------------------
void rcu_lock() noexcept;
void rcu_unlock() noexcept;
void rcu_synchronize() noexcept;
void rcu_barrier() noexcept;
// Queues n (its rcu_run_ is set); may evaluate queued evaluations.
void rcu_schedule(rcu_node* n) noexcept;

// [saferecl.rcu.general]/2: exactly one public, non-virtual base rcu_obj_base<T, D>.
template <class T>
concept rcu_protectable = requires { typename T::ycxx_rcu_base; } &&
                          std::is_same_v<typename T::ycxx_rcu_base::ycxx_rcu_object, T> &&
                          requires(T* p, typename T::ycxx_rcu_base* b) {
                            static_cast<typename T::ycxx_rcu_base*>(p);
                            static_cast<T*>(b);
                          };

// rcu_retire's record: the pointer and the deleter.
template <class T, class D>
struct rcu_retired final : rcu_node {
  T* p;
  [[no_unique_address]] D d;

  rcu_retired(T* q, D&& e) : rcu_node{nullptr, &run}, p(q), d(static_cast<D&&>(e)) {}
  static void run(rcu_node* n) noexcept {
    rcu_retired* self = static_cast<rcu_retired*>(n);
    self->d(self->p);
    delete self;
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [saferecl.rcu.domain]
class rcu_domain {
  constexpr rcu_domain() noexcept = default;
  friend rcu_domain& rcu_default_domain() noexcept;

public:
  rcu_domain(const rcu_domain&) = delete;
  rcu_domain& operator=(const rcu_domain&) = delete;

  void lock() noexcept { ycxx::detail::rcu_lock(); }
  bool try_lock() noexcept {
    ycxx::detail::rcu_lock();
    return true;
  }
  void unlock() noexcept { ycxx::detail::rcu_unlock(); }
};

// The one rcu_domain object; it has no state of its own (the domain's state is the runtime's).
inline rcu_domain& rcu_default_domain() noexcept {
  static constinit rcu_domain domain;
  return domain;
}

inline void rcu_synchronize(rcu_domain& = rcu_default_domain()) noexcept { ycxx::detail::rcu_synchronize(); }
inline void rcu_barrier(rcu_domain& = rcu_default_domain()) noexcept { ycxx::detail::rcu_barrier(); }

// [saferecl.rcu.base]
template <class T, class D = default_delete<T>>
class rcu_obj_base : ycxx::adl_free::rcu_node {
  [[no_unique_address]] D deleter_;

  static void run(ycxx::adl_free::rcu_node* n) noexcept {
    rcu_obj_base* self = static_cast<rcu_obj_base*>(n);
    self->deleter_(static_cast<T*>(self));
  }

public:
  // For the rcu-protectable check (ambiguous, so absent, when T has several rcu_obj_base bases).
  using ycxx_rcu_base = rcu_obj_base;
  using ycxx_rcu_object = T;

  void retire(D d = D(), rcu_domain& = rcu_default_domain()) noexcept {
    static_assert(ycxx::detail::rcu_protectable<T>, "rcu_obj_base::retire: T is not rcu-protectable");
    deleter_ = static_cast<D&&>(d);
    this->rcu_run_ = &run;
    ycxx::detail::rcu_schedule(this);
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
template <class T, class D = default_delete<T>>
void rcu_retire(T* p, D d = D(), rcu_domain& = rcu_default_domain()) {
  static_assert(is_move_constructible_v<D>, "rcu_retire: D must be move constructible");
  static_assert(is_invocable_v<D&, T*&>, "rcu_retire: d(p) must be well-formed");
  ycxx::detail::rcu_schedule(new ycxx::detail::rcu_retired<T, D>(p, static_cast<D&&>(d)));
}

} // namespace std
