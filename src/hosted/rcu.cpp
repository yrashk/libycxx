// libycxx hosted runtime: the state of the RCU domain (ycxx/hosted/rcu.hpp).
//
// Epochs. A global counter G (`epoch`, starting at 1) is advanced by every scheduled evaluation
// and every rcu_synchronize. Each thread that has entered a region owns a reader record, whose
// `start` is 0 outside a region and, inside one, the value of G its outermost lock read. A
// scheduled evaluation records e = fetch_add(G, 1); it may be evaluated once every record has
// start == 0 or start > e ("the readers are past e"). A region whose lock read G after that
// increment has start > e, so it never holds the evaluation back; in particular rcu_barrier
// called inside a region that began after the retire does not wait for its own region.
//
// Why "past e" is enough ([saferecl.rcu.general]/5). The outermost lock is
//   s = load(G) (seq_cst); store(start, s) (release); fence F_r (seq_cst); <reads of protected data>
// and the unlock store(start, 0) (seq_cst). Scheduling is e = fetch_add(G, 1) (seq_cst, under
// queue_m, so the queue is in epoch order); the evaluator reads e under queue_m, so the
// fetch_add happens before its fence F_s (seq_cst), after which it loads every start (seq_cst).
// Take any region R. The fences are ordered in the single total order S:
//  (a) F_r before F_s: the start store, sequenced before F_r, is seen by the load after F_s, or a
//      later store of that record ([atomics.order]/4: otherwise the load would be coherence-
//      ordered before the store and F_s would precede F_r). The evaluator sees
//      - s <= e: it waits for this record;
//      - s > e: R's load of G read a value at or after the fetch_add in G's modification order,
//        so it read from the fetch_add's release sequence: the retire (strongly) happens before
//        R's start, and /5 asks nothing;
//      - 0 or the start of a later region: R's unlock (or the lock that followed it) released
//        the store the evaluator acquired, so the end of R happens before the evaluation.
//  (b) F_s before F_r: whatever the evaluator reads, every read R makes after F_r sees the
//      unlink that happened before the retire (if it did not, it would be coherence-ordered
//      before the unlink, which happens before F_s, and [atomics.order]/4.2.4 would put F_r
//      before F_s). So R cannot reach the retired object: it began, in effect, after the retire.
//      (The draft's wording asks for an order between R's end and the evaluation here too; no
//      program that unlinks an object before retiring it can tell the difference, which is
//      the argument every RCU implementation makes.)
// rcu_synchronize is the same with e = its own fetch_add, which bounds every region that began
// before it; a region that begins afterwards reads a larger G, so a stream of new regions cannot
// starve it.
//
// Waiting. A thread waiting for the readers to pass e counts itself in `waiting` and blocks on
// `unlock_seq` (PAL address wait); an outermost unlock that sees `waiting` nonzero bumps the word
// and wakes all. The unlock's store of 0 and its load of `waiting` are seq_cst against the
// waiter's increment of `waiting`, its load of `unlock_seq` and its scan, so either the waiter
// sees start == 0 or the unlock sees the waiter (and then changes the word it waits on).
//
// Evaluation. Waiting for readers happens with no lock held; evaluations then run under
// evaluation_m, one batch at a time: the batch is detached from the queue (the prefix of
// epochs <= e) and evaluated in the same critical section, so a thread that holds evaluation_m
// knows that every entry no longer queued has been evaluated. A barrier therefore waits for the
// readers to pass its bound, takes evaluation_m (which also waits for a batch another thread
// took earlier) and evaluates the queued prefix. Outside a region the bound is the last queued
// epoch; inside one it is the region's start - 1, i.e. the entries retired before the region
// began. An entry retired after the caller's region began cannot be evaluated before the region
// ends ([saferecl.rcu.general]/5), so a barrier inside the region does not wait for it.
#include <new>
#include <ycxx/hosted/rcu.hpp>
#include <ycxx/hosted/thread_support.hpp>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
namespace {

using epoch_t = unsigned long long;

// The queue is evaluated by an outermost unlock or a retire once it holds this many.
constexpr __ycxx_pal_u32 batch_threshold = 1000;

struct alignas(64) reader_record {
  epoch_t start;        // atomic: 0 outside a region, else the epoch its outermost lock read
  __ycxx_pal_u32 __owned;   // atomic: a thread holds this record
  reader_record* next;  // the next heap record (immutable once published)
};

// The first threads use these; later ones allocate records (never freed, reused once released).
constexpr int fixed_records = 8;
constinit reader_record fixed[fixed_records] = {};
reader_record* __extra = nullptr; // atomic; push-only list of heap records

epoch_t epoch = 1;                  // atomic: G
__ycxx_pal_u32 waiting = 0;           // atomic: threads blocked until the readers pass an epoch
__ycxx_pal_u32 unlock_seq = 0;        // atomic: bumped by an unlock that sees `waiting`
constinit __futex_mutex queue_m;      // guards the queue
constinit __futex_mutex evaluation_m; // one batch of evaluations at a time
__rcu_node* queue_head = nullptr;     // in epoch order
__rcu_node** queue_tail = &queue_head;
epoch_t queue_last = 0;             // the epoch of the last queued node (under queue_m)
__ycxx_pal_u32 queued = 0;            // atomic (written under queue_m)

constinit thread_local unsigned depth = 0;               // the nesting depth of this thread's regions
constinit thread_local reader_record* __record = nullptr;  // this thread's record, once it has one
constinit thread_local bool evaluating = false;          // this thread runs a batch of evaluations

bool try_own(reader_record* r) noexcept {
  __ycxx_pal_u32 free = 0;
  return __atomic_load_n(&r->__owned, __ATOMIC_RELAXED) == 0 &&
         __atomic_compare_exchange_n(&r->__owned, &free, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

void readers_changed() noexcept {
  if (__atomic_load_n(&waiting, __ATOMIC_SEQ_CST) != 0) {
    __atomic_fetch_add(&unlock_seq, 1, __ATOMIC_SEQ_CST);
    ::__ycxx_pal_wake_all(&unlock_seq);
  }
}

// At thread end (after its thread_local objects are destroyed): gives the record back. A thread
// that ends inside a region ends the region. The record comes as the argument: the hook runs among
// the thread's key destructors, where the thread's own thread_local storage may already be gone
// (POSIX leaves their order unspecified; on Darwin a thread_local read there is fresh storage,
// zero again, with both native and emulated TLS), so it reads and writes no thread_local.
void release_record(void* arg) noexcept {
  reader_record* r = static_cast<reader_record*>(arg);
  __atomic_store_n(&r->start, epoch_t(0), __ATOMIC_SEQ_CST);
  readers_changed();
  __atomic_store_n(&r->__owned, 0, __ATOMIC_RELEASE);
}

[[__gnu__::__noinline__]] reader_record* acquire_record() noexcept {
  reader_record* r = nullptr;
  for (reader_record& __f : fixed)
    if (try_own(&__f)) {
      r = &__f;
      break;
    }
  if (!r)
    for (reader_record* __x = __atomic_load_n(&__extra, __ATOMIC_ACQUIRE); __x; __x = __x->next)
      if (try_own(__x)) {
        r = __x;
        break;
      }
  if (!r) {
    void* p = ::__ycxx_pal_allocate(sizeof(reader_record), alignof(reader_record));
    if (!p)
      ::__ycxx_pal_abort("rcu_domain::lock: cannot allocate a reader record");
    r = ::new (p) reader_record{0, 1, nullptr};
    reader_record* __head = __atomic_load_n(&__extra, __ATOMIC_RELAXED);
    do
      r->next = __head;
    while (!__atomic_compare_exchange_n(&__extra, &__head, r, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED));
  }
  __record = r;
  // If the hook cannot be registered the record stays owned after the thread ends: its start is
  // 0 then, so it holds nothing back; it is only never reused.
  static_cast<void>(::__ycxx_pal_at_thread_end(&release_record, r));
  return r;
}

// True when every region that read an epoch <= e has ended.
bool readers_past(epoch_t e) noexcept {
  const auto past = [e](const reader_record& r) noexcept {
    const epoch_t s = __atomic_load_n(&r.start, __ATOMIC_SEQ_CST);
    return s == 0 || s > e;
  };
  for (const reader_record& __f : fixed)
    if (!past(__f))
      return false;
  for (reader_record* __x = __atomic_load_n(&__extra, __ATOMIC_ACQUIRE); __x; __x = __x->next)
    if (!past(*__x))
      return false;
  return true;
}

// Blocks until every region whose start read an epoch <= e has ended. e must have been obtained
// (by fetch_add, or under queue_m) before the call.
void wait_for_readers(epoch_t e) noexcept {
  __atomic_thread_fence(__ATOMIC_SEQ_CST); // F_s
  if (readers_past(e))
    return;
  __atomic_fetch_add(&waiting, 1, __ATOMIC_SEQ_CST);
  for (;;) {
    const __ycxx_pal_u32 __seen = __atomic_load_n(&unlock_seq, __ATOMIC_SEQ_CST);
    if (readers_past(e))
      break;
    ::__ycxx_pal_wait(&unlock_seq, __seen);
  }
  __atomic_fetch_sub(&waiting, 1, __ATOMIC_RELAXED);
}

// Evaluates the queued entries of epoch <= e, after waiting for the readers to pass e, and after
// any batch another thread has taken.
void evaluate_through(epoch_t e) noexcept {
  queue_m.lock();
  const bool any = queue_head && queue_head->__rcu_epoch_ <= e;
  queue_m.unlock();
  if (any)
    wait_for_readers(e);
  evaluation_m.lock();
  queue_m.lock();
  __rcu_node* list = nullptr;
  if (queue_head && queue_head->__rcu_epoch_ <= e) {
    __rcu_node* last = queue_head;
    __ycxx_pal_u32 n = 1;
    for (; last->__rcu_next_ && last->__rcu_next_->__rcu_epoch_ <= e; ++n)
      last = last->__rcu_next_;
    list = queue_head;
    queue_head = last->__rcu_next_;
    last->__rcu_next_ = nullptr;
    if (!queue_head)
      queue_tail = &queue_head;
    __atomic_store_n(&queued, __atomic_load_n(&queued, __ATOMIC_RELAXED) - n, __ATOMIC_RELAXED);
  }
  queue_m.unlock();
  evaluating = true;
  while (list) {
    __rcu_node* __x = list;
    list = __x->__rcu_next_;
    __x->__rcu_run_(__x);
  }
  evaluating = false;
  evaluation_m.unlock();
}

// The epoch of the last queued entry (0 when the queue is empty).
epoch_t last_queued() noexcept {
  queue_m.lock();
  const epoch_t e = queue_head ? queue_last : 0;
  queue_m.unlock();
  return e;
}

void evaluate_if_due() noexcept {
  if (depth == 0 && !evaluating && __atomic_load_n(&queued, __ATOMIC_RELAXED) >= batch_threshold)
    evaluate_through(last_queued());
}

} // namespace

void __rcu_lock() noexcept {
  if (depth++ != 0)
    return;
  reader_record* r = __record;
  if (!r) [[unlikely]]
    r = acquire_record();
  if (::__ycxx::__detail::__single_threaded()) {
    // No other thread can evaluate meanwhile; one created later synchronizes with this thread.
    __atomic_store_n(&r->start, __atomic_load_n(&epoch, __ATOMIC_RELAXED), __ATOMIC_RELAXED);
    return;
  }
  __atomic_store_n(&r->start, __atomic_load_n(&epoch, __ATOMIC_SEQ_CST), __ATOMIC_RELEASE);
  __atomic_thread_fence(__ATOMIC_SEQ_CST); // F_r
}

void __rcu_unlock() noexcept {
  ::__ycxx::__detail::__precondition(depth != 0, "rcu_domain::unlock: no region of RCU protection is open");
  if (--depth != 0)
    return;
  if (::__ycxx::__detail::__single_threaded()) {
    __atomic_store_n(&__record->start, epoch_t(0), __ATOMIC_RELAXED);
  } else {
    __atomic_store_n(&__record->start, epoch_t(0), __ATOMIC_SEQ_CST);
    readers_changed();
  }
  evaluate_if_due();
}

void rcu_synchronize() noexcept {
  ::__ycxx::__detail::__precondition(depth == 0, "rcu_synchronize: called inside a region of RCU protection");
  wait_for_readers(__atomic_fetch_add(&epoch, 1, __ATOMIC_SEQ_CST));
}

void rcu_barrier() noexcept {
  if (evaluating) // from a scheduled evaluation: what this thread's batch holds is being evaluated
    return;
  // Inside a region: what was retired before the region began (start - 1); the region's own
  // start never holds that back.
  evaluate_through(depth == 0 ? last_queued() : __atomic_load_n(&__record->start, __ATOMIC_RELAXED) - 1);
}

void __rcu_schedule(__rcu_node* n) noexcept {
  n->__rcu_next_ = nullptr;
  queue_m.lock();
  n->__rcu_epoch_ = __atomic_fetch_add(&epoch, 1, __ATOMIC_SEQ_CST);
  queue_last = n->__rcu_epoch_;
  *queue_tail = n;
  queue_tail = &n->__rcu_next_;
  __atomic_store_n(&queued, __atomic_load_n(&queued, __ATOMIC_RELAXED) + 1, __ATOMIC_RELAXED);
  queue_m.unlock();
  evaluate_if_due();
}

}} // namespace __ycxx::__detail
