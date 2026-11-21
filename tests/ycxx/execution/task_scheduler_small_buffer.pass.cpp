// [exec.task.scheduler]: storage of the wrapped scheduler, and copies.
//   /3: the constructor initializes sch_ with allocate_shared<backend-for<Sch>>(alloc, sch);
//   /5: any allocation performed by calls on *this uses a copy of alloc: a scheduler with a large
//     object (256 bytes of state) is allocated through the given allocator (and nothing goes to
//     the global operator new), and freed through it when the last copy goes away;
//   /4 (Recommended practice): implementations should avoid dynamically allocated memory for
//     small scheduler objects. libycxx follows it, so a task_scheduler built from a run_loop's
//     scheduler or from inline_scheduler allocates nothing, neither through the global operator
//     new nor through an allocator passed to the constructor. This part is an
//     implementation-quality check (a "should"), not a conformance requirement;
//   the copy constructor and copy assignment are defaulted (synopsis) over a shared_ptr: they do
//     not throw and allocate nothing (copies refer to equal schedulers);
//   /6-7: copies compare equal to the original and to the wrapped scheduler; a copy keeps
//     scheduling on SCHED(s) after the original is destroyed or reassigned (/13.3, /10);
//   /1, /13.2, /14-15: bulk_chunked and bulk_unchunked of schedule(copy) go through ts-domain and
//     run every index once, on the wrapped scheduler's agent (/11-12).
// FLAGS: -pthread
#include <execution>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

// Global allocation counting.
int global_news = 0;
void* operator new(std::size_t n) {
  ++global_news;
  if (void* p = std::malloc(n ? n : 1))
    return p;
  std::abort();
}
void* operator new[](std::size_t n) {
  ++global_news;
  if (void* p = std::malloc(n ? n : 1))
    return p;
  std::abort();
}
void* operator new(std::size_t n, std::align_val_t a) {
  ++global_news;
  std::size_t al = static_cast<std::size_t>(a);
  if (void* p = std::aligned_alloc(al, (n + al - 1) / al * al))
    return p;
  std::abort();
}
void* operator new[](std::size_t n, std::align_val_t a) { return ::operator new(n, a); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

// An allocator that counts and takes its memory from malloc (not from operator new).
int alloc_allocs = 0, alloc_deallocs = 0;
template <class T>
struct CountingAlloc {
  using value_type = T;
  CountingAlloc() = default;
  template <class U>
  CountingAlloc(const CountingAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++alloc_allocs;
    void* p = std::aligned_alloc(alignof(std::max_align_t), (n * sizeof(T) + alignof(std::max_align_t) - 1) /
                                                                alignof(std::max_align_t) * alignof(std::max_align_t));
    if (!p)
      std::abort();
    return static_cast<T*>(p);
  }
  void deallocate(T* p, std::size_t) noexcept {
    ++alloc_deallocs;
    std::free(p);
  }
  template <class U>
  bool operator==(const CountingAlloc<U>&) const noexcept { return true; }
};

void reset_counts() { global_news = alloc_allocs = alloc_deallocs = 0; }

// A scheduler with 256 bytes of state; its operations complete inline and count their starts.
struct big_sched {
  using scheduler_concept = ex::scheduler_tag;
  int* starts = nullptr;
  std::array<unsigned char, 256> state{};

  struct sender {
    using sender_concept = ex::sender_tag;
    big_sched const* self;
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>();
    }
    struct attrs {
      big_sched const* self;
      big_sched query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return *self; }
    };
    attrs get_env() const noexcept { return {self}; }
    template <class R>
    struct op {
      using operation_state_concept = ex::operation_state_tag;
      R r;
      int* starts;
      void start() & noexcept {
        ++*starts;
        ex::set_value(std::move(r));
      }
    };
    template <class R>
    op<R> connect(R r) const noexcept {
      return {std::move(r), self->starts};
    }
  };
  // The sender refers to *this; a big_sched used by the tests outlives its senders (the
  // task_scheduler's backend holds the copy that schedules).
  sender schedule() const noexcept { return {this}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  bool operator==(const big_sched& o) const noexcept { return starts == o.starts && state == o.state; }
};
static_assert(ex::scheduler<big_sched> && sizeof(big_sched) >= 256);

using TS = ex::task_scheduler;
using LoopSched = decltype(std::declval<ex::run_loop&>().get_scheduler());
static_assert(std::is_nothrow_copy_constructible_v<TS> && std::is_nothrow_copy_assignable_v<TS>);

// Runs schedule(ts) | then(record thread) and returns the thread it ran on.
std::thread::id where(const TS& ts) {
  auto r = tt::sync_wait(ex::schedule(ts) | ex::then([] { return tt::get_id(); }));
  CHECK(r.has_value());
  return r ? std::get<0>(*r) : std::thread::id();
}

