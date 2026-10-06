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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
namespace {
namespace psr = std::execution::parallel_scheduler_replacement;

struct pool_item {
  pool_item* next = nullptr;
  void (*run)(pool_item*) noexcept = nullptr;
};

bool stop_requested(const psr::receiver_proxy& r) noexcept {
  auto __tok = r.try_query<std::inplace_stop_token>(std::get_stop_token);
  return __tok && __tok->stop_requested();
}

// Storage for an item of type T: the proxy's buffer when it fits, else the heap.
template <class _Tp>
_Tp* place(std::span<std::byte> s, bool& __heap) {
  void* p = s.data();
  std::size_t space = s.size();
  if (s.data() && std::align(alignof(_Tp), sizeof(_Tp), p, space)) {
    __heap = false;
    return static_cast<_Tp*>(p);
  }
  __heap = true;
  return static_cast<_Tp*>(::operator new(sizeof(_Tp), std::align_val_t(alignof(_Tp))));
}
template <class _Tp>
void unplace(_Tp* t, bool __heap) noexcept {
  t->~_Tp();
  if (__heap)
    ::operator delete(static_cast<void*>(t), std::align_val_t(alignof(_Tp)));
}

class thread_pool_backend final : public psr::parallel_scheduler_backend {
  std::mutex __m_;
  std::condition_variable __cv_;
  pool_item* __head_ = nullptr;
  pool_item* __tail_ = nullptr;
  bool stop_ = false;
  std::vector<std::thread> workers_;

  void push(pool_item* __it) noexcept {
    __it->next = nullptr;
    {
      std::lock_guard<std::mutex> __l(__m_);
      if (__tail_)
        __tail_->next = __it;
      else
        __head_ = __it;
      __tail_ = __it;
    }
    __cv_.notify_one();
  }
  void __work() noexcept {
    for (;;) {
      pool_item* __it;
      {
        std::unique_lock<std::mutex> __l(__m_);
        __cv_.wait(__l, [&] { return __head_ != nullptr || stop_; });
        if (!__head_)
          return;
        __it = __head_;
        __head_ = __it->next;
        if (!__head_)
          __tail_ = nullptr;
      }
      __it->run(__it);
    }
  }

  struct single : pool_item {
    psr::receiver_proxy* r;
    bool __heap;
  };
  struct bulk : pool_item {
    thread_pool_backend* pool;
    psr::bulk_item_receiver_proxy* r;
    std::size_t n, chunk, __chunks;
    std::size_t claimed = 0; // atomic: the next chunk index
    std::size_t done = 0;    // atomic: chunks finished
    bool __stopped = false;    // atomic
    bool __heap;
  };

  static void run_single(pool_item* p) noexcept {
    auto* __it = static_cast<single*>(p);
    psr::receiver_proxy* r = __it->r;
    unplace(__it, __it->__heap);
    if (stop_requested(*r))
      r->set_stopped();
    else
      r->set_value();
  }
  static void __run_bulk(pool_item* p) noexcept {
    auto* __it = static_cast<bulk*>(p);
    const std::size_t __chunks = __it->__chunks;
    const std::size_t i = __atomic_fetch_add(&__it->claimed, 1, __ATOMIC_ACQ_REL);
    if (i + 1 < __chunks)
      __it->pool->push(__it); // let another worker claim the next chunk
    if (stop_requested(*__it->r)) {
      __atomic_store_n(&__it->__stopped, true, __ATOMIC_RELEASE);
    } else if (!__atomic_load_n(&__it->__stopped, __ATOMIC_ACQUIRE)) {
      const std::size_t b = i * __it->chunk;
      const std::size_t e = b + __it->chunk < __it->n ? b + __it->chunk : __it->n;
      __it->r->execute(b, e);
    }
    // The item is only read before the increment: once another worker's increment is the last,
    // it completes the proxy and the item's storage goes.
    if (__atomic_add_fetch(&__it->done, 1, __ATOMIC_ACQ_REL) == __chunks)
      finish(__it);
  }
  static void finish(bulk* __it) noexcept {
    psr::bulk_item_receiver_proxy* r = __it->r;
    const bool __stopped = __atomic_load_n(&__it->__stopped, __ATOMIC_ACQUIRE);
    unplace(__it, __it->__heap);
    if (__stopped)
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
    bool __heap;
    bulk* __it;
    try {
      __it = place<bulk>(s, __heap);
    } catch (...) {
      r.set_error(std::current_exception());
      return;
    }
    ::new (static_cast<void*>(__it)) bulk();
    __it->run = &__run_bulk;
    __it->pool = this;
    __it->r = &r;
    __it->n = n;
    __it->chunk = chunk;
    __it->__chunks = (n + chunk - 1) / chunk;
    __it->__heap = __heap;
    push(__it);
  }

public:
  thread_pool_backend() {
    unsigned n = std::thread::hardware_concurrency();
    if (n == 0)
      n = 1;
    workers_.reserve(n);
    for (unsigned i = 0; i < n; ++i)
      workers_.emplace_back([this] { __work(); });
  }
  ~thread_pool_backend() override {
    {
      std::lock_guard<std::mutex> __l(__m_);
      stop_ = true;
    }
    __cv_.notify_all();
    for (auto& t : workers_)
      t.join();
  }

  void schedule(psr::receiver_proxy& r, std::span<std::byte> s) noexcept override {
    bool __heap;
    single* __it;
    try {
      __it = place<single>(s, __heap);
    } catch (...) {
      r.set_error(std::current_exception());
      return;
    }
    ::new (static_cast<void*>(__it)) single();
    __it->run = &run_single;
    __it->r = &r;
    __it->__heap = __heap;
    push(__it);
  }
  void schedule_bulk_chunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte> s) noexcept override {
    // About four chunks per worker: enough to balance uneven iterations, few enough to keep the
    // per-chunk overhead small.
    const std::size_t __parts = 4 * workers_.size();
    schedule_bulk(n, n / __parts + (n % __parts != 0), r, s);
  }
  void schedule_bulk_unchunked(std::size_t n, psr::bulk_item_receiver_proxy& r, std::span<std::byte> s) noexcept override {
    schedule_bulk(n, 1, r, s);
  }
};
} // namespace

std::shared_ptr<psr::parallel_scheduler_backend> __default_parallel_scheduler_backend() {
  // Constructed on first use; destroyed (its threads joined) at exit.
  static const std::shared_ptr<psr::parallel_scheduler_backend> __backend = std::make_shared<thread_pool_backend>();
  return __backend;
}
}}} // namespace __ycxx::__detail::__exec
