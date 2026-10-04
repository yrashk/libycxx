// libycxx hosted runtime: the state of the RCU domain (ycxx/hosted/rcu.hpp).
#include <ycxx/hosted/rcu.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/pal.h>

namespace ycxx::detail {
namespace {

// The queue is evaluated by an outermost unlock or a retire once it holds this many.
constexpr ycxx_pal_u32 batch_threshold = 1000;

struct alignas(64) reader_count {
  ycxx_pal_u32 n; // atomic: readers in regions that began in this phase
};

ycxx_pal_u32 phase = 0;             // atomic: 0 or 1
reader_count readers[2] = {};
ycxx_pal_u32 sync_waiting = 0;      // atomic: synchronizers blocked on a reader count
constinit futex_mutex sync_m;       // serializes rcu_synchronize
constinit futex_mutex queue_m;      // guards the queue
constinit futex_mutex evaluation_m; // one batch of evaluations at a time
rcu_node* queue_head = nullptr;
rcu_node** queue_tail = &queue_head;
ycxx_pal_u32 queued = 0;            // atomic (written under queue_m)

thread_local unsigned depth = 0;          // the nesting depth of this thread's regions
thread_local unsigned reader_phase = 0;   // the phase its outermost region counted in
thread_local bool evaluating = false;     // this thread runs a batch of evaluations

void leave(unsigned p) noexcept {
  if (__atomic_sub_fetch(&readers[p].n, 1, __ATOMIC_SEQ_CST) == 0 && __atomic_load_n(&sync_waiting, __ATOMIC_SEQ_CST) != 0)
    ::ycxx_pal_wake_all(&readers[p].n);
}

void synchronize() noexcept {
  sync_m.lock();
  const ycxx_pal_u32 p = __atomic_load_n(&phase, __ATOMIC_RELAXED);
  __atomic_store_n(&phase, p ^ 1, __ATOMIC_SEQ_CST);
  // A reader that counted itself in readers[p] but re-reads the new phase leaves again, so
  // readers[p] drains: by the sequentially consistent operations, a reader either is counted
  // here or sees the new phase (and with it everything that happened before this call).
  __atomic_fetch_add(&sync_waiting, 1, __ATOMIC_SEQ_CST);
  for (ycxx_pal_u32 c; (c = __atomic_load_n(&readers[p].n, __ATOMIC_SEQ_CST)) != 0;)
    ::ycxx_pal_wait(&readers[p].n, c);
  __atomic_fetch_sub(&sync_waiting, 1, __ATOMIC_RELAXED);
  sync_m.unlock();
}

// Evaluates everything queued so far, after the regions that began before it have ended.
// Holding evaluation_m also waits for a batch another thread took earlier.
void evaluate_queue() noexcept {
  evaluation_m.lock();
  queue_m.lock();
  rcu_node* list = queue_head;
  queue_head = nullptr;
  queue_tail = &queue_head;
  __atomic_store_n(&queued, 0, __ATOMIC_RELAXED);
  queue_m.unlock();
  if (list) {
    synchronize();
    evaluating = true;
    while (list) {
      rcu_node* n = list;
      list = n->rcu_next_;
      n->rcu_run_(n);
    }
    evaluating = false;
  }
  evaluation_m.unlock();
}

void evaluate_if_due() noexcept {
  if (depth == 0 && !evaluating && __atomic_load_n(&queued, __ATOMIC_RELAXED) >= batch_threshold)
    evaluate_queue();
}

} // namespace

void rcu_lock() noexcept {
  if (depth++ != 0)
    return;
  for (;;) {
    const ycxx_pal_u32 p = __atomic_load_n(&phase, __ATOMIC_SEQ_CST);
    __atomic_fetch_add(&readers[p].n, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&phase, __ATOMIC_SEQ_CST) == p) {
      reader_phase = p;
      return;
    }
    leave(p);
  }
}

void rcu_unlock() noexcept {
  ::ycxx::detail::precondition(depth != 0, "rcu_domain::unlock: no region of RCU protection is open");
  if (--depth != 0)
    return;
  leave(reader_phase);
  evaluate_if_due();
}

void rcu_synchronize() noexcept {
  ::ycxx::detail::precondition(depth == 0, "rcu_synchronize: called inside a region of RCU protection");
  synchronize();
}

void rcu_barrier() noexcept {
  ::ycxx::detail::precondition(depth == 0, "rcu_barrier: called inside a region of RCU protection");
  if (evaluating) // from a scheduled evaluation: what this thread's batch holds is being evaluated
    return;
  evaluate_queue();
}

void rcu_schedule(rcu_node* n) noexcept {
  n->rcu_next_ = nullptr;
  queue_m.lock();
  *queue_tail = n;
  queue_tail = &n->rcu_next_;
  __atomic_store_n(&queued, __atomic_load_n(&queued, __ATOMIC_RELAXED) + 1, __ATOMIC_RELAXED);
  queue_m.unlock();
  evaluate_if_due();
}

} // namespace ycxx::detail
