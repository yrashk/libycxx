// libycxx hosted runtime: the default backend of execution::parallel_scheduler
// ([exec.par.scheduler], [exec.parschedrepl]): a pool of threads (as many as
// hardware_concurrency, at least 1) taking work items from one FIFO queue.
//
// A work item lives in the proxy's preallocated backend storage (span<byte>; the library's
// proxies give 128 bytes), so scheduling allocates nothing; a bulk operation is one item that a
// worker re-queues after claiming a chunk index, so idle workers join in, and whoever finishes
// the last chunk completes the proxy. Each chunk first checks the proxy's stop token; a stop
// request turns the completion into set_stopped.
#include <condition_variable>
#include <memory>
#include <execution>
#include <mutex>
#include <thread>
#include <vector>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
namespace {
namespace psr = std::execution::parallel_scheduler_replacement;

struct pool_item {
  pool_item* next = nullptr;
  void (*run)(pool_item*) noexcept = nullptr;
};

bool stop_requested(const psr::receiver_proxy& r) noexcept {
  auto tok = r.try_query<std::inplace_stop_token>(std::get_stop_token);
  return tok && tok->stop_requested();
}

// Storage for an item of type T: the proxy's buffer when it fits, else the heap.
template <class T>
T* place(std::span<std::byte> s, bool& heap) {
  void* p = s.data();
  std::size_t space = s.size();
  if (s.data() && std::align(alignof(T), sizeof(T), p, space)) {
    heap = false;
    return static_cast<T*>(p);
  }
  heap = true;
  return static_cast<T*>(::operator new(sizeof(T), std::align_val_t(alignof(T))));
}
template <class T>
void unplace(T* t, bool heap) noexcept {
  t->~T();
  if (heap)
    ::operator delete(static_cast<void*>(t), std::align_val_t(alignof(T)));
}

class thread_pool_backend final : public psr::parallel_scheduler_backend {
  std::mutex m_;
  std::condition_variable cv_;
  pool_item* head_ = nullptr;
  pool_item* tail_ = nullptr;
  bool stop_ = false;
  std::vector<std::thread> workers_;

  void push(pool_item* it) noexcept {
    it->next = nullptr;
    {
      std::lock_guard<std::mutex> l(m_);
      if (tail_)
        tail_->next = it;
      else
        head_ = it;
      tail_ = it;
    }
    cv_.notify_one();
  }
  void work() noexcept {
    for (;;) {
      pool_item* it;
      {
        std::unique_lock<std::mutex> l(m_);
        cv_.wait(l, [&] { return head_ != nullptr || stop_; });
        if (!head_)
          return;
        it = head_;
        head_ = it->next;
        if (!head_)
          tail_ = nullptr;
      }
      it->run(it);
    }
  }

  struct single : pool_item {
    psr::receiver_proxy* r;
    bool heap;
  };
  struct bulk : pool_item {
    thread_pool_backend* pool;
    psr::bulk_item_receiver_proxy* r;
    std::size_t n, chunk, chunks;
    std::size_t claimed = 0; // atomic: the next chunk index
    std::size_t done = 0;    // atomic: chunks finished
    bool stopped = false;    // atomic
    bool heap;
  };

  static void run_single(pool_item* p) noexcept {
    auto* it = static_cast<single*>(p);
    psr::receiver_proxy* r = it->r;
    unplace(it, it->heap);
    if (stop_requested(*r))
      r->set_stopped();
    else
      r->set_value();
  }
  static void run_bulk(pool_item* p) noexcept {
    auto* it = static_cast<bulk*>(p);
    const std::size_t chunks = it->chunks;
    const std::size_t i = __atomic_fetch_add(&it->claimed, 1, __ATOMIC_ACQ_REL);
    if (i + 1 < chunks)
      it->pool->push(it); // let another worker claim the next chunk
    if (stop_requested(*it->r)) {
      __atomic_store_n(&it->stopped, true, __ATOMIC_RELEASE);
    } else if (!__atomic_load_n(&it->stopped, __ATOMIC_ACQUIRE)) {
      const std::size_t b = i * it->chunk;
      const std::size_t e = b + it->chunk < it->n ? b + it->chunk : it->n;
      it->r->execute(b, e);
    }
    // The item is only read before the increment: once another worker's increment is the last,
    // it completes the proxy and the item's storage goes.
    if (__atomic_add_fetch(&it->done, 1, __ATOMIC_ACQ_REL) == chunks)
      finish(it);
  }
  static void finish(bulk* it) noexcept {
    psr::bulk_item_receiver_proxy* r = it->r;
    const bool stopped = __atomic_load_n(&it->stopped, __ATOMIC_ACQUIRE);
    unplace(it, it->heap);
    if (stopped)
      r->set_stopped();
    else
      r->set_value();
  }
  void schedule_bulk(std::size_t n, std::size_t chunk, psr::bulk_item_receiver_proxy& r, std::span<std::byte> s) noexcept {
    if (n == 0) {
      if (stop_requested(r))
        r.set_stopped();
      else
        r.set_value();
      return;
    }
    bool heap;
    bulk* it;
    try {
      it = place<bulk>(s, heap);
    } catch (...) {
      r.set_error(std::current_exception());
      return;
    }
    ::new (static_cast<void*>(it)) bulk();
    it->run = &run_bulk;
    it->pool = this;
    it->r = &r;
    it->n = n;
    it->chunk = chunk;
    it->chunks = (n + chunk - 1) / chunk;
    it->heap = heap;
    push(it);
  }

public:
  thread_pool_backend() {
    unsigned n = std::thread::hardware_concurrency();
    if (n == 0)
      n = 1;
    workers_.reserve(n);
    for (unsigned i = 0; i < n; ++i)
      workers_.emplace_back([this] { work(); });
  }
  ~thread_pool_backend() override {
    {
      std::lock_guard<std::mutex> l(m_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto& t : workers_)
      t.join();
  }

  void schedule(psr::receiver_proxy& r, std::span<std::byte> s) noexcept override {
    bool heap;
    single* it;
    try {
      it = place<single>(s, heap);
    } catch (...) {
      r.set_error(std::current_exception());
      return;
    }
    ::new (static_cast<void*>(it)) single();
    it->run = &run_single;
    it->r = &r;
    it->heap = heap;
    push(it);
  }
  void schedule_bulk_chunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte> s) noexcept override {
    // About four chunks per worker: enough to balance uneven iterations, few enough to keep the
    // per-chunk overhead small.
    const std::size_t parts = 4 * workers_.size();
    schedule_bulk(n, n / parts + (n % parts != 0), r, s);
  }
  void schedule_bulk_unchunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte> s) noexcept override {
    schedule_bulk(n, 1, r, s);
  }
};
} // namespace

std::shared_ptr<psr::parallel_scheduler_backend> default_parallel_scheduler_backend() {
  // Constructed on first use; destroyed (its threads joined) at exit.
  static const std::shared_ptr<psr::parallel_scheduler_backend> backend = std::make_shared<thread_pool_backend>();
  return backend;
}
}}} // namespace ycxx::detail::exec
