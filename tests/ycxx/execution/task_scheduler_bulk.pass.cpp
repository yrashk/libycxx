// bulk_chunked / bulk_unchunked (and bulk) of a sender that completes on a task_scheduler
// ([exec.task.scheduler]/1: get_completion_domain<set_value_t>(s) is ts-domain):
//   /14-15: ts-domain's transform_sender connects the child with a receiver R whose environment
//     forwards the receiver's; the child's error or stopped is forwarded unchanged; its values
//     are decay-copied, and SCHED(s)'s backend's schedule_bulk_chunked(shape, r, s) /
//     schedule_bulk_unchunked(shape, r, s) runs f with lvalues of those copies;
//   /11-12: backend-for runs r.execute over chunks (or single indices) covering [0, shape)
//     exactly once, through bulk(just-sndr-like, par, ...) on the wrapped scheduler, whose
//     completion scheduler is the wrapped one: the iterations run on the run_loop's thread;
//   [exec.bulk]/9.1.2: an exception from f is an error holding it (or bad_alloc, or a
//     runtime_error).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <new>
#include <stdexcept>
#include <thread>
#include <vector>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

int main() {
  ex::run_loop loop;
  std::thread t([&] { loop.run(); });
  const auto loop_id = t.get_id();
  ex::task_scheduler ts(loop.get_scheduler());

  // bulk_chunked: every index once, on the loop's thread, with the child's value.
  {
    std::vector<int> hits(37, 0);
    bool elsewhere = false;
    auto r = tt::sync_wait(ex::schedule(ts) | ex::then([] { return 4; }) | ex::bulk_chunked(ex::par, 37, [&](int b, int e, int& v) {
                             CHECK(0 <= b && b < e && e <= 37);
                             if (tt::get_id() != loop_id)
                               elsewhere = true;
                             for (int i = b; i < e; ++i)
                               hits[i] += v;
                           }));
    CHECK(r && std::get<0>(*r) == 4 && !elsewhere);
    for (int h : hits)
      CHECK(h == 4);
  }
  // bulk_unchunked and bulk.
  {
    std::vector<int> hits(20, 0);
    auto r = tt::sync_wait(ex::schedule(ts) | ex::bulk_unchunked(ex::par, 20, [&](int i) { hits[i] += 1; }) |
                           ex::bulk(ex::par, 20, [&](int i) { hits[i] += 10; }));
    CHECK(r.has_value());
    for (int h : hits)
      CHECK(h == 11);
  }
  // The child's error is forwarded without calling f.
  {
    int calls = 0;
    bool caught = false;
    try {
      (void)tt::sync_wait(ex::schedule(ts) | ex::then([]() -> int { throw 5; }) | ex::bulk_chunked(ex::par, 8, [&](int, int, int) { ++calls; }));
    } catch (int e) {
      caught = e == 5;
    }
    CHECK(caught && calls == 0);
  }
  // f throws.
  {
    bool ok = false;
    try {
      (void)tt::sync_wait(ex::schedule(ts) | ex::bulk_unchunked(ex::par, 8, [](int i) {
                            if (i == 6)
                              throw 6;
                          }));
    } catch (int e) {
      ok = e == 6;
    } catch (const std::bad_alloc&) {
      ok = true;
    } catch (const std::runtime_error&) {
      ok = true;
    }
    CHECK(ok);
  }
  loop.finish();
  t.join();
  return 0;
}
