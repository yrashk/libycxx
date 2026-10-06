// parallel_scheduler ([exec.par.scheduler]) and task_scheduler ([exec.task.scheduler]):
//   get_parallel_scheduler() returns a scheduler with forward_progress_guarantee::parallel; two
//   are equal iff they share their backend; its schedule sender completes on a thread of the
//   backend's execution context. bulk_chunked/bulk_unchunked (and bulk) of a sender completing
//   on it run through the backend's bulk operations (with a parallel policy: across its threads),
//   each index exactly once.
//   task_scheduler(sch) type-erases an infallible scheduler; it compares equal to sch.
// FLAGS: -pthread
#include <execution>
#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

int main() {
  auto sch = ex::get_parallel_scheduler();
  static_assert(ex::scheduler<decltype(sch)>);
  CHECK(ex::get_forward_progress_guarantee(sch) == ex::forward_progress_guarantee::parallel);
  CHECK(sch == ex::get_parallel_scheduler());
  auto copy = sch;
  CHECK(copy == sch);

  // schedule: completes on another thread.
  auto id = tt::sync_wait(ex::schedule(sch) | ex::then([] { return tt::get_id(); }));
  CHECK(id && std::get<0>(*id) != tt::get_id());

  // bulk on the parallel scheduler: every index once; with par, on the backend's threads.
  {
    const int n = 10000;
    std::vector<std::atomic<int>> hits(n);
    std::mutex m;
    std::set<std::thread::id> threads;
    auto r = tt::sync_wait(ex::schedule(sch) | ex::then([] { return 3; }) | ex::bulk(ex::par, n, [&](int i, int v) {
                             hits[i] += v;
                             std::lock_guard l(m);
                             threads.insert(tt::get_id());
                           }));
    CHECK(r && std::get<0>(*r) == 3);
    for (auto& h : hits)
      CHECK(h == 3);
    CHECK(!threads.contains(tt::get_id()));
  }
  // bulk_chunked with seq: one chunk covering everything.
  {
    std::atomic<int> calls{0}, covered{0};
    auto r = tt::sync_wait(ex::schedule(sch) | ex::bulk_chunked(ex::seq, 100, [&](int b, int e) {
                             ++calls;
                             covered += e - b;
                           }));
    CHECK(r && calls == 1 && covered == 100);
  }
  // bulk_unchunked
  {
    std::atomic<long> sum{0};
    auto r = tt::sync_wait(ex::schedule(sch) | ex::bulk_unchunked(ex::par, 64, [&](int i) { sum += i; }));
    CHECK(r && sum == 63 * 64 / 2);
  }
  // when_all of work on the parallel scheduler.
  {
    auto w = ex::when_all(ex::schedule(sch) | ex::then([] { return 1; }), ex::schedule(sch) | ex::then([] { return 2; }));
    auto r = tt::sync_wait(std::move(w));
    CHECK(r && std::get<0>(*r) + std::get<1>(*r) == 3);
  }

  // task_scheduler over a run_loop scheduler.
  {
    ex::run_loop loop;
    std::thread t([&] { loop.run(); });
    const auto tid = t.get_id();
    ex::task_scheduler ts(loop.get_scheduler());
    static_assert(ex::scheduler<ex::task_scheduler>);
    CHECK(ts == loop.get_scheduler());
    CHECK(ts == ex::task_scheduler(loop.get_scheduler()));
    ex::run_loop other;
    CHECK(!(ts == other.get_scheduler()));
    auto r = tt::sync_wait(ex::schedule(ts) | ex::then([] { return tt::get_id(); }));
    CHECK(r && std::get<0>(*r) == tid);
    std::atomic<int> sum{0};
    auto rb = tt::sync_wait(ex::schedule(ts) | ex::bulk(ex::par, 50, [&](int i) { sum += i; }));
    CHECK(rb && sum == 49 * 50 / 2);
    loop.finish();
    t.join();
  }
  return 0;
}