int main() {
  ex::run_loop loop;
  std::thread worker([&] { loop.run(); });
  const auto loop_id = worker.get_id();

  // /4 (QoI): small schedulers, no allocation at construction, copy or copy assignment.
  {
    reset_counts();
    TS a(loop.get_scheduler());
    TS b(ex::inline_scheduler{});
    TS c(loop.get_scheduler(), CountingAlloc<int>());
    TS d(ex::inline_scheduler{}, CountingAlloc<std::byte>());
    CHECK(global_news == 0 && alloc_allocs == 0);
    TS e = a;
    TS f = d;
    e = b;
    f = c;
    a = d;
    CHECK(global_news == 0 && alloc_allocs == 0);
    CHECK(e == b && e == ex::inline_scheduler{} && f == c && f == loop.get_scheduler() && a == d);
  }
  CHECK(global_news == 0 && alloc_allocs == 0 && alloc_deallocs == 0);

  // /3, /5: a large scheduler goes through the given allocator, once; copies do not allocate.
  int starts = 0;
  big_sched big{&starts, {}};
  big.state[100] = 7;
  {
    reset_counts();
    std::optional<TS> orig;
    orig.emplace(big, CountingAlloc<int>());
    CHECK(alloc_allocs >= 1 && global_news == 0 && alloc_deallocs == 0);
    const int after_ctor = alloc_allocs;
    TS copy = *orig;
    TS other(ex::inline_scheduler{});
    TS assigned = other;
    assigned = *orig;
    CHECK(alloc_allocs == after_ctor && global_news == 0);
    // /6-7
    CHECK(copy == *orig && assigned == *orig && copy == big && !(copy == other) && !(copy == ex::inline_scheduler{}));
    // The original goes away; the copies still schedule on SCHED(s).
    orig.reset();
    CHECK(alloc_deallocs == 0);
    starts = 0;
    CHECK(tt::sync_wait(ex::schedule(copy)).has_value() && starts == 1);
    other = copy;
    copy = TS(ex::inline_scheduler{});
    CHECK(tt::sync_wait(ex::schedule(other)).has_value() && starts == 2);
    CHECK(tt::sync_wait(ex::schedule(assigned)).has_value() && starts == 3);
    CHECK(other == big && assigned == other && !(copy == big));
    // Bulk through ts-domain on a copy: every index once (the backend's bulk runs on big_sched).
    std::vector<int> hits(29, 0);
    auto r = tt::sync_wait(ex::schedule(other) | ex::then([] { return 2; }) |
                           ex::bulk_chunked(ex::par, 29, [&](int b, int e, int& v) {
                             for (int i = b; i < e; ++i)
                               hits[i] += v;
                           }));
    CHECK(r && std::get<0>(*r) == 2);
    auto r2 = tt::sync_wait(ex::schedule(assigned) | ex::then([] { return 2; }) |
                            ex::bulk_unchunked(ex::par, 29, [&](int i, int& v) { hits[i] += 10 * v; }));
    CHECK(r2 && std::get<0>(*r2) == 2);
    for (int h : hits)
      CHECK(h == 22);
    CHECK(alloc_deallocs == 0);
  }
  // The last copy is gone: the backend was freed through the allocator.
  CHECK(alloc_deallocs >= 1 && alloc_deallocs == alloc_allocs);

  // With the default allocator, the large scheduler is allocated (allocate_shared with
  // allocator<void> obtains its storage from ::operator new, [allocator.members]).
  {
    reset_counts();
    TS t(big);
    CHECK(global_news >= 1 && alloc_allocs == 0);
    const int n = global_news;
    TS u = t;
    u = t;
    CHECK(global_news == n);
  }

  // Small scheduler: copies schedule on the run_loop after the original is destroyed or
  // reassigned.
  {
    std::optional<TS> orig;
    orig.emplace(loop.get_scheduler());
    TS copy = *orig;
    TS assigned(ex::inline_scheduler{});
    assigned = *orig;
    orig.reset();
    CHECK(where(copy) == loop_id && where(assigned) == loop_id);
    TS keep = copy;
    copy = TS(ex::inline_scheduler{});
    CHECK(where(copy) == tt::get_id());
    CHECK(where(keep) == loop_id && keep == loop.get_scheduler() && keep == assigned);

    // bulk_chunked / bulk_unchunked through ts-domain on the copies: every index once, on the
    // loop's thread, with the child's value.
    std::vector<int> hits(41, 0);
    bool elsewhere = false;
    auto r = tt::sync_wait(ex::schedule(keep) | ex::then([] { return 3; }) |
                           ex::bulk_chunked(ex::par, 41, [&](int b, int e, int& v) {
                             if (tt::get_id() != loop_id)
                               elsewhere = true;
                             for (int i = b; i < e; ++i)
                               hits[i] += v;
                           }));
    CHECK(r && std::get<0>(*r) == 3 && !elsewhere);
    for (int h : hits)
      CHECK(h == 3);
    auto r2 = tt::sync_wait(ex::schedule(assigned) | ex::bulk_unchunked(ex::par, 41, [&](int i) {
                              if (tt::get_id() != loop_id)
                                elsewhere = true;
                              hits[i] += 1;
                            }));
    CHECK(r2.has_value() && !elsewhere);
    for (int h : hits)
      CHECK(h == 4);
  }

  loop.finish();
  worker.join();
  return 0;
}
